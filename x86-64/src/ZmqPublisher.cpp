//
// Created by Russ Sobti on 3/5/25.
//

#include <memory>
#include "StreamReceiver.hpp"
#include "DataWindow.hpp"
#include "DummyData.hpp"
#include "UDPNetworkConnection.hpp"
#include "cmn.h"
#include "ZmqDataProcessor.hpp"
#include <zmqpp/zmqpp.hpp>


std::unique_ptr<StreamReceiverInterface> receiverFactory(int receiver_port, float perror, bool debug, bool stats, int windowsize) {
    std::unique_ptr<StreamReceiverInterface> ptr;
    // set up ZMQ context
    const std::string& endpoint = "tcp://127.0.0.1:5555";
    if (perror == -1) {
        auto receiver = new StreamReceiver<ZMQDataProcessor, PacketMap<Packet>, UDPStreamReceiver>(
            ZMQDataProcessor(endpoint), PacketMap<Packet>(), UDPStreamReceiver(receiver_port), debug, windowsize
        );
        ptr.reset(receiver);
    } else {
        auto receiver = new StreamReceiver<ZMQDataProcessor, PacketMap<Packet>, FaultyUDPStreamReceiver>(
            ZMQDataProcessor(endpoint), PacketMap<Packet>(), FaultyUDPStreamReceiver(receiver_port, perror, true, 1), debug, windowsize
        );
        ptr.reset(receiver);
    }

    return ptr;
}

int main(int argc, char* argv[]) {
    // --- Command-line parsing ---
    // Usage: ./StreamReceiver <receiver_port> windowsize [filename] [--debug] [--statistics]

    std::cout << "This is main receiver" << std::endl;

    bool debug = false;
    bool stats = false;
    float perror = -1;
    int windowsize = WINDOW_SIZE;

    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if(arg == "--debug") {
            debug = true;
        } else if(arg == "--statistics") {
            stats = true;
        } else if (arg == "-perror") {
            perror = std::atof(argv[i+1]);
            std::cout << "set error " << perror << std::endl;
            i++;
        } else if (arg == "-window") {
            windowsize = std::atoi(argv[i+1]);
            i++;
        } else {
            args.push_back(arg);
        }
    }
    if(args.size() < 1) {
        std::cerr << "Usage: " << argv[0] << " <receiver_port> [-perror err] [-window windowsize] [--debug] [--statistics]" << std::endl;
        return EXIT_FAILURE;
    }

    int receiver_port = std::atoi(args[0].c_str());


    auto receiver = receiverFactory(receiver_port, perror, debug, stats, windowsize);
    receiver->receiveData();
    receiver->teardown();
}