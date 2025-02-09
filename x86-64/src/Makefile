# Makefile for StreamSender and StreamReceiver

# Compiler and flags
CXX      := g++
CXXFLAGS := -O2 -std=c++17 -Wall -Wextra 

# Automatically find all .cpp and .hpp files
SRCS := $(wildcard *.cpp)
HDRS := $(wildcard *.hpp)

# Main files (explicitly listed)
STREAMER_MAIN := MainStreamer.cpp StreamSender.cpp
RECEIVER_MAIN := MainReceiver.cpp StreamReceiver.cpp

# Filter out main files from SRCS to avoid duplicate compilation
COMMON_SRCS := $(filter-out $(STREAMER_MAIN) $(RECEIVER_MAIN), $(SRCS))

# Output executables
STREAMER := Streamer
RECEIVER := Receiver

# Object files
OBJS := $(COMMON_SRCS:.cpp=.o)

# Default target
all: $(STREAMER) $(RECEIVER)

# Build first prografinHeader
$(STREAMER): $(OBJS) $(STREAMER_MAIN:.cpp=.o)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build second program
$(RECEIVER): $(OBJS) $(RECEIVER_MAIN:.cpp=.o)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile all .cpp files to .o
%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(STREAMER_MAIN:.cpp=.o) $(RECEIVER_MAIN:.cpp=.o) $(STREAMER) $(RECEIVER)

.PHONY: all clean
