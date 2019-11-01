/*
 * TransactionManager.cpp
 *
 *  Created on: Apr 19, 2018
 *      Author: claudeb
 */

#include "TransactionManager.h"

#include <hdb/messages/Message.h>
#include <hdb/messages/LockGrant.h>
#include <hdb/utils/Debug.h>
#include <cassert>



namespace hdb {
namespace transactions {

TransactionManager::TransactionManager(hdb::configuration::SystemConfig *config, 
										hdb::communication::Communicator *communicator, 
										hdb::stats::ClientStats *stats): randomLockAssign(config->hashlock), globalNumberOfWarehouses(config->globalNumberOfWarehouses), timestampordering(config->timestamp), warehouseToLockServer(config->warehouseToLockServer){

	this->communicator = communicator;
	this->stats = stats;
	this->lockServerGlobalRanks = config->lockServerGlobalRanks;
	this->locksPerWarehouse = config->locksPerWarehouse;
	this->globalRank = config->globalRank;

	transaction_time = -1;
	transactionId = 0;
	involvedLockServers.clear();
	involvedVoteServers.clear();
	voteNeeded = false;

}

TransactionManager::~TransactionManager() {

}

void TransactionManager::startTransaction() {
	//++transactionId;
	involvedLockServers.clear();
	involvedVoteServers.clear();
	voteNeeded = false;
	uint64_t temp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
	if(transaction_time == temp){
		transaction_time = temp + 1;
		DLOG_ALWAYS("TransactionManager", "Increment time");
	}else{
		transaction_time = temp;
	}
	transactionId = (transaction_time<<24) + globalRank;
	DLOG("TransactionManager", "Starting transaction %d", transactionId);
}

void TransactionManager::abortTransaction(hdb::transactions::DataMode  txId) {

	DLOG("TransactionManager", "Transaction %d wants to abort", transactionId);
	bool isNewOrderTx = (txId == hdb::transactions::DataMode::NEWORD);
	stats->startTransactionEnd(isNewOrderTx, involvedLockServers.size());
	new (&abortRequest) hdb::messages::AbortTransaction(globalRank, transactionId);
	for (auto it = involvedLockServers.begin(); it != involvedLockServers.end(); ++it) {
		uint32_t lockServerId = (*it);
		communicator->sendMessage(&abortRequest, lockServerId);
	}
	communicator->waitForTransactionEndSignals(involvedLockServers.size());
	stats->stopTransactionEnd(false,isNewOrderTx);

}

void TransactionManager::commitTransaction(hdb::transactions::DataMode  txId) {

	if (involvedVoteServers.size() > 1 && !this->timestampordering) {
		DLOG("TransactionManager", "Transaction %d required vote", transactionId);
		stats->startVote();
		for (auto it = involvedVoteServers.begin(); it != involvedVoteServers.end(); ++it) {
			uint32_t lockServerId = (*it);
			new (&voteRequest) hdb::messages::VoteRequest(globalRank, transactionId);
			communicator->sendMessage(&voteRequest, lockServerId);
		}
		bool outcome = false;
		while (!communicator->checkVoteReady(&outcome, involvedVoteServers.size()))
			;
		stats->stopVote(outcome);
		DLOG("TransactionManager", "Transaction %d completed vote", transactionId);
	}

	DLOG("TransactionManager", "Transaction %d wants to commit", transactionId);
	bool isNewOrderTx = (txId == hdb::transactions::DataMode::NEWORD);
	stats->startTransactionEnd(isNewOrderTx, involvedLockServers.size());
	new (&endTransaction) hdb::messages::TransactionEnd(globalRank, transactionId);
	for (auto it = involvedLockServers.begin(); it != involvedLockServers.end(); ++it) {
		uint32_t lockServerId = (*it);
		communicator->sendMessage(&endTransaction, lockServerId);
	}
	communicator->waitForTransactionEndSignals(involvedLockServers.size());
	stats->stopTransactionEnd(true,isNewOrderTx);

}


inline static uint32_t hash(uint32_t  x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}


bool TransactionManager::requestLock(uint64_t lockId, hdb::locktable::LockMode mode) {

	if (this->timestampordering && !LOCK_READ_OR_WRITE(mode)) {
		return true;
	}

	uint32_t temp = (randomLockAssign ? hash(lockId) : lockId );
	uint32_t targetWarehouse =  (temp / locksPerWarehouse)  % globalNumberOfWarehouses;
	uint32_t targetLockServerInternalRank =  this->warehouseToLockServer[targetWarehouse]; 
 	
 	uint32_t warehouseSharedBy =  this->warehouseToLockServer[targetWarehouse + 1] - this->warehouseToLockServer[targetWarehouse];
	if(warehouseSharedBy > 1){
		targetLockServerInternalRank += (temp / (locksPerWarehouse/ warehouseSharedBy))  % warehouseSharedBy;
	}

	uint32_t targetGlobalRank = lockServerGlobalRanks[targetLockServerInternalRank];

	DLOG("TransactionManager", "Target lock manager is %d for request (%lu, %d)", targetGlobalRank, lockId, (uint32_t) mode);

	new (&lockRequest) hdb::messages::LockRequest(globalRank, transactionId, lockId, mode);
#ifdef USE_LOGGING
		lockRequest.clientSendTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
	bool communicationIsLocal = communicator->sendMessage(&lockRequest, targetGlobalRank);

	involvedLockServers.insert(targetGlobalRank);
	stats->issuedLockRequest(&lockRequest, !communicationIsLocal);

	if(mode == hdb::locktable::LockMode::X){
		involvedVoteServers.insert(targetGlobalRank);
	}
	
	hdb::messages::Message *message = communicator->getMessageBlocking();

	assert(message->messageType == hdb::messages::MessageType::LOCK_GRANT_MESSAGE);

	hdb::messages::LockGrant *grant = (hdb::messages::LockGrant *) message;
#ifdef USE_LOGGING
		grant->clientReceiveTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
	stats->receivedLockAnswer(grant);

	assert(grant->transactionNumber == transactionId);
	assert(grant->lockId == lockId);
	assert(grant->mode == mode);
	assert(grant->clientGlobalRank == globalRank);
        DLOG("TransactionManager", "Transaction get reply for lock %lu in transaction %lu ",lockId, transactionId);

	if(!grant->granted) {
		DLOG("TransactionManager", "Lock (%lu, %d) was not granted", lockId, (uint32_t) mode);
	}


	return grant->granted;

}


} /* namespace transactions */
} /* namespace hdb */
