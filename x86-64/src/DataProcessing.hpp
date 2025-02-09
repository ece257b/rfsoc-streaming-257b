#pragma once
#include <unistd.h>
#include <stdint.h>
#include <functional>
#include "Protocol.hpp"

// --- Structure to track unacknowledged packets --- 
struct PacketInfo {
    Packet* packet;
    size_t packet_size;
    std::chrono::steady_clock::time_point last_sent;
};

class DataProvider {
// Interface to read data sequentially. Can use for dummy, file, or stream
public:
    virtual int getData(size_t size, char* buffer) = 0;
};

class DataProcessor {
//  Interface for sequentially processing data
public:
    virtual int processData(size_t size, char* buffer) = 0;
};


template <typename DataType>
class DataWindow {
public:
    virtual DataType* reserve(uint16_t seq_num) = 0;
//     - Reserves space for seq_num in window, and returns pointer to the memory. nullptr if no space available

    virtual bool contains(uint16_t seq_num) = 0;
    virtual DataType* get(uint16_t seq_num) = 0;
    virtual bool erase(uint16_t seq_num) = 0;

    virtual bool isFull() = 0;
    virtual bool isEmpty() = 0;

    typedef std::function<bool(uint32_t, DataType*)> WindowCallback;
    virtual bool forEachData(WindowCallback callback) = 0;
};
