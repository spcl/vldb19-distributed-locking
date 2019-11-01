/*
 * Lock.cpp
 *
 *  Created on: Apr 17, 2018
 *      Author: claudeb
 */

#include "Lock.h"
#include <hdb/utils/Debug.h>
#include <cassert>

namespace hdb {
namespace locktable {

Lock2PL::Lock2PL() {

	this->currentMode = (LockMode) 0;
	this->lockOffset = 0;
	for (uint32_t i = 0; i < hdb::locktable::LockMode::MODE_COUNT; ++i) {
		grantedCounters[i] = 0;
	}

}

Lock2PL::Lock2PL(uint32_t lockOffset) {

	this->currentMode = (LockMode) 0;
	this->lockOffset = lockOffset;
	for (uint32_t i = 0; i < hdb::locktable::LockMode::MODE_COUNT; ++i) {
		grantedCounters[i] = 0;
	}

}
	 

bool Lock2PL::insertNewRequest(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant) {

#ifdef USE_LOGGING
	request.serverEnqueueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif

	if (currentMode == hdb::locktable::LockMode::NL) {
		instantGrant(request, grant);
		return true;
	} else if (requestQueue.empty() && LOCK_MODE_COMPATIBLE(currentMode, request.mode)) {
		instantGrant(request, grant);
		return true;
	} else{
		request.setExpiryTime();
		requestQueue.push(request);
		return false;
	}

}

void Lock2PL::instantGrant(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant) {

#ifdef USE_LOGGING
	request.serverDequeueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
	++(grantedCounters[request.mode]);
	computeLockMode();

	new (grant) hdb::messages::LockGrant(request.clientGlobalRank, request.transactionNumber, request.lockId, request.mode);
#ifdef USE_LOGGING
	grant->clientSendTime = request.clientSendTime;
	grant->serverReceiveTime = request.serverReceiveTime;
	grant->serverEnqueueTime = request.serverEnqueueTime;
	grant->serverDequeueTime = request.serverDequeueTime;
#endif

}

bool Lock2PL::processNextElementFromQueue(hdb::messages::LockGrant* grant) {

	if (!requestQueue.empty()) {
		hdb::messages::LockRequest request = requestQueue.front();

		if (LOCK_MODE_COMPATIBLE(currentMode, request.mode)) {
			requestQueue.pop();
#ifdef USE_LOGGING
			request.serverDequeueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
			++(grantedCounters[request.mode]);
			computeLockMode();
			new (grant) hdb::messages::LockGrant(request.clientGlobalRank, request.transactionNumber, request.lockId, request.mode);
#ifdef USE_LOGGING
			grant->clientSendTime = request.clientSendTime;
			grant->serverReceiveTime = request.serverReceiveTime;
			grant->serverEnqueueTime = request.serverEnqueueTime;
			grant->serverDequeueTime = request.serverDequeueTime;
#endif
			return true;
		}
	}

	return false;

}

bool Lock2PL::processExpiredElementsFromQueue(hdb::messages::LockGrant* grant) {

	if (!requestQueue.empty()) {
		hdb::messages::LockRequest request = requestQueue.front();

		if (request.requestExpired()) {
			requestQueue.pop();
#ifdef USE_LOGGING
			request.serverDequeueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
			new (grant) hdb::messages::LockGrant(request.clientGlobalRank, request.transactionNumber, request.lockId, request.mode, false);
#ifdef USE_LOGGING
			grant->clientSendTime = request.clientSendTime;
			grant->serverReceiveTime = request.serverReceiveTime;
			grant->serverEnqueueTime = request.serverEnqueueTime;
			grant->serverDequeueTime = request.serverDequeueTime;
#endif
			return true;
		}
	}

	return false;

}

void Lock2PL::releaseLock(hdb::locktable::LockMode modeToRelease) {

	assert((uint32_t ) modeToRelease < hdb::locktable::LockMode::MODE_COUNT);
	assert(grantedCounters[modeToRelease] > 0);

	--(grantedCounters[modeToRelease]);
	computeLockMode();

}

bool Lock2PL::queueHasPendingElements() {

	return !(requestQueue.empty());

}

void Lock2PL::computeLockMode() {

	currentMode = (LockMode) 0;
	for (uint32_t i = 0; i < LockMode::MODE_COUNT; ++i) {
		if (grantedCounters[i] > 0) {
			currentMode = (LockMode) i;
		}
	}

}




/*********************************************************************************/


LockWaitDie::LockWaitDie() {

	this->currentMode = (LockMode) 0;
	this->lockOffset = 0;
	for (uint32_t i = 0; i < hdb::locktable::LockMode::MODE_COUNT; ++i) {
		grantedCounters[i] = 0;
	}
	this->version = 0;

}

LockWaitDie::LockWaitDie(uint32_t lockOffset) {

	this->currentMode = (LockMode) 0;
	this->lockOffset = lockOffset;
	for (uint32_t i = 0; i < hdb::locktable::LockMode::MODE_COUNT; ++i) {
		grantedCounters[i] = 0;
	}
	this->version = 0;
}

void LockWaitDie::UpdateTimestamp(hdb::locktable::LockMode mode, uint64_t timestamp){
	if(WRITE_LOCK_MODE(mode)){
		this->version = timestamp;
	}  
}
	 

LockReturn LockWaitDie::insertNewRequest(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant) {

#ifdef USE_LOGGING
	request.serverEnqueueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
    
	if (currentMode == hdb::locktable::LockMode::NL) {
		instantGrant(request, grant);
		return LockReturn::Granted;
	} else if (requestQueue.empty() && LOCK_MODE_COMPATIBLE(currentMode, request.mode)) {
		instantGrant(request, grant);
		return LockReturn::Granted;
	} else if (request.GetGlobalTransactionId() < this->version ){
		instantExpire(request, grant);
		return LockReturn::Expired;
	}
	else{
		request.setExpiryTime();
		requestQueue.push(request);
		return LockReturn::Queued;
	}

}

void LockWaitDie::instantExpire(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant) {

#ifdef USE_LOGGING
		request.serverDequeueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
		new (grant) hdb::messages::LockGrant(request.clientGlobalRank, request.transactionNumber, request.lockId, request.mode, false);
#ifdef USE_LOGGING
		grant->clientSendTime = request.clientSendTime;
		grant->serverReceiveTime = request.serverReceiveTime;
		grant->serverEnqueueTime = request.serverEnqueueTime;
		grant->serverDequeueTime = request.serverDequeueTime;
#endif

}

void LockWaitDie::instantGrant(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant) {

#ifdef USE_LOGGING
	request.serverDequeueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
	++(grantedCounters[request.mode]);
	computeLockMode();

	UpdateTimestamp(request.mode, request.GetGlobalTransactionId());
	new (grant) hdb::messages::LockGrant(request.clientGlobalRank, request.transactionNumber, request.lockId, request.mode);
#ifdef USE_LOGGING
	grant->clientSendTime = request.clientSendTime;
	grant->serverReceiveTime = request.serverReceiveTime;
	grant->serverEnqueueTime = request.serverEnqueueTime;
	grant->serverDequeueTime = request.serverDequeueTime;
#endif

}

bool LockWaitDie::processNextElementFromQueue(hdb::messages::LockGrant* grant) {

	if (!requestQueue.empty()) {
		hdb::messages::LockRequest request = requestQueue.top();

		if (LOCK_MODE_COMPATIBLE(currentMode, request.mode)) {
			requestQueue.pop();
#ifdef USE_LOGGING
			request.serverDequeueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
			++(grantedCounters[request.mode]);
			computeLockMode();
			UpdateTimestamp(request.mode, request.GetGlobalTransactionId());
			new (grant) hdb::messages::LockGrant(request.clientGlobalRank, request.transactionNumber, request.lockId, request.mode);
#ifdef USE_LOGGING
			grant->clientSendTime = request.clientSendTime;
			grant->serverReceiveTime = request.serverReceiveTime;
			grant->serverEnqueueTime = request.serverEnqueueTime;
			grant->serverDequeueTime = request.serverDequeueTime;
#endif
			return true;
		}
	}

	return false;

}

bool LockWaitDie::processExpiredElementsFromQueue(hdb::messages::LockGrant* grant) {

	if (!requestQueue.empty()) {
		hdb::messages::LockRequest request = requestQueue.top();

		if (request.requestExpired()) {
			requestQueue.pop();
#ifdef USE_LOGGING
			request.serverDequeueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
			new (grant) hdb::messages::LockGrant(request.clientGlobalRank, request.transactionNumber, request.lockId, request.mode, false);
#ifdef USE_LOGGING
			grant->clientSendTime = request.clientSendTime;
			grant->serverReceiveTime = request.serverReceiveTime;
			grant->serverEnqueueTime = request.serverEnqueueTime;
			grant->serverDequeueTime = request.serverDequeueTime;
#endif
			return true;
		}
	}

	return false;

}

void LockWaitDie::releaseLock(hdb::locktable::LockMode modeToRelease) {

	assert((uint32_t ) modeToRelease < hdb::locktable::LockMode::MODE_COUNT);
	assert(grantedCounters[modeToRelease] > 0);

	--(grantedCounters[modeToRelease]);
	computeLockMode();

}

bool LockWaitDie::queueHasPendingElements() {

	return !(requestQueue.empty());

}

void LockWaitDie::computeLockMode() {

	currentMode = (LockMode) 0;
	for (uint32_t i = 0; i < LockMode::MODE_COUNT; ++i) {
		if (grantedCounters[i] > 0) {
			currentMode = (LockMode) i;
		}
	}

}

/**************************************************************************************************/

LockNoWait::LockNoWait() {

	this->currentMode = (LockMode) 0;
	this->lockOffset = 0;
	for (uint32_t i = 0; i < hdb::locktable::LockMode::MODE_COUNT; ++i) {
		grantedCounters[i] = 0;
	}
}

LockNoWait::LockNoWait(uint32_t lockOffset) {

	this->currentMode = (LockMode) 0;
	this->lockOffset = lockOffset;
	for (uint32_t i = 0; i < hdb::locktable::LockMode::MODE_COUNT; ++i) {
		grantedCounters[i] = 0;
	}
}

bool LockNoWait::insertNewRequest(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant) {

#ifdef USE_LOGGING
	request.serverEnqueueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif

	if (LOCK_MODE_COMPATIBLE(currentMode, request.mode)) {
		instantGrant(request, grant);
		return true;
	} else {	
		instantExpire(request, grant);
		return false;
	}
}

void LockNoWait::instantExpire(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant) {

#ifdef USE_LOGGING
		request.serverDequeueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
		new (grant) hdb::messages::LockGrant(request.clientGlobalRank, request.transactionNumber, request.lockId, request.mode, false);
#ifdef USE_LOGGING
		grant->clientSendTime = request.clientSendTime;
		grant->serverReceiveTime = request.serverReceiveTime;
		grant->serverEnqueueTime = request.serverEnqueueTime;
		grant->serverDequeueTime = request.serverDequeueTime;
#endif

}

void LockNoWait::instantGrant(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant) {

#ifdef USE_LOGGING
	request.serverDequeueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
	++(grantedCounters[request.mode]);
	computeLockMode();

	new (grant) hdb::messages::LockGrant(request.clientGlobalRank, request.transactionNumber, request.lockId, request.mode);
#ifdef USE_LOGGING
	grant->clientSendTime = request.clientSendTime;
	grant->serverReceiveTime = request.serverReceiveTime;
	grant->serverEnqueueTime = request.serverEnqueueTime;
	grant->serverDequeueTime = request.serverDequeueTime;
#endif

}

void LockNoWait::releaseLock(hdb::locktable::LockMode modeToRelease) {

	assert((uint32_t ) modeToRelease < hdb::locktable::LockMode::MODE_COUNT);
	assert(grantedCounters[modeToRelease] > 0);

	--(grantedCounters[modeToRelease]);
	computeLockMode();

}


void LockNoWait::computeLockMode() {
	currentMode = (LockMode) 0;
	for (uint32_t i = 0; i < LockMode::MODE_COUNT; ++i) {
		if (grantedCounters[i] > 0) {
			currentMode = (LockMode) i;
		}
	}
}

/**************************************************/


Row::Row() {
	rts = 0;
	wts = 0;
}
	 
/*
RowReturn Row::insertNewRequest(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant) {

#ifdef USE_LOGGING
	request.serverEnqueueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif

	if (!LOCK_READ_OR_WRITE(request.mode)) {
		instantGrant(request, grant);
		return RowReturn::Success;
	} 

	uint64_t TS = request.transactionNumber;

    if(request.mode == LockMode::S){
    	if(TS < this->wts){
    		instantExpire(request, grant);
			return RowReturn::Reject;
    	}
    	if(!prewrites.empty() && TS > (*prewrites.begin())  ){
    		reads.insert(TS);
    		return  RowReturn::Wait;
    	}
    	this->rts = std::max(this->rts, TS);
    	instantGrant(request, grant);
		return  RowReturn::Success;
    }
    else{ // for writes
    	if(TS < this->wts ||  TS < this->rts ){
    		instantExpire(request, grant);
			return RowReturn::Reject;
    	}
  		prewrites.insert(TS);
  		instantGrant(request, grant);
		return  RowReturn::Success;
    }
}


RowReturn Row::commit(hdb::messages::LockRequest request, hdb::messages::LockGrant* grant) {

 
	uint64_t TS = request.transactionNumber;

	// prewrites cannot be empty as we try to finish write
  	uint64_t minpts = *prewrites.begin();

	if(TS > minpts ){
		writes.insert(TS);
		return  RowReturn::Wait;
	}
	if(!reads.empty() && TS > (*reads.begin())  ){
		writes.insert(TS);
		return  RowReturn::Wait;
	}
    // DOIT
	size_t removed = prewrites.erase(TS);
	assert(removed && "The TS must be in prewrites");

	// update write timestamp;
    this->wts = TS;
 
	trigger(minpts);
}


void Row::trigger(uint64_t minpts, bool forcetrigger=false){

    // check whether minpts has been updated
	while(!prewrites.empty() && minpts != *prewrites.begin()){

		minpts = *prewrites.begin();
		//trigger reads
		for (auto it = reads.begin(); it != reads.end();  ){
			if(*it > minpts){
				break;
			}else{
				this->rts = std::mac(this->rts,*it);
				sendgranted();
				it = reads.erase(it);
			}
		}

		uint64_t minrts = *reads.begin();

		//trigger writes
		for (auto it = writes.begin(); it != writes.end();  ){
			if(*it > minpts || *it > minrts){
				break;
			}else{
				size_t removed = prewrites.erase(*it);
				assert(removed && "The TS must be in prewrites");
				this->wts = *it;
				sendgranted();
				it = writes.erase(it);
			}
		}
	} 

	// if there is no prewrites we can serve all reads
	// if there is no prewrites then there is no writes as well
	if(prewrites.empty()){
		for (auto it = reads.begin(); it != reads.end();  ){
				this->rts = std::max(this->rts,*it);
				sendgranted();
				it = reads.erase(it);
		}
	}


}

RowReturn Row::abort(hdb::messages::LockRequest request) {

	uint64_t TS = request.transactionNumber;
 
	size_t removed = reads.erase(TS);
	if(removed){
		// TODO trigger reads? 
	}

	if(!prewrites.empty()){
		uint64_t minpts = *prewrites.begin();
		removed = prewrites.erase(TS);
		trigger(minpts);
	}


	return RowReturn::Success;
}

*/


} /* namespace locktable */
} /* namespace hdb */
