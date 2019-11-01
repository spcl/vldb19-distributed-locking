/*
 * Message.h
 *
 *  Created on: Mar 26, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_MESSAGES_MESSAGE_H_
#define SRC_HDB_MESSAGES_MESSAGE_H_

#include <stdint.h>

namespace hdb {
namespace messages {

enum MessageType {

	LOCK_REQUEST_MESSAGE = 1,
	LOCK_GRANT_MESSAGE = 2,
	LOCK_RELEASE_MESSAGE = 3,
	VOTE_REQUEST_MESSAGE = 4,
	TX_END_MESSAGE = 5,
	SHUTDOWN_MESSAGE = 6,
	TX_ABORT_MESSAGE = 7

};

class Message {
public:

	Message(MessageType messageType, uint32_t messageSize, uint32_t clientGlobalRank, uint64_t transactionNumber);
	virtual ~Message();

public:
//	bool valid;
	MessageType messageType;

public:

	uint32_t clientGlobalRank;
	uint32_t messageSize;
	uint64_t transactionNumber;

};

} /* namespace messages */
} /* namespace hdb */

#endif /* SRC_HDB_MESSAGES_MESSAGE_H_ */
