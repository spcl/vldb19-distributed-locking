/*
 * Lock.h
 *
 *  Created on: Apr 17, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_LOCKTABLE_LOCK_H_
#define SRC_HDB_LOCKTABLE_LOCK_H_

#include <stdint.h>
#include <queue>
#include <list>
#include <deque>
#include <functional>
#include <set>
#include <map>
#include <hdb/locktable/LockMode.h>
#include <hdb/messages/LockRequest.h>
#include <hdb/messages/LockGrant.h>

namespace hdb {
namespace locktable {


enum LockReturn {
	Granted = 0, Expired = 1, Queued = 2
};


class Lock2PL {

public:

	Lock2PL();
	Lock2PL(uint32_t lockOffset);

public:

	uint32_t lockOffset;
	hdb::locktable::LockMode currentMode;

protected:

	uint32_t grantedCounters[hdb::locktable::LockMode::MODE_COUNT];
	std::queue<hdb::messages::LockRequest, 
						std::list<hdb::messages::LockRequest> > requestQueue;

public:

	bool insertNewRequest(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant);
	bool processNextElementFromQueue(hdb::messages::LockGrant *grant);
	void releaseLock(hdb::locktable::LockMode modeToRelease);

	bool processExpiredElementsFromQueue(hdb::messages::LockGrant *grant);
	bool queueHasPendingElements();

protected:

	void computeLockMode();
	void instantGrant(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant);
};





class LockWaitDie {

public:

	LockWaitDie();
	LockWaitDie(uint32_t lockOffset);

public:

	uint32_t lockOffset;
 	uint64_t version;
	hdb::locktable::LockMode currentMode;

protected:

	uint32_t grantedCounters[hdb::locktable::LockMode::MODE_COUNT];

	std::priority_queue<hdb::messages::LockRequest, 
						std::deque<hdb::messages::LockRequest>, 
						std::greater<std::deque<hdb::messages::LockRequest>::value_type>> requestQueue;

public:

	LockReturn insertNewRequest(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant);
	bool processNextElementFromQueue(hdb::messages::LockGrant *grant);
	void releaseLock(hdb::locktable::LockMode modeToRelease);


	bool checkTimestamp(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant);
	uint64_t GetTimestamp(hdb::locktable::LockMode mode);
	void UpdateTimestamp(hdb::locktable::LockMode mode, uint64_t timestamp);


public:

	bool processExpiredElementsFromQueue(hdb::messages::LockGrant *grant);
	bool queueHasPendingElements();

protected:

	void computeLockMode();
	void instantGrant(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant);
	void instantExpire(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant);

};

class LockNoWait {

public:

	LockNoWait();
	LockNoWait(uint32_t lockOffset);

public:

	uint32_t lockOffset;
	hdb::locktable::LockMode currentMode;

protected:

	uint32_t grantedCounters[hdb::locktable::LockMode::MODE_COUNT];


public:

	bool insertNewRequest(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant);
	bool processNextElementFromQueue(hdb::messages::LockGrant *grant);
	void releaseLock(hdb::locktable::LockMode modeToRelease);

protected:

	void computeLockMode();
	void instantGrant(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant);
	void instantExpire(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant);

};

// for timestamp ordering
class Row {

public:

	Row();
 
 
public:
	uint64_t rts; // read timestamp 
	uint64_t wts; // write timestamp

	std::set<uint64_t> reads;
	std::set<uint64_t> writes;
	std::set<uint64_t> prewrites;


	// it is used only for waiting S (read) requests
	std::map<uint64_t, hdb::messages::LockRequest> pendingreq;
};





} /* namespace locktable */
} /* namespace hdb */

#endif /* SRC_HDB_LOCKTABLE_LOCK_H_ */
