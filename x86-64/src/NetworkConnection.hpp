// Abstraction for UDP socket or AXI Stream
#pragma once
#include <stdio.h>
#include <sys/time.h>

class NetworkConnection {
public:
    virtual bool open() = 0;
    virtual ssize_t send(void* packet, size_t len) = 0;
    virtual ssize_t receive(void* buffer, size_t len) = 0;
    virtual bool ready(timeval timeout) = 0;
    virtual bool close() = 0;
};