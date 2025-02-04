// StreamSender.cpp
#include <iostream>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std::chrono;

// --- Protocol constants ---
const int PAYLOAD_SIZE       = 512;  // bytes of payload in DATA packets
const int HEADER_SIZE        = sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint16_t); // 4+2+1+2 = 9 bytes
const int DATA_PACKET_SIZE   = HEADER_SIZE + PAYLOAD_SIZE;  // full DATA packet size
const int CTRL_PACKET_SIZE   = HEADER_SIZE;                 // control (ACK/NACK) packets contain only the header

const int WINDOW_SIZE        = 5;     // sliding window size (default)
const uint32_t MAX_PACKETS   = 1000;   // total number of packets to send
const int TIMEOUT_MS         = 1000;   // retransmission timeout in milliseconds

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
    uint16_t checksum;     // 16-bit checksum (network order)
};
#pragma pack(pop)

// --- Simple Internet checksum (RFC1071 style) ---
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

// --- Structure to track each unacknowledged packet ---
struct PacketInfo {
    std::vector<char> buffer;
    steady_clock::time_point last_sent;
};

int main(int argc, char* argv[]) {
    // --- Get receiver IP and port from command line (defaults: 127.0.0.1:12345) ---
    std::string receiver_ip = "127.0.0.1";
    int receiver_port = 12345;
    if(argc >= 2)
        receiver_ip = argv[1];
    if(argc >= 3)
        receiver_port = std::atoi(argv[2]);

    // --- Create UDP socket ---
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    // Set non-blocking mode (for later ACK/NACK processing)
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    // --- Set up receiver address ---
    sockaddr_in receiver_addr;
    std::memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port   = htons(receiver_port);
    if(inet_pton(AF_INET, receiver_ip.c_str(), &receiver_addr.sin_addr) <= 0) {
        perror("Invalid receiver IP address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // === Negotiation handshake ===
    // First, send a handshake message so the receiver learns our address.
    const char* handshake = "STREAM_START";
    ssize_t sent_bytes = sendto(sockfd, handshake, strlen(handshake), 0,
                                (sockaddr*)&receiver_addr, sizeof(receiver_addr));
    if(sent_bytes < 0) {
        perror("handshake sendto failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    std::cout << "Sent handshake message. Waiting for negotiation packet..." << std::endl;

    // Now wait for a 4-byte negotiation packet.
    char neg_buf[4];
    while (true) {
        ssize_t n = recvfrom(sockfd, neg_buf, sizeof(neg_buf), 0, nullptr, nullptr);
        if(n == 4) {
            break; // Negotiation packet received.
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    // Parse negotiation packet: first 2 bytes = buffer size, next 2 bytes = packet size (both in network order)
    uint16_t net_buffer_size, net_packet_size;
    std::memcpy(&net_buffer_size, neg_buf, sizeof(uint16_t));
    std::memcpy(&net_packet_size, neg_buf + sizeof(uint16_t), sizeof(uint16_t));
    uint16_t negotiated_buffer_size = ntohs(net_buffer_size);
    uint16_t negotiated_packet_size = ntohs(net_packet_size);
    std::cout << "Negotiation completed: Buffer size = " << negotiated_buffer_size
              << ", Packet size = " << negotiated_packet_size << std::endl;

    // === End negotiation; now start streaming data packets ===

    // Data structures for sliding window.
    std::unordered_map<uint32_t, PacketInfo> window_packets; // key = sequence number
    uint32_t base = 0;      // lowest unacknowledged sequence number
    uint32_t next_seq = 0;  // next sequence number to send

    // Main loop: send all packets and process ACK/NACK responses.
    while(base < MAX_PACKETS) {
        // Send packets while the window is not full.
        while(next_seq < base + WINDOW_SIZE && next_seq < MAX_PACKETS) {
            std::vector<char> packet(DATA_PACKET_SIZE, 0);
            PacketHeader header;
            header.seq_num       = htonl(next_seq);
            header.window_size   = htons(WINDOW_SIZE);
            header.control_flags = FLAG_DATA;
            header.checksum      = 0; // Set to zero for checksum computation.

            // Copy header into packet buffer.
            std::memcpy(packet.data(), &header, HEADER_SIZE);
            // Fill payload with dummy data (e.g., repeated letter).
            char fill_char = 'A' + (next_seq % 26);
            std::memset(packet.data() + HEADER_SIZE, fill_char, PAYLOAD_SIZE);

            // Compute checksum over entire packet.
            uint16_t chksum = compute_checksum(packet.data(), packet.size());
            uint16_t net_chksum = htons(chksum);
            // Write checksum at offset 7 (after 4+2+1 bytes).
            std::memcpy(packet.data() + 7, &net_chksum, sizeof(uint16_t));

            // Send the DATA packet.
            ssize_t sent = sendto(sockfd, packet.data(), packet.size(), 0,
                                  (sockaddr*)&receiver_addr, sizeof(receiver_addr));
            if(sent < 0)
                perror("sendto failed");
            else
                std::cout << "Sent DATA packet seq: " << next_seq << std::endl;

            // Save the packet info for potential retransmission.
            PacketInfo info;
            info.buffer    = packet;
            info.last_sent = steady_clock::now();
            window_packets[next_seq] = info;
            next_seq++;
        }

        // Wait (using select) for incoming ACK/NACK responses.
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = 100000; // 100 ms

        int ret = select(sockfd + 1, &readfds, nullptr, nullptr, &tv);
        if(ret > 0 && FD_ISSET(sockfd, &readfds)) {
            // Read response.
            char ctrl_buf[DATA_PACKET_SIZE];
            sockaddr_in sender_addr;
            socklen_t sender_addr_len = sizeof(sender_addr);
            ssize_t recv_len = recvfrom(sockfd, ctrl_buf, sizeof(ctrl_buf), 0,
                                        (sockaddr*)&sender_addr, &sender_addr_len);
            if(recv_len >= HEADER_SIZE) {
                // Parse header.
                PacketHeader recv_header;
                std::memcpy(&recv_header, ctrl_buf, HEADER_SIZE);
                uint32_t pkt_seq = ntohl(recv_header.seq_num);
                uint16_t recv_window = ntohs(recv_header.window_size);
                uint8_t ctrl_flag = recv_header.control_flags;
                uint16_t recv_checksum = ntohs(recv_header.checksum);

                // Verify checksum: zero out checksum field in a temporary copy.
                std::vector<char> temp_buf(recv_len, 0);
                std::memcpy(temp_buf.data(), ctrl_buf, recv_len);
                std::memset(temp_buf.data() + 7, 0, sizeof(uint16_t));
                uint16_t computed_chksum = compute_checksum(temp_buf.data(), recv_len);
                if(computed_chksum != recv_checksum) {
                    std::cout << "Received control packet with invalid checksum, discarding." << std::endl;
                    continue;
                }
                // Process ACK/NACK messages.
                if(ctrl_flag == FLAG_ACK) {
                    std::cout << "Received ACK for seq: " << pkt_seq << std::endl;
                    // Cumulative ACK: remove all packets with seq ≤ pkt_seq.
                    if(pkt_seq >= base) {
                        for(uint32_t seq = base; seq <= pkt_seq; seq++)
                            window_packets.erase(seq);
                        base = pkt_seq + 1;
                    }
                }
                else if(ctrl_flag == FLAG_NACK) {
                    std::cout << "Received NACK for seq: " << pkt_seq << std::endl;
                    // Immediately retransmit the requested packet (if still pending).
                    auto it = window_packets.find(pkt_seq);
                    if(it != window_packets.end()) {
                        ssize_t s = sendto(sockfd, it->second.buffer.data(),
                                           it->second.buffer.size(), 0,
                                           (sockaddr*)&receiver_addr, sizeof(receiver_addr));
                        if(s < 0)
                            perror("Retransmit sendto failed");
                        else {
                            std::cout << "Retransmitted packet seq: " << pkt_seq << " upon NACK" << std::endl;
                            it->second.last_sent = steady_clock::now();
                        }
                    }
                }
            }
        }

        // Check for packets needing retransmission based on timeout.
        auto now = steady_clock::now();
        for(auto &kv : window_packets) {
            auto elapsed = duration_cast<milliseconds>(now - kv.second.last_sent).count();
            if(elapsed >= TIMEOUT_MS) {
                ssize_t s = sendto(sockfd, kv.second.buffer.data(), kv.second.buffer.size(), 0,
                                   (sockaddr*)&receiver_addr, sizeof(receiver_addr));
                if(s < 0)
                    perror("Timeout retransmit sendto failed");
                else {
                    std::cout << "Timeout retransmitted packet seq: " << kv.first << std::endl;
                    kv.second.last_sent = now;
                }
            }
        }

        // Short sleep to avoid busy-waiting.
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "All packets sent and acknowledged." << std::endl;
    close(sockfd);
    return 0;
}
