/*
 * DataLayer.h
 *
 *  Created on: Apr 29, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_COMMUNICATION_DATALAYER_H_
#define SRC_HDB_COMMUNICATION_DATALAYER_H_

#include <mpi.h>
#ifdef USE_FOMPI
#include <fompi.h>
#endif
#include <stdint.h>
#include <set>
#include <vector>

#include <hdb/configuration/SystemConfig.h>

namespace hdb {
namespace communication {


class DataLayer {
public:

	DataLayer(hdb::configuration::SystemConfig *config);
	virtual ~DataLayer();

public:

	void newOrder(uint32_t warehouse, bool remoteItemPresent, std::set<uint32_t> *involvedLockServers);
	void payment(uint32_t warehouse);
	void orderStat(uint32_t warehouse);
	void delivery(uint32_t warehouse);
	void slev(uint32_t warehouse);
	void ycsb(uint32_t warehouse);
	
protected:

	void *buffer;

#ifdef USE_FOMPI
	foMPI_Win window;
#else
	MPI_Win window;
#endif

	void *writeBuffer;
	void *readBuffer;

protected:

	const uint32_t comdelay;

	void readFromProcess(uint32_t targetProcess, uint32_t numberOfBytes);
	void writeToProcess(uint32_t targetProcess, uint32_t numberOfBytes);

protected:

	uint32_t localMachineId;
	uint32_t numberOfProcessesPerMachine;
	uint32_t numberOfLockServers;

	std::vector<uint32_t>& warehouseToLockServer;
	
	// one of them is not zero. 
//	uint32_t wareHousesPerLockServer;
//	uint32_t LockServersPerwareHouse;
	

	uint32_t *lockServerGlobalRanks;

};

} /* namespace communication */
} /* namespace hdb */

#endif /* SRC_HDB_COMMUNICATION_DATALAYER_H_ */
