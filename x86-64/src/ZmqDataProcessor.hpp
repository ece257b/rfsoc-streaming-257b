//
// Created by Russ Sobti on 3/5/25.
//

#ifndef ZMQDATAPROCESSOR_H
#define ZMQDATAPROCESSOR_H


#pragma once

#include <iostream>
#include <zmqpp/zmqpp.hpp>
#include "DataProcessing.hpp"  // Assuming Protocol.hpp contains the base DataProcessor declaration

// Derived class to publish data to a ZeroMQ socket.
class ZMQDataProcessor : public DataProcessor {
public:
	// Constructor accepts a reference to a ZeroMQ context and the endpoint to bind to.
	ZMQDataProcessor( const std::string& endpoint)
  		: context(zmqpp::context()), publisher(context, zmqpp::socket_type::publish)
	{
		try {
 			publisher.connect(endpoint);
		} catch (const zmqpp::zmq_internal_exception& e) {
			std::cerr << "Failed to bind ZeroMQ publisher socket: " << e.what() << std::endl;
			// Depending on your error-handling strategy, you might want to rethrow or handle the error.
		}
	}

	// Override the processData method to send data through the ZeroMQ socket.
	virtual int processData(size_t size, char* buffer) override {
		try {
			// Create a ZeroMQ message using the provided data buffer.
            zmqpp::message message;
            // send the data
			publisher.send(message);
            std::cout << "Sent message" << std::endl;
		} catch (const zmqpp::zmq_internal_exception& e) {
			std::cerr << "Failed to send message: " << e.what() << std::endl;
			return -1;
		}
		// Return the number of bytes processed (or a success code).
		return static_cast<int>(size);
	}

private:
  	zmqpp::context context;
	zmqpp::socket publisher;
};

#endif //ZMQDATAPROCESSOR_H
