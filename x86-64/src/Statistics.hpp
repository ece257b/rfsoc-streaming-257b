#include <chrono>
#include <stdint.h>
#include <iostream>

using namespace std::chrono;

class SenderStats {
private:
public:
    SenderStats();
    ~SenderStats();

    void reset() {
        sent_data_bytes = 0;
        sent_data_packets = 0;
    }
    void record_packet(uint32_t bytes) {
        sent_data_packets++;
        sent_data_bytes += bytes;
    };
    void report() {
        auto now = steady_clock::now();
        auto elapsed = duration_cast<seconds>(now - last_stats_time).count();
        if(elapsed >= 1) {
            double bps = sent_data_bytes * 8.0 / elapsed;
            if(bps >= 1e9)
                std::cout << "[STATISTICS] Throughput: " << (bps / 1e9) << " Gbps, "
                            << "Packets sent: " << sent_data_packets << std::endl;
            else
                std::cout << "[STATISTICS] Throughput: " << (bps / 1e6) << " Mbps, "
                            << "Packets sent: " << sent_data_packets << std::endl;
            last_stats_time = now;
            reset();
        }
    }
    uint32_t sent_data_bytes;
    uint32_t sent_data_packets;
    steady_clock::time_point last_stats_time;
};
