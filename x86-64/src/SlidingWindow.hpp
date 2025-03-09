#pragma once
#include <stdlib.h>

template <typename PacketType>
class SlidingWindow {
public:
    SlidingWindow(size_t window_size) : window_size(window_size) {
        clear();
    };
    ~SlidingWindow() {};
    
    PacketType* reserve(uint32_t seq_num) {
        if (!inBounds(seq_num)) return nullptr;
        NumberedPacket& packet = arr[indexOf(seq_num)];
        packet.seq_num = seq_num;
        return &packet.packet;
    };

    bool contains(uint32_t seq_num) {
        return inBounds(seq_num) && arr[indexOf(seq_num)].seq_num == seq_num;
    };

    PacketType* get(uint32_t seq_num) {
        if (!inBounds(seq_num)) return nullptr;
        NumberedPacket& packet = arr[indexOf(seq_num)];
        if (packet.seq_num == seq_num) return &packet.packet;
        return nullptr;
    };

    bool erase(uint32_t seq_num) {
        if (!inBounds(seq_num)) return false;
        arr[indexOf(seq_num)].seq_num = -1;
        return true;
    }
    
    bool advanceTo(uint32_t seq_num) {
        if (seq_num <= base_seq) return false;  // cannot advance backwards.
        base_seq = seq_num;
        base = indexOf(base_seq);
    };

    void clear() {
        base = 0;
        base_seq = 0;
        // for (int i = 0; i < window_size; i++) {
        //     arr[i].seq_num = -1;
        // }
        arr[0].seq_num = -1;
    }

protected:
    struct NumberedPacket {
        PacketType packet;
        uint32_t seq_num;
    }

    inline bool inBounds(uint32_t seq_num) {
        // All currently stored elements must have seq_num in bounds!
        return (seq_num >= base_seq) && (seq_num - base_seq < window_size);
    }
    inline size_t indexOf(uint32_t seq_num) {
        return seq_num % window_size;
    }
    inline uint32_t highestSeq() {
        return base_seq + window_size - 1;
    }

    size_t base = 0;
    uint32_t base_seq = 0;
    size_t window_size;
    NumberedPacket arr[WINDOW_SIZE] = {};
};
