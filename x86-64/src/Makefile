# Makefile for StreamSender and StreamReceiver

# Detect OS
UNAME_S := $(shell uname -s)

# Set compiler based on OS
ifeq ($(UNAME_S), Darwin)
    CXX = clang++
else
    CXX = g++
endif

CXXFLAGS := -std=c++11 -Wall -Wextra -g

ZMQ_LDFLAGS := -I /opt/homebrew/include -L /opt/homebrew/lib -L/usr/local/lib -lzmq -lboost_system -lboost_thread -lpthread -lzmqpp
LDFLAGS :=

# Automatically find all .cpp and .hpp files
SRCS := $(wildcard *.cpp)
HDRS := $(wildcard *.hpp)

# Main files (explicitly listed)
STREAMER_MAIN := MainStreamer.cpp
RECEIVER_MAIN := MainReceiver.cpp
ZMQ_MAIN := ZmqPublisher.cpp
STREAMER_BASIC_MAIN := MainBasicSender.cpp
RECEIVER_BASIC_MAIN := MainBasicReceiver.cpp

FPGA_STREAMER_TOP := FPGABasicTop.cpp

# Filter out main files from SRCS to avoid duplicate compilation
COMMON_SRCS := $(filter-out ${STREAMER_BASIC_MAIN} ${RECEIVER_BASIC_MAIN} $(STREAMER_MAIN) $(RECEIVER_MAIN) ${FPGA_STREAMER_TOP} $(ZMQ_MAIN), $(SRCS))

# Output executables
STREAMER := Streamer
RECEIVER := Receiver
ZMQPub := ZmqPublisher
STREAMER_BASIC := BasicStreamer
RECEIVER_BASIC := BasicReceiver

# Object files
OBJS := $(COMMON_SRCS:.cpp=.o)

# Default target
all: $(STREAMER) $(RECEIVER)

# Build first prografinHeader
$(STREAMER): $(OBJS) $(STREAMER_MAIN:.cpp=.o)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

# Build second program
$(RECEIVER): $(OBJS) $(RECEIVER_MAIN:.cpp=.o)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^
# Build ZMQ program

$(ZMQPub): $(OBJS) $(ZMQ_MAIN:.cpp=.o)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^


# Build first program Header
$(STREAMER_BASIC): $(OBJS) $(STREAMER_BASIC_MAIN:.cpp=.o)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

# Build first prografinHeader
$(RECEIVER_BASIC): $(OBJS) $(RECEIVER_BASIC_MAIN:.cpp=.o)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

# Compile all .cpp files to .o
%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(STREAMER_BASIC_MAIN:.cpp=.o) $(RECEIVER_BASIC_MAIN:.cpp=.o) $(STREAMER_MAIN:.cpp=.o) $(RECEIVER_MAIN:.cpp=.o) $(STREAMER) $(RECEIVER) ${STREAMER_BASIC} ${RECEIVER_BASIC} ${ZMQPub}

.PHONY: all clean
