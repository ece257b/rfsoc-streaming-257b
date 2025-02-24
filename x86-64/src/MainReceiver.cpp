#include <memory>
#include "StreamReceiver.hpp"
#include "DataWindow.hpp"
#include "DummyData.hpp"
#include "UDPNetworkConnection.hpp"
#include "cmn.h"

int main(int argc, char* argv[]) {
    // --- Command-line parsing ---
    // Usage: ./StreamReceiver <receiver_port> [filename] [--debug] [--statistics]

    std::cout << "This is main receiver" << std::endl;

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

    if(args.size() < 1) {
        std::cerr << "Usage: " << argv[0] << " <receiver_port> [filename] [--debug] [--statistics]" << std::endl;
        return EXIT_FAILURE;
    }
    int receiver_port = std::atoi(args[0].c_str());

    auto receiver = StreamReceiver<DummyProcessor, PacketMap<Packet>, UDPStreamReceiver>(debug);
    receiver.processor.print = debug;
    receiver.conn.setup(receiver_port);
    receiver.receiveData();
    receiver.teardown();
}
