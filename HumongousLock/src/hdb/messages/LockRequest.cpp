/*
 * LockRequest.cpp
 *
 *  Created on: Apr 16, 2018
 *      Author: claudeb
 */

#include "LockRequest.h"

#include <hdb/configuration/SystemConfig.h>

namespace hdb {
namespace messages {

LockRequest::LockRequest(uint32_t clientGlobalRank, uint64_t transactionNumber, uint64_t lockId, hdb::locktable::LockMode mode) : Message(LOCK_REQUEST_MESSAGE, sizeof(LockRequest), clientGlobalRank, transactionNumber) {

	this->lockId = lockId;
	this->mode = mode;

#ifdef USE_LOGGING
	this->clientSendTime = 0;
	this->serverReceiveTime = 0;
	this->serverEnqueueTime = 0;
	this->serverDequeueTime = 0;
#endif

}

uint64_t LockRequest::GetGlobalTransactionId() const{
	return transactionNumber;
}

bool operator < (const LockRequest& lhs, const LockRequest& rhs)
{
    return lhs.GetGlobalTransactionId() < rhs.GetGlobalTransactionId();
}

bool operator > (const LockRequest& lhs, const LockRequest& rhs)
{
    return lhs.GetGlobalTransactionId() > rhs.GetGlobalTransactionId();
}


LockRequest::~LockRequest() {

}

bool LockRequest::requestExpired() {
	return (expiryTime < std::chrono::high_resolution_clock::now());
}

void LockRequest::setExpiryTime() {
	expiryTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(REQUEST_TIME_OUT_MS);
}

} /* namespace messages */
} /* namespace hdb */
