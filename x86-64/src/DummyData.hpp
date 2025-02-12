#pragma once
#include "DataProcessing.hpp"
#include <iostream>

class DummyProvider : public DataProvider {
// Interface to read data sequentially. Can use for dummy, file, or stream
private:
    uint32_t count = 0;
public:
    int getData(size_t size, char* buffer) override {
        char fill_char = 'A' + (count % 26);
        std::memset(buffer, fill_char, size);
        return size;
    }
};

class DummyProcessor : public DataProcessor {
//  Interface for sequentially processing data
private:
    bool print;
public:
    DummyProcessor(bool print) : print(print) {}
    int processData(size_t size, char* buffer) override {
        if (print) std::cout << buffer << std::endl;
        return size;
    };
};