#pragma once
#include "Statistics.hpp"
#include "Protocol.hpp"
#include "StreamSender.hpp"
#include "NetworkUtils.hpp"
#include <chrono>
#include <thread>
#include <cassert>

using namespace std::chrono;

template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
StreamSender<DataProviderType, DataWindowType, NetworkConnectionType>::StreamSender(bool debug) : debug(debug) {
    static_assert(std::is_base_of<DataProvider, DataProviderType>::value, "type parameter of this class must derive from DataProvider");
    static_assert(std::is_base_of<DataWindow<PacketInfo>, DataWindowType>::value, "type parameter of this class must derive from DataWindow<PacketInfo>");
    static_assert(std::is_base_of<NetworkConnection, NetworkConnectionType>::value, "type parameter of this class must derive from NetworkConnection");
}

template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
StreamSender<DataProviderType, DataWindowType, NetworkConnectionType>::~StreamSender() {
    
}

template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
int StreamSender<DataProviderType, DataWindowType, NetworkConnectionType>::handshake() {
    const char* handshake = "STREAM_START";
    auto last_handshake_time = steady_clock::now();
    ssize_t s = conn.send(handshake, strlen(handshake));

    if(s < 0) {
        perror("handshake send failed");
        conn.close();
        exit(EXIT_FAILURE);
    }
    if(debug)
        std::cout << "Sent handshake message. Waiting for negotiation packet..." << std::endl;

    bool handshake_received = false;
    char neg_buf[4];
    while (!handshake_received) {
        auto now = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now - last_handshake_time).count();
        if(elapsed >= HANDSHAKE_TIMEOUT_MS) {
            s = conn.send(handshake, strlen(handshake));
            if(s < 0)
                perror("handshake resend failed");
            else if(debug)
                std::cout << "Resent handshake message..." << std::endl;
            last_handshake_time = now;
        }
        // ssize_t n = recvfrom(sockfd, neg_buf, sizeof(neg_buf), 0, nullptr, nullptr);
        ssize_t n = conn.receive(neg_buf, neg_buf); // ?
        if(n == 4) {
            handshake_received = true;
            break;
        }
        std::this_thread::sleep_for(milliseconds(10));
    }
    // Parse negotiation packet: two shorts (buffer size, packet size) in network order.
    uint16_t net_buffer_size, net_packet_size;
    std::memcpy(&net_buffer_size, neg_buf, sizeof(uint16_t));
    std::memcpy(&net_packet_size, neg_buf + sizeof(uint16_t), sizeof(uint16_t));
    uint16_t negotiated_buffer_size = ntohs(net_buffer_size);
    uint16_t negotiated_packet_size = ntohs(net_packet_size);
    if(debug)
        std::cout << "Negotiation completed: Buffer size = " << negotiated_buffer_size
                  << ", Packet size = " << negotiated_packet_size << std::endl;
    
    assert(negotiated_buffer_size == BUFFER_SIZE);
    assert(negotiated_packet_size == PACKET_SIZE);
}

template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
int StreamSender<DataProviderType, DataWindowType, NetworkConnectionType>::stream() {
    while (base < max_packets) {
        while (next_seq < base + WINDOW_SIZE && next_seq < max_packets) {
            sendPacket(preparePacket(next_seq));
            next_seq++;
        }

        processACKs();

        auto now = steady_clock::now();
        if (!window.isEmpty()) {
            window.resetIter();
            do {
                PacketInfo* info = window.getIter();
                auto elapsed = std::chrono::duration_cast<milliseconds>(now - info->last_sent);
                if (elapsed.count() >= TIMEOUT_MS) {
                    sendPacket(info);
                }
            } while (window.nextIter());  // Iterate through entire window
        }
        
        std::this_thread::sleep_for(milliseconds(10));

        stats.report();
    }
}

template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
PacketInfo* StreamSender<DataProviderType, DataWindowType, NetworkConnectionType>::preparePacket(uint32_t seq_num) {
    PacketInfo* info = window.reserve(seq_num);
    Packet* packet = &info->packet;
    PacketHeader* header = &packet->header;
    char* dataBuffer = packet->data;

    header->seq_num = htonl(seq_num);
    header->window_size = htons(WINDOW_SIZE);
    header->control_flags = FLAG_DATA;
    header->checksum = 0;

    size_t size = provider.getData(PAYLOAD_SIZE, dataBuffer);
    info->packet_size = size;

    uint16_t chksum = compute_checksum(packet, DATA_PACKET_SIZE);
    uint16_t net_chksum = htons(chksum);
    header->checksum = net_chksum;

    return info;
}

template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
int StreamSender<DataProviderType, DataWindowType, NetworkConnectionType>::sendPacket(PacketInfo* info) {
    ssize_t sent = conn.send(&info->packet, info->packet_size);

    if(sent < 0) {
        perror("sendto failed");
    } else {
        if(debug)
            std::cout << "Sent DATA packet seq: " << next_seq << std::endl;
        stats.record_packet(sent);
    }

    info->last_sent = steady_clock::now();
    return info->packet_size;
}


template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
int StreamSender<DataProviderType, DataWindowType, NetworkConnectionType>::processACKs() {
    // Process incoming ACK/NACK responses.
    if (conn.ready({0, 100000})) {
        Packet packet;
        ssize_t recv_len = conn.receive(&packet, sizeof(packet));
        if(recv_len >= HEADER_SIZE) {
            uint32_t pkt_seq = ntohl(packet.header.seq_num);
            uint8_t ctrl_flag = packet.header.control_flags;
            // Verify checksum.
            if(!verifyChecksum(&packet, recv_len)) {
                if(debug) std::cerr << "Received control packet with invalid checksum, discarding." << std::endl;
                // continue;
            }

            if(ctrl_flag == FLAG_ACK) {
                if(debug) std::cout << "Received ACK for seq: " << pkt_seq << std::endl;
                if(pkt_seq >= base) {
                    for(uint32_t seq = base; seq <= pkt_seq; seq++)
                        window.erase(seq);
                    base = pkt_seq + 1;
                }
            } else if(ctrl_flag == FLAG_NACK) {
                if(debug) std::cout << "Received NACK for seq: " << pkt_seq << std::endl;

                PacketInfo* info = window.get(pkt_seq);
                if (info) {
                    sendPacket(info);
                } else {
                    std::cout << "FATAL ERROR: Window did not have NACKd packet " << pkt_seq << std::endl;
                }
            } else {
                if (debug) {
                    std::cout << "Something wrong!" << std::endl;
                }
            }
        }
    }
}

template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
int StreamSender<DataProviderType, DataWindowType, NetworkConnectionType>::teardown() {
    PacketHeader header;
    prepareFINPacket(&header, FLAG_FIN);

    bool fin_ack_received = false;
    // counter for retransmission of FIN-ACK
    int fin_ack_retransmissions = 0;
    while(!fin_ack_received && fin_ack_retransmissions < 5) {
        ssize_t s = conn.send(&header, sizeof(header));
        if(s < 0)
            perror("sendto FIN failed");
        else if(debug)
            std::cout << "Sent FIN packet" << std::endl;
        
        if (conn.ready({1, 0})) {
            char ack_buf[DATA_PACKET_SIZE];
            ssize_t r = conn.receive(ack_buf, sizeof(ack_buf));
            if(r >= HEADER_SIZE) {
                PacketHeader* ack_hdr = reinterpret_cast<PacketHeader*>(ack_buf);
                if(ack_hdr->control_flags == FLAG_FIN_ACK) {
                    fin_ack_received = true;
                    if(debug)
                        std::cout << "Received FIN-ACK. Closing connection." << std::endl;
                }
            }
        }
        std::this_thread::sleep_for(milliseconds(100));
        fin_ack_retransmissions++;
    }
    PacketHeader finHeader;
    prepareFINPacket(&finHeader, FLAG_ACK);
    ssize_t s2 = conn.send(&finHeader, sizeof(PacketHeader));
    if(s2 < 0)
        perror("sendto final ACK failed");
    else if(debug)
        std::cout << "Sent final ACK for FIN" << std::endl;

    conn.close();
    return 0;
}

template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
void StreamSender<DataProviderType, DataWindowType, NetworkConnectionType>::prepareFINPacket(
        PacketHeader* header, ControlFlag flag) {
    header->seq_num = htonl(max_packets);
    header->window_size = htons(WINDOW_SIZE);
    header->control_flags = flag;
    header->checksum = 0;
    
    uint16_t chksum = compute_checksum(header, CTRL_PACKET_SIZE);
    header->checksum = htons(chksum);
}