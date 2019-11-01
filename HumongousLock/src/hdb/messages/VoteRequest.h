/*
 * VoteRequest.h
 *
 *  Created on: Apr 17, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_MESSAGES_VOTEREQUEST_H_
#define SRC_HDB_MESSAGES_VOTEREQUEST_H_

#include <stdint.h>

#include <hdb/messages/Message.h>

namespace hdb {
namespace messages {

class VoteRequest : public Message {

public:

	VoteRequest(uint32_t clientGlobalRank = 0, uint64_t transactionNumber = 0);
	virtual ~VoteRequest();

};

} /* namespace messages */
} /* namespace hdb */

#endif /* SRC_HDB_MESSAGES_VOTEREQUEST_H_ */
