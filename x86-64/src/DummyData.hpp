#pragma once
#include "DataProcessing.hpp"
#include <iostream>

class DummyProvider : public DataProvider {
// Interface to read data sequentially. Can use for dummy, file, or stream
public:
    DummyProvider(uint32_t num_packets=1000) : total_packets(num_packets) {}
    uint32_t count = 0;
    uint32_t total_packets;
    int getData(size_t size, char* buffer) override {
        if (count >= total_packets) {
            return 0;
        }
        char fill_char = 'A' + (count % 26);
        std::memset(buffer, fill_char, size);
        count++;
        return size;
    }
};

class DummyProcessor : public DataProcessor {
//  Interface for sequentially processing data
public:
    DummyProcessor(bool print=true) : print(print) {}
    bool print;
    int processData(size_t size, char* buffer) override {
        if (print) {
            std::cout.write(buffer, size);
            std::cout << std::endl;
        }
        return size;
    };
};