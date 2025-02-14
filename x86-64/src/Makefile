# Makefile for StreamSender and StreamReceiver

# Compiler and flags
CXX      := g++
CXXFLAGS := -O2 -std=c++11 -Wall -Wextra 

# Automatically find all .cpp and .hpp files
SRCS := $(wildcard *.cpp)
HDRS := $(wildcard *.hpp)

# Main files (explicitly listed)
STREAMER_MAIN := MainStreamer.cpp
RECEIVER_MAIN := MainReceiver.cpp

STREAMER_BASIC_MAIN := MainBasicSender.cpp
RECEIVER_BASIC_MAIN := MainBasicReceiver.cpp

# Filter out main files from SRCS to avoid duplicate compilation
COMMON_SRCS := $(filter-out ${STREAMER_BASIC_MAIN} ${RECEIVER_BASIC_MAIN} $(STREAMER_MAIN) $(RECEIVER_MAIN), $(SRCS))

# Output executables
STREAMER := Streamer
RECEIVER := Receiver

STREAMER_BASIC := BasicStreamer
RECEIVER_BASIC := BasicReceiver

# Object files
OBJS := $(COMMON_SRCS:.cpp=.o)

# Default target
all: $(STREAMER) $(RECEIVER) ${STREAMER_BASIC} ${RECEIVER_BASIC}

# Build first prografinHeader
$(STREAMER): $(OBJS) $(STREAMER_MAIN:.cpp=.o)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build second program
$(RECEIVER): $(OBJS) $(RECEIVER_MAIN:.cpp=.o)
	$(CXX) $(CXXFLAGS) -o $@ $^


# Build first prografinHeader
$(STREAMER_BASIC): $(OBJS) $(STREAMER_BASIC_MAIN:.cpp=.o)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build first prografinHeader
$(RECEIVER_BASIC): $(OBJS) $(RECEIVER_BASIC_MAIN:.cpp=.o)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile all .cpp files to .o
%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(STREAMER_BASIC_MAIN:.cpp=.o) $(RECEIVER_BASIC_MAIN:.cpp=.o) $(STREAMER_MAIN:.cpp=.o) $(RECEIVER_MAIN:.cpp=.o) $(STREAMER) $(RECEIVER) ${STREAMER_BASIC} ${RECEIVER_BASIC}

.PHONY: all clean
