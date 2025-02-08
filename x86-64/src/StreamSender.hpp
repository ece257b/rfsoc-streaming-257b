#pragma once

#include "DataProcessing.hpp"
#include "NetworkUtils.hpp"
#include <string>

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

class StreamSender
{
private:
    DataProvider& provider;
    DataWindow& window;
    sockaddr_in receiver_addr;
    int sockfd = 0;
    bool debug = false;
    uint32_t base = 0;      // lowest unacknowledged sequence number
    uint32_t next_seq = 0;  // next sequence number to send
    uint32_t max_packets = DEFAULT_MAX_PACKETS;

    int handshake();
    int sendData(uint32_t seq_num);
    int processACKs();
    int getTimedOut();
public:
    StreamSender(DataProvider& provider, DataWindow& window, bool debug);
    ~StreamSender();

    int setup(int receiver_port, std::string& receiver_ip);
    int stream();
    int teardown();
};
