/*
 * SimpleClient.cpp
 *
 *  Created on: Apr 19, 2018
 *      Author: claudeb
 */

#include "SimpleClient.h"

#include <random>
#include <hdb/utils/Debug.h>
#include <hdb/communication/DataLayer.h>


namespace hdb {
namespace transactions {

SimpleClient::SimpleClient(hdb::configuration::SystemConfig *config, uint64_t numberOfTransactions, uint64_t numberOfLocksPerTransaction, uint32_t remoteprob) : TransactionAgent(config) {

	this->numberOfTransactions = numberOfTransactions;
	this->numberOfLocksPerTransaction = numberOfLocksPerTransaction;
	this->remoteprob = remoteprob;

	uint32_t machineId = config->globalRank / config->localNumberOfProcesses;

	// first lock agent on this nodes
	uint32_t firstLockAgent = machineId * (config->globalNumberOfLockTableAgents /  config->numberOfNodes);
	// last lock agent on this nodes
	uint32_t lastLockAgent = (machineId + 1)* (config->globalNumberOfLockTableAgents /  config->numberOfNodes);

	this->tableRangeStart = 0;


	
	for(uint32_t i = 0; i < firstLockAgent; i++){
		uint32_t locksOnThisAgent = config->AllLocksPerAgent[i];
		this->tableRangeStart += locksOnThisAgent;
	}

	this->NumberOfLocksOnThisMachine = 0;
	for(uint32_t i = firstLockAgent; i < lastLockAgent; i++){
		uint32_t locksOnThisAgent = config->AllLocksPerAgent[i];
		this->NumberOfLocksOnThisMachine += locksOnThisAgent;
	}

}

SimpleClient::~SimpleClient() {

}

void hdb::transactions::SimpleClient::generate() {

	//std::random_device device;
	//std::mt19937 gen(123);
	//std::uniform_int_distribution<int> randomLock(0, config->globalNumberOfLocks-1);
	//std::uniform_int_distribution<int> randomMode(1, hdb::locktable::LockMode::MODE_COUNT-1);
	srand(123 + config->globalRank);
	
	for (uint64_t txId = 0; txId < numberOfTransactions; ++txId) {
		hdb::transactions::Transaction *transaction = new hdb::transactions::Transaction();
		while (transaction->requests.size() < numberOfLocksPerTransaction) {
			uint64_t lockId = 0;
			bool lockIsRemote = (rand() % 100) < remoteprob;

			if (!lockIsRemote) {
				uint32_t localLockOffset = rand() % (NumberOfLocksOnThisMachine);
				lockId = tableRangeStart + localLockOffset;

			} else {
				do{
				lockId = rand() % config->globalNumberOfLocks;
				}while(lockId >= tableRangeStart && (lockId < (tableRangeStart + NumberOfLocksOnThisMachine)) );
			}

			hdb::locktable::LockMode mode = (hdb::locktable::LockMode) ((rand() % 5) + 1);

			// De-dublicate
			bool isInSet = false;
			for(uint64_t a=0; a<transaction->requests.size(); ++a) {
				if(transaction->requests[a].first == lockId) {
					isInSet = true;
					break;
				}
			}

			if(!isInSet) {
				transaction->requests.push_back(lockid_mode_pair_t(lockId, mode));
			}
		}

		transactions.push_back(*transaction);
	}

}

void hdb::transactions::SimpleClient::execute() {
	tpccExecute(0, numberOfTransactions);
}

} /* namespace transactions */
} /* namespace hdb */


