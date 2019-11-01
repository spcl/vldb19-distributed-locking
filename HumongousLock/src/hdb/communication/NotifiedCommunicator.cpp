/*
 * NotifiedCommunicator.cpp
 *
 *  Created on: Apr 28, 2018
 *      Author: claudeb
 */

#include "NotifiedCommunicator.h"

#ifdef USE_FOMPI
#include <chrono>
#include <thread>
#include <hdb/messages/MessageUtils.h>
#include <hdb/utils/Debug.h>

namespace hdb {
namespace communication {

NotifiedCommunicator::NotifiedCommunicator(hdb::configuration::SystemConfig* config): comdelay(config->comdelay) {

	globalRank = config->globalRank;
	internalRank = config->internalRank;
	numberOfProcessesPerMachine = config->localNumberOfProcesses;
	localMachineId = globalRank / numberOfProcessesPerMachine;
	isLockServerEndPoint = config->isLockTableAgent;
	windowSizeInSlots = (isLockServerEndPoint) ? (config->globalNumberOfProcesses - config->globalNumberOfLockTableAgents) : (1);

	//DLOG_ALWAYS("NotifiedCommunicator", "Process %d allocating space for %d messages (%lu bytes)", globalRank, windowSizeInSlots, windowSizeInSlots * MAX_MESSAGE_SIZE);

	foMPI_Win_allocate(windowSizeInSlots * MAX_MESSAGE_SIZE, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &(buffer), &(window));

	memset(buffer, 0, windowSizeInSlots * MAX_MESSAGE_SIZE);

	//DLOG_ALWAYS("NotifiedCommunicator", "Process %d has memset'ed %d messages", globalRank, windowSizeInSlots);

	MPI_Barrier(MPI_COMM_WORLD);

	//DLOG_ALWAYS("NotifiedCommunicator", "Process %d passed initialization barrier", globalRank);

	foMPI_Win_lock_all(0, window);
	foMPI_Notify_init(window, foMPI_ANY_SOURCE, foMPI_ANY_TAG, 1, &(request));

	foMPI_Start(&request);

	//DLOG_ALWAYS("NotifiedCommunicator", "Process %d is ready for communication", globalRank);

}

NotifiedCommunicator::~NotifiedCommunicator() {

	foMPI_Win_unlock_all(window);
	foMPI_Win_free(&(window));

}

bool NotifiedCommunicator::sendMessage(hdb::messages::Message* message, uint32_t targetGlobalRank) {

	uint32_t targetOffset = (!isLockServerEndPoint) ? (internalRank) : (0);
	uint32_t messageSize = message->messageSize;

	//DLOG_ALWAYS("NotifiedCommunicator", "Process %d sending message to process %d. Placing %d bytes at offset %d", globalRank, targetGlobalRank, messageSize, targetOffset);
     
    bool islocal = targetGlobalRank/numberOfProcessesPerMachine == localMachineId;
    if(!islocal && this->comdelay != 0){
		    auto const sleep_end_time = std::chrono::high_resolution_clock::now() +
                                std::chrono::nanoseconds(this->comdelay);
    while (std::chrono::high_resolution_clock::now() < sleep_end_time);
	}
	foMPI_Put_notify(message, messageSize, MPI_BYTE, targetGlobalRank, targetOffset * MAX_MESSAGE_SIZE, messageSize, MPI_BYTE, window, targetOffset);
	foMPI_Win_flush_local(targetGlobalRank, window);

	//DLOG_ALWAYS("NotifiedCommunicator", "Process %d sending message to process %d. Flush completed.", globalRank, targetGlobalRank, messageSize, targetOffset);

	return islocal;

}

hdb::messages::Message* NotifiedCommunicator::getMessage() {

	int flag = 0;
	foMPI_Test(&request, &flag, &status);
	foMPI_Start(&request);

	//foMPI_Wait(&request, &status);
	//DLOG_ALWAYS("NotifiedCommunicator", "Flag of %d is %d", globalRank, flag);

	if(flag == 1) {
		int32_t offset = status.MPI_TAG;
	//	DLOG_ALWAYS("NotifiedCommunicator", "Process %d received message from process %d", globalRank, offset);
		return (hdb::messages::Message *) ((((char *) buffer) + offset * MAX_MESSAGE_SIZE));
	} else {
		return NULL;
	}

}

hdb::messages::Message* NotifiedCommunicator::getMessageBlocking() {

	foMPI_Wait(&request, &status);
	foMPI_Start(&request);
	int32_t offset = status.MPI_TAG;
	return (hdb::messages::Message *) ((((char *) buffer) + offset * MAX_MESSAGE_SIZE));

}

} /* namespace communication */
} /* namespace hdb */



#endif
