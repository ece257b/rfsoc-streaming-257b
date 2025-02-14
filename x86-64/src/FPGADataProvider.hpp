#include "DataProcessing.hpp"

class FPGADataProvider : public DataProvider {
public:
    // void setup(...) {}       // setup hls stream if necessary
    int getData(size_t size, char* buffer) override {
        // @Kishore
        // read size bytes from hls stream and write to buffer
    }
};
