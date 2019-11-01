/*
 * ReplayClient.h
 *
 *  Created on: Apr 22, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_TRANSACTIONS_REPLAYCLIENT_H_
#define SRC_HDB_TRANSACTIONS_REPLAYCLIENT_H_

#include <stdint.h>
#include <vector>
#include <string>
#include <string.h>

#include <hdb/transactions/TransactionAgent.h>
#include <hdb/configuration/SystemConfig.h>
#include <hdb/locktable/LockMode.h>

namespace hdb {
namespace transactions {

class ReplayClient: public TransactionAgent {

public:

	ReplayClient(hdb::configuration::SystemConfig *config, std::string fileLocation, uint64_t numberOfTransactions);
	virtual ~ReplayClient();

public:

	void generate();
	void execute();

protected:

	uint32_t warehouseId;
	uint32_t clientId;

	uint64_t localclientid;
	uint64_t clientsPerWarehouse;


	
	std::string fileLocation;
	uint64_t numberOfTransactions;
	uint64_t globalTableSize;

};

} /* namespace transactions */
} /* namespace hdb */

#endif /* SRC_HDB_TRANSACTIONS_REPLAYCLIENT_H_ */
