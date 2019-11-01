/*
 * SimpleClient.h
 *
 *  Created on: Apr 19, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_TRANSACTIONS_SIMPLECLIENT_H_
#define SRC_HDB_TRANSACTIONS_SIMPLECLIENT_H_

#include <stdint.h>

#include <hdb/transactions/TransactionAgent.h>
#include <hdb/configuration/SystemConfig.h>

namespace hdb {
namespace transactions {

class SimpleClient : public TransactionAgent {

public:

	SimpleClient(hdb::configuration::SystemConfig *config, uint64_t numberOfTransactions, uint64_t numberOfLocksPerTransaction, uint32_t remoteprob);
	virtual ~SimpleClient();

public:

	void generate();
	void execute();

protected:
	uint32_t remoteprob;
	uint32_t tableRangeStart;
	uint32_t NumberOfLocksOnThisMachine;
	uint64_t numberOfTransactions;
	uint64_t numberOfLocksPerTransaction;

};

} /* namespace transactions */
} /* namespace hdb */

#endif /* SRC_HDB_TRANSACTIONS_SIMPLECLIENT_H_ */
