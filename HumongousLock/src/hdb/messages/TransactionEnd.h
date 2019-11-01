/*
 * TransactionEnd.h
 *
 *  Created on: Apr 17, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_MESSAGES_TRANSACTIONEND_H_
#define SRC_HDB_MESSAGES_TRANSACTIONEND_H_

#include <stdint.h>

#include <hdb/messages/Message.h>

namespace hdb {
namespace messages {

class TransactionEnd : public Message {

public:

	TransactionEnd(uint32_t clientGlobalRank = 0, uint64_t transactionNumber = 0);
	virtual ~TransactionEnd();

};

} /* namespace messages */
} /* namespace hdb */

#endif /* SRC_HDB_MESSAGES_TRANSACTIONEND_H_ */
