#pragma once
#include "DataProcessing.hpp"

// - class StreamReceiver
//   - setup()
//   - receiveStream()
//     - Main receive streaming loop
//     - receivePacket
//   - receiveData()
//     - Receive a data packet. Process if in order. Store it in DataWindow if out of order
//   - sendNACK()
//   - sendACK()
//   - teardown()

class StreamReceiver
{
private:
    DataProcessor& processor;
    DataWindow& window;

    int receiveData();
    int sendACK();
    int sendNACK();

public:
    StreamReceiver(/* args */);
    ~StreamReceiver();

    int setup();
    int receiveStream();
    int teardown();
};