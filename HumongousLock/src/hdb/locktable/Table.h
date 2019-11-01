/*
 * Table.h
 *
 *  Created on: Apr 18, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_LOCKTABLE_TABLE_H_
#define SRC_HDB_LOCKTABLE_TABLE_H_

#include <stdint.h>
#include <map>
#include <set>


#include <hdb/locktable/Lock.h>
#include <hdb/locktable/LockMode.h>
#include <hdb/messages/LockRequest.h>
#include <hdb/messages/LockGrant.h>
#include <hdb/messages/LockRelease.h>
#include <hdb/messages/TransactionEnd.h>
#include <hdb/communication/Communicator.h>
#include <hdb/messages/AbortTransaction.h>

namespace hdb {
namespace locktable {

// general interface for Table implementations
class Table {

public:

	virtual void insertNewRequest(hdb::messages::LockRequest *request) = 0;
	virtual void releaseLock(hdb::messages::LockRelease *release) = 0;
	virtual void releaseTransaction(hdb::messages::TransactionEnd *endTransaction) = 0;
	virtual void abortTransaction(hdb::messages::AbortTransaction *abortTransaction) = 0;
	
	virtual void handleExpiredRequests() = 0;
};

/*
Original version of simple 2PL coded by Claude
*/

class Table2PL: public Table {

	typedef std::pair<hdb::locktable::Lock2PL *, hdb::locktable::LockMode> lockpointer_mode_pair_t;


public:
	Table2PL(uint64_t numberOfElements, hdb::communication::Communicator *communicator);
	virtual ~Table2PL();

public:

	void insertNewRequest(hdb::messages::LockRequest *request) override;
	void releaseLock(hdb::messages::LockRelease *release) override;
	void releaseTransaction(hdb::messages::TransactionEnd *endTransaction) override;
	void abortTransaction(hdb::messages::AbortTransaction *abortTransaction) override;
	void handleExpiredRequests() override;


protected:
	void processAndSendNotifications(uint32_t lockOffset);

	std::set<Lock2PL *> locksWithPendingRequests;
	uint32_t numberOfElements;
	std::vector<lockpointer_mode_pair_t> aquiredLockCopy;
	hdb::messages::LockGrant *grantMessageBuffer;
	hdb::communication::Communicator *communicator;
	std::multimap<uint64_t, lockpointer_mode_pair_t> transactionLockMap;


	Lock2PL *locks;

};



class TableWaitDie: public Table {
	typedef std::pair<hdb::locktable::LockWaitDie *, hdb::locktable::LockMode> lockpointer_mode_pair_t;

public:
	TableWaitDie(uint64_t numberOfElements, hdb::communication::Communicator *communicator);
	virtual ~TableWaitDie();

public:

	void insertNewRequest(hdb::messages::LockRequest *request) override;
	void releaseLock(hdb::messages::LockRelease *release) override;
	void releaseTransaction(hdb::messages::TransactionEnd *endTransaction) override;
	void abortTransaction(hdb::messages::AbortTransaction *abortTransaction) override;
	void handleExpiredRequests() override;


protected:


	void processAndSendNotifications(uint32_t lockOffset);

	std::set<LockWaitDie *> locksWithPendingRequests;
	uint32_t numberOfElements;
	std::vector<lockpointer_mode_pair_t> aquiredLockCopy;
	hdb::messages::LockGrant *grantMessageBuffer;
	hdb::communication::Communicator *communicator;
	std::multimap<uint64_t, lockpointer_mode_pair_t> transactionLockMap;


	LockWaitDie *locks;
};




class TableNoWait: public Table {
	typedef std::pair<hdb::locktable::LockNoWait *, hdb::locktable::LockMode> lockpointer_mode_pair_t;

public:
	TableNoWait(uint64_t numberOfElements, hdb::communication::Communicator *communicator);
	virtual ~TableNoWait();

public:

	void insertNewRequest(hdb::messages::LockRequest *request) override;
	void releaseLock(hdb::messages::LockRelease *release) override;
	void releaseTransaction(hdb::messages::TransactionEnd *endTransaction) override;
	void abortTransaction(hdb::messages::AbortTransaction *abortTransaction) override;
	void handleExpiredRequests() override;
 

protected:
 
	uint32_t numberOfElements;
	std::vector<lockpointer_mode_pair_t> aquiredLockCopy;
	hdb::communication::Communicator *communicator;
	std::multimap<uint64_t, lockpointer_mode_pair_t> transactionLockMap;
	hdb::messages::LockGrant *grantMessageBuffer;


	LockNoWait *locks;
};



/*
	We don't know the implementation
	We need some prewrites!

*/

class TableTimestamp: public Table {

public:
	TableTimestamp(uint64_t numberOfElements, hdb::communication::Communicator *communicator);
	virtual ~TableTimestamp();

public:

	void insertNewRequest(hdb::messages::LockRequest *request) override;
	void releaseLock(hdb::messages::LockRelease *release) override;
	void releaseTransaction(hdb::messages::TransactionEnd *endTransaction) override;
	void abortTransaction(hdb::messages::AbortTransaction *abortTransaction) override;
 	void handleExpiredRequests() override;

protected:
 	void trigger(uint64_t minpts,hdb::locktable::Row* row);

	uint32_t numberOfElements;
	hdb::communication::Communicator *communicator;
	hdb::messages::LockGrant *grantMessageBuffer;

        hdb::locktable::Row *rows;
	std::vector<hdb::locktable::Row *> aquiredLockCopy;
	std::multimap<uint64_t, hdb::locktable::Row *> transactionLockMap;


};

} /* namespace locktable */
} /* namespace hdb */

#endif /* SRC_HDB_LOCKTABLE_TABLE_H_ */
