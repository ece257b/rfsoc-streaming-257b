#pragma once
#include <chrono>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include "Protocol.hpp"

using namespace std::chrono;

class SenderStats {
public:
    SenderStats(bool sender=true, bool csv_mode=false, bool report_percent=true, std::ostream& stream=std::cout) 
            : stream(stream) {
        this->csv_mode = csv_mode;
        this->report_percent = csv_mode ? false : report_percent;
        senderText = sender ? "sent" : "received";
    };
    ~SenderStats() = default;

    void reset() {
        data_bytes = 0;
        data_packets = 0;
        acks = 0;
        nacks = 0;
        corrupted_packets = 0;
        ignored = 0;
    }
    void record_packet(uint32_t bytes) {
        data_packets++;
        data_bytes += bytes;
    }
    void record_ack(uint8_t flag=FLAG_ACK) {
        if (flag == FLAG_ACK) {
            acks++;
        } else if (flag == FLAG_NACK) {
            nacks++;
        }
    }
    void record_corrupted() {
        corrupted_packets++;
    }
    void record_ignored() {
        ignored++;
    }
    void report(bool final=false) {
        auto now = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now - last_stats_time).count();
        if(final || elapsed >= 1000) {
            double mbps = data_bytes * 8.0 / elapsed * 1000 / 1e6;
            if (mbps == 0 || std::isinf(mbps)) {
                last_stats_time = now;
                reset();
                return;
            }
            
            if (csv_mode) {
                stream << "STATS," << mbps << "," << elapsed << "," 
                        << data_packets << "," << acks << "," << nacks << ","
                        << corrupted_packets << "," << ignored << std::endl;
            } else if (report_percent) {
                stream << "[STATISTICS] Throughput: " << mbps << " Mbps, "
                            << "Packets " << senderText << ": " << data_packets 
                            << " ACKS%: " << percent(acks, data_packets)
                            << " NACKS%: " << percent(nacks, data_packets) 
                            << " Corrupted%: " << percent(corrupted_packets, data_packets) 
                            << " Ignored%: " << percent(ignored, data_packets) 
                            << std::endl;
            } else {
                stream << "[STATISTICS] Throughput: " << mbps << " Mbps, "
                            << "Packets " << senderText << ": " << data_packets 
                            << " ACKS: " << acks 
                            << " NACKS: " << nacks 
                            << " Corrupted: " << corrupted_packets 
                            << " Ignored: " << ignored 
                            << std::endl;
            }
            last_stats_time = now;
            reset();
        }
    }

    uint32_t data_bytes;
    uint32_t data_packets;
    uint32_t acks;
    uint32_t nacks;
    uint32_t corrupted_packets;
    uint32_t ignored;
    steady_clock::time_point last_stats_time;

private:
    bool report_percent = true;
    bool csv_mode = false;
    std::string senderText = "";
    std::ostream& stream;

    float percent(uint32_t stat, uint32_t total) {
        return (float)((stat * 10000) / total) / 100.0;
    } 
};

/*

data_proc(out_stream) {
    static char[] buf;
    static counter

    if (counter < max) {
        instream.read(buf[counter]);
    } else {
        write header
        for (i = 0; i < max; i++) {
            out_stream.write(buf[i]);
        }
    }
}


max = 100 * 64;

100 cycles, accumulate data from syntheitc
101 cycle: send all data on outstream

101 cycle: eth subsystem reads header[0]
102 cycle: eth subsystem reads header[1]
103 cycle: read data[0]
203 read data[max]


*/