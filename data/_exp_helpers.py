import numpy
import scipy
import scipy.stats

MEASURES = [
        # Measures from trace files (.exp)
        'ClientGlobalRank',
        'ClientDataAccessTimeAccumulated',
        'ClientLockingTimeAccumulated',
        'ClientLocksNL',
        'ClientLocksIS',
        'ClientLocksIX',
        'ClientLocksS',
        'ClientLocksSIX',
        'ClientLocksX',
        'ClientTotalLocks',
        'ClientRemoteLocks',
        'ClientLocalLocks',
        'ClientDeniedLocks',
        'ClientGrantedLocks',
        'ClientLockRequestTotalTime',
        'ClientLockRequestNetworkTime',
        'ClientLockRequestQueueingTime',
        'ClientLockRequestServerTime',
        'ClientVotingTimeAccumulated',
        'ClientTotalVotes',
        'ClientVotesYes',
        'ClientVotesNo',
        'ClientEndingTimeAccumulated',
        'ClientTotalTransactions',
        'ClientTransactionsCommitted',
        'ClientTransactionsAborted',
        'ClientNewOrderTransactions',
        'ClientNewOrderTransactionsCommitted',
        'ClientServersContacted',
        'ClientExecutionTime',
        # Measures added by round-trip to feather
        'index',
        # Measures added by scripts
        'ClientDataAccessTimePerTx',
        'ClientEndingTimePerTx',
        'ClientExecutionTimePerTx',
        'ClientLockRequestNetworkTimePerLock',
        'ClientLockRequestQueueingTimePerLock',
        'ClientLockRequestServerTimePerLock',
        'ClientLockingTimePerTx',
        'ClientOtherTimePerTx',
        'ClientServersContactedPerTx',
        'ClientVotingTimePerTx',
        'ClientVotingTimePerVote',
        'SystemCommitRate',
        'SystemLockingThroughput',
        'SystemNewOrderTransactionThroughput',
        'SystemSuccessfulTransactionThroughput',
        'SystemSuccessfulNewOrderTransactionThroughput',
        'SystemTransactionThroughput',
    ]
MEASURES = MEASURES +\
        [m + " mean" for m in MEASURES] + \
        [m + " count" for m in MEASURES] + \
        [m + " ci950" for m in MEASURES]

DIMENSIONS = [
        'distribution',
        'isolation_mode',
        'mechanism',
        'network_delay',
        'num_cores',
        'num_cores_per_node',
        'num_nodes',
        'num_server_agents',
        'num_server_agents_per_node',
        'num_tx_agents',
        'num_tx_agents_per_node',
        'num_warehouses',
        'option1',
        'option2',
        'percent_remote',
        'workload',
        '__filename',
    ]


def summarize(df):
    unkown_columns = [c for c in df.columns.values if c not in MEASURES + DIMENSIONS]
    if unkown_columns:
        raise RuntimeError('Unknown columns "{}"'.format(unkown_columns))

    def error(confidence=0.95):
        def error_(data):
            a = 1.0 * numpy.array(data)
            n = len(a)
            if n == 1:
                return 0
            m, se = numpy.mean(a), scipy.stats.sem(a)
            return se * scipy.stats.t._ppf((1+confidence)/2., n-1)
        error_.__name__ = 'ci' + ''.join([c for c in str(100 * confidence) if c.isdigit()])
        return error_

    dimensions = [c for c in df.columns.values if c in DIMENSIONS]
    aggs = {c : ['mean', 'count', error()] for c in df.columns.values if c in MEASURES}

    df = df.groupby(by=[c for c in dimensions if c != '__filename']) \
           .agg(aggs).reset_index()
    df.columns = [' '.join(col).strip() for col in df.columns.values]

    return df

