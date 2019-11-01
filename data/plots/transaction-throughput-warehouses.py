#!/usr/bin/env python3

import argparse
import itertools
import pandas as pd
from matplotlib import pyplot as plt, font_manager
from exp_helpers import MEASURES

parser = argparse.ArgumentParser()
parser.add_argument('-p', '--paper', action='store_true',
                    help='Produce only configurations used in paper.')
args = parser.parse_args()

############## DATA ################

# Read data and group by configs (=everything that is neither a series nor on an axis)
df = pd.read_csv('./transaction-throughput.csv').fillna('')
dimensions = [c for c in sorted(df.columns.values) if c not in MEASURES]

df = df[df['workload'] == 'tpcc']

if args.paper:
    df = df[(df['network_delay'] == '') &
            (df['isolation_mode'] == 'serializable') &
            (df['mechanism'] != '2pl-wd')]

plot_dims = ['num_tx_agents', 'num_cores', 'num_nodes', 'num_server_agents', 'num_warehouses']
config_dims = [d for d in dimensions if d not in plot_dims]
nonunique_config_dims = [d for d in config_dims if len(df[d].unique()) > 1]

groups = df.groupby(by=config_dims)

# Produce a plot for each config
for g in groups:
    config_name = "".join(["-{}={}".format(k, v) for k, v in zip(config_dims, list(g[0]))
                           if k in nonunique_config_dims])

    plt.style.use(['./ethplot.mplstyle'])
    markers = ['o', 'x', '*', '^']
    linestyles = ['-', '--', ':', '-.']
    prop_cycle = plt.rcParams['axes.prop_cycle']
    colors = prop_cycle.by_key()['color']

    fig = plt.figure(figsize=(5, 4.5))
    ax = fig.add_subplot(1, 1, 1)

    ax.set_xlabel('Transaction Processing Agents')
    ax.set_ylabel('Throughput\n[million successful transactions/sec]')

    ax.get_xaxis().tick_bottom()
    ax.get_yaxis().tick_left()
    ax.set_xscale("log")
    ax.set_yscale("log")

    df_g = g[1]

    # Plot total transaction throughput
    lines = []
    X = set()
    num_warehouses = [int(n) for n in df['num_warehouses'].unique() if n != '']
    for i, nwh in enumerate(sorted(num_warehouses)):
        linestyle = linestyles[i]

        df_m = df_g[df_g['num_warehouses'] == nwh]

        if len(df_m.index) == 0: continue

        yAxisPoints1 = df_m['SystemSuccessfulTransactionThroughput mean']
        yAxisError1  = df_m['SystemSuccessfulTransactionThroughput ci950']
        xAxisPoints1 = df_m['num_tx_agents']

        X |= set(xAxisPoints1)

        label = "{} warehouses".format(nwh)
        marker = markers[i]
        color = colors[i]
        line = ax.errorbar(xAxisPoints1, yAxisPoints1, yAxisError1,
                           linewidth=2, markersize=6, label=label,
                           color=color, linestyle=linestyle, marker=marker)
        lines.append(line)

    ax.legend(handles=lines, loc='upper left')

    X = list(sorted(list(X)))
    ax.set_xticks(X)
    ax.set_xticklabels(X)
    Y = [2**(i-2) for i in range(8)]
    ax.set_yticks(Y)
    ax.set_yticklabels(Y)
    ax.set_ylim(bottom=0.15)

    plt.savefig("./transaction-throughput-warehouses{}.pdf".format(config_name), format='pdf')
    plt.close()
