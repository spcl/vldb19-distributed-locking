/*
 * TransactionAgent.cpp
 *
 *  Created on: Mar 27, 2018
 *      Author: claudeb
 */

#include "TransactionAgent.h"

#include <stdio.h>
#include <unistd.h>

#include <hdb/messages/LockRequest.h>
#include <hdb/messages/LockGrant.h>
#include <hdb/messages/LockRelease.h>
#include <hdb/messages/VoteRequest.h>
#include <hdb/messages/TransactionEnd.h>
#include <hdb/messages/Shutdown.h>
#include <hdb/utils/Debug.h>

namespace hdb {
namespace transactions {

TransactionAgent::TransactionAgent(hdb::configuration::SystemConfig *config) {

	this->config = config;
	this->stats.globalRank = config->globalRank;
	this->communicator = new hdb::communication::Communicator(config);
	this->manager = new hdb::transactions::TransactionManager(config, communicator, &stats);

}

TransactionAgent::~TransactionAgent() {

	delete manager;
	delete communicator;

}

 

void TransactionAgent::shutdown() {
	hdb::messages::Shutdown *message = new hdb::messages::Shutdown(config->globalRank);
	for (uint32_t i = 0; i < config->globalNumberOfLockTableAgents; ++i) {
		communicator->sendMessage(message, config->lockServerGlobalRanks[i]);
	}
}

void TransactionAgent::debugPrintTransactions() {

	for(uint64_t i=0; i<transactions.size(); ++i) {
		hdb::transactions::Transaction *tx = &transactions[i];
		printf("[%d] ", tx->transactionTypeId);
		for(uint64_t j=0; j<tx->requests.size(); ++j) {
			printf("{%lu, %d} ", tx->requests[j].first, tx->requests[j].second);
		}
		printf("\n----\n");
	}
	fflush(stdout);
}

void TransactionAgent::tpccExecute(uint32_t warehouse, uint32_t workloadSize) {

		uint64_t numberoftransactions = transactions.size();

		stats.startExecution();

		manager->start_time=std::chrono::high_resolution_clock::now();

		auto endtime = manager->start_time + std::chrono::seconds(config->timelimit);
		
		for (uint64_t txId = 0; txId < workloadSize; ++txId) {
			hdb::transactions::Transaction *transaction = &transactions[txId % numberoftransactions];
			manager->startTransaction();
			uint64_t lockSize = transaction->requests.size();
			bool success = true;

			stats.startLocking();
			for (uint64_t lId = 0; lId < lockSize; ++lId) {
				success = manager->requestLock(transaction->requests[lId].first, transaction->requests[lId].second);
				if (!success) {
					break;
				}
			}
			stats.stopLocking();

			if(success){
				stats.startDataAccess();
				switch(transaction->transactionTypeId){
					case hdb::transactions::DataMode::NONE:
						break;
					case hdb::transactions::DataMode::NEWORD:
						communicator->dataLayer->newOrder(warehouse, manager->involvedVoteServers.size() > 1, &(manager->involvedLockServers));
						break;
					case hdb::transactions::DataMode::PAYMENT:
						communicator->dataLayer->payment(warehouse);
						break;
					case hdb::transactions::DataMode::ORDSTAT:
						communicator->dataLayer->orderStat(warehouse);	
						break;
					case hdb::transactions::DataMode::DELIVERY:
						communicator->dataLayer->delivery(warehouse);
						break;
					case hdb::transactions::DataMode::SLEV:
						communicator->dataLayer->slev(warehouse);
						break;
					case hdb::transactions::DataMode::YCSB:
						communicator->dataLayer->ycsb(warehouse);
						break;
					default:
						DLOG_ALWAYS("TransactionAgent", "Encounter transaction of type %d", transaction->transactionTypeId);
				}
				stats.stopDataAccess();
			}

			if(success) {
				manager->commitTransaction(transaction->transactionTypeId);
			} else {
				manager->abortTransaction(transaction->transactionTypeId);
			}

			if(txId % 8192 == 0){
				if(endtime < std::chrono::high_resolution_clock::now()){
					DLOG("TransactionAgent", "Terminate earlier");
					break;
				}
			}
		}

		stats.stopExecution();
}

} /* namespace transactions */
} /* namespace hdb */


