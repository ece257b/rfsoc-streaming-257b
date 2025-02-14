#pragma once
#include <chrono>
#include <thread>
#include "Protocol.hpp"
#include "DataProcessing.hpp"
#include "NetworkConnection.hpp"

template<typename DataProcessorType, typename NetworkConnType>
class BasicReceiver {
    // using DataProcessorType = DataProcessor;      // "Hack" for autocomplete in VSCode, won't compile with this
    // using NetworkConnType = NetworkConnection;

public:
    BasicReceiver() {
        static_assert(std::is_base_of<DataProcessor, DataProcessorType>::value, "type parameter of this class must derive from DataProvider");
        static_assert(std::is_base_of<NetworkConnection, NetworkConnType>::value, "type parameter of this class must derive from NetworkConnection");
    }

    int receiveStream() {
        conn.open();
        Packet recvBuffer;
        int seq_num = 0;

        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            std::cout << "Current seq " << seq_num << std::endl;
            ssize_t recvBytes = conn.receive(&recvBuffer, sizeof(recvBuffer));
            if(recvBytes < 0) {
                continue;
            }

            std::cout << "Received " << recvBytes << std::endl;
            
            if (!verifyChecksum(&recvBuffer, recvBytes)) {
                std::cout << "Invalid Checksum! Ignoring." << std::endl;
                continue;
            }

            uint32_t pkt_seq = ntohl(recvBuffer.header.seq_num);
            uint8_t ctrl_flag = recvBuffer.header.control_flags;
            
            if (ctrl_flag == FLAG_DATA && pkt_seq == seq_num) {
                std::cout << "Processing Packet " << seq_num << std::endl;
                processor.processData(sizeof(recvBuffer.data), recvBuffer.data);
                seq_num += 1;
            } else if (ctrl_flag == FLAG_FIN) {
                break;
            }

            sendACK(seq_num);
        }
        teardown();
    }
    int teardown() {
        std::cout << "Teardown!" << std::endl;
        conn.close();
    }

    DataProcessorType processor;
    NetworkConnType conn;

private:
    int sendACK(int seq_num) {
        PacketHeader ack_hdr;
        ack_hdr.seq_num = htonl(seq_num);
        ack_hdr.window_size = htons(WINDOW_SIZE);
        ack_hdr.control_flags = FLAG_ACK;
        ack_hdr.checksum = 0;

        uint16_t ack_chksum = compute_checksum(&ack_hdr, sizeof(ack_hdr));
        ack_hdr.checksum = htons(ack_chksum);

        std::cout << "Sending ACK for " << seq_num << std::endl;
        return conn.send(&ack_hdr, sizeof(ack_hdr));
    }
};