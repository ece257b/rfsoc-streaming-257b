#include <memory>
#include "StreamSender.hpp"
#include "DataWindow.hpp"
#include "DummyData.hpp"
#include "UDPNetworkConnection.hpp"
#include "cmn.h"

int main(int argc, char* argv[]) {
    // --- Command-line parsing ---
    // Usage: ./StreamSender <receiver_ip> <receiver_port> [filename] [--debug] [--statistics]

    std::cout << "This is main streamer" << std::endl;

    bool debug = false;
    bool stats = false;
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if(arg == "--debug") {
            debug = true;
        } else if(arg == "--statistics") {
            stats = true;
        } else {
            args.push_back(arg);
        }
    }
    UNUSED(stats);

    if(args.size() < 2) {
        std::cerr << "Usage: " << argv[0] << " <receiver_ip> <receiver_port> [filename] [--debug] [--statistics]" << std::endl;
        return EXIT_FAILURE;
    }
    std::string receiver_ip = args[0];
    int receiver_port = std::atoi(args[1].c_str());

    auto sender = StreamSender<DummyProvider, PacketMap<PacketInfo>, UDPStreamSender>(debug);
    sender.conn.setup(receiver_port, receiver_ip);
    sender.provider.total_packets = std::atoi(argv[3]);
    sender.stream();
    sender.teardown();
}
