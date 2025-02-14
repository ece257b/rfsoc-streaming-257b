#pragma once
#include <cstring>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string>

inline int createUDPSocket() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    // Set non-blocking mode.
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    
    return sockfd;
}

inline sockaddr_in setupReceiver(int sockfd, int receiver_port, std::string& receiver_ip) {
    sockaddr_in receiver_addr;
    std::memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port   = htons(receiver_port);
    if(inet_pton(AF_INET, receiver_ip.c_str(), &receiver_addr.sin_addr) <= 0) {
        perror("Invalid receiver IP address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    return receiver_addr;
}
