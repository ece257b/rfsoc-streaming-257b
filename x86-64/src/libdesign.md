Sender

- class StreamSender
  - This class should contain all protocol specific logic, and delegate data reading and buffering to DataProvider and DataWindow
  - setup()
    - Set up connection and negotiation handshake
  - stream()
    - Main streaming loop
    - Send window of packets: calls sendData()
    - calls processPacket()
    - getTimedOut() and sendPacket() if necessary
  - sendData(seqnum)
    - Send one packet on socket.
    - Gets data from DataWindow
      - If not in buffer, gets data from DataProvider
  - receivePacket()
    - handles ACK/NACK logic
  - getTimedOut()
    - gets the first timed out packet if one is available
  - teardown()
    - FIN/FINACK logic

- class DataProvider
  - Interface to read data sequentially. Can use for dummy, file, or stream
  - int open()
  - int close()
  - int getData(size, *buffer)

- class DataWindow
  - Interface for a sliding window data buffer. Manages memory for sliding window
  - *buffer reserve(seq_num)
    - Reserves space for seq_num in window, and returns pointer to the memory. nullptr if no space available
  - bool contains(seq_num)
  - bool get(seq_num, **buffer)
  - bool erase(seq_num)
  - bool pop(seq_num, **buffer)
    - get the element and erase it
  - bool isFull()
  - bool isEmpty()


Client

- class StreamReceiver
  - setup()
  - receiveStream()
    - Main receive streaming loop
    - receivePacket
  - receiveData()
    - Receive a data packet. Process if in order. Store it in DataWindow if out of order
  - sendNACK()
  - sendACK()
  - teardown()

- class DataProcessor
  - Interface for sequentially processing data
  - int open()
  - int close()
  - int processData(size, *buffer)
    - May need a copy here? Need to make sure that we finish writing buffer before DataWindow clears it

