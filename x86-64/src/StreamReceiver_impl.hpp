#pragma once
#include <thread>
#include <cassert>
#include "StreamReceiver.hpp"
#include "NetworkConnection.hpp"
#include "cmn.h"

template<typename DataProcessorType, typename NetworkConnectionType>
StreamReceiver<DataProcessorType, NetworkConnectionType>::StreamReceiver(
        DataProcessorType&& processor, NetworkConnectionType&& conn, bool debug, uint32_t window_size) :
            conn(std::move(conn)), processor(std::move(processor)), 
            window(window_size), ackTimes(window_size), nackTimes(window_size),
            stats(false), debug(debug), window_size(window_size) {
    static_assert(std::is_base_of<DataProcessorType, DataProcessorType>::value, "type parameter of this class must derive from DataProcessorType");
    static_assert(std::is_base_of<NetworkConnection, NetworkConnectionType>::value, "type parameter of this class must derive from NetworkConnection");
}

template<typename DataProcessorType, typename NetworkConnectionType>
StreamReceiver<DataProcessorType, NetworkConnectionType>::~StreamReceiver() {}

template<typename DataProcessorType, typename NetworkConnectionType>
int StreamReceiver<DataProcessorType, NetworkConnectionType>::StreamReceiver::receiveData() {
    conn.open();
    handshake();

    bool running = true;
    int count = 0;
    base = 0;
    expected_seq = 0;
    // auto last_nack = steady_clock::now();
    while (running) {
        stats.report();

        Packet packet;
        PacketHeader& header = packet.header;
        ssize_t recv_len = conn.receive(&packet, sizeof(packet));

        if(recv_len < 0) {
            std::this_thread::sleep_for(microseconds(SENDER_STREAMING_WAIT_US));
            continue;
        }
        if(recv_len < HEADER_SIZE) {
            if(debug)
                std::cerr << "Received packet too small, ignoring." << std::endl;
            continue;
        }

        uint32_t seq_num = ntohl(header.seq_num);
        uint16_t pkt_window = ntohs(header.window_size);
        uint8_t ctrl_flag = header.control_flags;

        if (!verifyChecksum(&packet, recv_len)) {
            if(debug)
                std::cerr << "Invalid checksum for packet seq " << seq_num << " Len: " << recv_len << ", discarding." << std::endl;
            stats.record_corrupted();
            nackTimes.erase(seq_num);  // might want to re-nack this guy!
            continue;
        }

        bool didntIgnore = false;

        if (ctrl_flag == FLAG_DATA) {
            if (seq_num == expected_seq) {
                if(debug)
                    std::cout << "Processing exp seq " << seq_num << std::endl; 
                count += processPacket(&packet, recv_len - sizeof(packet.header));
                count += processOutOfOrder();    // maybe we should send an ACK here if we process many packets?

                assert(advanceAllWindows(expected_seq));
                didntIgnore = true;

            } else if (seq_num > expected_seq) {
                if (!window.contains(seq_num)) {
                    Packet* windowPacketInfo = window.reserve(seq_num);
                    if (windowPacketInfo) {
                        memcpy(windowPacketInfo, (void*)&packet, sizeof(Packet));  // May want to change algo here to avoid memcpy...
                        if(debug)
                            std::cout << "Stored out-of-order packet seq: " << seq_num << " (exp " << expected_seq << ")" << std::endl;
                        didntIgnore = true;
                    } else {
                        if (debug) std::cout << seq_num << " out of bounds of Window!" << std::endl;
                    }
                }

                for(uint32_t missing = expected_seq; missing < seq_num; missing++) {
                    if (!window.contains(missing)) {
                        if (sendACK(missing, FLAG_NACK)) {
                            if(debug) std::cout << "Sent NACK for missing seq: " << missing << std::endl;
                        }
                    }
                }
            } else {
                // seq_num < expected_seq
                // We got a sequence number we've already seen
                if (sendACK(expected_seq)) {
                    if (debug) std::cout << "Already seen " << seq_num << " , sent ACK for " << expected_seq << std::endl;
                }
            }

            if (!didntIgnore) {
                stats.record_ignored();
            }

            // --- Send cumulative ACK only at end of sliding window ---
            if(expected_seq > 0 && (expected_seq % pkt_window == 0)) {  // % pkt_window may not be best
                if (sendACK(expected_seq)) {
                    if (debug) std::cout << "End of window, sending ACK for " << expected_seq << std::endl;
                }
            }
        } else if (ctrl_flag == FLAG_FIN) {
            sendFINACK(seq_num);
            running = false;
        }

        assert(window.inBounds(expected_seq));
    }
    return count;
}


template<typename DataProcessorType, typename NetworkConnectionType>
int StreamReceiver<DataProcessorType, NetworkConnectionType>::StreamReceiver::sendACK(
        uint32_t seq_num, uint8_t flag, bool checkPastACKs) {
    
    if (flag == FLAG_ACK || flag == FLAG_NACK) {
        // Don't care if FINACK
        auto& ack_window = (flag == FLAG_ACK) ? ackTimes : nackTimes;
        bool sentBefore = ack_window.contains(seq_num);
        timepoint* time = ack_window.reserve(seq_num);
        if (!time) {
            std::cerr << "ACK Window was out of range for " << flag << " " << seq_num << std::endl;
        }
        timepoint now = std::chrono::steady_clock::now();
        if (checkPastACKs) {
            bool shouldSend = (
                !sentBefore || //Short circuiting getting an invalid time
                std::chrono::duration_cast<microseconds>(now - *time).count() > RETRY_ACK_US
            );
            if (!shouldSend) return 0;
        }
        *time = now;
    }
    
    PacketHeader ack_hdr;
    ack_hdr.seq_num = htonl(seq_num);
    ack_hdr.window_size = htons(window_size);
    ack_hdr.control_flags = flag;
    ack_hdr.checksum = 0;

    uint16_t ack_chksum = compute_checksum(&ack_hdr, sizeof(ack_hdr));
    ack_hdr.checksum = htons(ack_chksum);

    stats.record_ack(flag);

    return conn.send(&ack_hdr, sizeof(ack_hdr));
}

template<typename DataProcessorType, typename NetworkConnectionType>
int StreamReceiver<DataProcessorType, NetworkConnectionType>::StreamReceiver::processOutOfOrder() {
    // while(out_of_order.count(expected_seq)) {
    //     if(debug)
    //         std::cout << "Processing buffered packet seq: " << expected_seq << std::endl;
    //     out_of_order.erase(expected_seq);
    //     expected_seq++;
    // }
    Packet* packet = window.get(expected_seq);
    int count = 0;
    while (packet) {
        count += processPacket(packet, sizeof(packet->data));
        if(debug)
            std::cout << "Processed Out of Order " << (expected_seq - 1) << std::endl; 
        
        packet = window.get(expected_seq);
    }
    return count;
}


template<typename DataProcessorType, typename NetworkConnectionType>
int StreamReceiver<DataProcessorType, NetworkConnectionType>::StreamReceiver::handshake() {
    // === Negotiation Handshake ===
    while (true) {
        char handshake_buf[64];
        ssize_t n = conn.receive(handshake_buf, sizeof(handshake_buf));
        if(n < 0) {
            std::this_thread::sleep_for(milliseconds(RETRY_MS));
            if (debug) std::cout << "No handshake received" << std::endl;
            continue;
        }
        if(n != sizeof(HANDSHAKE) || strcmp(handshake_buf, HANDSHAKE)) {
            std::cerr << "Error: Unexpected handshake message: " << handshake_buf << std::endl;
            continue;
        }
        break;
    }

    if(debug)
        std::cout << "Received handshake from sender. Sending negotiation packet..." << std::endl;
    // Prepare negotiation packet: two shorts (buffer size and packet size) in network order.
    const uint16_t negotiated_buffer_size = 1024;           // example buffer size
    const uint16_t negotiated_packet_size   = DATA_PACKET_SIZE;
    char negotiation_packet[4];
    uint16_t net_buffer_size = htons(negotiated_buffer_size);
    uint16_t net_packet_size = htons(negotiated_packet_size);
    std::memcpy(negotiation_packet, &net_buffer_size, sizeof(uint16_t));
    std::memcpy(negotiation_packet + sizeof(uint16_t), &net_packet_size, sizeof(uint16_t));
    size_t s = conn.send(negotiation_packet, sizeof(negotiation_packet));
    if(s < 0) {
        perror("sendto negotiation packet failed");
        return 1;
    } else if(debug) {
        std::cout << "Negotiation packet sent to sender." << std::endl;
    }
    return 0;
}

template<typename DataProcessorType, typename NetworkConnectionType>
bool StreamReceiver<DataProcessorType, NetworkConnectionType>::StreamReceiver::sendFINACK(uint32_t seq_num) {
    stats.report(true);
    Packet packet;
    for (int i = 0; i < 5; i++) {
        sendACK(seq_num, FLAG_FIN_ACK);

        if (conn.ready({0, SENDER_ACK_WAIT_US * 1000})) {
            ssize_t recv_len = conn.receive(&packet, sizeof(packet));
            if (recv_len >= HEADER_SIZE && packet.header.control_flags == FLAG_ACK && verifyChecksum(&packet, recv_len)) {

                if(debug)
                    std::cout << "Received final ACK." << std::endl;
                return true;
            }
        }
    }
    return false;
}

template<typename DataProcessorType, typename NetworkConnectionType>
int StreamReceiver<DataProcessorType, NetworkConnectionType>::StreamReceiver::teardown() {
    window.clear();
    ackTimes.clear();
    nackTimes.clear();

    conn.close();
    return 0;
}


template<typename DataProcessorType, typename NetworkConnectionType>
bool StreamReceiver<DataProcessorType, NetworkConnectionType>::StreamReceiver::advanceAllWindows(uint32_t seq_num) {
    return window.advanceTo(seq_num) && ackTimes.advanceTo(seq_num) && nackTimes.advanceTo(seq_num);
}

template<typename DataProcessorType, typename NetworkConnectionType>
bool StreamReceiver<DataProcessorType, NetworkConnectionType>::StreamReceiver::processPacket(Packet* packet, ssize_t size) {
    processor.processData(size, packet->data);
    stats.record_packet(size);
    expected_seq++;
    return true;
}


/*

PacketInfo
packet
has_data


How do we know if Packet has no data and never ACKd
- seq_num != (!contains)

How do we know if Packet has data
- seq_num == (contains) && has_data

How do we know if Packet has no data and HAS ACKd
- contains && !has_data



ACKing
- set has_data = false if it didn't has_data

*/