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
#include <fstream>
#include <algorithm>

using namespace std::chrono;

// --- Protocol constants ---
const int PAYLOAD_SIZE       = 512;  // bytes of payload in DATA packets
const int HEADER_SIZE        = sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint16_t); // 9 bytes total
const int DATA_PACKET_SIZE   = HEADER_SIZE + PAYLOAD_SIZE;  // full DATA packet size
const int CTRL_PACKET_SIZE   = HEADER_SIZE;                 // control (ACK/NACK) packets contain only the header

const int WINDOW_SIZE        = 6666;     // sliding window size
const int DEFAULT_MAX_PACKETS = 1000000000;   // default number of packets in dummy mode
const int TIMEOUT_MS         = 1000;   // retransmission timeout (ms)
const int HANDSHAKE_TIMEOUT_MS = 1000; // handshake timeout (ms)

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

// --- Structure to track unacknowledged packets ---
struct PacketInfo {
    std::vector<char> buffer;
    steady_clock::time_point last_sent;
};

int main(int argc, char* argv[]) {
    // --- Parse command line arguments ---
    // Usage: ./StreamSender <receiver_ip> <receiver_port> [filename]
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <receiver_ip> <receiver_port> [filename]" << std::endl;
        return EXIT_FAILURE;
    }
    std::string receiver_ip = argv[1];
    int receiver_port = std::atoi(argv[2]);

    bool file_mode = false;
    std::string filename;
    std::vector<char> file_data;  // will hold the file contents (if in file mode)
    uint32_t max_packets = DEFAULT_MAX_PACKETS;
    if (argc >= 4) {
        file_mode = true;
        filename = argv[3];
        // Open file in binary mode and load its contents.
        std::ifstream infile(filename, std::ios::binary | std::ios::ate);
        if (!infile) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return EXIT_FAILURE;
        }
        std::streamsize filesize = infile.tellg();
        infile.seekg(0, std::ios::beg);
        file_data.resize(filesize);
        if (!infile.read(file_data.data(), filesize)) {
            std::cerr << "Failed to read file: " << filename << std::endl;
            return EXIT_FAILURE;
        }
        // Calculate number of packets needed to stream the file.
        max_packets = static_cast<uint32_t>((filesize + PAYLOAD_SIZE - 1) / PAYLOAD_SIZE);
        std::cout << "Streaming file \"" << filename << "\", size " << filesize
                  << " bytes in " << max_packets << " packets." << std::endl;
    } else {
        std::cout << "Operating in dummy mode: streaming " << DEFAULT_MAX_PACKETS << " dummy packets." << std::endl;
    }

    // --- Create UDP socket ---
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    // Set non-blocking mode (for ACK/NACK processing)
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    // --- Set up receiver address ---
    sockaddr_in receiver_addr;
    std::memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port   = htons(receiver_port);
    if (inet_pton(AF_INET, receiver_ip.c_str(), &receiver_addr.sin_addr) <= 0) {
        perror("Invalid receiver IP address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // === Negotiation handshake ===
    // The sender sends a handshake message ("STREAM_START") and waits for a negotiation packet
    // (4 bytes: two shorts in network order: buffer size and packet size).
    const char* handshake = "STREAM_START";
    auto last_handshake_time = steady_clock::now();

    // Send the initial handshake.
    ssize_t sent_bytes = sendto(sockfd, handshake, strlen(handshake), 0,
                                (sockaddr*)&receiver_addr, sizeof(receiver_addr));
    if (sent_bytes < 0) {
        perror("handshake send failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    std::cout << "Sent handshake message. Waiting for negotiation packet..." << std::endl;

    bool handshake_received = false;
    char neg_buf[4];
    while (!handshake_received) {
        auto now = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now - last_handshake_time).count();
        if (elapsed >= HANDSHAKE_TIMEOUT_MS) {
            // Resend handshake message.
            ssize_t s = sendto(sockfd, handshake, strlen(handshake), 0,
                               (sockaddr*)&receiver_addr, sizeof(receiver_addr));
            if (s < 0)
                perror("handshake resend failed");
            else
                std::cout << "Resent handshake message..." << std::endl;
            last_handshake_time = now;
        }
        ssize_t n = recvfrom(sockfd, neg_buf, sizeof(neg_buf), 0, nullptr, nullptr);
        if (n == 4) {
            handshake_received = true;
            break;
        }
        std::this_thread::sleep_for(milliseconds(10));
    }
    // Parse negotiation packet: first 2 bytes = buffer size, next 2 bytes = packet size.
    uint16_t net_buffer_size, net_packet_size;
    std::memcpy(&net_buffer_size, neg_buf, sizeof(uint16_t));
    std::memcpy(&net_packet_size, neg_buf + sizeof(uint16_t), sizeof(uint16_t));
    uint16_t negotiated_buffer_size = ntohs(net_buffer_size);
    uint16_t negotiated_packet_size = ntohs(net_packet_size);
    std::cout << "Negotiation completed: Buffer size = " << negotiated_buffer_size
              << ", Packet size = " << negotiated_packet_size << std::endl;

    // === End negotiation; now start streaming data packets ===
    std::unordered_map<uint32_t, PacketInfo> window_packets; // key = sequence number
    uint32_t base = 0;      // lowest unacknowledged sequence number
    uint32_t next_seq = 0;  // next sequence number to send

    while (base < max_packets) {
        // Send packets while the sliding window is not full.
        while (next_seq < base + WINDOW_SIZE && next_seq < max_packets) {
            std::vector<char> packet(DATA_PACKET_SIZE, 0);
            PacketHeader header;
            header.seq_num       = htonl(next_seq);
            header.window_size   = htons(WINDOW_SIZE);
            header.control_flags = FLAG_DATA;
            header.checksum      = 0; // Set to zero for checksum computation

            // Copy header into packet buffer.
            std::memcpy(packet.data(), &header, HEADER_SIZE);

            // --- Fill payload: either from the file or dummy data ---
            if (file_mode) {
                // Calculate offset and number of bytes to copy.
                size_t offset = next_seq * PAYLOAD_SIZE;
                size_t bytes_remaining = file_data.size() > offset ? file_data.size() - offset : 0;
                size_t bytes_to_copy = std::min(static_cast<size_t>(PAYLOAD_SIZE), bytes_remaining);
                if (bytes_to_copy > 0) {
                    std::memcpy(packet.data() + HEADER_SIZE, file_data.data() + offset, bytes_to_copy);
                }
                // (If the last packet is smaller than PAYLOAD_SIZE, the remainder stays zero.)
            } else {
                // Dummy data: fill with a repeated letter.
                char fill_char = 'A' + (next_seq % 26);
                std::memset(packet.data() + HEADER_SIZE, fill_char, PAYLOAD_SIZE);
            }

            // Compute checksum over the entire packet.
            uint16_t chksum = compute_checksum(packet.data(), packet.size());
            uint16_t net_chksum = htons(chksum);
            // Write checksum into the header at offset 7 (after 4+2+1 bytes).
            std::memcpy(packet.data() + 7, &net_chksum, sizeof(uint16_t));

            // Send the DATA packet.
            ssize_t sent = sendto(sockfd, packet.data(), packet.size(), 0,
                                  (sockaddr*)&receiver_addr, sizeof(receiver_addr));
            if (sent < 0)
                perror("sendto failed");
//            else
//                std::cout << "Sent DATA packet seq: " << next_seq << std::endl;

            // Save the packet info for potential retransmission.
            PacketInfo info;
            info.buffer    = packet;
            info.last_sent = steady_clock::now();
            window_packets[next_seq] = info;
            next_seq++;
        }

        // --- Process incoming ACK/NACK responses ---
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = 100000; // 100 ms timeout for select()

        int ret = select(sockfd + 1, &readfds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(sockfd, &readfds)) {
            char ctrl_buf[DATA_PACKET_SIZE];
            sockaddr_in sender_addr;
            socklen_t sender_addr_len = sizeof(sender_addr);
            ssize_t recv_len = recvfrom(sockfd, ctrl_buf, sizeof(ctrl_buf), 0,
                                        (sockaddr*)&sender_addr, &sender_addr_len);
            if (recv_len >= HEADER_SIZE) {
                PacketHeader recv_header;
                std::memcpy(&recv_header, ctrl_buf, HEADER_SIZE);
                uint32_t pkt_seq = ntohl(recv_header.seq_num);
                uint16_t recv_window = ntohs(recv_header.window_size);
                uint8_t ctrl_flag = recv_header.control_flags;
                uint16_t recv_checksum = ntohs(recv_header.checksum);

                // Verify checksum.
                std::vector<char> temp_buf(recv_len, 0);
                std::memcpy(temp_buf.data(), ctrl_buf, recv_len);
                std::memset(temp_buf.data() + 7, 0, sizeof(uint16_t));
                uint16_t computed_chksum = compute_checksum(temp_buf.data(), recv_len);
                if (computed_chksum != recv_checksum) {
                    std::cout << "Received control packet with invalid checksum, discarding." << std::endl;
                    continue;
                }
                // Process ACK/NACK.
                if (ctrl_flag == FLAG_ACK) {
//                    std::cout << "Received ACK for seq: " << pkt_seq << std::endl;
                    if (pkt_seq >= base) {
                        for (uint32_t seq = base; seq <= pkt_seq; seq++)
                            window_packets.erase(seq);
                        base = pkt_seq + 1;
                    }
                } else if (ctrl_flag == FLAG_NACK) {
                    std::cout << "Received NACK for seq: " << pkt_seq << std::endl;
                    auto it = window_packets.find(pkt_seq);
                    if (it != window_packets.end()) {
                        ssize_t s = sendto(sockfd, it->second.buffer.data(),
                                           it->second.buffer.size(), 0,
                                           (sockaddr*)&receiver_addr, sizeof(receiver_addr));
                        if (s < 0)
                            perror("Retransmit sendto failed");
                        else {
//                            std::cout << "Retransmitted packet seq: " << pkt_seq << " upon NACK" << std::endl;
                            it->second.last_sent = steady_clock::now();
                        }
                    }
                }
            }
        }

        // --- Check for packets needing retransmission (timeout) ---
        auto now = steady_clock::now();
        for (auto &kv : window_packets) {
            auto elapsed = duration_cast<milliseconds>(now - kv.second.last_sent).count();
            if (elapsed >= TIMEOUT_MS) {
                ssize_t s = sendto(sockfd, kv.second.buffer.data(), kv.second.buffer.size(), 0,
                                   (sockaddr*)&receiver_addr, sizeof(receiver_addr));
                if (s < 0)
                    perror("Timeout retransmit sendto failed");
                else {
                    std::cout << "Timeout retransmitted packet seq: " << kv.first << std::endl;
                    kv.second.last_sent = now;
                }
            }
        }

        std::this_thread::sleep_for(milliseconds(10));
    }

    std::cout << "All packets sent and acknowledged." << std::endl;
    close(sockfd);
    return 0;
}
