#!/usr/bin/env python3

from sys import stdin
import pandas as pd

df = pd.read_json(stdin, lines=True)

trx_df = df.groupby('trx_id').agg({ 'partition_no': ['count', 'nunique'] }).reset_index()

print(trx_df)
