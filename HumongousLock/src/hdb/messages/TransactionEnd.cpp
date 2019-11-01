/*
 * TransactionEnd.cpp
 *
 *  Created on: Apr 17, 2018
 *      Author: claudeb
 */

#include <hdb/messages/TransactionEnd.h>

namespace hdb {
namespace messages {

TransactionEnd::TransactionEnd(uint32_t clientGlobalRank, uint64_t transactionNumber) : Message(TX_END_MESSAGE, sizeof(TransactionEnd), clientGlobalRank, transactionNumber) {

}

TransactionEnd::~TransactionEnd() {

}

} /* namespace messages */
} /* namespace hdb */
