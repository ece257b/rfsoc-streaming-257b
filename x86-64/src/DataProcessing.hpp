#include <unistd.h>
#include <stdint.h>

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


class DataWindow {
public:
    virtual char* reserve(uint16_t seq_num) = 0;
//     - Reserves space for seq_num in window, and returns pointer to the memory. nullptr if no space available

    virtual bool contains(uint16_t seq_num) = 0;
    virtual bool get(uint16_t seq_num, char* buffer) = 0;
    virtual bool erase(uint16_t seq_num) = 0;
    virtual bool pop(uint16_t seq_num, char* buffer) = 0;

    virtual bool isFull() = 0;
    virtual bool isEmpty() = 0;

};
