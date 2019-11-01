/*
 * LockRelease.cpp
 *
 *  Created on: Apr 17, 2018
 *      Author: claudeb
 */

#include "LockRelease.h"

namespace hdb {
namespace messages {

LockRelease::LockRelease(uint32_t clientGlobalRank, uint64_t transactionNumber, uint64_t lockId) : Message(LOCK_RELEASE_MESSAGE, sizeof(LockRelease), clientGlobalRank, transactionNumber) {

	this->lockId = lockId;

}

LockRelease::~LockRelease() {
	// TODO Auto-generated destructor stub
}

} /* namespace messages */
} /* namespace hdb */
