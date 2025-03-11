from scapy.all import Raw, Ether, ARP, sendp, sniff

# Function to process received packets
def packet_callback(packet):
    print(packet[Raw].load)

# Sniff for packets on the localhost interface
sniff(iface="lo0", prn=packet_callback, count=1, timeout=20)  # Change 'lo0' to your interface if necessary