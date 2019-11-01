/*
 * ClientStats.cpp
 *
 *  Created on: Apr 22, 2018
 *      Author: claudeb
 */

#include <hdb/stats/ClientStats.h>

#include <mpi.h>

namespace hdb {
namespace stats {

ClientStats::ClientStats() {

	this->globalRank = 0;
	this->accumulatedDataAccessTime = 0;
	this->accumulatedLockingTime = 0;
	this->totalLocksRequests = 0;
	this->totalRemoteLocks = 0;
	this->totalLocalLocks = 0;
	this->totalDeniedLocks = 0;
	this->totalGrantedLocks = 0;
#ifdef USE_LOGGING
	this->lockRequestTotalTimeAccumulated = 0;
	this->lockRequestNetworkTimeAccumulated = 0;
	this->lockRequestServerProcessingTimeAccumulated = 0;
	this->lockRequestQueueingTimeAccumulated = 0;
#endif
	this->accumulatedVotingTime = 0;
	this->totalVotes = 0;
	this->totalVotesYes = 0;
	this->totalVotesNo = 0;
	this->accumulatedEndingTime = 0;
	this->totalTransactions = 0;
	this->totalTransactionsCommitted = 0;
	this->totalTransactionsAborted = 0;
	this->totalNewOrderTransactions = 0;
	this->totalNewOrderTransactionsCommitted = 0;
	this->totalServersInvolved = 0;
	this->executionTime = 0;
	for (uint32_t i = 0; i < hdb::locktable::LockMode::MODE_COUNT; ++i) {
		this->totalRequestedLocksByType[i] = 0;
	}

}

ClientStats::~ClientStats() {

}

void ClientStats::sendDataToRoot() {
	MPI_Send(this, sizeof(ClientStats), MPI_BYTE, 0, CLIENT_STAT_TAG, MPI_COMM_WORLD);
}

void ClientStats::dumpToFile(FILE* output) {

	fprintf(output, "####\n");
	fprintf(output, "ClientGlobalRank\t%d\n", globalRank);
	fprintf(output, "ClientDataAccessTimeAccumulated\t%lu\n", accumulatedDataAccessTime);
	fprintf(output, "ClientLockingTimeAccumulated\t%lu\n", accumulatedLockingTime);
	fprintf(output, "ClientLocksNL\t%lu\n", totalRequestedLocksByType[0]);
	fprintf(output, "ClientLocksIS\t%lu\n", totalRequestedLocksByType[1]);
	fprintf(output, "ClientLocksIX\t%lu\n", totalRequestedLocksByType[2]);
	fprintf(output, "ClientLocksS\t%lu\n", totalRequestedLocksByType[3]);
	fprintf(output, "ClientLocksSIX\t%lu\n", totalRequestedLocksByType[4]);
	fprintf(output, "ClientLocksX\t%lu\n", totalRequestedLocksByType[5]);
	fprintf(output, "ClientTotalLocks\t%lu\n", totalLocksRequests);
	fprintf(output, "ClientRemoteLocks\t%lu\n", totalRemoteLocks);
	fprintf(output, "ClientLocalLocks\t%lu\n", totalLocalLocks);
	fprintf(output, "ClientDeniedLocks\t%lu\n", totalDeniedLocks);
	fprintf(output, "ClientGrantedLocks\t%lu\n", totalGrantedLocks);
#ifdef USE_LOGGING
	fprintf(output, "ClientLockRequestTotalTime\t%lu\n", lockRequestTotalTimeAccumulated);
	fprintf(output, "ClientLockRequestNetworkTime\t%lu\n", lockRequestNetworkTimeAccumulated);
	fprintf(output, "ClientLockRequestQueueingTime\t%lu\n", lockRequestQueueingTimeAccumulated);
	fprintf(output, "ClientLockRequestServerTime\t%lu\n", lockRequestServerProcessingTimeAccumulated);
#endif
	fprintf(output, "ClientVotingTimeAccumulated\t%lu\n", accumulatedVotingTime);
	fprintf(output, "ClientTotalVotes\t%lu\n", totalVotes);
	fprintf(output, "ClientVotesYes\t%lu\n", totalVotesYes);
	fprintf(output, "ClientVotesNo\t%lu\n", totalVotesNo);
	fprintf(output, "ClientEndingTimeAccumulated\t%lu\n", accumulatedEndingTime);
	fprintf(output, "ClientTotalTransactions\t%lu\n", totalTransactions);
	fprintf(output, "ClientTransactionsCommitted\t%lu\n", totalTransactionsCommitted);
	fprintf(output, "ClientTransactionsAborted\t%lu\n", totalTransactionsAborted);
	fprintf(output, "ClientNewOrderTransactions\t%lu\n", totalNewOrderTransactions);
	fprintf(output, "ClientNewOrderTransactionsCommitted\t%lu\n", totalNewOrderTransactionsCommitted);
	fprintf(output, "ClientServersContacted\t%lu\n", totalServersInvolved);
	fprintf(output, "ClientExecutionTime\t%lu\n", executionTime);

}

void ClientStats::startDataAccess() {
	gettimeofday(&(start), NULL);
}

void ClientStats::stopDataAccess() {
	struct timeval stop;
	gettimeofday(&(stop), NULL);
	this->accumulatedDataAccessTime += (stop.tv_sec * 1000000L + stop.tv_usec) - (start.tv_sec * 1000000L + start.tv_usec);
}

void ClientStats::startLocking() {
	gettimeofday(&(start), NULL);
}

void ClientStats::stopLocking() {
	struct timeval stop;
	gettimeofday(&(stop), NULL);
	this->accumulatedLockingTime += (stop.tv_sec * 1000000L + stop.tv_usec) - (start.tv_sec * 1000000L + start.tv_usec);

}

void ClientStats::issuedLockRequest(hdb::messages::LockRequest* request, bool remoteServer) {
	++totalLocksRequests;
	++(totalRequestedLocksByType[(uint32_t) (request->mode)]);
	if (remoteServer) {
		++totalRemoteLocks;
	} else {
		++totalLocalLocks;
	}
}

void ClientStats::receivedLockAnswer(hdb::messages::LockGrant* grant) {
	if (grant->granted) {
		++totalGrantedLocks;
	} else {
		++totalDeniedLocks;
	}
#ifdef USE_LOGGING
	lockRequestTotalTimeAccumulated += grant->clientReceiveTime - grant->clientSendTime;
	lockRequestNetworkTimeAccumulated += (grant->clientReceiveTime - grant->clientSendTime) - (grant->serverSendTime - grant->serverReceiveTime);
	lockRequestQueueingTimeAccumulated += grant->serverDequeueTime - grant->serverEnqueueTime;
	lockRequestServerProcessingTimeAccumulated += (grant->serverEnqueueTime - grant->serverReceiveTime) + (grant->serverSendTime - grant->serverDequeueTime);
#endif
}

void ClientStats::startVote() {
	gettimeofday(&(start), NULL);
}

void ClientStats::stopVote(bool outcome) {
	struct timeval stop;
	gettimeofday(&(stop), NULL);
	this->accumulatedVotingTime += (stop.tv_sec * 1000000L + stop.tv_usec) - (start.tv_sec * 1000000L + start.tv_usec);
	++totalVotes;
	if (outcome) {
		++totalVotesYes;
	} else {
		++totalVotesNo;
	}
}

void ClientStats::startTransactionEnd(bool isNewOrder, uint64_t numberOfInvolvedServers) {
	gettimeofday(&(start), NULL);
	++totalTransactions;
	if (isNewOrder)
		++totalNewOrderTransactions;
	totalServersInvolved += numberOfInvolvedServers;
}

void ClientStats::stopTransactionEnd(bool outcome, bool isNewOrder) {
	struct timeval stop;
	gettimeofday(&(stop), NULL);
	this->accumulatedEndingTime += (stop.tv_sec * 1000000L + stop.tv_usec) - (start.tv_sec * 1000000L + start.tv_usec);
	if (outcome) {
		++totalTransactionsCommitted;
	} else {
		++totalTransactionsAborted;
	}
	if(isNewOrder && outcome){
		++totalNewOrderTransactionsCommitted;
	}
}

void ClientStats::startExecution() {
	gettimeofday(&(executionStart), NULL);
}

void ClientStats::stopExecution() {
	struct timeval stop;
	gettimeofday(&(stop), NULL);
	this->executionTime = (stop.tv_sec * 1000000L + stop.tv_usec) - (executionStart.tv_sec * 1000000L + executionStart.tv_usec);
}

} /* namespace stats */
} /* namespace hdb */


