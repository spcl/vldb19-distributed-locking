/*
 * LockTableAgent.cpp
 *
 *  Created on: Mar 27, 2018
 *      Author: claudeb
 */

#include "LockTableAgent.h"

#include <stdlib.h>
#include <unistd.h>

#include <hdb/messages/MessageUtils.h>
#include <hdb/messages/LockRequest.h>
#include <hdb/messages/LockGrant.h>
#include <hdb/messages/LockRelease.h>
#include <hdb/messages/VoteRequest.h>
#include <hdb/messages/AbortTransaction.h>
#include <hdb/messages/TransactionEnd.h>
#include <hdb/messages/Shutdown.h>
#include <hdb/utils/Debug.h>

namespace hdb {
namespace locktable {

LockTableAgent::LockTableAgent(hdb::configuration::SystemConfig *config) {

	this->config = config;
	this->communicator = new hdb::communication::Communicator(config);

    uint32_t lockPerLockAgent  = config->locksOnThisAgent; 
    DLOG("LockTableAgent","Lockagent[%d] has %d locks" ,config->internalRank, lockPerLockAgent); 
    if( config->waitdie ){
    	this->table = new hdb::locktable::TableWaitDie(lockPerLockAgent, communicator);
    } else if(config->nowait){
    	this->table = new hdb::locktable::TableNoWait(lockPerLockAgent, communicator);
    } else if(config->timestamp){
    	this->table = new hdb::locktable::TableTimestamp(lockPerLockAgent, communicator);
    } else { // simple 2pl
    	this->table = new hdb::locktable::Table2PL(lockPerLockAgent, communicator);
    }

	
	this->remainingClients = config->globalNumberOfProcesses - config->globalNumberOfLockTableAgents;
}

LockTableAgent::~LockTableAgent() {

	delete table;
	delete communicator;

}

void LockTableAgent::execute() {

	DLOG("LockTableAgent", "Process %d running lock table %d", config->globalRank, config->internalRank);

	hdb::messages::Message *message = NULL;

	uint32_t handleCounter = 0;
	while (remainingClients > 0) {
		message = communicator->getMessage();
		if (message != NULL) {
			processMessage(message);
		}
		++handleCounter;
		if(handleCounter % 1024 == 0) {
			table->handleExpiredRequests();
		}
	}

}

void LockTableAgent::processMessage(hdb::messages::Message* message) {

	DLOG("LockTableAgent", "Received message %d, %d, %d", message->messageType, message->clientGlobalRank, message->transactionNumber);

	switch (message->messageType) {

	case hdb::messages::MessageType::LOCK_REQUEST_MESSAGE: {
#ifdef USE_LOGGING
		((hdb::messages::LockRequest *) message)->serverReceiveTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
		table->insertNewRequest((hdb::messages::LockRequest *) message);
		break;
	}

	case hdb::messages::MessageType::LOCK_RELEASE_MESSAGE: {
		table->releaseLock((hdb::messages::LockRelease *) message);
		break;
	}

	case hdb::messages::MessageType::VOTE_REQUEST_MESSAGE: {
		communicator->vote(message->clientGlobalRank, true);
		break;
	}

	case hdb::messages::MessageType::TX_END_MESSAGE: {
		table->releaseTransaction((hdb::messages::TransactionEnd *) message);
		communicator->signalTransactionEnd(message->clientGlobalRank);
		break;
	}

	case hdb::messages::MessageType::TX_ABORT_MESSAGE: {
		table->abortTransaction((hdb::messages::AbortTransaction *) message);
		communicator->signalTransactionEnd(message->clientGlobalRank);
		break;
	}

	case hdb::messages::MessageType::SHUTDOWN_MESSAGE: {
		--remainingClients;
		break;
	}

	default: {
		exit(-1);
		break;
	}
	}
}

} /* namespace locktable */
} /* namespace hdb */

