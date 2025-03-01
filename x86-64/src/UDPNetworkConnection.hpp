#pragma once
#include <cstring>
#include <string>
#include <random>
#include <stdio.h>
#include "NetworkUtils.hpp"
#include "NetworkConnection.hpp"


class UDPNetworkConnection : public NetworkConnection {
public:
// NetworkConnection Interface
    // open() implemented by UDPStreamSender and UDPStreamReceiver

    ssize_t send(void* packet, size_t len) override {
        return sendto(sockfd, packet, len, 0, (sockaddr*)&receiver_addr, sizeof(receiver_addr));
    }
    ssize_t receive(void* buffer, size_t len) override {
        sockaddr_in ack_addr;
        socklen_t ack_addr_len = sizeof(ack_addr);
        return recvfrom(sockfd, buffer, len, 0, (sockaddr*)&ack_addr, &ack_addr_len);
    }
    bool ready(timeval tv) override {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        int ret = select(sockfd+1, &readfds, nullptr, nullptr, &tv);
        return (ret > 0 && FD_ISSET(sockfd, &readfds));
    }
    bool close() override {
        int success = ::close(sockfd);  // :: scope resolution to call std close()
        sockfd = -1;
        return success;
    }
protected:
    int sockfd = -1;
    int receiver_port = -1;
    sockaddr_in receiver_addr;
};

class UDPStreamSender : public UDPNetworkConnection {
public:
    UDPStreamSender() {};

    void setup(int receiver_port, std::string& receiver_ip) {
        this->receiver_port = receiver_port;
        this->receiver_ip = receiver_ip;
    }

// NetworkConnection Interface
    bool open() override {
        sockfd = createUDPSocket();
        receiver_addr = setupReceiver(sockfd, receiver_port, receiver_ip);
        std::cout << "Sender Bound to " << receiver_ip << " " << receiver_port << std::endl;
        return true;
    }

protected:
    std::string receiver_ip;
};



class UDPStreamReceiver : public UDPNetworkConnection {
public:
    UDPStreamReceiver() {};
    UDPStreamReceiver(int receiver_port) {
        setup(receiver_port);
    };

    void setup(int receiver_port) {
        this->receiver_port = receiver_port;
    }

// NetworkConnection Interface
    bool open() override {
        sockfd = createUDPSocket();
        
        std::memset(&receiver_addr, 0, sizeof(receiver_addr));
        receiver_addr.sin_family      = AF_INET;
        receiver_addr.sin_addr.s_addr = INADDR_ANY;
        receiver_addr.sin_port        = htons(receiver_port);
        if(bind(sockfd, (sockaddr*)&receiver_addr, sizeof(receiver_addr)) < 0) {
            perror("bind failed");
            ::close(sockfd);
            exit(EXIT_FAILURE);
        }
        std::cout << "Receiver listening on port " << receiver_port << std::endl;
        
        return true;
    }

    ssize_t receive(void* buffer, size_t len) override {
        sockaddr_in ack_addr;   // TODO: we don't actually need these right?
        socklen_t ack_addr_len = sizeof(ack_addr);
        ssize_t ret = recvfrom(sockfd, buffer, len, 0, (sockaddr*)&ack_addr, &ack_addr_len);
        receiver_addr = ack_addr;   // Different for StreamReceiver ?
        return ret;
    }
};

class FaultyUDPStreamReceiver : public UDPStreamReceiver {
    // This class is intended for testing purposes only to simulate low channel quality
public:
    FaultyUDPStreamReceiver(int receiver_port, float error_rate, bool data_only=false, int seed=-1) 
            : UDPStreamReceiver(receiver_port), data_only(data_only), error_rate(error_rate) {
        if (seed == -1) {
            std::random_device rd;
            std::mt19937 gen(rd());
        } else {
            std::mt19937 gen(seed);
        }
        std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    };


    ssize_t receive(void* buffer, size_t len) override {
        ssize_t r = UDPStreamReceiver::receive(buffer, len);
        if (data_only && len <= HEADER_SIZE) {
            return r;
        }
        if (dis(gen) < error_rate) {
            char* addr = (char*)buffer + HEADER_SIZE + 1;
            *addr = *addr ^ 0b0001000;  // flip a random bit
        }
        return r;
    }

private:
    bool data_only;     // Only flip bits in transmissions len > HEADER_SIZE
    float error_rate;   // Flip a bit in this proportion of transmissions
    std::uniform_real_distribution<float> dis;
    std::mt19937 gen;

};