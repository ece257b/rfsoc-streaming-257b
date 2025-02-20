#pragma once

#include "StreamReceiver.hpp"

#define TEMPLATE(ret) template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType> ret StreamReceiver<DataProviderType, DataWindowType, NetworkConnectionType>

template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
StreamReceiver<DataProviderType, DataWindowType, NetworkConnectionType>::StreamReceiver(bool debug) : debug(debug) {}

template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
StreamReceiver<DataProviderType, DataWindowType, NetworkConnectionType>::~StreamReceiver() {}

// template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
// int StreamReceiver<DataProviderType, DataWindowType, NetworkConnectionType>::StreamReceiver::receiveData() {
TEMPLATE(int)::StreamReceiver::receiveData() {
    conn.open();
    handshake();

    bool running = true;
    base = 0;
    expected_seq = 0;
    while (running) {
        Packet packet;
        ssize_t recv_len = conn.receive(&packet, sizeof(packet));

        if(recv_len < 0) {
            std::this_thread::sleep_for(milliseconds(10));
            continue;
        }
        if(recv_len < HEADER_SIZE) {
            if(debug)
                std::cerr << "Received packet too small, ignoring." << std::endl;
            continue;
        }

        if (!verifyChecksum(&packet, recv_len)) {
            if(debug)
                std::cerr << "Invalid checksum for packet seq: " << seq_num << ", discarding." << std::endl;
            continue;
        }

        uint32_t seq_num = ntohl(header.seq_num);
        uint8_t ctrl_flag = header.control_flags;
        if (ctrl_flag == FLAG_DATA) {
            if (seq_num == expected_seq) {
                processor.process(&packet.data, recv_len - sizeof(packet.header));
                expected_seq++;
                
                processOutOfOrder();    // maybe we should send an ACK here if we process many packets?
            } else if (seq_num > expected_seq) {
                if (!window.contains(seq_num)) {
                    Packet* windowPacket = window.reserve(seq_num);
                    memcpy(windowPacket, &packet);  // May want to change algo here to avoid memcpy...
                    if(debug)
                        std::cout << "Stored out-of-order packet seq: " << seq_num << std::endl;
                }
                for(uint32_t missing = expected_seq; missing < seq_num; missing++) {
                    if (!window.contains(missing)) {
                        sendACK(expected_seq, FLAG_NACK);     // Should we have "cumulative" NACK..? to avoid sending many packets
                        if(debug)
                            std::cout << "Sent NACK for missing seq: " << missing << std::endl;
                    }
                    // TODO: Timeout for resending many NACKs?
                }
            } else {
                // seq_num < expected_seq
                // We got a sequence number we've already seen
                sendACK(expected_seq);
            }

            // --- Send cumulative ACK only at end of sliding window ---
            if(expected_seq > 0 && (expected_seq % pkt_window == 0)) {  // % pkt_window may not be best
                sendACK(expected_seq); // expected_seq - 1 ??
            }
        }
    }
    teardown();
}


template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
int StreamReceiver<DataProviderType, DataWindowType, NetworkConnectionType>::StreamReceiver::sendACK(
        uint32_t seq_num, uint8_t flag) {
    
    PacketHeader ack_hdr;
    ack_hdr.seq_num = htonl(seq_num);
    ack_hdr.window_size = htons(WINDOW_SIZE);
    ack_hdr.control_flags = FLAG_ACK;
    ack_hdr.checksum = 0;

    uint16_t ack_chksum = compute_checksum(&ack_hdr, sizeof(ack_hdr));
    ack_hdr.checksum = htons(ack_chksum);

    std::cout << "Sending ACK for " << seq_num << std::endl;
    return conn.send(&ack_hdr, sizeof(ack_hdr));
}

template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
int StreamReceiver<DataProviderType, DataWindowType, NetworkConnectionType>::StreamReceiver::teardown() {
    // processor.close(); // ?
    window.clear();
    conn.close();
}

template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
int StreamReceiver<DataProviderType, DataWindowType, NetworkConnectionType>::StreamReceiver::processOutOfOrder() {
    // while(out_of_order.count(expected_seq)) {
    //     if(debug)
    //         std::cout << "Processing buffered packet seq: " << expected_seq << std::endl;
    //     out_of_order.erase(expected_seq);
    //     expected_seq++;
    // }
    if (window.empty()) return;
    window.resetIter();
    Packet* packet = window.getIter();
    do {
        bool hasNext = window.nextIter();   // increment iter and check if done
        processor.process(packet->data, sizeof(packet->data));
        uint32_t processed_seq = packet->header.seq_num;

        packet = window.getIter();
        window.erase(processed_seq);
        if (!hasNext) break;
    };
}