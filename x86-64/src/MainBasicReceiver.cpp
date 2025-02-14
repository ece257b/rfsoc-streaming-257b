#include "BasicReceiver.hpp"
#include "UDPNetworkConnection.hpp"
#include "DummyData.hpp"

int main(int argc, char* args[]) {
    BasicReceiver<DummyProcessor, UDPStreamReceiver> receiver = BasicReceiver<DummyProcessor, UDPStreamReceiver>();

    int receiver_port = std::atoi(args[1]);
    receiver.conn.setup(receiver_port);

    receiver.receiveStream();
}