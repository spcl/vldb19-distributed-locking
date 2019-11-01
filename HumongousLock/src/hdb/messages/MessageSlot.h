/*
 * MessageSlot.h
 *
 *  Created on: Apr 16, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_MESSAGES_MESSAGESLOT_H_
#define SRC_HDB_MESSAGES_MESSAGESLOT_H_

#include <hdb/messages/MessageUtils.h>

#include <atomic>

namespace hdb {
namespace messages {

struct MessageSlot {
        uint64_t valid;
	char data[MAX_MESSAGE_SIZE];

} __attribute__((aligned(64)));

} /* namespace messages */
} /* namespace hdb */

#endif /* SRC_HDB_MESSAGES_MESSAGESLOT_H_ */
