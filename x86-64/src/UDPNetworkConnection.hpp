#pragma once
#include <cstring>
#include <string>
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
    }

protected:
    std::string receiver_ip;
};



class UDPStreamReceiver : public UDPNetworkConnection {
public:
    UDPStreamReceiver() {};

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
    }

    ssize_t receive(void* buffer, size_t len) override {
        sockaddr_in ack_addr;   // TODO: we don't actually need these right?
        socklen_t ack_addr_len = sizeof(ack_addr);
        ssize_t ret = recvfrom(sockfd, buffer, len, 0, (sockaddr*)&ack_addr, &ack_addr_len);
        receiver_addr = ack_addr;   // Different for StreamReceiver ?
        return ret;
    }
};