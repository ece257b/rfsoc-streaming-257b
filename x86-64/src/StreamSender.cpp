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
#include <string>
#include <sstream>
#include <iterator>

using namespace std::chrono;

// --- Protocol constants ---
const int PAYLOAD_SIZE       = 512;  // bytes of payload in DATA packets
const int HEADER_SIZE        = sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint16_t); // 9 bytes total
const int DATA_PACKET_SIZE   = HEADER_SIZE + PAYLOAD_SIZE;  // full DATA packet size
const int CTRL_PACKET_SIZE   = HEADER_SIZE;                 // control packets contain only header

const int WINDOW_SIZE        = 6666;     // sliding window size
const int DEFAULT_MAX_PACKETS = 10;   // default number of packets in dummy mode
const int TIMEOUT_MS         = 1000;   // retransmission timeout (ms)
const int HANDSHAKE_TIMEOUT_MS = 1000; // handshake timeout (ms)

// --- Control flag definitions ---
enum ControlFlag {
    FLAG_DATA     = 0,
    FLAG_ACK      = 1,
    FLAG_NACK     = 2,
    FLAG_FIN      = 3,
    FLAG_FIN_ACK  = 4
};

// --- Packed packet header ---
#pragma pack(push, 1)
struct PacketHeader {
    uint32_t seq_num;      // sequence number (network order)
    uint16_t window_size;  // window size (network order)
    uint8_t control_flags; // control flag
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
    // --- Command-line parsing ---
    // Usage: ./StreamSender <receiver_ip> <receiver_port> [filename] [--debug] [--statistics]
    bool debug = false;
    bool stats = false;
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if(arg == "--debug") {
            debug = true;
        } else if(arg == "--statistics") {
            stats = true;
        } else {
            args.push_back(arg);
        }
    }
    if(args.size() < 2) {
        std::cerr << "Usage: " << argv[0] << " <receiver_ip> <receiver_port> [filename] [--debug] [--statistics]" << std::endl;
        return EXIT_FAILURE;
    }
    std::string receiver_ip = args[0];
    int receiver_port = std::atoi(args[1].c_str());

    bool file_mode = false;
    std::string filename;
    std::vector<char> file_data;
    uint32_t max_packets = DEFAULT_MAX_PACKETS;
    if(args.size() >= 3) {
        file_mode = true;
        filename = args[2];
        std::ifstream infile(filename, std::ios::binary | std::ios::ate);
        if(!infile) {
            std::cerr << "Error: Failed to open file: " << filename << std::endl;
            return EXIT_FAILURE;
        }
        std::streamsize filesize = infile.tellg();
        infile.seekg(0, std::ios::beg);
        file_data.resize(filesize);
        if (!infile.read(file_data.data(), filesize)) {
            std::cerr << "Error: Failed to read file: " << filename << std::endl;
            return EXIT_FAILURE;
        }
        max_packets = static_cast<uint32_t>((filesize + PAYLOAD_SIZE - 1) / PAYLOAD_SIZE);
        if(debug)
            std::cout << "File mode: streaming \"" << filename << "\" (" << filesize
                      << " bytes in " << max_packets << " packets)" << std::endl;
    } else {
        if(debug)
            std::cout << "Dummy mode: streaming " << DEFAULT_MAX_PACKETS << " dummy packets" << std::endl;
    }

    // --- Create UDP socket ---
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    // Set non-blocking mode.
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

    // === Negotiation Handshake ===
    const char* handshake = "STREAM_START";
    auto last_handshake_time = steady_clock::now();
    ssize_t s = sendto(sockfd, handshake, strlen(handshake), 0,
                       (sockaddr*)&receiver_addr, sizeof(receiver_addr));
    if(s < 0) {
        perror("handshake send failed");
        close(sockfd);
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
            s = sendto(sockfd, handshake, strlen(handshake), 0,
                       (sockaddr*)&receiver_addr, sizeof(receiver_addr));
            if(s < 0)
                perror("handshake resend failed");
            else if(debug)
                std::cout << "Resent handshake message..." << std::endl;
            last_handshake_time = now;
        }
        ssize_t n = recvfrom(sockfd, neg_buf, sizeof(neg_buf), 0, nullptr, nullptr);
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

    // === Data Streaming (Sliding Window) ===
    std::unordered_map<uint32_t, PacketInfo> window_packets;
    uint32_t base = 0;      // lowest unacknowledged sequence number
    uint32_t next_seq = 0;  // next sequence number to send

    // --- Statistics counters for sender ---
    uint64_t sent_data_bytes = 0;
    uint64_t sent_data_packets = 0;
    auto last_stats_time = steady_clock::now();

    while (base < max_packets) {
        // Send packets while the window is not full.
        while (next_seq < base + WINDOW_SIZE && next_seq < max_packets) {
            std::vector<char> packet(DATA_PACKET_SIZE, 0);
            PacketHeader header;
            header.seq_num = htonl(next_seq);
            header.window_size = htons(WINDOW_SIZE);
            header.control_flags = FLAG_DATA;
            header.checksum = 0;
            std::memcpy(packet.data(), &header, HEADER_SIZE);

            // Fill payload either from file data or with dummy content.
            if(file_mode) {
                size_t offset = next_seq * PAYLOAD_SIZE;
                size_t bytes_remaining = (file_data.size() > offset) ? file_data.size() - offset : 0;
                size_t bytes_to_copy = std::min(static_cast<size_t>(PAYLOAD_SIZE), bytes_remaining);
                if(bytes_to_copy > 0) {
                    std::memcpy(packet.data() + HEADER_SIZE, file_data.data() + offset, bytes_to_copy);
                }
            } else {
                char fill_char = 'A' + (next_seq % 26);
                std::memset(packet.data() + HEADER_SIZE, fill_char, PAYLOAD_SIZE);
            }
            uint16_t chksum = compute_checksum(packet.data(), packet.size());
            uint16_t net_chksum = htons(chksum);
            std::memcpy(packet.data() + 7, &net_chksum, sizeof(uint16_t));

            ssize_t sent = sendto(sockfd, packet.data(), packet.size(), 0,
                                  (sockaddr*)&receiver_addr, sizeof(receiver_addr));
            if(sent < 0) {
                perror("sendto failed");
            } else {
                if(debug)
                    std::cout << "Sent DATA packet seq: " << next_seq << std::endl;
                sent_data_bytes += sent;
                sent_data_packets++;
            }

            PacketInfo info;
            info.buffer = packet;
            info.last_sent = steady_clock::now();
            window_packets[next_seq] = info;
            next_seq++;
        }

        // Process incoming ACK/NACK responses.
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100 ms
        int ret = select(sockfd+1, &readfds, nullptr, nullptr, &tv);
        if(ret > 0 && FD_ISSET(sockfd, &readfds)) {
            char ctrl_buf[DATA_PACKET_SIZE];
            sockaddr_in sender_addr;
            socklen_t sender_addr_len = sizeof(sender_addr);
            ssize_t recv_len = recvfrom(sockfd, ctrl_buf, sizeof(ctrl_buf), 0,
                                        (sockaddr*)&sender_addr, &sender_addr_len);
            if(recv_len >= HEADER_SIZE) {
                PacketHeader recv_header;
                std::memcpy(&recv_header, ctrl_buf, HEADER_SIZE);
                uint32_t pkt_seq = ntohl(recv_header.seq_num);
                uint8_t ctrl_flag = recv_header.control_flags;
                // Verify checksum.
                std::vector<char> temp_buf(recv_len, 0);
                std::memcpy(temp_buf.data(), ctrl_buf, recv_len);
                std::memset(temp_buf.data() + 7, 0, sizeof(uint16_t));
                uint16_t computed_chksum = compute_checksum(temp_buf.data(), recv_len);
                uint16_t recv_checksum = ntohs(recv_header.checksum);
                if(computed_chksum != recv_checksum) {
                    if(debug)
                        std::cerr << "Received control packet with invalid checksum, discarding." << std::endl;
                    continue;
                }
                if(ctrl_flag == FLAG_ACK) {
                    if(debug)
                        std::cout << "Received ACK for seq: " << pkt_seq << std::endl;
                    if(pkt_seq >= base) {
                        for(uint32_t seq = base; seq <= pkt_seq; seq++)
                            window_packets.erase(seq);
                        base = pkt_seq + 1;
                    }
                } else if(ctrl_flag == FLAG_NACK) {
                    if(debug)
                        std::cout << "Received NACK for seq: " << pkt_seq << std::endl;
                    auto it = window_packets.find(pkt_seq);
                    if(it != window_packets.end()) {
                        ssize_t s = sendto(sockfd, it->second.buffer.data(),
                                           it->second.buffer.size(), 0,
                                           (sockaddr*)&receiver_addr, sizeof(receiver_addr));
                        if(s < 0)
                            perror("Retransmit sendto failed");
                        else if(debug)
                            std::cout << "Retransmitted packet seq: " << pkt_seq << " upon NACK" << std::endl;
                        it->second.last_sent = steady_clock::now();
                    }
                }
            }
        }

        // Retransmit packets that have timed out.
        auto now = steady_clock::now();
        for(auto &kv : window_packets) {
            auto elapsed = duration_cast<milliseconds>(now - kv.second.last_sent).count();
            if(elapsed >= TIMEOUT_MS) {
                ssize_t s = sendto(sockfd, kv.second.buffer.data(), kv.second.buffer.size(), 0,
                                   (sockaddr*)&receiver_addr, sizeof(receiver_addr));
                if(s < 0)
                    perror("Timeout retransmit sendto failed");
                else if(debug)
//                    std::cout << "Timeout retransmitted packet seq: " << kv.first << std::endl;
                kv.second.last_sent = now;
            }
        }
        std::this_thread::sleep_for(milliseconds(10));

        // --- Statistics reporting (if --statistics provided) ---
        if(stats) {
            auto now = steady_clock::now();
            auto elapsed = duration_cast<seconds>(now - last_stats_time).count();
            if(elapsed >= 1) {
                double bps = sent_data_bytes * 8.0 / elapsed;
                if(bps >= 1e9)
                    std::cout << "[STATISTICS] Throughput: " << (bps / 1e9) << " Gbps, "
                              << "Packets sent: " << sent_data_packets << std::endl;
                else
                    std::cout << "[STATISTICS] Throughput: " << (bps / 1e6) << " Mbps, "
                              << "Packets sent: " << sent_data_packets << std::endl;
                sent_data_bytes = 0;
                sent_data_packets = 0;
                last_stats_time = now;
            }
        }
    } // end while (data streaming)

    // --- FIN handshake for EOF in file mode ---
    if(file_mode) {
        // Build FIN packet.
        PacketHeader fin_header;
        // Use max_packets as the sequence number for FIN.
        fin_header.seq_num = htonl(max_packets);
        fin_header.window_size = htons(WINDOW_SIZE);
        fin_header.control_flags = FLAG_FIN;
        fin_header.checksum = 0;
        std::vector<char> fin_packet(CTRL_PACKET_SIZE, 0);
        std::memcpy(fin_packet.data(), &fin_header, HEADER_SIZE);
        uint16_t fin_chksum = compute_checksum(fin_packet.data(), fin_packet.size());
        uint16_t net_fin_chksum = htons(fin_chksum);
        std::memcpy(fin_packet.data() + 7, &net_fin_chksum, sizeof(uint16_t));

        bool fin_ack_received = false;
        // counter for retransmission of FIN-ACK
        int fin_ack_retransmissions = 0;
        while(!fin_ack_received && fin_ack_retransmissions < 5) {
            ssize_t s = sendto(sockfd, fin_packet.data(), fin_packet.size(), 0,
                               (sockaddr*)&receiver_addr, sizeof(receiver_addr));
            if(s < 0)
                perror("sendto FIN failed");
            else if(debug)
                std::cout << "Sent FIN packet" << std::endl;
            // Wait for FIN-ACK.
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);
            timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            int ret = select(sockfd+1, &readfds, nullptr, nullptr, &tv);
            if(ret > 0 && FD_ISSET(sockfd, &readfds)) {
                char ack_buf[DATA_PACKET_SIZE];
                sockaddr_in ack_addr;
                socklen_t ack_addr_len = sizeof(ack_addr);
                ssize_t r = recvfrom(sockfd, ack_buf, sizeof(ack_buf), 0,
                                     (sockaddr*)&ack_addr, &ack_addr_len);
                if(r >= HEADER_SIZE) {
                    PacketHeader ack_hdr;
                    std::memcpy(&ack_hdr, ack_buf, HEADER_SIZE);
                    if(ack_hdr.control_flags == FLAG_FIN_ACK) {
                        fin_ack_received = true;
                        if(debug)
                            std::cout << "Received FIN-ACK. Closing connection." << std::endl;
                    }
                }
            }
            std::this_thread::sleep_for(milliseconds(100));
            fin_ack_retransmissions++;
        }
        // Send final ACK.
        PacketHeader final_ack;
        final_ack.seq_num = htonl(max_packets);
        final_ack.window_size = htons(WINDOW_SIZE);
        final_ack.control_flags = FLAG_ACK;  // use standard ACK for final acknowledgment
        final_ack.checksum = 0;
        std::vector<char> final_ack_packet(CTRL_PACKET_SIZE, 0);
        std::memcpy(final_ack_packet.data(), &final_ack, HEADER_SIZE);
        uint16_t final_ack_chksum = compute_checksum(final_ack_packet.data(), final_ack_packet.size());
        uint16_t net_final_ack_chksum = htons(final_ack_chksum);
        std::memcpy(final_ack_packet.data() + 7, &net_final_ack_chksum, sizeof(uint16_t));
        ssize_t s2 = sendto(sockfd, final_ack_packet.data(), final_ack_packet.size(), 0,
                            (sockaddr*)&receiver_addr, sizeof(receiver_addr));
        if(s2 < 0)
            perror("sendto final ACK failed");
        else if(debug)
            std::cout << "Sent final ACK for FIN" << std::endl;
    } else {
        if(debug)
            std::cout << "All packets sent and acknowledged." << std::endl;
    }

    close(sockfd);
    return 0;
}
