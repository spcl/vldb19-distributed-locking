/*
 * HybridCommunicator.cpp
 *
 *  Created on: Apr 28, 2018
 *      Author: claudeb
 */

#include "HybridCommunicator.h"

#ifdef USE_FOMPI
#include <chrono>
#include <thread>
#include <hdb/messages/MessageUtils.h>
#include <hdb/utils/Debug.h>

namespace hdb {
namespace communication {

HybridCommunicator::HybridCommunicator(hdb::configuration::SystemConfig* config): comdelay(config->comdelay) {

	globalRank = config->globalRank;
	internalRank = config->internalRank;
	numberOfProcessesPerMachine = config->localNumberOfProcesses;
	localMachineId = globalRank / numberOfProcessesPerMachine;
	isLockServerEndPoint = config->isLockTableAgent;
	windowSizeInSlots = (isLockServerEndPoint) ? (config->globalNumberOfProcesses - config->globalNumberOfLockTableAgents) : (1);

	// Slots only for local communication
	uint32_t transAgentsPerNode = (config->globalNumberOfProcesses - config->globalNumberOfLockTableAgents) / config->numberOfNodes;

    this->Slots = (isLockServerEndPoint) ? (transAgentsPerNode) : (1);


	foMPI_Win_allocate(windowSizeInSlots * MAX_MESSAGE_SIZE, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &(buffer), &(window));

	memset(buffer, 0, windowSizeInSlots * MAX_MESSAGE_SIZE);

	receiveBuffer = (void**)malloc(sizeof(void*)*Slots);
	if(isLockServerEndPoint){
 		for(uint32_t i = 0 ; i < Slots; i++){
 			uint32_t offset = localMachineId*transAgentsPerNode + i;
 			receiveBuffer[i] = ((((char *) buffer) + offset * MAX_MESSAGE_SIZE));
 		}
	}else{
		receiveBuffer[0] = buffer;
	}

	requests = (MPI_Request*)malloc(sizeof(MPI_Request)*Slots);
    for(uint32_t i=0; i < Slots; i++){
            MPI_Irecv(receiveBuffer[i],MAX_MESSAGE_SIZE, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[i]);
    }
 
	MPI_Barrier(MPI_COMM_WORLD);
 
	foMPI_Win_lock_all(0, window);
	foMPI_Notify_init(window, foMPI_ANY_SOURCE, foMPI_ANY_TAG, 1, &(request));

	foMPI_Start(&request);

 
}

HybridCommunicator::~HybridCommunicator() {
    for(uint32_t i = 0; i< Slots; i++){
        MPI_Cancel(&requests[i]);
    }
    free(receiveBuffer);
	foMPI_Win_unlock_all(window);
	foMPI_Win_free(&(window));

}

bool HybridCommunicator::sendMessage(hdb::messages::Message* message, uint32_t targetGlobalRank) {
 
	uint32_t messageSize = message->messageSize;
 
    bool islocal = targetGlobalRank/numberOfProcessesPerMachine == localMachineId;
    if(!islocal && this->comdelay != 0){
		auto const sleep_end_time = std::chrono::high_resolution_clock::now() +
                                std::chrono::nanoseconds(this->comdelay);
    	while (std::chrono::high_resolution_clock::now() < sleep_end_time);
	}
	if(islocal){
		MPI_Send(message, messageSize, MPI_BYTE, targetGlobalRank, 0, MPI_COMM_WORLD);
	}else{
		uint32_t targetOffset = (!isLockServerEndPoint) ? (internalRank) : (0);
		foMPI_Put_notify(message, messageSize, MPI_BYTE, targetGlobalRank, targetOffset * MAX_MESSAGE_SIZE, messageSize, MPI_BYTE, window, targetOffset);
		foMPI_Win_flush_local(targetGlobalRank, window);
	}

 
	return islocal;

}

hdb::messages::Message* HybridCommunicator::getMessage() {

	int flag = 0;

    int indx;
    MPI_Testany(Slots, requests, &indx, &flag, MPI_STATUS_IGNORE);
    if(flag){
        MPI_Irecv(receiveBuffer[indx],MAX_MESSAGE_SIZE, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[indx]);
        return (hdb::messages::Message*) receiveBuffer[indx];
    }

	foMPI_Test(&request, &flag, &status);
	foMPI_Start(&request);
	if(flag == 1) {
		int32_t offset = status.MPI_TAG;
 		return (hdb::messages::Message *) ((((char *) buffer) + offset * MAX_MESSAGE_SIZE));
	}  

	return NULL;
}

hdb::messages::Message* HybridCommunicator::getMessageBlocking() {

	hdb::messages::Message* msg = NULL;
	while(msg == NULL){
		msg = getMessage();
	}
	return msg;

}

} /* namespace communication */
} /* namespace hdb */



#endif
