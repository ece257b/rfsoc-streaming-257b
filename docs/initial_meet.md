## Initial Meeting with Agrim
Daniel, Kishore, Russ, Agrim
01/22/2025

Sorry for incorrect spelling everywhere, feel free to edit

Take RFSock and stream data to a server
Requires 3 subprojects
- RF Config: already familiar, PHYFW part for RFSock
- Streaming to ZMQ
	- start with streaming random data to PC
	- Start with streaming to Wireshark
		- Meeting with Jiva, tried getting this on Wireshark
		- Read about what exists out there, how do we stream anything? Just how to send to a server using IP with any high level interface
		- Dumping data into binary file, read and verify with Matlab
	- Create an actual protocol where we're sending the frame, then storing it, etc etc
		- Checksum? Parity?
		- Don't reinvent the wheel, use a good protocol if it exists. Maybe RDMA
		- We basically need a High Bandwidth transport protocol, but with some check/ACK. Need to log the dropped stuff so can't 
	- Duplex? Start with bidirectional basic, so we can do either way
- Connect to SRSRan
	- Using ZMQ interface
	- Open Source 5G base station stack
	- Connect it to a radio and you can create a real base station and connect your phone
	- USRP is low bandwidth so SRSRan, inconsistency with dropping packets at high bandwidth
		- This is basically the goal, to test our streaming with higher bandwidth and hope to see improvement over USRP
		- We should report where packets are getting dropped, which side of streaming, etc 
	- "Lofty goal"
- Midterm
	- Filedump, basic verify samples to and from RFSock, some idea about transport
	- Small simulator that we can run within a server?

