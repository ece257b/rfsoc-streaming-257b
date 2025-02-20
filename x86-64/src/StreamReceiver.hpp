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


template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
class StreamReceiver
{

public:
    StreamReceiver(bool debug);
    ~StreamReceiver();

    int receiveData();
    int teardown();

    NetworkConnectionType conn;
    DataProviderType provider;
private:
    DataWindowType window;
    SenderStats stats;
    
    bool debug = false;
    uint32_t base = 0;      // lowest unacknowledged sequence number
    uint32_t expected_seq = 0;  // next sequence number to send

    void handshake();
    int sendACK(uint32_t seq_num, uint8_t flag=FLAG_ACK);
    int processOutOfOrder(); 
};