#include <memory>
#include "StreamSender.hpp"
#include "DummyData.hpp"
#include "FileData.hpp"
#include "UDPNetworkConnection.hpp"
#include "cmn.h"

std::unique_ptr<StreamSenderInterface> senderFactory(int receiver_port, std::string& receiver_ip, std::istream& istream, int num_dummy_packets, bool debug, bool stats, int windowsize) {
    UNUSED(stats);
    std::unique_ptr<StreamSenderInterface> ptr;

    if (num_dummy_packets == -1) {
        std::cout << "streaming from file" << std::endl;
        auto sender = new StreamSender<FileReader, UDPStreamSender>(
            FileReader(istream), UDPStreamSender(receiver_port, receiver_ip), debug, windowsize
        );
        ptr.reset(sender);
    } else {
        std::cout << "streaming dummy data" << std::endl;
        auto sender = new StreamSender<DummyProvider, UDPStreamSender>(
            DummyProvider(num_dummy_packets), UDPStreamSender(receiver_port, receiver_ip), debug, windowsize
        );
        ptr.reset(sender);
    }


    return ptr;
}

int main(int argc, char* argv[]) {
    // --- Command-line parsing ---
    // Usage: ./StreamReceiver <receiver_port> windowsize [filename] [--debug] [--statistics]

    std::cout << "This is main receiver" << std::endl;

    bool debug = false;
    bool stats = false;
    int windowsize = WINDOW_SIZE;
    std::string filename = "";
    int num_dummy_packets = 1000;

    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if(arg == "--debug") {
            debug = true;
        } else if(arg == "--statistics") {
            stats = true;
        } else if (arg == "-window") {
            windowsize = std::atoi(argv[i+1]);
            i++;
        } else if (arg == "-file") {
            filename = argv[i+1];
            i++;
        } else if (arg == "-num") {
            num_dummy_packets = std::atoi(argv[i+1]);
            i++;
        } else {
            args.push_back(arg);
        }
    }
    if(args.size() < 1) {
        std::cerr << "Usage: " << argv[0] << " <receiver_ip> <receiver_port> [-file filename] [-num num_dummy_packets] [-window windowsize] [--debug] [--statistics]" << std::endl;
        return EXIT_FAILURE;
    }

    std::string receiver_ip = args[0];
    int receiver_port = std::atoi(args[1].c_str());

    std::ifstream fstream;

    if (filename != "") {
        fstream.open(filename, std::ios::binary);
        num_dummy_packets = -1;
    }

    auto receiver = senderFactory(receiver_port, receiver_ip, fstream, num_dummy_packets, debug, stats, windowsize);
    receiver->stream();
    receiver->teardown();
}
