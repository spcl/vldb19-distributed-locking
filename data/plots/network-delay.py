#!/usr/bin/env python3

import itertools
import pandas as pd
from cycler import cycler
from matplotlib import pyplot as plt, font_manager, transforms
from exp_helpers import MEASURES

# Read data and group by configs (=everything that is neither a series nor on an axis)
df = pd.read_csv('./transaction-throughput.csv').fillna('')
dimensions = [c for c in sorted(df.columns.values) if c not in MEASURES]

df = df[df['network_delay'] != '']
df['network_delay'] = df['network_delay'] / 1000

plot_dims = ['network_delay', 'mechanism', 'isolation_mode', 'option1', 'option2']
config_dims = [d for d in dimensions if d not in plot_dims]
nonunique_config_dims = [d for d in config_dims if len(df[d].unique()) > 1]

groups = df.groupby(by=config_dims)

# Produce a plot for each config
for g in groups:
    config_name = "".join(["-{}={}".format(k, v) for k, v in zip(config_dims, list(g[0]))
                           if k in nonunique_config_dims])

    df_g = g[1]

    if len(df_g.index) < 2: continue

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
    ax.set_xscale("log")

    ax.set_xlabel('Network delay [microsec]')
    ax.set_ylabel('Throughput [million transactions/sec]')

    trans = transforms.blended_transform_factory(ax.transData, ax.transAxes)
    for x, label in [(1, 'IB EDR'), (16, 'AWS EFA')]:
        ax.axvline(x, color='red')
        plt.text(x, 1.01, label, transform=trans, va='bottom', ha='center')

    ax.get_xaxis().tick_bottom()
    ax.get_yaxis().tick_left()

    lines = []
    for i, mode in enumerate(sorted(df['isolation_mode'].unique())):
        if mode == '': continue
        linestyle = linestyles[i]

        for j, mechanism in enumerate(sorted(df['mechanism'].unique())):
            df_m = df_g[(df_g['isolation_mode'] == mode) & (df_g['mechanism'] == mechanism)]

            if len(df_m.index) == 0: continue

            xAxisPoints = df_m['network_delay']
            yAxisPoints = df_m['SystemTransactionThroughput mean']
            yAxisError  = df_m['SystemTransactionThroughput ci950']

            marker = markers[j]
            color = colors[j]
            label = mechanism.upper() + labels[mode]

            line = ax.errorbar(xAxisPoints, yAxisPoints, yAxisError,
                               color=color, linestyle=linestyle, marker=marker,
                               linewidth=2, markersize=6, label=label)
            lines.append(line)

    ax.legend(handles=lines)

    X = [x for x in df_g['network_delay'].unique() if x != 0]
    ax.set_xticks(X)
    ax.set_xticklabels(['{:.4g}'.format(x) for x in X])
    ax.set_ylim(bottom=0)

    plt.savefig("./network-delay{}.pdf".format(config_name), format='pdf')
    plt.close()
