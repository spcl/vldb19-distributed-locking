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
df = df[dimensions + ['ClientTotalTransactions', 'ClientNewOrderTransactions', 'ClientExecutionTime',
                      'ClientTransactionsCommitted']]

# Average per run
df = df.groupby(by=dimensions) \
       .agg({'ClientTotalTransactions': 'sum',
             'ClientNewOrderTransactions': 'sum',
             'ClientTransactionsCommitted': 'sum',
             'ClientExecutionTime': 'mean',
             }) \
       .reset_index()

# Compute successful NO transactions as estimate from commit ratio and total NO transactions
if 'ClientNewOrderTransactionsCommitted' not in df.columns.values:
    df['ClientNewOrderTransactionsCommitted'] = float('NaN')
df['SystemCommitRate'] = df['ClientTransactionsCommitted'] / df['ClientTotalTransactions']
missing_successful_no = df['ClientNewOrderTransactionsCommitted'].isna()
df.loc[missing_successful_no, 'ClientNewOrderTransactionsCommitted'] = \
        (df['ClientNewOrderTransactions'] * df['SystemCommitRate'])[missing_successful_no]

# Derive other measures
df['SystemTransactionThroughput'] = df['ClientTotalTransactions'] / df['ClientExecutionTime']
df['SystemNewOrderTransactionThroughput'] = df['ClientNewOrderTransactions'] / df['ClientExecutionTime']
df['SystemSuccessfulTransactionThroughput'] = df['ClientTransactionsCommitted'] / df['ClientExecutionTime']
df['SystemSuccessfulNewOrderTransactionThroughput'] = df['ClientNewOrderTransactionsCommitted'] / df['ClientExecutionTime']

# Average per configuration
if args.summarize:
    df = summarize(df)

# Write result
df.sort_values(ascending=[True, True, False], by=['num_server_agents_per_node', 'percent_remote', 'num_nodes']) \
  .to_csv(args.output, index=False)
