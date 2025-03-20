import os
import subprocess
import statistics
import csv
import argparse

# Default sender configuration.
run_config = {
    "ip": "10.0.0.40",      # Default receiver IP address.
    "port": "12345",        # Default receiver port.
    "total_packets": None,  # To be provided as an argument.
    "window_size": None     # To be provided as an argument.
}

compile_config = {
    "payload_size": 2500
}

def append_to_csv(csvname, run_config, compile_config, throughput):
    newfile = not os.path.exists(csvname)
    with open(csvname, mode="a", newline="") as file:
        writer = csv.writer(file)
        row = []
        header = []
        for key in run_config:
            header.append(key)
            row.append(run_config[key])
        for key in compile_config:
            header.append(key)
            row.append(compile_config[key])
        header.append("mbps")
        row.append(throughput)
        if newfile:
            writer.writerow(header)
        writer.writerow(row)

def run_test(csvname, config: dict):
    sender_args = [
        "./Streamer",
        config["ip"],
        config["port"],
        "-num", str(config["total_packets"]),
        "-window", str(config["window_size"]),
        "--csv"
    ]
    print("Spawning Sender with command:", " ".join(sender_args))
    sender = subprocess.Popen(sender_args, stdout=subprocess.PIPE)
    
    stdout, _ = sender.communicate()
    print("Sender finished!")
    data = stdout.decode()
    
    # Extract throughput values from lines beginning with "STATS"
    values = [
        float(line.split(",")[1])
        for line in data.strip().split("\n")
        if line.startswith("STATS")
    ]
    if values:
        mean = statistics.mean(values)
    else:
        mean = 0
    print(f"Mean Throughput: {mean}")

    append_to_csv(csvname, config, compile_config, mean)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Sender Benchmark Script")
    parser.add_argument("--window_size", type=int, required=True,
                        help="Window size for the test")
    parser.add_argument("--total_packets", type=int, required=True,
                        help="Total number of packets to send")
    parser.add_argument("--csv", type=str, default="sender_result.csv",
                        help="Output CSV file for the result")
    parser.add_argument("--ip", type=str, default="10.0.0.40",
                        help="Receiver IP address")
    parser.add_argument("--port", type=str, default="12345",
                        help="Receiver port")
    args = parser.parse_args()

    # Update run configuration based on command-line arguments.
    run_config["window_size"] = args.window_size
    run_config["total_packets"] = args.total_packets
    run_config["ip"] = args.ip
    run_config["port"] = args.port

    # Change directory to where the Streamer binary is located.
    os.chdir("../x86-64/src/")
    run_test(args.csv, run_config)