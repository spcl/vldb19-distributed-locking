#!/usr/bin/env python3

import argparse
import itertools
import pandas as pd
import matplotlib; matplotlib.use('Agg')
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
    df = df[(df['network_delay'] == '') & (df['num_warehouses'] == 64)]

plot_dims = ['num_tx_agents', 'num_cores', 'num_nodes', 'num_server_agents',
             'mechanism', 'option1', 'option2', 'isolation_mode']
config_dims = [d for d in dimensions if d not in plot_dims]
nonunique_config_dims = [d for d in config_dims if len(df[d].unique()) > 1]

groups = df.groupby(by=config_dims)

# Produce a plot for each config
for g in groups:
    config_name = "".join(["-{}={}".format(k, v) for k, v in zip(config_dims, list(g[0]))
                           if k in nonunique_config_dims])

    plt.style.use(['./ethplot.mplstyle'])
    markers = ['o', 'x', '*', '^']
    linestyles = ['-', '--', ':']
    labels = {'readcommitted': ' (RC)',
              'serializable': ' (Ser)',
              'repeatableread': ' (RR)'}
    prop_cycle = plt.rcParams['axes.prop_cycle']
    colors = prop_cycle.by_key()['color']

    fig = plt.figure()
    ax = fig.add_subplot(1, 1, 1)

    ax.set_xlabel('Transaction Processing Agents')
    ax.set_ylabel('Abort Rate [%]')

    ax.get_xaxis().tick_bottom()
    ax.get_yaxis().tick_left()
    ax.set_xscale('log')

    df_g = g[1]

    # Plot total transaction throughput
    lines = []
    for i, mode in enumerate(df['isolation_mode'].unique()):
        if mode == '': continue
        linestyle = linestyles[i]

        for j, mechanism in enumerate(sorted(df['mechanism'].unique())):
            df_m = df_g[(df_g['isolation_mode'] == mode) & (df_g['mechanism'] == mechanism)]

            if len(df_m.index) == 0: continue

            yAxisPoints1 = (1 - df_m['SystemCommitRate mean']) * 100
            xAxisPoints1 = df_m['num_tx_agents']

            label = mechanism.upper() + labels[mode]
            marker = markers[j]
            color = colors[j]
            line = ax.errorbar(xAxisPoints1, yAxisPoints1,
                               linewidth=2, markersize=6, label=label,
                               color=color, linestyle=linestyle, marker=marker)
            lines.append(line)

    ax.legend(handles=lines)

    X = df_g['num_tx_agents'].unique()
    ax.set_xticks(X)
    ax.set_xticklabels(X)
    ax.set_ylim([0, 100])

    plt.savefig("./abortion-rate-warehouses{}.pdf".format(config_name), format='pdf')
    plt.close()
