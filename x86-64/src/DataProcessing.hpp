#pragma once
#include <unistd.h>
#include <stdint.h>
#include <type_traits>
#include "Protocol.hpp"

// --- Structure to track unacknowledged packets --- 
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
    virtual DataType* reserve(uint32_t seq_num) = 0;

    // API to access elements by sequence number
    virtual bool contains(uint32_t seq_num) = 0;
    virtual DataType* get(uint32_t seq_num) = 0;
    virtual bool erase(uint32_t seq_num) = 0;

    // API to access elements by iteration, order defined by implementation.
    virtual void resetIter() = 0;
    virtual DataType* getIter() = 0;
    virtual bool nextIter() = 0;
    virtual bool isIterDone() = 0;
    
    virtual bool isFull() = 0;
    virtual bool isEmpty() = 0;
    virtual size_t size() = 0; 
    virtual void clear() = 0; 
};
