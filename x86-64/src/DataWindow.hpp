#pragma once
#include <vector>
#include <unordered_map>
#include "Protocol.hpp"
#include "DataProcessing.hpp"

class PacketMap: public DataWindow<PacketInfo> {
private:
    std::unordered_map<uint32_t, PacketInfo> packetmap;
public:
    PacketMap() {};
    ~PacketMap() {
        for (auto item : packetmap) {
            free(item.second.packet);
        }
    };
    
    PacketInfo* reserve(uint16_t seq_num) override {
        Packet* packet = new Packet();
        packet->header.seq_num = seq_num;
        PacketInfo info;
        info.packet = packet;
        auto it = packetmap.insert(std::make_pair(seq_num, info)).first;
        return &(*it).second;
    };

    bool contains(uint16_t seq_num) override {
        return packetmap.count(seq_num) != 0;
    };

    PacketInfo* get(uint16_t seq_num) override {
        auto it = packetmap.find(seq_num);
        if (it == packetmap.end()) {
            return nullptr;
        } 
        return &(*it).second;
    };

    bool erase(uint16_t seq_num) override {
        auto it = packetmap.find(seq_num);
        if (it != packetmap.end()) {
            free(it->second.packet);
            packetmap.erase(it);
            return true;
        }
        return false;
    };

    bool isFull() override {
        return packetmap.size() >= WINDOW_SIZE;
    };

    bool isEmpty() override {
        return packetmap.empty();
    };

    bool forEachData(WindowCallback callback) override {
        for (auto item : packetmap) {
            if (!callback(item.first, &item.second)) {
                return false;
            };
        }
        return true;
    }
};

