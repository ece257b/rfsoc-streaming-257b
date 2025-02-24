#include <chrono>
#include <thread>
#include <cstring>
#include "Protocol.hpp"
#include "DataProcessing.hpp"
#include "NetworkConnection.hpp"

template<typename DataProviderType, typename NetworkConnType>
class BasicSender {
    // using DataProviderType = DataProvider;      // "Hack" for autocomplete in VSCode, won't compile with this
    // using NetworkConnType = NetworkConnection;

public:
    BasicSender(size_t max_packets) : max_packets(max_packets) {
        static_assert(std::is_base_of<DataProvider, DataProviderType>::value, "type parameter of this class must derive from DataProvider");
        static_assert(std::is_base_of<NetworkConnection, NetworkConnType>::value, "type parameter of this class must derive from DataWindow<PacketInfo>");
    }

    int stream() {
        conn.open();
        uint32_t seq_num = 0;
        PacketInfo info;
        info.packet.header.seq_num = -1;    // init to -1 so first packet gets prepared.
        Packet recvBuffer;

        while (seq_num < max_packets) {
            // @Kishore you may need to comment out or add some #ifdef guards around thread sleep calls
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            std::cout << "Current seq " << seq_num << std::endl;
            // std::cout << "Current seq " << seq_num << " Info Seq " << htonl(info.packet.header.seq_num) << std::endl;

            // No need to prepare packet again if we already have prepared it in previous iter.
            if (ntohl(info.packet.header.seq_num) != seq_num) {
                preparePacket(&info, seq_num);
            }
            bool success = sendPacket(info);
            if (!success) {
                std::cerr << "Sending Packet Failed! Retrying" << std::endl;
                continue;
            }
            
            // @Kishore not exactly sure how waiting for timout would work on FPGA.. 
            if (conn.ready({0, 10000})) {   // Wait 10000us for timeout
                size_t bytesReceived = conn.receive(&recvBuffer, sizeof(recvBuffer));

                if (!verifyChecksum(&recvBuffer, bytesReceived)) {
                    std::cout << "Invalid Checksum! Ignoring." << std::endl;
                    continue;
                }

                uint32_t pkt_seq = ntohl(recvBuffer.header.seq_num);
                uint8_t ctrl_flag = recvBuffer.header.control_flags;

                if(ctrl_flag == FLAG_ACK) {
                    std::cout << "Received ACK for " << pkt_seq << std::endl;
                    if (pkt_seq == seq_num + 1) {
                        // Receiver is ready for next packet
                        // Next iteration of loop will Prepare, then Send packet.
                        seq_num = pkt_seq;
                    }
                } else {
                    std::cout << "Received somethiing weirdd" << std::endl;
                }
            }
            // Packet timed out. Next iteration of loop will Send same packet.
        };

        info = PacketInfo();
        info.packet.header.control_flags = FLAG_FIN;
        info.packet_size = CTRL_PACKET_SIZE;
        info.packet.header.checksum = 0;

        uint16_t chksum = compute_checksum(&info.packet, DATA_PACKET_SIZE);
        info.packet.header.checksum = htons(chksum);
        std::cout << "Sending FIN!" << std::endl;
        sendPacket(info);

        // TODO: FINACK etc
        teardown();
    }

    // Leaving these public so we can setup Provider and Conn from main as necessary
    DataProviderType provider;
    NetworkConnType conn;
private:
    size_t max_packets;

    bool sendPacket(PacketInfo& info) {
        size_t bytesSent = conn.send(&info.packet, info.packet_size);
        if (bytesSent != info.packet_size) {
            std::cerr << "Bytes sent did not match packet size! " << bytesSent << " != " << info.packet_size << std::endl;
            return false;
        }
        return true;
    }

    bool preparePacket(PacketInfo* info, int seq_num) {
        Packet* packet = &info->packet;
        PacketHeader* header = &packet->header;
        char* dataBuffer = packet->data;

        header->seq_num = htonl(seq_num);
        header->window_size = htons(WINDOW_SIZE);
        header->control_flags = FLAG_DATA;
        header->checksum = 0;

        size_t size = provider.getData(PAYLOAD_SIZE, dataBuffer);
        info->packet_size = size + HEADER_SIZE;

        uint16_t chksum = compute_checksum(packet, DATA_PACKET_SIZE);
        uint16_t net_chksum = htons(chksum);
        header->checksum = net_chksum;

        return true;
    }

    int teardown() {
        conn.close();
    }
};
