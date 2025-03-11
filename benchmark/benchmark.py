import os
import subprocess
import statistics
import csv
from pprint import pprint

run_config = {
    "ip": "127.0.0.1",
    "port": "12345",
    "total_packets": 400000,
    "window_size": 1000,
    "perror": 0
}

compile_config = {
    "payload_size": 2500
}


WINDOW_CSV_PATH = os.path.join(os.getcwd(), "window.csv")
ERROR_CSV_PATH = os.path.join(os.getcwd(), "error.csv")

def append_to_csv(csvname, run_config, compile_config, throughput):
    newfile = not os.path.exists(csvname)

    with open(csvname, mode="a", newline="") as file:
        writer = csv.writer(file)
        row = []
        csvkey = []
        for key in run_config:
            csvkey.append(key)
            row.append(run_config[key])

        for key in compile_config:
            csvkey.append(key)
            row.append(compile_config[key])
        
        csvkey.append("mbps")
        row.append(throughput)
        
        if newfile:
            writer.writerow(csvkey)
        writer.writerow(row)


def run_test(csvname, config: dict):
    sender_args = ["./Streamer", config["ip"], config["port"], "-num", config["total_packets"], "-window", config["window_size"], "--csv"]
    sender_args = [str(a) for a in sender_args]
    sender = subprocess.Popen(
        sender_args, stdout=subprocess.PIPE
    )

    receiver_args = ["./Receiver", config["port"], "-perror", config["perror"], "-window", config["window_size"], "--csv"]
    receiver_args = [str(a) for a in receiver_args]
    receiver = subprocess.Popen(
        receiver_args, stdout=subprocess.PIPE
    )

    print("Spawned Sender and Receiver")
    
    try:
        receiver.wait(5)
    except subprocess.TimeoutExpired as e:
        print("Timed out, terminating")
        receiver.terminate()
        sender.terminate()

    print("Receiver Done!")
    stdout, _ = receiver.communicate()
    data = stdout.decode()
    values = [
        float(line.split(",")[1]) 
        for line in data.strip().split("\n") 
        if line.startswith("STATS")
    ]
    mean = statistics.mean(values)
    print(f"Mean Throughput {mean}")

    append_to_csv(csvname, config, compile_config, mean)


if __name__ == "__main__":
    os.chdir("../x86-64/src/")

    # for windowsize in range(1, 1000, 100):
    #     run_config["window_size"] = windowsize
    #     pprint(run_config)
    #     run_test(WINDOW_CSV_PATH, run_config)

    run_config["window_size"] = 1000

    for error in [0.0001, 0.001, 0.01, 0.1]:
        run_config["perror"] = error
        pprint(run_config)
        run_test(ERROR_CSV_PATH, run_config)
        