/*
 * ClientStats.h
 *
 *  Created on: Apr 22, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_STATS_CLIENTSTATS_H_
#define SRC_HDB_STATS_CLIENTSTATS_H_

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include <hdb/locktable/LockMode.h>
#include <hdb/messages/LockRequest.h>
#include <hdb/messages/LockGrant.h>

#define CLIENT_STAT_TAG (1<<2)

namespace hdb {
namespace stats {

class ClientStats {

public:

	ClientStats();
	virtual ~ClientStats();

public:

	void sendDataToRoot();
	void dumpToFile(FILE *output);

public:

	uint32_t globalRank;

public:

	void startDataAccess();
	void stopDataAccess();
	uint64_t accumulatedDataAccessTime;

public:

	void startLocking();
	void stopLocking();
	void issuedLockRequest(hdb::messages::LockRequest *request, bool remoteServer);
	void receivedLockAnswer(hdb::messages::LockGrant *grant);

	uint64_t accumulatedLockingTime;
	uint64_t totalRequestedLocksByType[hdb::locktable::LockMode::MODE_COUNT];
	uint64_t totalLocksRequests;
	uint64_t totalRemoteLocks;
	uint64_t totalLocalLocks;
	uint64_t totalDeniedLocks;
	uint64_t totalGrantedLocks;

#ifdef USE_LOGGING
	uint64_t lockRequestTotalTimeAccumulated;
	uint64_t lockRequestNetworkTimeAccumulated;
	uint64_t lockRequestQueueingTimeAccumulated;
	uint64_t lockRequestServerProcessingTimeAccumulated;
#endif

public:

	void startVote();
	void stopVote(bool outcome);

	uint64_t accumulatedVotingTime;
	uint64_t totalVotes;
	uint64_t totalVotesYes;
	uint64_t totalVotesNo;

public:

	void startTransactionEnd(bool isNewOrder, uint64_t numberOfInvolvedServers);
	void stopTransactionEnd(bool outcome, bool isNewOrder);

	uint64_t accumulatedEndingTime;
	uint64_t totalTransactions;
	uint64_t totalTransactionsCommitted;
	uint64_t totalTransactionsAborted;
	uint64_t totalNewOrderTransactions;
	uint64_t totalNewOrderTransactionsCommitted;
	uint64_t totalServersInvolved;


public:

	void startExecution();
	void stopExecution();

	uint64_t executionTime;

protected:

	struct timeval executionStart; //
	struct timeval start; //

};

} /* namespace stats */
} /* namespace hdb */

#endif /* SRC_HDB_STATS_CLIENTSTATS_H_ */
