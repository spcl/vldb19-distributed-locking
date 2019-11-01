/*
 * Transaction.cpp
 *
 *  Created on: Apr 18, 2018
 *      Author: claudeb
 */

#include "Transaction.h"

namespace hdb {
namespace transactions {

Transaction::Transaction(hdb::transactions::DataMode transactionTypeId) {
	this->transactionTypeId = transactionTypeId;
}
 

} /* namespace transactions */
} /* namespace hdb */


