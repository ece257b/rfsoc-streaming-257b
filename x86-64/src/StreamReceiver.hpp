#pragma once
#include <unordered_map>
#include "SlidingWindow.hpp"
#include "Statistics.hpp"

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


// Small abstraction to allow us to hold a reference to any (templated) StreamReceiver
class StreamReceiverInterface {
public:
    virtual ~StreamReceiverInterface() {};
    virtual int receiveData() = 0;
    virtual int teardown() = 0;
};

template<typename DataProcessorType, typename NetworkConnectionType>
class StreamReceiver : public StreamReceiverInterface {

public:
    StreamReceiver(
        DataProcessorType&& processor, NetworkConnectionType&& conn,
        bool debug, uint32_t window_size=WINDOW_SIZE
    );
    ~StreamReceiver();

    int receiveData() override;
    int teardown() override;

    NetworkConnectionType conn;
    DataProcessorType processor;

protected:
    SlidingWindow<Packet> window;

    typedef std::chrono::time_point<std::chrono::steady_clock> timepoint;
    SlidingWindow<timepoint> ackTimes;
    SlidingWindow<timepoint> nackTimes;

    SenderStats stats;
    
    bool debug = false;
    uint32_t base = 0;      // lowest unacknowledged sequence number
    uint32_t expected_seq = 0;  // next sequence number to send
    uint32_t window_size;

    int handshake();
    int sendACK(uint32_t seq_num, uint8_t flag=FLAG_ACK, bool checkPastACKs=true);
    bool sendFINACK(uint32_t seq_num);
    int processOutOfOrder(); 
    bool advanceAllWindows(uint32_t seq_num); 
    bool processPacket(Packet* packet, ssize_t size); 
};

#include "StreamReceiver_impl.hpp"