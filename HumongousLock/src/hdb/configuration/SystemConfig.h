/*
 * SystemConfig.h
 *
 *  Created on: Mar 27, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_CONFIGURATION_SYSTEMCONFIG_H_
#define SRC_HDB_CONFIGURATION_SYSTEMCONFIG_H_

#include <stdint.h>
#include <sys/types.h>
#include <mpi.h>
#include <vector>
#include <string>

#define REQUEST_TIME_OUT_MS (100)

namespace hdb {
namespace configuration {

class SystemConfig {

public:

	MPI_Comm localCommunicator;
 
	std::vector<std::string> transaction_constraints; 
	uint32_t comdelay; 
	bool waitdie;
	bool nowait;
	bool timestamp;
	bool hashlock;

	bool isLockTableAgent;
    bool syntheticmode;

    uint32_t timelimit;
	uint32_t numberOfNodes;

	int32_t globalRank;
	int32_t globalNumberOfProcesses;

	int32_t localRank;
	int32_t localNumberOfProcesses;

	uint32_t locksPerWarehouse;
	uint32_t locksOnThisAgent;
	uint32_t globalNumberOfLocks;
	uint32_t globalNumberOfWarehouses;
	uint32_t globalNumberOfTransactionAgents;

	uint32_t internalRank;
	uint32_t globalNumberOfLockTableAgents;
	uint32_t *lockServerGlobalRanks;

	std::vector<uint32_t> warehouseToLockServer;
	std::vector<uint32_t> AllLocksPerAgent;

};

} /* namespace configuration */
} /* namespace hdb */

#endif /* SRC_HDB_CONFIGURATION_SYSTEMCONFIG_H_ */
