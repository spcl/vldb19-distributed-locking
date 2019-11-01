/*
 * TransactionManager.h
 *
 *  Created on: Apr 19, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_TRANSACTIONS_TRANSACTIONMANAGER_H_
#define SRC_HDB_TRANSACTIONS_TRANSACTIONMANAGER_H_

#include <stdint.h>
#include <set>
#include <string>

#include <hdb/configuration/SystemConfig.h>
#include <hdb/communication/Communicator.h>
#include <hdb/locktable/LockMode.h>
#include <hdb/messages/TransactionEnd.h>
#include <hdb/messages/VoteRequest.h>
#include <hdb/messages/LockRequest.h>
#include <hdb/stats/ClientStats.h>
#include <hdb/messages/AbortTransaction.h>
#include <hdb/transactions/Transaction.h>
namespace hdb {
namespace transactions {

class TransactionManager {

public:

	TransactionManager(hdb::configuration::SystemConfig *config, hdb::communication::Communicator *communicator, hdb::stats::ClientStats *stats);
	virtual ~TransactionManager();

public:

	void startTransaction();
	void abortTransaction(hdb::transactions::DataMode  txId);
	void commitTransaction(hdb::transactions::DataMode  txId);

public:

	bool requestLock(uint64_t lockId, hdb::locktable::LockMode mode);

public:

	uint64_t transactionId;
	std::set<uint32_t> involvedLockServers;
	std::set<uint32_t> involvedVoteServers;

	uint64_t transaction_time;
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
	
	
	bool voteNeeded;

protected:
    const bool timestampordering;
	hdb::communication::Communicator *communicator;
	hdb::stats::ClientStats *stats;

	uint32_t globalRank;
	uint32_t *lockServerGlobalRanks;
	uint32_t locksPerWarehouse;
	const bool randomLockAssign;
	const uint32_t globalNumberOfWarehouses;

	std::vector<uint32_t>& warehouseToLockServer;
protected:

	hdb::messages::AbortTransaction abortRequest;
	hdb::messages::TransactionEnd endTransaction;
	hdb::messages::VoteRequest voteRequest;
	hdb::messages::LockRequest lockRequest;

};

} /* namespace transactions */
} /* namespace hdb */

#endif /* SRC_HDB_TRANSACTIONS_TRANSACTIONMANAGER_H_ */
