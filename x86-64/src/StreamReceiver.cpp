// StreamReceiver.cpp
#include <iostream>
#include <chrono>
#include <thread>
#include <map>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std::chrono;

// --- Protocol constants (must match sender) ---
const int PAYLOAD_SIZE       = 512;
const int HEADER_SIZE        = sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint16_t); // 9 bytes
const int DATA_PACKET_SIZE   = HEADER_SIZE + PAYLOAD_SIZE;
const int CTRL_PACKET_SIZE   = HEADER_SIZE;

// --- Control flag definitions ---
enum ControlFlag {
    FLAG_DATA = 0,
    FLAG_ACK  = 1,
    FLAG_NACK = 2
};

// --- Packed packet header ---
#pragma pack(push, 1)
struct PacketHeader {
    uint32_t seq_num;      // sequence number (network order)
    uint16_t window_size;  // window size (network order)
    uint8_t control_flags; // 0 = DATA, 1 = ACK, 2 = NACK
    uint16_t checksum;     // checksum (network order)
};
#pragma pack(pop)

// --- Simple Internet checksum ---
uint16_t compute_checksum(const void* data, size_t len) {
    uint32_t sum = 0;
    const uint16_t* ptr = reinterpret_cast<const uint16_t*>(data);
    while(len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    if(len > 0)
        sum += *(const uint8_t*)ptr;
    while(sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return static_cast<uint16_t>(~sum);
}

int main(int argc, char* argv[]) {
    // --- Default listen port ---
    int listen_port = 12345;
    bool debug = false;

    // --- Parse command line arguments ---
    // Usage: ./StreamReceiver [port] [--debug]
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if(arg == "--debug") {
            debug = true;
        } else {
            listen_port = std::atoi(argv[i]);
        }
    }

    // --- Create UDP socket and bind it ---
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port        = htons(listen_port);
    if(bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    std::cout << "Receiver listening on port " << listen_port
              << (debug ? " with DEBUG enabled." : ".") << std::endl;

    // === Negotiation handshake ===
    // Wait for the sender's handshake message.
    char handshake_buf[64];
    sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    ssize_t n = recvfrom(sockfd, handshake_buf, sizeof(handshake_buf)-1, 0,
                         (sockaddr*)&sender_addr, &sender_addr_len);
    if(n < 0) {
        perror("recvfrom handshake failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    handshake_buf[n] = '\0'; // Null-terminate.
    if(std::string(handshake_buf) != "STREAM_START") {
        std::cout << "Unexpected handshake message: " << handshake_buf << std::endl;
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    std::cout << "Received handshake from sender. Sending negotiation packet..." << std::endl;
    // Prepare negotiation packet: two shorts (buffer size and packet size) in network order.
    const uint16_t negotiated_buffer_size = 1024;           // Example buffer size.
    const uint16_t negotiated_packet_size   = DATA_PACKET_SIZE; // Negotiated packet size.
    char negotiation_packet[4];
    uint16_t net_buffer_size = htons(negotiated_buffer_size);
    uint16_t net_packet_size = htons(negotiated_packet_size);
    std::memcpy(negotiation_packet, &net_buffer_size, sizeof(uint16_t));
    std::memcpy(negotiation_packet + sizeof(uint16_t), &net_packet_size, sizeof(uint16_t));
    ssize_t sent = sendto(sockfd, negotiation_packet, sizeof(negotiation_packet), 0,
                          (sockaddr*)&sender_addr, sender_addr_len);
    if(sent < 0) {
        perror("sendto negotiation packet failed");
    } else {
        std::cout << "Negotiation packet sent to sender." << std::endl;
    }

    // === End negotiation; now proceed to receive DATA packets ===
    uint32_t expected_seq = 0;
    // Map for storing out-of-order packets.
    std::map<uint32_t, std::vector<char>> out_of_order;

    // --- Debug counters (if debug mode is enabled) ---
    uint64_t debug_data_bytes = 0;
    uint64_t debug_data_packets = 0;
    uint64_t debug_ack_sent = 0;
    uint64_t debug_nack_sent = 0;
    auto last_debug_time = steady_clock::now();

    while (true) {
        char buffer[DATA_PACKET_SIZE];
        sockaddr_in curr_sender_addr;
        socklen_t curr_sender_addr_len = sizeof(curr_sender_addr);
        ssize_t recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                    (sockaddr*)&curr_sender_addr, &curr_sender_addr_len);
        if(recv_len < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else if(recv_len < HEADER_SIZE) {
            std::cout << "Received packet too small, ignoring." << std::endl;
            continue;
        } else {
            PacketHeader header;
            std::memcpy(&header, buffer, HEADER_SIZE);
            uint32_t seq_num     = ntohl(header.seq_num);
            uint16_t pkt_window  = ntohs(header.window_size);
            uint8_t ctrl_flag    = header.control_flags;
            uint16_t recv_checksum = ntohs(header.checksum);

            // Verify checksum.
            std::vector<char> temp_buf(recv_len, 0);
            std::memcpy(temp_buf.data(), buffer, recv_len);
            std::memset(temp_buf.data() + 7, 0, sizeof(uint16_t));
            uint16_t computed_checksum = compute_checksum(temp_buf.data(), recv_len);
            if(computed_checksum != recv_checksum) {
                std::cout << "Invalid checksum for packet seq: " << seq_num << ", discarding." << std::endl;
                continue;
            }

            // Process only DATA packets.
            if(ctrl_flag == FLAG_DATA) {
                if(debug) {
                    debug_data_bytes += recv_len;
                    debug_data_packets++;
                }
//                std::cout << "Received DATA packet seq: " << seq_num << std::endl;
                if(seq_num == expected_seq) {
//                    std::cout << "Processing packet seq: " << seq_num << std::endl;
                    expected_seq++;
                    // Process any buffered in-order packets.
                    while(out_of_order.count(expected_seq)) {
                        std::cout << "Processing buffered packet seq: " << expected_seq << std::endl;
                        out_of_order.erase(expected_seq);
                        expected_seq++;
                    }
                }
                else if(seq_num > expected_seq) {
                    if(out_of_order.find(seq_num) == out_of_order.end()) {
                        std::vector<char> pkt(buffer, buffer + recv_len);
                        out_of_order[seq_num] = pkt;
                        std::cout << "Stored out-of-order packet seq: " << seq_num << std::endl;
                    }
                    // Send NACKs for each missing packet.
                    for(uint32_t missing = expected_seq; missing < seq_num; missing++) {
                        std::vector<char> nack_packet(CTRL_PACKET_SIZE, 0);
                        PacketHeader nack_header;
                        nack_header.seq_num      = htonl(missing);
                        nack_header.window_size  = htons(pkt_window);
                        nack_header.control_flags = FLAG_NACK;
                        nack_header.checksum     = 0;
                        std::memcpy(nack_packet.data(), &nack_header, HEADER_SIZE);
                        uint16_t nack_chksum = compute_checksum(nack_packet.data(), nack_packet.size());
                        uint16_t net_nack_chksum = htons(nack_chksum);
                        std::memcpy(nack_packet.data() + 7, &net_nack_chksum, sizeof(uint16_t));
                        ssize_t sent_bytes = sendto(sockfd, nack_packet.data(), nack_packet.size(), 0,
                                                    (sockaddr*)&curr_sender_addr, curr_sender_addr_len);
                        if(sent_bytes < 0)
                            perror("sendto NACK failed");
                        else {
                            std::cout << "Sent NACK for missing seq: " << missing << std::endl;
                            if(debug) debug_nack_sent++;
                        }
                    }
                }
                else {
                    std::cout << "Duplicate packet seq: " << seq_num << " ignored." << std::endl;
                    // Send NACK for first missing packet.
                    std::vector<char> nack_packet(CTRL_PACKET_SIZE, 0);
                    PacketHeader nack_header;
                    nack_header.seq_num      = htonl(expected_seq);
                    nack_header.window_size  = htons(pkt_window);
                    nack_header.control_flags = FLAG_NACK;
                    nack_header.checksum     = 0;
                    std::memcpy(nack_packet.data(), &nack_header, HEADER_SIZE);
                    uint16_t nack_chksum = compute_checksum(nack_packet.data(), nack_packet.size());
                    uint16_t net_nack_chksum = htons(nack_chksum);
                    std::memcpy(nack_packet.data() + 7, &net_nack_chksum, sizeof(uint16_t));
                    ssize_t sent_bytes = sendto(sockfd, nack_packet.data(), nack_packet.size(), 0,
                                                (sockaddr*)&curr_sender_addr, curr_sender_addr_len);
                    if(sent_bytes < 0)
                        perror("sendto NACK failed");
                    else {
                        std::cout << "Sent NACK for duplicate seq: " << expected_seq << std::endl;
                        if(debug) debug_nack_sent++;
                    }
                }

                // --- Send ACK only at the end of a sliding window ---
                // We send a cumulative ACK only when the number of in-order packets delivered
                // (expected_seq) is a multiple of the window size.
                if (expected_seq > 0 && (expected_seq % pkt_window == 0)) {
                    uint32_t ack_seq = expected_seq - 1;
                    std::vector<char> ack_packet(CTRL_PACKET_SIZE, 0);
                    PacketHeader ack_header;
                    ack_header.seq_num      = htonl(ack_seq);
                    ack_header.window_size  = htons(pkt_window);
                    ack_header.control_flags = FLAG_ACK;
                    ack_header.checksum     = 0;
                    std::memcpy(ack_packet.data(), &ack_header, HEADER_SIZE);
                    uint16_t ack_chksum = compute_checksum(ack_packet.data(), ack_packet.size());
                    uint16_t net_ack_chksum = htons(ack_chksum);
                    std::memcpy(ack_packet.data() + 7, &net_ack_chksum, sizeof(uint16_t));
                    ssize_t sent_bytes = sendto(sockfd, ack_packet.data(), ack_packet.size(), 0,
                                                (sockaddr*)&curr_sender_addr, curr_sender_addr_len);
                    if(sent_bytes < 0)
                        perror("sendto ACK failed");
                    else {
//                        std::cout << "Sent ACK for seq: " << ack_seq << std::endl;
                        if(debug) debug_ack_sent++;
                    }
                }
            }
            // Ignore non-DATA packets.
        } // end processing received packet

        // --- Debug reporting: every second ---
        if(debug) {
            auto now = steady_clock::now();
            auto elapsed = duration_cast<seconds>(now - last_debug_time);
            if(elapsed.count() >= 1) {
                double bps = debug_data_bytes * 8.0 / elapsed.count();
                if(bps >= 1e9) {
                    double gbps = bps / 1e9;
                    std::cout << "[DEBUG] Throughput: " << gbps << " GB/s, "
                              << "DATA packets: " << debug_data_packets << ", "
                              << "ACKs sent: " << debug_ack_sent << ", "
                              << "NACKs sent: " << debug_nack_sent << std::endl;
                } else {
                    double mbps = bps / 1e6;
                    std::cout << "[DEBUG] Throughput: " << mbps << " MB/s, "
                              << "DATA packets: " << debug_data_packets << ", "
                              << "ACKs sent: " << debug_ack_sent << ", "
                              << "NACKs sent: " << debug_nack_sent << std::endl;
                }
                debug_data_bytes = 0;
                debug_data_packets = 0;
                debug_ack_sent = 0;
                debug_nack_sent = 0;
                last_debug_time = now;
            }
        }
    } // end while(true)

    close(sockfd);
    return 0;
}
