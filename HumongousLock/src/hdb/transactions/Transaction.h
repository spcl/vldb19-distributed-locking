/*
 * Transaction.h
 *
 *  Created on: Apr 18, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_TRANSACTIONS_TRANSACTION_H_
#define SRC_HDB_TRANSACTIONS_TRANSACTION_H_

#include <string>
#include <stdint.h>
#include <vector>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/binary_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/list.hpp>

#include <hdb/locktable/LockMode.h>


typedef std::pair<uint64_t, hdb::locktable::LockMode> lockid_mode_pair_t;

namespace hdb {
namespace transactions {


enum DataMode: uint8_t {
	NONE = 0, NEWORD = 1, PAYMENT = 2, ORDSTAT = 3, DELIVERY = 4 , SLEV = 5, YCSB = 6
};

static DataMode DATA_MODE_FROM_STRING(char *dataMode) {

		if(strcmp(dataMode,"neword") == 0) {
			return DataMode::NEWORD;
		} else if (strcmp(dataMode,"payment") == 0) {
			return DataMode::PAYMENT;
		} else if (strcmp(dataMode,"ordstat") == 0) {
			return DataMode::ORDSTAT;
		} else if (strcmp(dataMode,"delivery") == 0) {
			return DataMode::DELIVERY;
		} else if (strcmp(dataMode,"slev") == 0) {
			return DataMode::SLEV;
		} else {
			return DataMode::YCSB;
		}
}


class Transaction {

public:

	Transaction(hdb::transactions::DataMode transactionTypeId = hdb::transactions::DataMode::NONE);

public:

	hdb::transactions::DataMode transactionTypeId;
	std::vector<lockid_mode_pair_t> requests;
private:   
    friend class boost::serialization::access;
	  
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
	    ar & transactionTypeId;
	    ar & requests;
	}


};

} /* namespace transactions */
} /* namespace hdb */

#endif /* SRC_HDB_TRANSACTIONS_TRANSACTION_H_ */
