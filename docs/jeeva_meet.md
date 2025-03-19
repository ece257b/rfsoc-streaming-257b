## Meeting with Jeeva
Agrim, Jeeva, Kishore, Russ, Daniel

01/24/25

- Streaming Design End Goal
    - Rx IQ from antenna + processing
    - Add headers, packet fragmentation
        - Standard Ethernet Headers, but we should understand which headers are required
    - Feed data to Ethernet subsystem
    - Transfer data to Host PC
    - Capture over SFP + Remove headers, write data to file
    - Feed to SRS RAN
- Streaming Step 1
    - Unidirectional synthetic data transfer
    - Headers + Packet frag required because over ethernet need to stay under max packet size
    - Optical Fiber connection
    - Write data to file

- Learning Things
    - Ethernet subsystem Xilinx Docs
        - Function and debug status
    - Wireshark
        - Interface ports, read data from interface
    - Host receiving/transfer data codebase
        - max speed of read/write etc
    - ? SRS RAN zmq interface
        - Decide if we should do this in 1-2 weeks?
    - ? UHD protocol

- All codes should be access compliant so we can interface between modules easily

- Integration / Timeline
    - FPGA -> Host PC synth data to file
    - Host -> FPGA observe on ILA
    - Test max speeds ^
    - Interface DAC/ADC data and perform streaming
        - Set sampling rate so we can't stop the data so can't risk loosing data
    - Time synchronize both transfers
        - Read and write is happening at the same time, so need to go back and forth
    - zmq ... etc

- TODOs
    - Next two weeks deliverables
        - FPGA (Kishore)
            - look at docs for ethernet block, how to write counter, interface with this block, generate synth data
            - Explore ethernet on both ends so that we don't miss some compatibility
        - Host (Daniel + Russ)
            - Ethernet frame spec, type headers, what does it look like, what fields do we need, etc
                - communicate clearly to Kishore on FPGA on spec for headers etc
            - look at packet headers and fragmentation specifics
            - Understand wireshark
            - Host to host test on loopback
        - zmq
            - SRS RAN simulation mode working
            - Agrim write basic program to subscribe to TX, adds noise, publish to RX, "sim loop within sim loop"
                - If this works then we just need to point subscribers to whats from FPGA 

