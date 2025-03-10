#pragma once
#include "Statistics.hpp"
#include "Protocol.hpp"
#include "StreamSender.hpp"
#include "NetworkUtils.hpp"
#include "cmn.h"
#include <chrono>
#include <thread>
#include <cassert>

using namespace std::chrono;

template<typename DataProviderType, typename NetworkConnectionType>
StreamSender<DataProviderType, NetworkConnectionType>::StreamSender(
        DataProviderType&& provider, NetworkConnectionType&& conn, bool debug, uint32_t window_size) 
            : window(window_size), debug(debug), window_size(window_size), conn(std::move(conn)), provider(std::move(provider)) {
    static_assert(std::is_base_of<DataProvider, DataProviderType>::value, "type parameter of this class must derive from DataProvider");
    static_assert(std::is_base_of<NetworkConnection, NetworkConnectionType>::value, "type parameter of this class must derive from NetworkConnection");
}

template<typename DataProviderType, typename NetworkConnectionType>
StreamSender<DataProviderType, NetworkConnectionType>::~StreamSender() {
    
}

template<typename DataProviderType, typename NetworkConnectionType>
int StreamSender<DataProviderType, NetworkConnectionType>::handshake() {
    auto last_handshake_time = steady_clock::now();
    ssize_t s = conn.send((void*) HANDSHAKE, HANDSHAKE_SIZE);

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
            ssize_t s = conn.send((void*) HANDSHAKE, HANDSHAKE_SIZE);
            if(s < 0)
                perror("handshake resend failed");
            else if(debug)
                std::cout << "Resent handshake message..." << std::endl;
            last_handshake_time = now;
        }
        // ssize_t n = recvfrom(sockfd, neg_buf, sizeof(neg_buf), 0, nullptr, nullptr);
        ssize_t n = conn.receive(neg_buf, sizeof(neg_buf)); // ?
        if(n == 4) {
            handshake_received = true;
            break;
        }
        std::this_thread::sleep_for(milliseconds(RETRY_MS));
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
    assert(negotiated_packet_size == DATA_PACKET_SIZE);
    return 0;
}

template<typename DataProviderType, typename NetworkConnectionType>
int StreamSender<DataProviderType, NetworkConnectionType>::stream() {
    conn.open();
    handshake();

    int count = 0;
    uint32_t final_seq = 0;
    bool done_streaming = false;
    while (base < max_packets) {
        while (next_seq < base + window_size && next_seq < max_packets && !done_streaming) {
            PacketInfo* info = preparePacket(next_seq);
            if (info == nullptr) {
                done_streaming = true;
                final_seq = next_seq;
                break;  // No data left, done streaming!
            }
            sendPacket(info);
            next_seq++;
        }

        processACKs();

        // Send all timed out packets
        auto now = steady_clock::now();
        for (uint32_t i = base; i < base + window_size; i++) {
            PacketInfo* info = window.get(i);
            if (info) {
                auto elapsed = std::chrono::duration_cast<milliseconds>(now - info->last_sent);
                if (elapsed.count() >= TIMEOUT_MS) {
                    sendPacket(info);
                }
            } else {
                if (debug) std::cout << "WARNING Didn't find " << i << " in window, base=" << base << std::endl;
                break;
            }
        }

        if (base == final_seq && done_streaming) {
            // We have received an ACK for final seq, and we don't have any data left to stream.
            break;
        }
        
        std::this_thread::sleep_for(microseconds(SENDER_STREAMING_WAIT_US));

        stats.report();
    }
    return count;
}

template<typename DataProviderType, typename NetworkConnectionType>
PacketInfo* StreamSender<DataProviderType, NetworkConnectionType>::preparePacket(uint32_t seq_num) {
    PacketInfo* info = window.reserve(seq_num);
    Packet* packet = &info->packet;
    PacketHeader* header = &packet->header;
    char* dataBuffer = packet->data;

    header->seq_num = htonl(seq_num);
    header->window_size = htons(window_size);
    header->control_flags = FLAG_DATA;
    header->checksum = 0;

    size_t size = provider.getData(PAYLOAD_SIZE, dataBuffer);
    if (size == 0) {
        window.erase(seq_num);
        return nullptr; // No data left! Done streaming.
    }
    info->data_size = size;

    uint16_t chksum = compute_checksum(packet, info->packet_size());
    uint16_t net_chksum = htons(chksum);
    header->checksum = net_chksum;

    return info;
}

template<typename DataProviderType, typename NetworkConnectionType>
int StreamSender<DataProviderType, NetworkConnectionType>::sendPacket(PacketInfo* info) {
    ssize_t sent = conn.send(&info->packet, info->packet_size());

    if(sent < 0) {
        perror("sendto failed");
    } else {
        if(debug)
            std::cout << "Sent DATA packet seq: " << ntohl(info->packet.header.seq_num) << " Len: " << sent << std::endl;
        stats.record_packet(sent);
    }

    info->last_sent = steady_clock::now();
    return info->packet_size();
}


template<typename DataProviderType, typename NetworkConnectionType>
int StreamSender<DataProviderType, NetworkConnectionType>::processACKs() {
    // Process incoming ACK/NACK responses.
    timeval delay = {0, SENDER_ACK_WAIT_US}; // Wait long for the first ACK.
    while (conn.ready(delay)) {
        Packet packet;
        ssize_t recv_len = conn.receive(&packet, sizeof(packet));
        if(recv_len >= HEADER_SIZE) {
            uint32_t pkt_seq = ntohl(packet.header.seq_num);
            uint8_t ctrl_flag = packet.header.control_flags;
            // Verify checksum.
            if(!verifyChecksum(&packet, recv_len)) {
                if(debug) std::cerr << "Received control packet with invalid checksum, discarding." << std::endl;
                stats.record_corrupted();
            }

            if(ctrl_flag == FLAG_ACK) {
                if(debug) std::cout << "Received ACK for seq: " << pkt_seq << std::endl;
                stats.record_ack();
                if(pkt_seq >= base) {
                    window.advanceTo(pkt_seq);
                    base = pkt_seq;
                } else {
                    stats.record_ignored();
                }
            } else if(ctrl_flag == FLAG_NACK) {
                if(debug) std::cout << "Received NACK for seq: " << pkt_seq << std::endl;
                stats.record_ack(FLAG_NACK);
                PacketInfo* info = window.get(pkt_seq);
                if (info) {
                    auto now = steady_clock::now();
                    auto elapsed = duration_cast<milliseconds>(now - info->last_sent).count();
                    if (!info->retried || elapsed > RETRY_MS) {     
                        // If we haven't retried this packet from a NACK already OR we did a while ago, resend it.
                        sendPacket(info);
                        info->retried = true;
                    } else {
                        if (debug) std::cout << "Not retrying yet..." << std::endl;
                        stats.record_ignored();
                    }
                } else {
                    std::cerr << "FATAL ERROR: Window did not have NACKd packet " << pkt_seq << std::endl;
                }
            } else {
                if (debug) {
                    std::cout << "Unexpected Flag" << std::endl;
                }
            }
        }
        delay = {0, SENDER_SUBSEQUENT_ACK_WAIT_US};  // Don't wait long for subsequent ACKs
    }
    return true;
}

template<typename DataProviderType, typename NetworkConnectionType>
int StreamSender<DataProviderType, NetworkConnectionType>::teardown() {
    stats.report(true);
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
        std::this_thread::sleep_for(milliseconds(RETRY_MS));
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

template<typename DataProviderType, typename NetworkConnectionType>
void StreamSender<DataProviderType, NetworkConnectionType>::prepareFINPacket(
        PacketHeader* header, ControlFlag flag) {
    header->seq_num = htonl(max_packets);
    header->window_size = htons(window_size);
    header->control_flags = flag;
    header->checksum = 0;
    
    uint16_t chksum = compute_checksum(header, CTRL_PACKET_SIZE);
    header->checksum = htons(chksum);
}