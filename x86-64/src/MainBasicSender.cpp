#include "BasicSender.hpp"
#include "UDPNetworkConnection.hpp"
#include "DummyData.hpp"

int main(int argc, char* args[]) {
    BasicSender<DummyProvider, UDPStreamSender> sender = BasicSender<DummyProvider, UDPStreamSender>(10);

    std::string receiver_ip = args[1];
    int receiver_port = std::atoi(args[2]);
    sender.conn.setup(receiver_port, receiver_ip);

    sender.stream();
}