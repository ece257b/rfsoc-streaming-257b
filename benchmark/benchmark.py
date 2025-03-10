import os
import subprocess

run_config = {
    "ip": "127.0.0.1",
    "port": "12345",
    "total_packets": 1000000,
    "window_size": 1000,
    "perror": 0
}

def run_test(config: dict):
    sender_args = ["./Streamer", config["ip"], config["port"], "-num", config["total_packets"], "-window", config["window_size"], "--csv"]
    sender_args = [str(a) for a in sender_args]
    sender = subprocess.Popen(
        sender_args
    )

    receiver_args = ["./Receiver", config["port"], "-perror", config["perror"], "-window", config["window_size"], "--csv"]
    receiver_args = [str(a) for a in receiver_args]
    receiver = subprocess.Popen(
        receiver_args
    )

    print("Spawned Sender and Receiver")
    
    receiver.wait()
    print("Receiver Done!")


if __name__ == "__main__":
    os.chdir("../x86-64/src/")

    run_test(run_config)
