from scapy.all import Ether, ARP, sendp, sniff

# Create an Ethernet frame with an ARP request (Ethernet + ARP)
eth_frame = Ether(dst="ff:ff:ff:ff:ff:ff")

# Send the Ethernet frame over the localhost interface
sendp(eth_frame, iface="lo0")  # Change 'lo0' to the appropriate loopback interface if needed

# # Function to process received packets
# def packet_callback(packet):
#     if packet.haslayer(Ether):
#         print(f"Received Ethernet Frame: {packet.summary()}")

# # Sniff for packets on the localhost interface
# sniff(iface="lo0", prn=packet_callback, count=1, timeout=5)  # Change 'lo0' to your interface if necessary
