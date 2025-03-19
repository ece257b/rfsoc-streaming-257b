import os
import pandas as pd
import matplotlib.pyplot as plt

# Read CSV file

WINDOW_CSV_PATH = os.path.join(os.getcwd(), "window.csv")
ERROR_CSV_PATH = os.path.join(os.getcwd(), "error.csv")

def plot(csv, xcol, ycol, logx=False, logy=False):
    df = pd.read_csv(csv)
    fig, ax = plt.subplots()
    
    xdata = df[xcol]
    ydata = df[ycol]
    if logx:
        ax.set_xscale('log')
    if logy:
        ax.set_yscale('log')
    ax.plot(xdata, ydata, marker='o', linestyle='-', color='b')


    ax.set_xlabel(xcol)
    ax.set_ylabel(ycol)

    ax.set_title(f"Plot of {xcol} vs {ycol}")
    ax.grid(True)
    plt.show()

# plot(WINDOW_CSV_PATH, "window_size", "mbps")

# plot(ERROR_CSV_PATH, "perror", "mbps", logx=True)


def multiplot_error_windowsize(csv):
    df = pd.read_csv(csv)
    df = df.sort_values(by="window_size")
    fig, ax = plt.subplots()


    errors = df['perror'].unique()
    colors = ['b', 'g', 'r', 'c', 'm', 'y', 'k']

    for i, error in enumerate(errors):
        temp_df = df[df['perror'] == error]
        plt.plot(temp_df['window_size'], temp_df['mbps'], color=colors[i % len(colors)], label=f'Error: {error}', marker='o')

    plt.title('Throughput (Mbps) vs Windowsize')
    plt.xlabel('Windowsize')
    plt.ylabel('Throughput (Mbps)')
    plt.legend()
    plt.show()


multiplot_error_windowsize("full_test.csv")