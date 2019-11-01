/*
 * SendReceiveCommunicator.cpp
 *
 *  Created on: Apr 30, 2018
 *      Author: claudeb
 */

#include "SendReceiveCommunicator.h"

#include <hdb/messages/MessageUtils.h>
#include <mpi.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <hdb/utils/Debug.h>

namespace hdb {
namespace communication {

SendReceiveCommunicator::SendReceiveCommunicator(hdb::configuration::SystemConfig *config): comdelay(config->comdelay) {

	numberOfProcessesPerMachine = config->localNumberOfProcesses;
	localMachineId = config->globalRank / numberOfProcessesPerMachine;
	globalMachineId = config->globalRank;


    this->Slots = (config->isLockTableAgent) ? (config->globalNumberOfProcesses - config->globalNumberOfLockTableAgents) : (1);

    receiveBuffer = (void**)malloc(sizeof(void*)*Slots);
    for(uint32_t i=0; i < Slots; i++){
            receiveBuffer[i] = malloc(MAX_MESSAGE_SIZE);
    }

	requests = (MPI_Request*)malloc(sizeof(MPI_Request)*Slots);
    for(uint32_t i=0; i < Slots; i++){
            MPI_Irecv(receiveBuffer[i],MAX_MESSAGE_SIZE, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[i]);
    }

}

SendReceiveCommunicator::~SendReceiveCommunicator() {
    for(uint32_t i = 0; i< Slots; i++){
        MPI_Cancel(&requests[i]);
        free(receiveBuffer[i]);
    }
    free(receiveBuffer);
    free(requests);

}

bool SendReceiveCommunicator::sendMessage(hdb::messages::Message* message, uint32_t targetGlobalRank) {
    

    bool islocal = targetGlobalRank/numberOfProcessesPerMachine == localMachineId;
    if(!islocal && this->comdelay != 0){
		    auto const sleep_end_time = std::chrono::high_resolution_clock::now() +
                                std::chrono::nanoseconds(this->comdelay);
            while (std::chrono::high_resolution_clock::now() < sleep_end_time);
	}
	MPI_Send(message, message->messageSize, MPI_BYTE, targetGlobalRank, 0, MPI_COMM_WORLD);

	return islocal;
}

hdb::messages::Message* SendReceiveCommunicator::getMessage() {

    int flag;
    int indx;
    MPI_Testany(Slots, requests, &indx, &flag, MPI_STATUS_IGNORE);
    if(flag){
            MPI_Irecv(receiveBuffer[indx],MAX_MESSAGE_SIZE, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[indx]);
            return (hdb::messages::Message*) receiveBuffer[indx];
    }
	return NULL;
}

hdb::messages::Message* SendReceiveCommunicator::getMessageBlocking() {
    int indx;
    MPI_Waitany(Slots, requests, &indx, MPI_STATUS_IGNORE);
    MPI_Irecv(receiveBuffer[indx],MAX_MESSAGE_SIZE, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &requests[indx]);
    return (hdb::messages::Message*) receiveBuffer[indx];
}

} /* namespace communication */
} /* namespace hdb */
