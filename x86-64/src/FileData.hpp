#pragma once
#include "DataProcessing.hpp"
#include <iostream>

class FileReader : public DataProvider {
// Interface to read data sequentially. Can use for dummy, file, or stream
public:
    FileReader(std::istream& stream) : stream(stream) {}
    int getData(size_t size, char* buffer) override {
        stream.read(buffer, size);
        return stream.gcount();
    }
protected:
    std::istream& stream;
};


class FileWriter : public DataProcessor {
//  Interface for sequentially processing data
public:
    FileWriter(std::ostream& stream) : stream(stream) {}
    int processData(size_t size, char* buffer) override {
        stream.write(buffer, size);
        return size;
    };
protected:
    std::ostream& stream;
};

// https://stackoverflow.com/questions/11826554/standard-no-op-output-stream
// Stream that doesn't do anything
class NullStream : public std::ostream {
public:
    NullStream() : std::ostream(&noop) {} 
private: 
    class NullBuffer : public std::streambuf {
    public:
        int overflow(int c) { return c; }
    };
    NullBuffer noop; 
};
