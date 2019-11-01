/*
 * AbortTransaction.cpp
 *
 */

#include <hdb/messages/AbortTransaction.h>

namespace hdb {
namespace messages {

AbortTransaction::AbortTransaction(uint32_t clientGlobalRank, uint64_t transactionNumber) : Message(TX_ABORT_MESSAGE, sizeof(AbortTransaction), clientGlobalRank, transactionNumber) {

}

AbortTransaction::~AbortTransaction() {

}

} /* namespace messages */
} /* namespace hdb */
