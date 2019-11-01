/*
 * LockRelease.h
 *
 *  Created on: Apr 17, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_MESSAGES_LOCKRELEASE_H_
#define SRC_HDB_MESSAGES_LOCKRELEASE_H_

#include <stdint.h>

#include <hdb/messages/Message.h>

namespace hdb {
namespace messages {

class LockRelease : public Message {

public:

	LockRelease(uint32_t clientGlobalRank, uint64_t transactionNumber, uint64_t lockId);
	virtual ~LockRelease();

public:

	uint64_t lockId;
};

} /* namespace messages */
} /* namespace hdb */

#endif /* SRC_HDB_MESSAGES_LOCKRELEASE_H_ */
