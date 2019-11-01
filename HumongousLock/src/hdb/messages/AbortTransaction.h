/*
 * AbortTransaction.h
 *
 */

#ifndef SRC_HDB_MESSAGES_ABORTTRANSACTION_H_
#define SRC_HDB_MESSAGES_ABORTTRANSACTION_H_

#include <stdint.h>

#include <hdb/messages/Message.h>

namespace hdb {
namespace messages {

class AbortTransaction : public Message {

public:

	AbortTransaction(uint32_t clientGlobalRank = 0, uint64_t transactionNumber = 0);
	virtual ~AbortTransaction();

};

} /* namespace messages */
} /* namespace hdb */

#endif /* SRC_HDB_MESSAGES_ABORTTRANSACTION_H_ */
