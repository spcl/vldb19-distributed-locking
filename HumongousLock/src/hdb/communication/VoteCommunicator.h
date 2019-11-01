/*
 * VoteCommunicator.h
 *
 *  Created on: Apr 17, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_COMMUNICATION_VOTECOMMUNICATOR_H_
#define SRC_HDB_COMMUNICATION_VOTECOMMUNICATOR_H_

#include <stdint.h>
#include <mpi.h>
#include <hdb/configuration/SystemConfig.h>

#ifdef USE_FOMPI
#include <fompi.h>
#endif


namespace hdb {
namespace communication {

class VoteCommunicator {

public:

	VoteCommunicator(hdb::configuration::SystemConfig* config);
	virtual ~VoteCommunicator();

protected:

	volatile uint64_t voteCounter;
#ifdef USE_FOMPI
	foMPI_Win windowObject;
#else
	MPI_Win windowObject;
#endif

protected:

	uint32_t globalRank;
	uint32_t numberOfProcessesPerMachine;
	uint32_t localMachineId;

public:
	const uint32_t comdelay;
	void vote(uint32_t targetRank, bool outcome);
	bool checkVoteReady(bool *outcome, uint32_t targetCount);
	void reset();

};

} /* namespace communication */
} /* namespace hdb */

#endif /* SRC_HDB_COMMUNICATION_VOTECOMMUNICATOR_H_ */
