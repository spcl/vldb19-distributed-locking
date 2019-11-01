/*
 * LockGrant.cpp
 *
 *  Created on: Apr 17, 2018
 *      Author: claudeb
 */

#include "LockGrant.h"

namespace hdb {
namespace messages {

LockGrant::LockGrant(uint32_t clientGlobalRank, uint64_t transactionNumber, uint64_t lockId, hdb::locktable::LockMode mode, bool granted) : Message(LOCK_GRANT_MESSAGE, sizeof(LockGrant), clientGlobalRank, transactionNumber) {

	this->lockId = lockId;
	this->mode = mode;
	this->granted = granted;

#ifdef USE_LOGGING
	this->clientSendTime = 0;
	this->serverReceiveTime = 0;
	this->serverEnqueueTime = 0;
	this->serverDequeueTime = 0;
	this->serverSendTime = 0;
	this->clientReceiveTime = 0;
#endif

}

LockGrant::~LockGrant() {

}

} /* namespace messages */
} /* namespace hdb */
