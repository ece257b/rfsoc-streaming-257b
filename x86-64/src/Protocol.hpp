#pragma once

#include <stdint.h>
#include <vector>
#include <chrono>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <cstdio>


const int PAYLOAD_SIZE       = 512;  // bytes of payload in DATA packets
const int WINDOW_SIZE        = 1000;     // default sliding window size
const int DEFAULT_MAX_PACKETS = 1000000;   // default number of packets in dummy mode
const int TIMEOUT_MS         = 100;   // retransmission timeout (ms)
const int HANDSHAKE_TIMEOUT_MS = 1000; // handshake timeout (ms)

const int BUFFER_SIZE = 1024;

constexpr char HANDSHAKE[13]  = "STREAM_START";
constexpr size_t HANDSHAKE_SIZE = sizeof(HANDSHAKE);

const int SENDER_ACK_WAIT_US = 1000;
static_assert(SENDER_ACK_WAIT_US < 1000000, "timeval constructed with 1000*1000 us will fail.");
const int SENDER_SUBSEQUENT_ACK_WAIT_US = std::max(SENDER_ACK_WAIT_US / 100, 1);        // Don't wait 0

const int SENDER_STREAMING_WAIT_US = 10; // FIXME: Setting to zero breaks things..
const int RETRY_MS = 1;
const int RETRY_ACK_US = 1000;

// --- Control flag definitions ---
enum ControlFlag {
    FLAG_DATA     = 0,
    FLAG_ACK      = 1,
    FLAG_NACK     = 2,
    FLAG_FIN      = 3,
    FLAG_FIN_ACK  = 4
};

// --- Packed packet header ---
#pragma pack(push, 1)
struct PacketHeader {
    uint32_t seq_num;      // sequence number (network order)
    uint16_t window_size;  // window size (network order)
    uint8_t control_flags; // control flag
    uint16_t checksum;     // checksum (network order)
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Packet {
    PacketHeader header;
    char data[PAYLOAD_SIZE];
};
#pragma pack(pop)

const int HEADER_SIZE        = sizeof(PacketHeader);

const int DATA_PACKET_SIZE   = HEADER_SIZE + PAYLOAD_SIZE;  // full DATA packet size
const int CTRL_PACKET_SIZE   = HEADER_SIZE;                 // control packets contain only header


// --- Simple Internet checksum (RFC1071 style) ---
inline uint16_t compute_checksum(const void* data, size_t len) {
    uint32_t sum = 0;
    const uint16_t* ptr = reinterpret_cast<const uint16_t*>(data);
    while(len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    if(len > 0)
        sum += *(const uint8_t*)ptr;
    while(sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return static_cast<uint16_t>(~sum);
}

inline bool verifyChecksum(Packet* packet, ssize_t length) {
    uint16_t temp = packet->header.checksum;
    packet->header.checksum = 0;
    uint16_t computed = htons(compute_checksum(packet, length));
    bool match = (temp == computed);
    packet->header.checksum = temp;
    return match;
}