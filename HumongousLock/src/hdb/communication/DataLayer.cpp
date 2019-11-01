/*
 * DataLayer.cpp
 *
 *  Created on: Apr 29, 2018
 *      Author: claudeb
 */

#include "DataLayer.h"
#include <chrono>
#include <thread>
#include <hdb/utils/Debug.h>

#define TPCC_BUFFER_SIZE (131072)

#define CUSTOMER_SIZE (933)
#define ORDER_SIZE (256)
#define NEW_ORDER_SIZE (64)
#define LINE_ITEM_SIZE (170)
#define PAYMENT_DATA_SIZE (64)
#define DELIVERY_DATA_SIZE (96)
#define STOCK_LEVEL_SIZE (482)
#define YCSB_RECORD_SIZE (1032)

#define MAX(a,b) ((a) > (b) ? (a) : (b))

namespace hdb {
namespace communication {

DataLayer::DataLayer(hdb::configuration::SystemConfig* config): comdelay(config->comdelay), warehouseToLockServer(config->warehouseToLockServer) {
 

	this->numberOfLockServers = config->globalNumberOfLockTableAgents;
	this->lockServerGlobalRanks = config->lockServerGlobalRanks;

	this->numberOfProcessesPerMachine = config->localNumberOfProcesses;
	this->localMachineId = config->globalRank / numberOfProcessesPerMachine;

 
	if (config->isLockTableAgent) {
#ifdef USE_FOMPI
		foMPI_Win_allocate(TPCC_BUFFER_SIZE, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &(buffer), &(window));
#else
		MPI_Win_allocate(TPCC_BUFFER_SIZE, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &(buffer), &(window));	
#endif
		writeBuffer = NULL;
		readBuffer = NULL;
	} else {
#ifdef USE_FOMPI
		foMPI_Win_allocate(0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &(buffer), &(window));
#else
		MPI_Win_allocate(0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &(buffer), &(window));
#endif	
		writeBuffer = malloc(TPCC_BUFFER_SIZE);
		readBuffer = malloc(TPCC_BUFFER_SIZE);
	}

#ifdef USE_FOMPI
	foMPI_Win_lock_all(0, window);
#else
	MPI_Win_lock_all(0, window);
#endif

	MPI_Barrier(MPI_COMM_WORLD);

}

DataLayer::~DataLayer() {
	if (readBuffer != NULL & writeBuffer != NULL) {
		free(readBuffer);
		free(writeBuffer);
	}
#ifdef USE_FOMPI
	foMPI_Win_unlock_all(window);
	foMPI_Win_free(&(window));
#else
	MPI_Win_unlock_all(window);
	MPI_Win_free(&(window));
#endif

}

void DataLayer::newOrder(uint32_t warehouse, bool remoteItemPresent, std::set<uint32_t> *involvedLockServers) {

	uint32_t targetLockServerInternalRank =  warehouseToLockServer[warehouse];

	uint32_t warehouseSharedBy =  warehouseToLockServer[warehouse + 1] - warehouseToLockServer[warehouse];
	if(warehouseSharedBy > 1){
		targetLockServerInternalRank += rand() % warehouseSharedBy;
	}
 
 
	uint32_t targetLockServerGlobalRank = lockServerGlobalRanks[targetLockServerInternalRank];
	writeToProcess(targetLockServerGlobalRank, ORDER_SIZE+NEW_ORDER_SIZE); // Put order and new order
	if (!remoteItemPresent) {
		writeToProcess(targetLockServerGlobalRank, LINE_ITEM_SIZE * 10); // Put 10 order lines
	} else {
		uint32_t remoteElements = involvedLockServers->size();
		uint32_t localElements = (10 >= remoteElements) ? 10 - remoteElements :  0;
		writeToProcess(targetLockServerGlobalRank, LINE_ITEM_SIZE * localElements); // Put local order lines
		for (auto it = involvedLockServers->begin(); it != involvedLockServers->end(); ++it) {
			uint32_t lockServerId = (*it);
			if(lockServerId != targetLockServerGlobalRank) {
				writeToProcess(lockServerId, LINE_ITEM_SIZE); // Put remote order lines
			}
		}
	}

}

void DataLayer::payment(uint32_t warehouse) {

	uint32_t targetLockServerInternalRank =  warehouseToLockServer[warehouse];

	uint32_t warehouseSharedBy =  warehouseToLockServer[warehouse + 1] - warehouseToLockServer[warehouse];
	if(warehouseSharedBy > 1){
		targetLockServerInternalRank += rand() % warehouseSharedBy;
	}
	uint32_t targetLockServerGlobalRank = lockServerGlobalRanks[targetLockServerInternalRank];
	readFromProcess(targetLockServerGlobalRank, CUSTOMER_SIZE); // Select customer
	writeToProcess(targetLockServerGlobalRank, PAYMENT_DATA_SIZE); // Update payment data

}

void DataLayer::orderStat(uint32_t warehouse) {

	uint32_t targetLockServerInternalRank =  warehouseToLockServer[warehouse];

	uint32_t warehouseSharedBy =  warehouseToLockServer[warehouse + 1] - warehouseToLockServer[warehouse];
	if(warehouseSharedBy > 1){
		targetLockServerInternalRank += rand() % warehouseSharedBy;
	}
	uint32_t targetLockServerGlobalRank = lockServerGlobalRanks[targetLockServerInternalRank];
	readFromProcess(targetLockServerGlobalRank, CUSTOMER_SIZE); // Select customer
	bool remoteElementPresent = rand() % 10;
	if(!remoteElementPresent) {
		readFromProcess(targetLockServerGlobalRank, LINE_ITEM_SIZE*10);
	} else {
		readFromProcess(targetLockServerGlobalRank, LINE_ITEM_SIZE*9);
		uint32_t remoteLockServer = lockServerGlobalRanks[rand() % numberOfLockServers];
		readFromProcess(remoteLockServer, LINE_ITEM_SIZE);
	}

}

void DataLayer::delivery(uint32_t warehouse) {

	uint32_t targetLockServerInternalRank =  warehouseToLockServer[warehouse];

	uint32_t warehouseSharedBy =  warehouseToLockServer[warehouse + 1] - warehouseToLockServer[warehouse];
	if(warehouseSharedBy > 1){
		targetLockServerInternalRank += rand() % warehouseSharedBy;
	}
	uint32_t targetLockServerGlobalRank = lockServerGlobalRanks[targetLockServerInternalRank];
	readFromProcess(targetLockServerGlobalRank, ORDER_SIZE); // Get order
	writeToProcess(targetLockServerGlobalRank, DELIVERY_DATA_SIZE); // Update delivery count and delete new-order

}

void DataLayer::slev(uint32_t warehouse) {

	uint32_t targetLockServerInternalRank =  warehouseToLockServer[warehouse];

	uint32_t warehouseSharedBy =  warehouseToLockServer[warehouse + 1] - warehouseToLockServer[warehouse];
	if(warehouseSharedBy > 1){
		targetLockServerInternalRank += rand() % warehouseSharedBy;
	}
	uint32_t targetLockServerGlobalRank = lockServerGlobalRanks[targetLockServerInternalRank];
	for(uint32_t i=0; i<20; ++i) {
		bool remoteElementPresent = rand() % 10;
		if(!remoteElementPresent) {
			readFromProcess(targetLockServerGlobalRank, 10*(LINE_ITEM_SIZE + STOCK_LEVEL_SIZE));
		} else {
			readFromProcess(targetLockServerGlobalRank, 9*(LINE_ITEM_SIZE + STOCK_LEVEL_SIZE));
			uint32_t remoteLockServer = lockServerGlobalRanks[rand() % numberOfLockServers];
			readFromProcess(remoteLockServer, LINE_ITEM_SIZE + STOCK_LEVEL_SIZE);
		}
	}

}

void DataLayer::ycsb(uint32_t warehouse) {

	uint32_t targetLockServerInternalRank =  warehouseToLockServer[warehouse];

	uint32_t warehouseSharedBy =  warehouseToLockServer[warehouse + 1] - warehouseToLockServer[warehouse];
	if(warehouseSharedBy > 1){
		targetLockServerInternalRank += rand() % warehouseSharedBy;
	}

	uint32_t targetLockServerGlobalRank = lockServerGlobalRanks[targetLockServerInternalRank];
	if(rand() % 2 == 0){
		readFromProcess(targetLockServerGlobalRank, YCSB_RECORD_SIZE);
	} else {
		writeToProcess(targetLockServerGlobalRank, YCSB_RECORD_SIZE);
	}
}

void DataLayer::readFromProcess(uint32_t targetProcess, uint32_t numberOfBytes) {

	bool islocal = targetProcess/numberOfProcessesPerMachine == localMachineId;
	if(!islocal && this->comdelay != 0){
		    auto const sleep_end_time = std::chrono::high_resolution_clock::now() +
                                std::chrono::nanoseconds(this->comdelay);
    while (std::chrono::high_resolution_clock::now() < sleep_end_time);
	}

#ifdef USE_FOMPI
	foMPI_Get(readBuffer, numberOfBytes, MPI_BYTE, targetProcess, 0, numberOfBytes, MPI_BYTE, window);
	foMPI_Win_flush(targetProcess, window);
#else
	MPI_Get(readBuffer, numberOfBytes, MPI_BYTE, targetProcess, 0, numberOfBytes, MPI_BYTE, window);
	MPI_Win_flush(targetProcess, window);
#endif
}

void DataLayer::writeToProcess(uint32_t targetProcess, uint32_t numberOfBytes) {
	
	bool islocal = targetProcess/numberOfProcessesPerMachine == localMachineId;
	if(!islocal && this->comdelay != 0){
		    auto const sleep_end_time = std::chrono::high_resolution_clock::now() +
                                std::chrono::nanoseconds(this->comdelay);
    while (std::chrono::high_resolution_clock::now() < sleep_end_time);
	}
#ifdef USE_FOMPI
	foMPI_Put(writeBuffer, numberOfBytes, MPI_BYTE, targetProcess, 0, numberOfBytes, MPI_BYTE, window);
	foMPI_Win_flush_local(targetProcess, window);
#else
	MPI_Put(writeBuffer, numberOfBytes, MPI_BYTE, targetProcess, 0, numberOfBytes, MPI_BYTE, window);
	MPI_Win_flush_local(targetProcess, window);
#endif
}

} /* namespace communication */
} /* namespace hdb */
