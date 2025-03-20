# rfsoc-streaming-257b
UCSD ECE 257B Project - Streaming from RFSoc

This repo contains an implementation of our Sliding Window ACK/NACK based Streaming protocol, currently implemented for one way streaming between two PCs.
The implementation has abstractions allowing it to be extended in the future to stream between an RFSoC (ZCU111 FPGA) and a PC and zmq+srsRAN.

For ECE 257B Cross-Evaluation, please follow [Build](#build), [Running the Code](#running-the-code), [Benchmarking](#benchmarking), and [Protocol Validation](#protocol-validation).


- [rfsoc-streaming-257b](#rfsoc-streaming-257b)
  - [Usage](#usage)
    - [Build](#build)
    - [Running the Code](#running-the-code)
    - [Benchmarking](#benchmarking)
    - [Protocol Validation](#protocol-validation)
    - [Command Line Args in Detail](#command-line-args-in-detail)
  - [File Overview](#file-overview)
    - [`x86-64/src`](#x86-64src)
      - [Streaming Protocol Implementation](#streaming-protocol-implementation)
      - [Abstractions and Implementations](#abstractions-and-implementations)
      - [Basic TCP Implementation](#basic-tcp-implementation)
    - [`Vitis HLS codes`](#vitis-hls-codes)
    - [`docs`](#docs)
    - [`x86-64/initial`](#x86-64initial)
    - [`benchmark`](#benchmark)


## Usage

### Build
Requirements
- Tested for MacOS or Ubuntu
- Streaming
  - C++11
- python 3.8+, matplotlib, pandas for plotting


To build the `MainStreamer` and `MainReceiver`, run the following, which will generate the `Streamer` and `Receiver` executables in `x86-64/src`

```sh
cd x86-64/src
make
```

### Running the Code

To test the performance of our protocol, we can run `Streamer` and `Receiver` with different window sizes and various packet drop probability and monitor the throughput.

```sh
cd x86-64/src
make

# Stream 10000 dummy packets on localhost to port 12345, with windowsize 100
./Streamer 127.0.0.1 12345 -num 100000 -window 100

# RUN ME IN A NEW SHELL
# Receive the stream on port 12345
./Receiver 12345 -window 100

# Alternatively, with packet drop probability...
# Receive the stream on port 12345, with 1% of packets dropped (simulating bad channel/congestion)
./Receiver 12345 -window 100 -perror 0.01
```

Running these commands will print the statistics, where you can see the Throughput in Mbps.
Feel free to experiment with various values of `window` and `perror`, just ensure the `window` parameters for `Streamer` and `Receiver` match.

### Benchmarking

Now we can run the benchmarking script to see how probability of error affects the optimal window size, and see what the peak throughput is.

```sh
cd x86-64/src
make

cd ../../benchmark

 # (delete previous test results, if they exist)
rm -f full_test.csv   

# Run the benchmark, with 14 different window sizes and 4 different values of perror
# This should take around 5 mins
python3 benchmark.py

# Plot the results saved in full_test.csv
python3 plot.py
```

You should end up with something like [this](docs/ThroughputVsMbps.png).

### Protocol Validation

To verify the streaming protocol is 100% reliable, we can try streaming a file and verify that the output matches.
We can induce random errors to verify that the checksum/NACK mechanism is working as intended, and all data is received correctly even when packets need to be dropped because of low channel quality/congestion.


```sh
cd x86-64/src
make all

# Generate a random file
head -c 1000000 </dev/urandom >myfile   

# Stream myfile on localhost to port 12345, with windowsize 100
./Streamer 127.0.0.1 12345 -file myfile -window 100


# RUN ME IN A NEW SHELL
# Receive the stream on port 12345, saving the data to outfile, with windowsize 100 and simulated packet drop probability 0.1 (10%)
./Receiver 12345 -file outfile -window 100 -perror 0.1


# Compare the files
diff myfile outfile
```

If all is well, the `diff` command will have no output!

### Command Line Args in Detail
```sh
./Streamer <receiver_ip> <receiver_port> [-file filename] [-num num_dummy_packets] [-window windowsize] [--debug] [--csv] [--superdumb]
```

- Required: `receiver_ip` and `receiver_port` define the destination of the `Receiver` of the stream. Use `127.0.0.1` for localhost/loopback.
- `-file filename` stream from a file
- `-num num_dummy_packets` stream some number of dummy packets (instead of file data)
- `-window windowsize` specify the window size
- `--debug` print debug logs
- `--csv` print statistics as CSV
- `--superdumb` don't generate dummy data, just use the data already in the buffer for max speed

```sh
./Receiver <receiver_port> [-file filename] [-perror err] [-window windowsize] [--debug] [--csv]
```

- Required: `receiver_port` define the port the `Receiver` should listen on. Must match the `receiver_port` from `Streamer`
- `-file filename` output received data to a file
- `-window windowsize` specify the window size
- `--debug` print debug logs
- `--csv` print statistics as CSV


## File Overview

If you're here, you're most likely looking for the [Streaming Protocol Implementation](#streaming-protocol-implementation) or the [benchmarking scripts](#benchmark)

### `x86-64/src`

#### Streaming Protocol Implementation
- `MainStreamer.cpp`
  - Top level Stream Sender program for running tests
- `MainReceiver.cpp`
  - Top level Stream Receiver program for running tests
- `StreamSender.hpp, StreamSender_impl.hpp`
  - Contains implementation for the Streaming Protocol logic on the sender side, as well as the `StreamSender` interface.
  - The `StreamSender` is templated to abstract a `DataProvider` and `NetworkConnection`
- `StreamReceiver.hpp, StreamReceiver_impl.hpp`
  - Contains implementation for the Streaming Protocol logic on the receiver side, as well as the `StreamReceiver` interface.
  - The `StreamReceiver` is templated to abstract a `DataProcessor` and `NetworkConnection`

#### Abstractions and Implementations
- `DataProcessing.hpp`
  - Contains the `DataProvider` abstraction for abstracting getting data to stream
    - `FileData.hpp : FileReader` reads data from a file
    - `DummyData.hpp : DummyProvider` creates dummy data for testing
    - `FPGADataProvider.hpp : FPGADataProvider` stub for future implementation of reading RF data on FPGA
  - Contains the `DataProcessor` abstraction for abstracting what to do with received data
    - `FileData.hpp : FileWriter` writes received data to a file
    - `DummyData.hpp : DummyProcessor` prints received data or does nothing
    - `ZmqDataProcessor.hpp : ZmqDataProcessor` publishes received data to a ZeroMQ socket, for future connection to srsRAN
- `NetworkConnection.hpp`
  - Contains the `NetworkConnection` abstraction for abstracting sending/receiving data over the network
    - `UDPNetworkConnection.hpp`
      - `UDPStreamSender / UDPStreamReceiver` create simple udp sockets to send packets
      - `FaultyUDPStreamReceiver` acts as a `UDPStreamReceiver`, except has some probability to flip a bit in the received packet, simulating low channel quality or congestion on the channel.
    - `FPGANetworkConnection.hpp : FPGANetworkConnection` stub for future implementation of sending data to Ethernet Subsystem on RFSoC

#### Basic TCP Implementation
- `MainBasicSender.cpp / BasicSender.hpp` implement simple TCP streaming using the `DataProvider` and `NetworkConnection` abstractions
- `MainBasicReceiver.cpp / BasicReceiver.hpp` implement simple TCP streaming using the `DataProcessor` and `NetworkConnection` abstractions

### `Vitis HLS codes`
- Contains HLS implementation for FPGA of basic data packetization to send to Ethernet Subsystem
- Currently not integrated with Streaming Protocol

### `docs`
- `ICD.md` Contains Interface Control Document for Streaming Protocol
  - TODO: Rendered in `ICD.pdf`
- Meeting notes

### `x86-64/initial`
- `StreamReceiver.cpp/StreamSender.cpp` Contains initial implementation of the streaming protocol

### `benchmark`
- `benchmark.py` Contains script for running streaming benchmark with multiple window sizes and different simulated probability of error
  - Reports result in a `csv`
- `plot.py` Plot data from `csv` generated by `benchmark.py`

