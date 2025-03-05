import zmq

# Create a ZeroMQ context
context = zmq.Context()

# Create a SUB (subscriber) socket
socket = context.socket(zmq.SUB)

# Connect to the publisher on port 5555
socket.connect("tcp://localhost:5555")

# Subscribe to all topics (empty string subscribes to everything)
socket.setsockopt_string(zmq.SUBSCRIBE, "")

print("Listening for messages on port 5555...")

try:
    while True:
        # Receive a message and decode it as a string
        message = socket.recv_string()
        print("Received:", message)
except KeyboardInterrupt:
    print("Shutting down...")
finally:
    socket.close()
    context.term()
