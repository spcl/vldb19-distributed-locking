/*
 * ReplayClient.cpp
 *
 *  Created on: Apr 22, 2018
 *      Author: claudeb
 */

#include "ReplayClient.h"

#include <stdio.h>
#include <algorithm>
#include <hdb/transactions/Transaction.h>
#include <hdb/utils/Debug.h>
#include <sstream>
#include <fstream>
namespace hdb {
namespace transactions {

ReplayClient::ReplayClient(hdb::configuration::SystemConfig* config, std::string fileLocation, uint64_t numberOfTransactions) :
		TransactionAgent(config) {

	this->numberOfTransactions = numberOfTransactions;

  
	this->clientId = config->internalRank; 
	
	uint32_t clientsPerWarehouse =  config->globalNumberOfTransactionAgents / config->globalNumberOfWarehouses;
	
	if(clientsPerWarehouse == 0){
        float delta = (float)config->globalNumberOfWarehouses / (float)config->globalNumberOfTransactionAgents;
		this->warehouseId = static_cast<uint32_t>(this->clientId * delta) ;
		this->clientsPerWarehouse = 1;
		this->localclientid = 0;
	} else {

		uint32_t idx = 0;
		for( ; idx < config->globalNumberOfWarehouses; idx++){
			if(this->clientId>=config->warehouseToLockServer[idx] && this->clientId<config->warehouseToLockServer[idx+1]){
				//found 
				break;
			}
		}

		this->warehouseId = idx;
		this->clientsPerWarehouse = config->warehouseToLockServer[idx+1] - config->warehouseToLockServer[idx];
		this->localclientid = this->clientId - config->warehouseToLockServer[idx];
	} 

	this->fileLocation = fileLocation;

	this->globalTableSize = config->globalNumberOfLocks;
}

ReplayClient::~ReplayClient() {

}

void ReplayClient::generate() {

	hdb::transactions::Transaction *item = NULL;
	uint64_t currentTransactionId = 0;

	uint64_t transactioncounter = 0;
	uint64_t skip = 0; 

 	transactions.reserve(numberOfTransactions);

 
	// Check binary first. 
	std::ostringstream stringStream;
  	stringStream << fileLocation << "/wh" << this->warehouseId + 1 << "_" << numberOfTransactions << "_" <<localclientid<< "_" << clientsPerWarehouse;

	//sprintf(fileName, "%s/wh%06d_%u_%u_%u.bin", fileLocation.c_str(), this->warehouseId + 1,numberOfTransactions,localclientid,clientsPerWarehouse);
	for(std::string str : config->transaction_constraints){
		stringStream << "_";
		stringStream << str;
	}
	stringStream << ".bin";

	std::string copyOfStr = stringStream.str();

    
    std::ifstream ifs(copyOfStr.c_str());
    if(ifs.good()) { // file exist 
     	boost::archive::binary_iarchive ia(ifs);
     	uint32_t size = 0;
     	ia >> size;
        transactions.resize(size);
        ia >> boost::serialization::make_array(transactions.data(), size);
     	return; 
    }
      DLOG_ALWAYS("ReplayClient", "binary did not exist");
//  return; 
	char fileName[1024];
	memset(fileName, 0, 1024);
	sprintf(fileName, "%s/wh%06d.csv", fileLocation.c_str(), this->warehouseId + 1 );
	FILE *file = fopen(fileName, "r");


	while (transactions.size() < numberOfTransactions) {

		uint64_t transactionId;
		uint64_t lockId;
		char lockMode[2];
		char queryType[16];

		char lineBuffer[128];
		bool validRead = (fgets(lineBuffer, sizeof(lineBuffer), file) != NULL);
		if(validRead){
			int readItems = sscanf(lineBuffer, "%lu,%lu,%2[^,],%s", &transactionId, &lockId, lockMode, queryType);
			validRead = (readItems == 4);
		}
		if (!validRead) {
			if (skip != transactionId && item != NULL) {
				transactions.push_back(*item);
			}
		        delete item;

            //stop processing
			break;
		}


		if(skip == transactionId){
			continue;
		}

		if (transactionId != currentTransactionId) { // new transaction

			
			if (item != NULL) {
				if(item->requests.size() < 750) {
					transactions.push_back(*item);
				} else {
					DLOG_ALWAYS("ReplayClient", "Ignoring transaction that has %lu locks", item->requests.size());
				}
				delete item;
				item = NULL;
			}

			currentTransactionId = transactionId;

			// check constraints
			if( !config->transaction_constraints.empty() ){
				// if not in the list
				if (std::find(config->transaction_constraints.begin(), config->transaction_constraints.end(), queryType) ==  config->transaction_constraints.end()){
					skip = transactionId;
					continue;
				}
			}

			transactioncounter++;

			if( (transactioncounter - 1) % clientsPerWarehouse != localclientid){
				skip = transactionId;
				continue;
			}
 
			// the trace has only one type per transaction
			item = new hdb::transactions::Transaction(hdb::transactions::DATA_MODE_FROM_STRING(queryType));
			DLOG("ReplayClient", "Starting new transaction");
		} 
		

		DLOG("ReplayClient", "Read workload item (%lu %lu %s %s).", transactionId, lockId, lockMode, queryType);
		item->requests.push_back(lockid_mode_pair_t(lockId , hdb::locktable::LOCK_MODE_FROM_STRING(lockMode)));
	}

	fclose(file);

//	 printf("Saving %s\n", copyOfStr.c_str());

	std::ofstream ofs(copyOfStr.c_str());
    {
        boost::archive::binary_oarchive oa(ofs);
        uint32_t size  = transactions.size();
        oa << size;
        oa << boost::serialization::make_array(transactions.data(), size);
    }
       
//	DLOG("ReplayClient", "Client %d finished reading file and has %lu transactions", config->internalRank, transactions.size());

}

void ReplayClient::execute() {

	tpccExecute(warehouseId, numberOfTransactions);
}

} /* namespace transactions */
} /* namespace hdb */
