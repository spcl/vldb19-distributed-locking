#!/usr/bin/env python3

import argparse
import pandas as pd
from _exp_helpers import DIMENSIONS

parser = argparse.ArgumentParser(description='Convert data frame in JSON lines format to HDF5.')
parser.add_argument('-i', '--input',  help='Input .jsonl file')
parser.add_argument('-o', '--output', help='Output .hdf5 file')
args = parser.parse_args()

# Load data in JSON lines format
df = pd.read_json(args.input, lines=True).fillna('')

# Compute derived dimensions
df['num_tx_agents_per_node'] = df['num_cores_per_node'] - df['num_server_agents_per_node']
df['num_cores'] = df['num_nodes'] * df['num_cores_per_node']
df['num_tx_agents'] = df['num_nodes'] * df['num_tx_agents_per_node']
df['num_server_agents'] = df['num_nodes'] * df['num_server_agents_per_node']

# Make naming consistent
df['isolation_mode'] = df['isolation_mode'].replace({'read_com': 'readcommitted', 'rep_read': 'repeatableread'})

# Add empty values for missing dimensions
for d in DIMENSIONS:
    if d not in df.columns.values:
        df[d] = ''

# Derive mechanism name
df['mechanism'] = '2pl-bw'
df.loc[(df['option1'] == 'timestamp') | (df['option2'] == 'timestamp'), 'mechanism'] = 'to'
df.loc[(df['option1'] == 'waitdie') | (df['option2'] == 'waitdie'), 'mechanism'] = '2pl-wd'
df.loc[(df['option1'] == 'nowait') | (df['option2'] == 'nowait'), 'mechanism'] = '2pl-nw'

# Derive workload name and YCSB config
df['distribution'] = ''
df.loc[(df['option1'] == 'uni') | (df['option2'] == 'uni'), 'distribution'] = 'uni'
df.loc[(df['option1'] == 'med') | (df['option2'] == 'med'), 'distribution'] = 'med'
df.loc[(df['option1'] == 'high') | (df['option2'] == 'high'), 'distribution'] = 'high'

# Write into HDF5 format
df.reset_index().to_hdf(args.output,'data')
