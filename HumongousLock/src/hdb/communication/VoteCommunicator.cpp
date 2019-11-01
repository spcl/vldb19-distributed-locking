/*
 * VoteCommunicator.cpp
 *
 *  Created on: Apr 17, 2018
 *      Author: claudeb
 */

#include <hdb/communication/VoteCommunicator.h>

#include <stdio.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <hdb/utils/Debug.h>

namespace hdb {
namespace communication {

VoteCommunicator::VoteCommunicator(hdb::configuration::SystemConfig* config): comdelay(config->comdelay)  {

	voteCounter = 0;
	globalRank = config->globalRank;
	numberOfProcessesPerMachine = config->localNumberOfProcesses;
	localMachineId = config->globalRank / numberOfProcessesPerMachine;
	
	//DLOG_ALWAYS("NotifiedCommunicator", "Process %d allocating space for voting", globalRank);

#ifdef USE_FOMPI
	foMPI_Win_create((void *)&(voteCounter), sizeof(uint64_t), 1, MPI_INFO_NULL, MPI_COMM_WORLD, &(windowObject));
#else
	MPI_Win_create((void *) &(voteCounter), sizeof(uint64_t), 1, MPI_INFO_NULL, MPI_COMM_WORLD, &(windowObject));
#endif

	MPI_Barrier(MPI_COMM_WORLD);

#ifdef USE_FOMPI
	foMPI_Win_lock_all(0, windowObject);
#else
	MPI_Win_lock_all(0, windowObject);
#endif

	//DLOG_ALWAYS("NotifiedCommunicator", "Process %d is ready for voting", globalRank);
}

VoteCommunicator::~VoteCommunicator() {

#ifdef USE_FOMPI
	foMPI_Win_unlock_all(windowObject);
#else
	MPI_Win_unlock_all(windowObject);
#endif

}

void VoteCommunicator::vote(uint32_t targetRank, bool outcome) {

	uint64_t value = 1;
	uint64_t oldValue = 0;
	if (!outcome) {
		value = value << 32;
	}


    bool islocal = targetRank/numberOfProcessesPerMachine == localMachineId;
    if(!islocal && this->comdelay != 0){
		    auto const sleep_end_time = std::chrono::high_resolution_clock::now() +
                                std::chrono::nanoseconds(this->comdelay);
    while (std::chrono::high_resolution_clock::now() < sleep_end_time);
	}
#ifdef USE_FOMPI
	foMPI_Fetch_and_op(&(value), &(oldValue), MPI_UINT64_T, targetRank, 0, foMPI_SUM, windowObject);
	foMPI_Win_flush(targetRank, windowObject);
#else
	MPI_Fetch_and_op(&(value), &(oldValue), MPI_UINT64_T, targetRank, 0, MPI_SUM, windowObject);
	MPI_Win_flush(targetRank, windowObject);
#endif

}

bool VoteCommunicator::checkVoteReady(bool* outcome, uint32_t targetCount) {

	uint64_t currentValue = 0;
 

#ifdef USE_FOMPI
	//foMPI_Fetch_and_op(NULL, &(currentValue), MPI_UINT64_T, globalRank, 0, foMPI_NO_OP, windowObject);
	foMPI_Get(&currentValue, 1, MPI_UINT64_T, globalRank, 0, 1, MPI_UINT64_T, windowObject);
	foMPI_Win_flush(globalRank, windowObject);
#else
	//MPI_Fetch_and_op(NULL, &(currentValue), MPI_UINT64_T, globalRank, 0, MPI_NO_OP, windowObject);
	MPI_Get(&currentValue, 1, MPI_UINT64_T, globalRank, 0, 1, MPI_UINT64_T, windowObject);
	MPI_Win_flush(globalRank, windowObject);
#endif

	uint64_t yesVotes = (currentValue << 32) >> 32;
	uint64_t noVotes = (currentValue >> 32);

	bool voteReady = (yesVotes + noVotes >= targetCount);
	*outcome = (yesVotes == targetCount);

	//printf("Yes: %lu | No: %lu | CurrentValue: %lu | Load: %lu\n", yesVotes, noVotes, currentValue, voteCounter);

	return voteReady;

}

void VoteCommunicator::reset() {

 
	uint64_t value = 0;
	uint64_t oldValue = 0;
#ifdef USE_FOMPI
	foMPI_Fetch_and_op(&(value), &(oldValue), MPI_UINT64_T, globalRank, 0, foMPI_REPLACE, windowObject);
	foMPI_Win_flush(globalRank, windowObject);
#else
	MPI_Fetch_and_op(&(value), &(oldValue), MPI_UINT64_T, globalRank, 0, MPI_REPLACE, windowObject);
	MPI_Win_flush(globalRank, windowObject);
#endif
}

} /* namespace communication */
} /* namespace hdb */

