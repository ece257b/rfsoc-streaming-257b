
// - class StreamSender
//   - This class should contain all protocol specific logic, and delegate data reading and buffering to DataProvider and DataWindow
//   - setup()
//     - Set up connection and negotiation handshake
//   - stream()
//     - Main streaming loop
//     - Send window of packets: calls sendData()
//     - calls processPacket()
//     - getTimedOut() and sendPacket() if necessary
//   - sendData(seqnum)
//     - Send one packet on socket.
//     - Gets data from DataWindow
//       - If not in buffer, gets data from DataProvider
//   - receivePacket()
//     - handles ACK/NACK logic
//   - getTimedOut()
//     - gets the first timed out packet if one is available
//   - teardown()
//     - FIN/FINACK logic
