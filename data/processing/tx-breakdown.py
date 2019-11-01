#!/usr/bin/env python3

import argparse
import pandas as pd

from exp_helpers import MEASURES, summarize

parser = argparse.ArgumentParser()
parser.add_argument('-i', '--input',  help='Input .hdf5 file')
parser.add_argument('-o', '--output', help='Output .csv file')
parser.add_argument('-s', '--summarize', action='store_true',
                    help='Summarize runs of the same configuration?')
args = parser.parse_args()

# Open data
df = pd.read_hdf(args.input)
dimensions = [c for c in df.columns.values if c not in MEASURES]

# Project to interesting attributes
df = df[dimensions +
        ['ClientLockingTimeAccumulated',
         'ClientDataAccessTimeAccumulated',
         'ClientVotingTimeAccumulated',
         'ClientEndingTimeAccumulated',
         'ClientExecutionTime',
         'ClientTotalTransactions']]

# Average per configuration and derive other measures
df = df.groupby(by=dimensions).sum().reset_index()
df['ClientLockingTimePerTx'] = df['ClientLockingTimeAccumulated'] / df['ClientTotalTransactions']
df['ClientDataAccessTimePerTx'] = df['ClientDataAccessTimeAccumulated'] / df['ClientTotalTransactions']
df['ClientVotingTimePerTx'] = df['ClientVotingTimeAccumulated'] / df['ClientTotalTransactions']
df['ClientEndingTimePerTx'] = df['ClientEndingTimeAccumulated'] / df['ClientTotalTransactions']
df['ClientExecutionTimePerTx'] = df['ClientExecutionTime'] / df['ClientTotalTransactions']
df['ClientOtherTimePerTx'] = df['ClientExecutionTimePerTx'] \
        - df['ClientLockingTimePerTx'] \
        - df['ClientDataAccessTimePerTx'] \
        - df['ClientVotingTimePerTx'] \
        - df['ClientEndingTimePerTx']

# Average per configuration
if args.summarize:
    df = summarize(df)

# Write result
df.sort_values(ascending=False, by=['num_server_agents_per_node', 'workload', 'num_nodes']) \
  .to_csv(args.output, index=False)
