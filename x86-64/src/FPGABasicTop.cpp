#include "BasicSender.hpp"
#include "UDPNetworkConnection.hpp"
#include "FPGADataProvider.hpp"

// @Kishore
int top(...) {
    BasicSender<FPGADataProvider, UDPStreamSender> sender = BasicSender<FPGADataProvider, UDPStreamSender>(10);

    // Setup if needed
    // sender.provider.setup()
    // sender.conn.setup();

    sender.stream();
}