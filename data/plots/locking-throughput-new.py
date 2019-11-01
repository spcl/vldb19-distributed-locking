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

# Read data and group by configs (=everything that is neither a series nor on an axis)
df = pd.read_csv('./locking-throughput.csv').fillna('')
dimensions = [c for c in sorted(df.columns.values) if c not in MEASURES]

if args.paper:
    df = df[(df['network_delay'] == '') & (df['num_warehouses'] == 2048)]

plot_dims = ['num_tx_agents', 'num_cores', 'num_nodes', 'num_server_agents', 'isolation_mode',
             'mechanism', 'option1', 'option2']
config_dims = [d for d in dimensions if d not in plot_dims]
nonunique_config_dims = [d for d in config_dims if len(df[d].unique()) > 1]

groups = df.groupby(by=config_dims)

# Produce a plot for each config
for g in groups:
    config_name = "".join(["-{}={}".format(k, v) for k, v in zip(config_dims, list(g[0]))
                           if k in nonunique_config_dims])

    plt.style.use(['./ethplot.mplstyle'])
    markers = ['o', 'x', '*', '^']
    linestyles = ['-', '--', '-.']
    labels = {'readcommitted': ' (RC)',
              'serializable': ' (Ser)',
              'repeatableread': ' (RR)'}
    prop_cycle = plt.rcParams['axes.prop_cycle']
    colors = prop_cycle.by_key()['color']

    fig = plt.figure()
    ax = fig.add_subplot(1, 1, 1)

    ax.set_xlabel('Transaction Processing Agents')
    ax.set_ylabel('Throughput [million requests/sec]')

    ax.get_xaxis().tick_bottom()
    ax.get_yaxis().tick_left()

    df_g = g[1]

    lines = []
    for i, mode in enumerate(sorted(df['isolation_mode'].unique())):
        if mode == '': continue
        linestyle = linestyles[i]

        for j, mechanism in enumerate(sorted(df['mechanism'].unique())):
            df_m = df_g[(df_g['isolation_mode'] == mode) & (df_g['mechanism'] == mechanism)]

            if len(df_m.index) == 0: continue

            yAxisPoints = df_m['SystemLockingThroughput mean']
            yAxisError  = df_m['SystemLockingThroughput ci950']
            xAxisPoints = df_m['num_tx_agents']

            marker = markers[j]
            color = colors[j]
            label = mechanism.upper() + labels[mode]
            line = ax.errorbar(xAxisPoints, yAxisPoints, yAxisError,
                               linestyle=linestyle, marker=marker,
                               linewidth=2, markersize=6, label=label)
            lines.append(line)

    ax.legend(handles=lines, loc='upper left')

    ax.set_xlim(left=0)
    ax.set_ylim(bottom=0)

    X = df_g['num_tx_agents'].unique()
    X = [x for x in X if x in [16, 128, 256, 512, 1024, 2048]]
    ax.set_xticks(X)

    plt.savefig("./locking-throughput-new{}.pdf".format(config_name), format='pdf')
    plt.close()
