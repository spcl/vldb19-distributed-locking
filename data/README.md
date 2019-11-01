## Overview

The plots are produced in a pipeline consisting of the following steps:

1. Extract the numbers from the experiment traces as JSON and combine them into a single JSON lines file.
1. Convert that file into an HDF5 file for faster processing.
1. For each plot, extract the numbers to plot into a CSV file.
1. Produce the plots from the CSV files.

## Prerequesites

Install the Python packages from the `requirements.txt` file:

```bash
pip3 install -r requirements.txt
```

## Collect data from experiment traces into JSON lines

The format of the traces has slightly changed between the version of the paper and the version in this repository.
Hence, there are two different scripts that extract the data from the traces.
All subsequent steps are the same.

To use the traces that were used in the paper (which are stored the `traces_paper` directory), run the following command:

```bash
./collect_data_paper.sh
```

To use traces that you produced yourself, extract the trace files into the `traces` directory (without further subdirectories),
then furn the following command:

```bash
./collect_data.sh
```

## Convert JSON lines to HDF5

Run the following command:

```bash
./jsonl2hdf5.py -i data.json -o data.hdf5
```

## Extract CSV data for plotting

There is roughly one script per plot that extracts the data required for the plot and stores it as a CSV file.
Each script mainly consists of cleansing the data, selecting the interesting subset, deriving new measures, and a first summarization step.
To run all scripts, use the following command
(to extract the data for a single plot, just run the corresponding script manually):

```bash
for script in ./processing/*.py
do
    echo $script
    $script -i data.hdf5 -o ./plots/$(basename ${script%.py}).csv -s
done
```

## Produce plots

There is one script per plot.
To produce all plots, run the following command
(to produce a single plot, just run the corresponding script manually **in the `plots` folder**):

```bash
(
    cd plots
    for script in ./*.py
    do
        echo $script
        $script --paper
    done
)
```

This produces a large number of plots.
Below are the filenames of the files used for the paper
given as `<pretty name> -- <original name>`.
The original name refers to the name given by the plot scripts
when run on the data derived from the original traces.

XXX: Update these

* Figure 3a: `transaction-throughput-new-num_warehouses=2048.pdf`
* Figure 3b: `locking-throughput-new.pdf`
* Figure 4:  `tx-breakdown-num_warehouses=2048.pdf`
* Figure 5a: `transaction-throughput-new-num_warehouses=64.pdf`
* Figure 5b: `transaction-throughput-new-num_warehouses=1024.pdf`
* Figure 6a: `transaction-throughput-warehouses-mechanism=2pl-bw-option1=.pdf`
* Figure 6b: `transaction-throughput-warehouses-mechanism=2pl-nw-option1=nowait.pdf`
* Figure 6c: `transaction-throughput-warehouses-mechanism=to-option1=timestamp.pdf`
* Figure 7a: `tx-breakdown-num_warehouses=64.pdf`
* Figure 7b: `abortion-rate-warehouses.pdf`
* Figure 8:  `network-delay.pdf`
