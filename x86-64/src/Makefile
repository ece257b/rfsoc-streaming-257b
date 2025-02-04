# Makefile for StreamSender and StreamReceiver

# Compiler and flags
CXX      := g++
CXXFLAGS := -O2 -std=c++11 -Wall

# Source files
SENDER_SRC   := StreamSender.cpp
RECEIVER_SRC := StreamReceiver.cpp

# Executable names
SENDER   := StreamSender
RECEIVER := StreamReceiver

# Default target
all: $(SENDER) $(RECEIVER)

# Build StreamSender
$(SENDER): $(SENDER_SRC)
	$(CXX) $(CXXFLAGS) -o $(SENDER) $(SENDER_SRC)

# Build StreamReceiver
$(RECEIVER): $(RECEIVER_SRC)
	$(CXX) $(CXXFLAGS) -o $(RECEIVER) $(RECEIVER_SRC)

# Clean up build artifacts
clean:
	rm -f $(SENDER) $(RECEIVER)

.PHONY: all clean
