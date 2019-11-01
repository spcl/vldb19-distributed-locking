/*
 * Message.cpp
 *
 *  Created on: Mar 26, 2018
 *      Author: claudeb
 */

#include "Message.h"

namespace hdb {
namespace messages {

Message::Message(MessageType messageType, uint32_t messageSize, uint32_t clientGlobalRank, uint64_t transactionNumber) {

	this->messageType = messageType;
	this->messageSize = messageSize;
	this->clientGlobalRank = clientGlobalRank;
	this->transactionNumber = transactionNumber;

}

Message::~Message() {
	// TODO Auto-generated destructor stub
}

} /* namespace messages */
} /* namespace hdb */
