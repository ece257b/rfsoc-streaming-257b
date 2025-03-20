import os
import subprocess
import statistics
import csv
import argparse

# Base configuration; ip, port, and total_packets remain fixed.
run_config = {
    "ip": "192.168.1.1",
    "port": "12345",
    "total_packets": 400000,
    "window_size": None,  # to be provided via argument
    "perror": None       # to be provided via argument
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
    # Construct the receiver command-line arguments.
    receiver_args = [
        "./Receiver",
        config["port"],
        "-perror", str(config["perror"]),
        "-window", str(config["window_size"]),
        "--csv"
    ]
    receiver = subprocess.Popen(receiver_args, stdout=subprocess.PIPE)
    print("Spawned Receiver with command:", " ".join(receiver_args))
    
    try:
        receiver.wait(30)  # wait up to 30 seconds
    except subprocess.TimeoutExpired:
        print("Timed out, terminating Receiver")
        receiver.terminate()
    
    print("Receiver done!")
    stdout, _ = receiver.communicate()
    data = stdout.decode()
    
    # Extract throughput values from lines starting with "STATS"
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
    parser = argparse.ArgumentParser(description="Receiver Benchmark Script")
    parser.add_argument("--window_size", type=int, required=True,
                        help="Window size for the test")
    parser.add_argument("--perror", type=float, required=True,
                        help="Packet error rate for the test")
    parser.add_argument("--csv", type=str, default="result.csv",
                        help="Output CSV file for the result")
    args = parser.parse_args()

    # Update run configuration with provided arguments.
    run_config["window_size"] = args.window_size
    run_config["perror"] = args.perror

    # Change directory to where the Receiver binary is located.
    os.chdir("../x86-64/src/")
    run_test(args.csv, run_config)
