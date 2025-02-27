#pragma once

#include <memory>
#include <string>
#include "Statistics.hpp"
#include "DataProcessing.hpp"
#include "NetworkUtils.hpp"
#include "Protocol.hpp"
#include "NetworkConnection.hpp"

// - class StreamSender
//   - This class should contain all protocol specific logic, and delegate data reading and buffering to DataProvider and DataWindow
//   - setup()
//     - Set up connection and negotiation handshake
//   - stream()
//     - Main streaming loop
//     - Send window of packets: calls sendData()
//     - calls processPacket()
//     - getTimedOut() and sendPacket() if necessary
//   - sendData(seqnum)
//     - Send one packet on socket.
//     - Gets data from DataWindow
//       - If not in buffer, gets data from DataProvider
//   - processACKs()
//     - handles ACK/NACK logic
//   - getTimedOut()
//     - gets the first timed out packet if one is available
//   - teardown()
//     - FIN/FINACK logic

template<typename DataProviderType, typename DataWindowType, typename NetworkConnectionType>
class StreamSender
{   
private:
    DataWindowType window;
    SenderStats stats;
    
    bool debug = false;
    uint32_t base = 0;      // lowest unacknowledged sequence number
    uint32_t next_seq = 0;  // next sequence number to send
    uint32_t max_packets = DEFAULT_MAX_PACKETS;
    uint32_t window_size;

    int handshake();
    PacketInfo* preparePacket(uint32_t seq_num);
    int sendPacket(PacketInfo* info);
    int processACKs();
    void prepareFINPacket(PacketHeader* header, ControlFlag flag);
public:
    StreamSender(bool debug, uint32_t window_size=WINDOW_SIZE);
    ~StreamSender();

    int stream();
    int teardown();

    NetworkConnectionType conn;
    DataProviderType provider;
};

#include "StreamSender_impl.hpp"  // Include template definitions