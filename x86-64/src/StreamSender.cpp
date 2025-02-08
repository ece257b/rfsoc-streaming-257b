#include "Protocol.hpp"
#include "StreamSender.hpp"
#include "NetworkUtils.hpp"
#include <chrono>
#include <thread>
#include <cassert>

using namespace std::chrono;

StreamSender::StreamSender(DataProvider& provider, DataWindow& window, bool debug) 
        : provider(provider), window(window), debug(debug) {
    
}

int StreamSender::setup(int receiver_port, std::string& receiver_ip) {
    sockfd = createUDPSocket();
    receiver_addr = setupReceiver(sockfd, receiver_port, receiver_ip);
}

int StreamSender::handshake() {
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
    
    assert(negotiated_buffer_size == BUFFER_SIZE);
    assert(negotiated_packet_size == PACKET_SIZE);
}

int StreamSender::stream() {
    while (base < max_packets) {
        while (next_seq < base + WINDOW_SIZE && next_seq < max_packets) {
            sendData(next_seq);
            next_seq++;
        }

        processACKs();

        int timed_out_seq = -1;
        while ((timed_out_seq = getTimedOut()) != -1) {
            sendData(timed_out_seq);
        }
        
        std::this_thread::sleep_for(milliseconds(10));
    }
}

int StreamSender::sendData(uint32_t seq_num) {
    char* buffer = window.reserve(seq_num); // TODO: Should change window to return PacketInfo or something
    PacketHeader header;
    header.seq_num = htonl(next_seq);
    header.window_size = htons(WINDOW_SIZE);
    header.control_flags = FLAG_DATA;
    header.checksum = 0;
    std::memcpy(buffer, &header, HEADER_SIZE);

    provider.getData(PAYLOAD_SIZE, buffer + HEADER_SIZE);

    uint16_t chksum = compute_checksum(buffer, DATA_PACKET_SIZE);
    uint16_t net_chksum = htons(chksum);
    std::memcpy(buffer + CHECKSUM_OFFSET, &net_chksum, sizeof(uint16_t));

    ssize_t sent = sendto(sockfd, buffer, DATA_PACKET_SIZE, 0,(sockaddr*)&receiver_addr, sizeof(receiver_addr));

    // Error check and set packet info.
}
