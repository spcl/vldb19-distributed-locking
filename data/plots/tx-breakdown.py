#!/usr/bin/env python3

import argparse
import numpy as np
import pandas as pd
from matplotlib import pyplot as plt, font_manager
from palettable import tableau
from exp_helpers import MEASURES

parser = argparse.ArgumentParser()
parser.add_argument('-p', '--paper', action='store_true',
                    help='Produce only configurations used in paper.')
args = parser.parse_args()

# Read data and group by configs (=everything that is neither a series nor on an axis)
df = pd.read_csv('./tx-breakdown.csv').fillna('')
df = df.sort_values(by=['num_tx_agents'])
dimensions = [c for c in sorted(df.columns.values) if c not in MEASURES]

if args.paper:
    df = df[(df['network_delay'] == '') &
            (df['isolation_mode'] == 'serializable') &
            (df['mechanism'] == '2pl-bw') &
            ((df['num_warehouses'] ==   64) |
             (df['num_warehouses'] == 2048))]

plot_dims = ['num_tx_agents', 'num_cores', 'num_nodes', 'num_server_agents']
config_dims = [d for d in dimensions if d not in plot_dims]
nonunique_config_dims = [d for d in config_dims if len(df[d].unique()) > 1]

groups = df.groupby(by=config_dims)

# Produce a plot for each config
for g in groups:
    config_name = "".join(["-{}={}".format(k, v) for k, v in zip(config_dims, list(g[0]))
                           if k in nonunique_config_dims])

    plt.style.use(['./ethplot.mplstyle'])

    fig, ax = plt.subplots()

    df_g = g[1]

    measA = df_g['ClientLockingTimePerTx mean']    # (155.61, 158.31, 162.97, 161.16, 164.4, 164.09, 163.07)
    measB = df_g['ClientDataAccessTimePerTx mean'] # (0.72, 3.16, 6.43, 5.69, 7.1, 7.5, 7.22)
    measC = df_g['ClientVotingTimePerTx mean']     # (0, 0.71, 1.34, 1.35, 1.52, 1.54, 1.49)
    measD = df_g['ClientEndingTimePerTx mean']     # (12.39, 12.63, 13.01, 12.95, 13.11, 13.14, 13.11)
    measE = df_g['ClientOtherTimePerTx mean']      # (0.38, 0.4, 0.4, 0.41, 0.41, 0.41, 0.41)
    X = df_g['num_tx_agents']
    ind = np.arange(len(df_g.index))

    bottomA = measA * 0
    bottomB = bottomA + measA
    bottomC = bottomB + measB
    bottomD = bottomC + measC
    bottomE = bottomD + measD

    ##############################################################

    width = 0.5       # the width of the bars

    #colors = colorbrewer.qualitative.Paired_6.mpl_colors
    colors = tableau.TableauLight_10.mpl_colors

    rects1 = ax.bar(ind, measA.tolist(), width, bottom=bottomA.tolist(), color=colors[0], hatch='/', ec='black')
    rects2 = ax.bar(ind, measB.tolist(), width, bottom=bottomB.tolist(), color=colors[1], hatch='\\', ec='black')
    rects3 = ax.bar(ind, measC.tolist(), width, bottom=bottomC.tolist(), color=colors[2], hatch='', ec='black')
    rects4 = ax.bar(ind, measD.tolist(), width, bottom=bottomD.tolist(), color=colors[3], hatch='.', ec='black')
    rects5 = ax.bar(ind, measE.tolist(), width, bottom=bottomE.tolist(), color=colors[4], hatch='', ec='black')

    ##############################################################

    ax.set_ylim(bottom=0)
    y_max = 400
    (ncol, loc, bbox_to_anchor) = (5, 'lower right', (1, 0.99))
    if (bottomE + measE).max() > y_max:
        ax.set_ylim(top=y_max)
        (ncol, loc, bbox_to_anchor) = (3, 'lower left', (0, 0.99))
        for (x, y) in zip(ind, bottomE + measE):
            if y > y_max:
                plt.text(x, 403, '{:.4g}'.format((y)), ha='center')

    ##############################################################

    ax.set_ylabel('Transaction Latency [microseconds]')
    ax.get_xaxis().tick_bottom()
    ax.get_yaxis().tick_left()
    ax.xaxis.grid(False)
    ax.set_xlabel('Transaction Processing Agents')

    ax.set_xticks(ind)
    ax.set_xticklabels(X, weight='light')
    ax.legend((rects1[0], rects2[0], rects3[0], rects4[0], rects5[0]),
              ('Requests', 'Data Access', '2PC voting', 'TX end', 'Other'),
              prop={ 'weight': 'light' }, ncol=ncol, loc=loc,
              bbox_to_anchor=bbox_to_anchor)

    plt.savefig("./tx-breakdown{}.pdf".format(config_name), format='pdf')
    plt.close()
