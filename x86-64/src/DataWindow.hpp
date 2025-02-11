#pragma once
#include <vector>
#include <unordered_map>
#include "Protocol.hpp"
#include "DataProcessing.hpp"

class PacketMap: public DataWindow<PacketInfo> {
private:
    std::unordered_map<uint16_t, PacketInfo> packetmap;
    std::unordered_map<uint16_t, PacketInfo>::iterator iter;
public:
    PacketMap() {};
    ~PacketMap() {};
    
    PacketInfo* reserve(uint16_t seq_num) override {
        PacketInfo info;
        info.packet.header.seq_num = seq_num;
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
        return packetmap.erase(seq_num);
    };
    bool isFull() override {
        return packetmap.size() >= WINDOW_SIZE;
    };
    size_t size() override {
        return packetmap.size();
    }

    bool isEmpty() override {
        return packetmap.empty();
    };

    // Use unordered_map iteration for efficient iteration
    // May want to switch to ordered map if we need to guarantee order?
    void resetIter() override {
        iter = packetmap.begin();
    }
    PacketInfo* getIter() override {
        return &(*iter).second;
    }
    bool nextIter() override {
        iter++;
        return isIterDone();
    }
    bool isIterDone() override {
        return iter == packetmap.end();
    };
};

