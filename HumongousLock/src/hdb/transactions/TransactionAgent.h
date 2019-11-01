/*
 * TransactionAgent.h
 *
 *  Created on: Mar 27, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_TRANSACTIONS_TRANSACTIONAGENT_H_
#define SRC_HDB_TRANSACTIONS_TRANSACTIONAGENT_H_

#include <vector>

#include <hdb/configuration/SystemConfig.h>
#include <hdb/communication/Communicator.h>
#include <hdb/transactions/TransactionManager.h>
#include <hdb/transactions/Transaction.h>
#include <hdb/stats/ClientStats.h>
#include <hdb/communication/DataLayer.h>

namespace hdb {
namespace transactions {

class TransactionAgent {

public:

	TransactionAgent(hdb::configuration::SystemConfig *config);
	virtual ~TransactionAgent();

public:

	virtual void generate() = 0;
	virtual void execute() = 0;
 
	void shutdown();

	void tpccExecute(uint32_t warehouse, uint32_t workloadSize);

public:

	void debugPrintTransactions();

public:

	hdb::stats::ClientStats stats;

protected:

	hdb::configuration::SystemConfig *config;
	hdb::communication::Communicator *communicator;
	hdb::transactions::TransactionManager *manager;

protected:

	std::vector<hdb::transactions::Transaction> transactions;

};

} /* namespace transactions */
} /* namespace hdb */

#endif /* SRC_HDB_TRANSACTIONS_TRANSACTIONAGENT_H_ */
