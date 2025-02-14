#include "NetworkConnection.hpp"


class FPGANetworkConnection : public NetworkConnection {
public:
    // void setup(...)  if needed
    // @Kishore
    bool open() override {}
    ssize_t send(void* packet, size_t len) override {}
    ssize_t receive(void* buffer, size_t len) override {}
    bool ready(timeval timeout) override {}
    bool close() override {}
};