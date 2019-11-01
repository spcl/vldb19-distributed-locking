/*
 * LockRequest.h
 *
 *  Created on: Apr 16, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_MESSAGES_LOCKREQUEST_H_
#define SRC_HDB_MESSAGES_LOCKREQUEST_H_

#include <stdint.h>
#include <chrono>

#include <hdb/messages/Message.h>
#include <hdb/locktable/LockMode.h>

namespace hdb {
namespace messages {

class LockRequest : public Message {

public:

	LockRequest(uint32_t clientGlobalRank = 0, uint64_t transactionNumber = 0, uint64_t lockId = 0, hdb::locktable::LockMode mode = hdb::locktable::LockMode::NL);

	virtual ~LockRequest();

public:

	uint64_t lockId;
	hdb::locktable::LockMode mode;

public:

	uint64_t GetGlobalTransactionId() const;


	friend bool operator < (const LockRequest& lhs, const LockRequest& rhs);
    friend bool operator > (const LockRequest& lhs, const LockRequest& rhs);
 



	bool requestExpired();
	void setExpiryTime();
	std::chrono::high_resolution_clock::time_point expiryTime;



#ifdef USE_LOGGING
public:

	uint64_t clientSendTime;
	uint64_t serverReceiveTime;
	uint64_t serverEnqueueTime;
	uint64_t serverDequeueTime;
#endif

};

} /* namespace messages */
} /* namespace hdb */

#endif /* SRC_HDB_MESSAGES_LOCKREQUEST_H_ */
