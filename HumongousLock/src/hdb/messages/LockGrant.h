/*
 * LockGrant.h
 *
 *  Created on: Apr 17, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_MESSAGES_LOCKGRANT_H_
#define SRC_HDB_MESSAGES_LOCKGRANT_H_

#include <stdint.h>

#include <hdb/messages/Message.h>
#include <hdb/locktable/LockMode.h>

namespace hdb {
namespace messages {

class LockGrant : public Message {
public:

	LockGrant(uint32_t clientGlobalRank, uint64_t transactionNumber, uint64_t lockId, hdb::locktable::LockMode mode, bool granted = true);
	virtual ~LockGrant();

public:

	uint64_t lockId;
	hdb::locktable::LockMode mode;
	bool granted;

#ifdef USE_LOGGING
public:

	uint64_t clientSendTime;
	uint64_t serverReceiveTime;
	uint64_t serverEnqueueTime;
	uint64_t serverDequeueTime;
	uint64_t serverSendTime;
	uint64_t clientReceiveTime;

#endif

};

} /* namespace messages */
} /* namespace hdb */

#endif /* SRC_HDB_MESSAGES_LOCKGRANT_H_ */
