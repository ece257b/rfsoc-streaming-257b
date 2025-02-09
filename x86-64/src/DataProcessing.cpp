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
    
    PacketInfo* reserve(uint16_t seq_num) {
        Packet* packet = new Packet();
        packet->header.seq_num = seq_num;
        PacketInfo info;
        info.packet = packet;
        auto it = packetmap.insert(std::make_pair(seq_num, info)).first;
        return &(*it).second;
    };

    bool contains(uint16_t seq_num) {
        return packetmap.count(seq_num) != 0;
    };

    PacketInfo* get(uint16_t seq_num) {
        auto it = packetmap.find(seq_num);
        if (it == packetmap.end()) {
            return nullptr;
        } 
        return &(*it).second;
    };

    bool erase(uint16_t seq_num) {
        auto it = packetmap.find(seq_num);
        if (it != packetmap.end()) {
            free(it->second.packet);
            packetmap.erase(it);
            return true;
        }
        return false;
    };

    bool isFull() {
        return packetmap.size() >= WINDOW_SIZE;
    };

    bool isEmpty() {
        return packetmap.empty();
    };

    bool forEachData(WindowCallback callback) {
        for (auto item : packetmap) {
            if (!callback(item.first, &item.second)) {
                return false;
            };
        }
        return true;
    }
};


class DummyProvider : public DataProvider {
// Interface to read data sequentially. Can use for dummy, file, or stream
private:
    uint32_t count = 0;
public:
    int getData(size_t size, char* buffer) {
        char fill_char = 'A' + (count % 26);
        std::memset(buffer, fill_char, size);
        return size;
    }
};

class DummyProcessor : public DataProvider {
//  Interface for sequentially processing data
private:
    bool print;
public:
    DummyProcessor(bool print) : print(print) {}
    int processData(size_t size, char* buffer) {
        if (print) std::cout << buffer << std::endl;
        return size;
    };
};
