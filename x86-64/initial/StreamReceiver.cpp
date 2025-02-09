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
#include <string>
#include <sstream>
#include <iterator>

// For Random Error
#include <random>

using namespace std::chrono;

// --- Protocol constants (must match sender) ---
const int PAYLOAD_SIZE       = 512;
const int HEADER_SIZE        = sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint16_t); // 9 bytes
const int DATA_PACKET_SIZE   = HEADER_SIZE + PAYLOAD_SIZE;
const int CTRL_PACKET_SIZE   = HEADER_SIZE;

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
    // --- Command-line parsing ---
    // Usage: ./StreamReceiver [port] [--debug] [--statistics]
    bool debug = false;
    bool stats = false;
    bool random_error = false;
    // set up random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000);
    int listen_port = 12345;
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if(arg == "--debug") {
            debug = true;
        } else if(arg == "--statistics") {
            stats = true;
        }
        else if(arg == "--random-error") {
            random_error = true; // simulate 0.1% packet loss
        }
        else {
            args.push_back(arg);
        }
    }
    if(!args.empty()) {
        listen_port = std::atoi(args[0].c_str());
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
    if(debug)
        std::cout << "Receiver listening on port " << listen_port << std::endl;

    // === Negotiation Handshake ===
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
    handshake_buf[n] = '\0';
    if(std::string(handshake_buf) != "STREAM_START") {
        std::cerr << "Error: Unexpected handshake message: " << handshake_buf << std::endl;
        close(sockfd);
        exit(EXIT_FAILURE);
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
    ssize_t s = sendto(sockfd, negotiation_packet, sizeof(negotiation_packet), 0,
                       (sockaddr*)&sender_addr, sender_addr_len);
    if(s < 0) {
        perror("sendto negotiation packet failed");
    } else if(debug) {
        std::cout << "Negotiation packet sent to sender." << std::endl;
    }

    uint32_t expected_seq = 0;
    std::map<uint32_t, std::vector<char>> out_of_order;

    // --- Statistics counters ---
    uint64_t stats_data_bytes = 0;
    uint64_t stats_data_packets = 0;
    uint64_t stats_ack_sent = 0;
    uint64_t stats_nack_sent = 0;
    auto last_stats_time = steady_clock::now();

    bool running = true;
    while(running) {
        char buffer[DATA_PACKET_SIZE];
        sockaddr_in curr_sender_addr;
        socklen_t curr_sender_addr_len = sizeof(curr_sender_addr);
        ssize_t recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                    (sockaddr*)&curr_sender_addr, &curr_sender_addr_len);
        if(recv_len < 0) {
            std::this_thread::sleep_for(milliseconds(10));
            continue;
        }
        if(recv_len < HEADER_SIZE) {
            if(debug)
                std::cerr << "Received packet too small, ignoring." << std::endl;
            continue;
        }

        PacketHeader header;
        std::memcpy(&header, buffer, HEADER_SIZE);
        uint32_t seq_num = ntohl(header.seq_num);
        uint16_t pkt_window = ntohs(header.window_size);
        uint8_t ctrl_flag = header.control_flags;
        uint16_t recv_checksum = ntohs(header.checksum);

        // Verify checksum.
        std::vector<char> temp_buf(recv_len, 0);
        std::memcpy(temp_buf.data(), buffer, recv_len);
        std::memset(temp_buf.data() + 7, 0, sizeof(uint16_t));
        uint16_t computed_checksum = compute_checksum(temp_buf.data(), recv_len);
        bool random_error_active = false;
        if(random_error) {
            int random_number = dis(gen);
            if(random_number <= 10) {
                random_error_active = true;
            }

        }
        if((computed_checksum != recv_checksum) || random_error_active) {
            if(debug)
                std::cerr << "Invalid checksum for packet seq: " << seq_num << ", discarding." << std::endl;
            continue;
        }

        // --- Handle FIN packet (EOF) ---
        if(ctrl_flag == FLAG_FIN) {
            if(debug)
                std::cout << "Received FIN packet." << std::endl;
            // Build and send FIN-ACK.
            PacketHeader fin_ack;
            fin_ack.seq_num = header.seq_num;
            fin_ack.window_size = header.window_size;
            fin_ack.control_flags = FLAG_FIN_ACK;
            fin_ack.checksum = 0;
            std::vector<char> fin_ack_packet(CTRL_PACKET_SIZE, 0);
            std::memcpy(fin_ack_packet.data(), &fin_ack, HEADER_SIZE);
            uint16_t fin_ack_chksum = compute_checksum(fin_ack_packet.data(), fin_ack_packet.size());
            uint16_t net_fin_ack_chksum = htons(fin_ack_chksum);
            std::memcpy(fin_ack_packet.data() + 7, &net_fin_ack_chksum, sizeof(uint16_t));
            // send it 5 times
            ssize_t s = sendto(sockfd, fin_ack_packet.data(), fin_ack_packet.size(), 0,
                               (sockaddr*)&curr_sender_addr, curr_sender_addr_len);


            if(s < 0)
                perror("sendto FIN-ACK failed");
//            else if(debug)
                std::cout << "Sent FIN-ACK packet." << std::endl;

            // Wait for final ACK.
            bool final_ack_received = false;
            while(!final_ack_received) {
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(sockfd, &fds);
                timeval tv;
                tv.tv_sec = 1;
                tv.tv_usec = 0;
                int ret = select(sockfd+1, &fds, nullptr, nullptr, &tv);
                if(ret > 0 && FD_ISSET(sockfd, &fds)) {
                    char ack_buf[DATA_PACKET_SIZE];
                    sockaddr_in ack_addr;
                    socklen_t ack_addr_len = sizeof(ack_addr);
                    ssize_t r = recvfrom(sockfd, ack_buf, sizeof(ack_buf), 0,
                                         (sockaddr*)&ack_addr, &ack_addr_len);
                    if(r >= HEADER_SIZE) {
                        PacketHeader ack_hdr;
                        std::memcpy(&ack_hdr, ack_buf, HEADER_SIZE);
                        if(ack_hdr.control_flags == FLAG_ACK) {
                            final_ack_received = true;
                            if(debug)
                                std::cout << "Received final ACK. Closing connection." << std::endl;
                        }
                        exit(EXIT_SUCCESS);
                    }
                }
            }
            break; // Exit main loop.
        }

        // --- Process DATA packets ---
        if(ctrl_flag == FLAG_DATA) {
            stats_data_bytes += recv_len;
            stats_data_packets++;
            if(seq_num == expected_seq) {
                if(debug)
                    std::cout << "Processing packet seq: " << seq_num << std::endl;
                expected_seq++;
                while(out_of_order.count(expected_seq)) {
                    if(debug)
                        std::cout << "Processing buffered packet seq: " << expected_seq << std::endl;
                    out_of_order.erase(expected_seq);
                    expected_seq++;
                }
            } else if(seq_num > expected_seq) {
                if(!out_of_order.count(seq_num)) {
                    std::vector<char> pkt(buffer, buffer + recv_len);
                    out_of_order[seq_num] = pkt;
                    if(debug)
                        std::cout << "Stored out-of-order packet seq: " << seq_num << std::endl;
                }
                // Immediately send NACKs for missing packets.
                for(uint32_t missing = expected_seq; missing < seq_num; missing++) {
                    std::vector<char> nack_packet(CTRL_PACKET_SIZE, 0);
                    PacketHeader nack_hdr;
                    nack_hdr.seq_num = htonl(missing);
                    nack_hdr.window_size = htons(pkt_window);
                    nack_hdr.control_flags = FLAG_NACK;
                    nack_hdr.checksum = 0;
                    std::memcpy(nack_packet.data(), &nack_hdr, HEADER_SIZE);
                    uint16_t nack_chksum = compute_checksum(nack_packet.data(), nack_packet.size());
                    uint16_t net_nack_chksum = htons(nack_chksum);
                    std::memcpy(nack_packet.data() + 7, &net_nack_chksum, sizeof(uint16_t));
                    ssize_t s = sendto(sockfd, nack_packet.data(), nack_packet.size(), 0,
                                       (sockaddr*)&curr_sender_addr, curr_sender_addr_len);
                    if(s < 0)
                        perror("sendto NACK failed");
                    else {
                        if(debug)
                            std::cout << "Sent NACK for missing seq: " << missing << std::endl;
                        stats_nack_sent++;
                    }
                }
            } else {
              // Send ACK for expected_seq - 1.
                std::vector<char> ack_packet(CTRL_PACKET_SIZE, 0);
                PacketHeader ack_hdr;
                ack_hdr.seq_num = htonl(expected_seq - 1);
                ack_hdr.window_size = htons(pkt_window);
                ack_hdr.control_flags = FLAG_ACK;
                ack_hdr.checksum = 0;
                std::memcpy(ack_packet.data(), &ack_hdr, HEADER_SIZE);
                uint16_t ack_chksum = compute_checksum(ack_packet.data(), ack_packet.size());
                uint16_t net_ack_chksum = htons(ack_chksum);
                std::memcpy(ack_packet.data() + 7, &net_ack_chksum, sizeof(uint16_t));
                ssize_t s = sendto(sockfd, ack_packet.data(), ack_packet.size(), 0,
                                   (sockaddr*)&curr_sender_addr, curr_sender_addr_len);

                if(debug)
                    std::cout << "Duplicate packet seq: " << seq_num << " ignored." << std::endl;
            }

            // --- Send cumulative ACK only at end of sliding window ---
            if(expected_seq > 0 && (expected_seq % pkt_window == 0)) {
                uint32_t ack_seq = expected_seq - 1;
                std::vector<char> ack_packet(CTRL_PACKET_SIZE, 0);
                PacketHeader ack_hdr;
                ack_hdr.seq_num = htonl(ack_seq);
                ack_hdr.window_size = htons(pkt_window);
                ack_hdr.control_flags = FLAG_ACK;
                ack_hdr.checksum = 0;
                std::memcpy(ack_packet.data(), &ack_hdr, HEADER_SIZE);
                uint16_t ack_chksum = compute_checksum(ack_packet.data(), ack_packet.size());
                uint16_t net_ack_chksum = htons(ack_chksum);
                std::memcpy(ack_packet.data() + 7, &net_ack_chksum, sizeof(uint16_t));
                ssize_t s = sendto(sockfd, ack_packet.data(), ack_packet.size(), 0,
                                   (sockaddr*)&curr_sender_addr, curr_sender_addr_len);
                if(s < 0)
                    perror("sendto ACK failed");
                else {
                    if(debug)
                        std::cout << "Sent ACK for seq: " << ack_seq << std::endl;
                    stats_ack_sent++;
                }
            }
        }

        // --- Statistics reporting every second ---
        if(stats) {
            auto now = steady_clock::now();
            auto elapsed = duration_cast<seconds>(now - last_stats_time).count();
            if(elapsed >= 1) {
                double bps = stats_data_bytes * 8.0 / elapsed;
                if(bps >= 1e9)
                    std::cout << "[STATISTICS] Throughput: " << (bps / 1e9) << " Gbps, "
                              << "DATA packets: " << stats_data_packets << ", ACKs sent: " << stats_ack_sent
                              << ", NACKs sent: " << stats_nack_sent << std::endl;
                else
                    std::cout << "[STATISTICS] Throughput: " << (bps / 1e6) << " Mbps, "
                              << "DATA packets: " << stats_data_packets << ", ACKs sent: " << stats_ack_sent
                              << ", NACKs sent: " << stats_nack_sent << std::endl;
                stats_data_bytes = 0;
                stats_data_packets = 0;
                stats_ack_sent = 0;
                stats_nack_sent = 0;
                last_stats_time = now;
            }
        }
    } // end while

    close(sockfd);
    return 0;
}
