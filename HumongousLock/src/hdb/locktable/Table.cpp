/*
 * Table.cpp
 *
 *  Created on: Apr 18, 2018
 *      Author: claudeb
 */

#include "Table.h"

#include <cassert>
#include <hdb/transactions/Transaction.h>
#include <hdb/utils/Debug.h>
#include <limits>

namespace hdb {
namespace locktable {

Table2PL::Table2PL(uint64_t numberOfElements, hdb::communication::Communicator *communicator){

	this->numberOfElements = numberOfElements;
	this->locks = new hdb::locktable::Lock2PL[numberOfElements];
        if(this->locks==NULL){
                 DLOG_ALWAYS("TABLE","Memory allocation error of locks %lu",numberOfElements);
	} else{
	        for (uint64_t i = 0; i < numberOfElements; ++i) {
                     this->locks[i].lockOffset = i;
        	}
	}
	this->communicator = communicator;
	this->grantMessageBuffer = (hdb::messages::LockGrant *) calloc(1, sizeof(hdb::messages::LockGrant));

	this->aquiredLockCopy.reserve(512);
}

Table2PL::~Table2PL() {

	delete[] locks;

}

void Table2PL::insertNewRequest(hdb::messages::LockRequest* request) {

	uint32_t lockOffset = request->lockId % numberOfElements;
   	bool instantGrant = locks[lockOffset].insertNewRequest(*(request), grantMessageBuffer);
	DLOG("LockTable", "Lock %d mode %d inserted", lockOffset, (uint32_t) request->mode);

	if (instantGrant) {

		uint64_t transactionId = grantMessageBuffer->transactionNumber;
		transactionLockMap.insert( { transactionId, { locks + lockOffset, grantMessageBuffer->mode } });
#ifdef USE_LOGGING
		grantMessageBuffer->serverSendTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
		communicator->sendMessage(grantMessageBuffer, grantMessageBuffer->clientGlobalRank);

	} else{ // wait
		locksWithPendingRequests.insert(locks + lockOffset);
	}
}

void Table2PL::releaseLock(hdb::messages::LockRelease* release) {

	uint32_t lockOffset = release->lockId % numberOfElements;

	uint64_t transactionId = release->transactionNumber;
	auto range = transactionLockMap.equal_range(transactionId);
	for (auto i = range.first; i != range.second; ++i) {
		if (i->second.first->lockOffset == lockOffset) {
			locks[lockOffset].releaseLock(i->second.second);
			transactionLockMap.erase(i);
			processAndSendNotifications(lockOffset); // trigger pending locks
			return;
		}
	}

}

void Table2PL::releaseTransaction(hdb::messages::TransactionEnd* endTransaction) {

	uint64_t transactionId = endTransaction->transactionNumber;
	auto range = transactionLockMap.equal_range(transactionId);
	uint32_t numberOfAquiredLock = transactionLockMap.count(transactionId);
	aquiredLockCopy.reserve(numberOfAquiredLock);

	for (auto i = range.first; i != range.second; ++i) {
		aquiredLockCopy.push_back(i->second);
	}
	transactionLockMap.erase(transactionId);

	for (uint32_t i = 0; i < numberOfAquiredLock; ++i) {
		aquiredLockCopy[i].first->releaseLock(aquiredLockCopy[i].second);
		processAndSendNotifications(aquiredLockCopy[i].first->lockOffset); // trigger pending locks
	}
	aquiredLockCopy.clear();

}

void Table2PL::abortTransaction(hdb::messages::AbortTransaction *Transaction){
	uint64_t transactionId = Transaction->transactionNumber;
	auto range = transactionLockMap.equal_range(transactionId);
	uint32_t numberOfAquiredLock = transactionLockMap.count(transactionId);
	aquiredLockCopy.reserve(numberOfAquiredLock);

	for (auto i = range.first; i != range.second; ++i) {
		aquiredLockCopy.push_back(i->second);
	}
	transactionLockMap.erase(transactionId);

	for (uint32_t i = 0; i < numberOfAquiredLock; ++i) {
		aquiredLockCopy[i].first->releaseLock(aquiredLockCopy[i].second);
		processAndSendNotifications(aquiredLockCopy[i].first->lockOffset); // trigger pending locks
	}
	aquiredLockCopy.clear();
}

void Table2PL::processAndSendNotifications(uint32_t lockOffset) {

	while (locks[lockOffset].processNextElementFromQueue(grantMessageBuffer)) {

		uint64_t transactionId = grantMessageBuffer->transactionNumber;
		DLOG("LockTable", "Insert into map TX %lu (%d, %d) with %d mode %d", transactionId, grantMessageBuffer->clientGlobalRank,
				grantMessageBuffer->transactionNumber, lockOffset, (uint32_t) grantMessageBuffer->mode);
		transactionLockMap.insert( { transactionId, { locks + lockOffset, grantMessageBuffer->mode } });
#ifdef USE_LOGGING
		grantMessageBuffer->serverSendTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
		communicator->sendMessage(grantMessageBuffer, grantMessageBuffer->clientGlobalRank);

	}

	if (locks[lockOffset].queueHasPendingElements()) {
		locksWithPendingRequests.insert(locks + lockOffset);
	}

}

void Table2PL::handleExpiredRequests() {

	for (auto it = locksWithPendingRequests.begin(); it != locksWithPendingRequests.end(); ++it) {

		while ((*it)->processExpiredElementsFromQueue(grantMessageBuffer)) {

#ifdef USE_LOGGING
			grantMessageBuffer->serverSendTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
			communicator->sendMessage(grantMessageBuffer, grantMessageBuffer->clientGlobalRank);

		}
	}

	for (auto it = locksWithPendingRequests.begin(); it != locksWithPendingRequests.end();) {
		if ((*it)->queueHasPendingElements()) {
			++it;
		} else {
			locksWithPendingRequests.erase(it++);
		}

	}

}



/**********************************************************************************/


TableWaitDie::TableWaitDie(uint64_t numberOfElements, hdb::communication::Communicator *communicator){

	this->numberOfElements = numberOfElements;
	this->locks = new hdb::locktable::LockWaitDie[numberOfElements];
        if(this->locks==NULL){
                 DLOG_ALWAYS("TABLE","Memory allocation error of locks %lu",numberOfElements);
	} else{
	        for (uint64_t i = 0; i < numberOfElements; ++i) {
                     this->locks[i].lockOffset = i;
        	}
	}
	this->communicator = communicator;
	this->grantMessageBuffer = (hdb::messages::LockGrant *) calloc(1, sizeof(hdb::messages::LockGrant));

	this->aquiredLockCopy.reserve(512);
}

TableWaitDie::~TableWaitDie() {

	delete[] locks;

}

void TableWaitDie::insertNewRequest(hdb::messages::LockRequest* request) {

	uint32_t lockOffset = request->lockId % numberOfElements;

   	LockReturn insertstatus = locks[lockOffset].insertNewRequest(*(request), grantMessageBuffer);

	DLOG("LockTable", "Lock %d mode %d inserted", lockOffset, (uint32_t) request->mode);

	if (insertstatus==LockReturn::Granted) {

		uint64_t transactionId = grantMessageBuffer->transactionNumber;
		transactionLockMap.insert( { transactionId, { locks + lockOffset, grantMessageBuffer->mode } });
#ifdef USE_LOGGING
		grantMessageBuffer->serverSendTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
		communicator->sendMessage(grantMessageBuffer, grantMessageBuffer->clientGlobalRank);

	} else if (insertstatus==LockReturn::Expired){

#ifdef USE_LOGGING
		grantMessageBuffer->serverEnqueueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
		grantMessageBuffer->serverSendTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
		communicator->sendMessage(grantMessageBuffer, grantMessageBuffer->clientGlobalRank);

	} else{ // wait
		locksWithPendingRequests.insert(locks + lockOffset);
	}
}

void TableWaitDie::releaseLock(hdb::messages::LockRelease* release) {

	uint32_t lockOffset = release->lockId % numberOfElements;

	uint64_t transactionId = release->transactionNumber;
	auto range = transactionLockMap.equal_range(transactionId);
	for (auto i = range.first; i != range.second; ++i) {
		if (i->second.first->lockOffset == lockOffset) {
			locks[lockOffset].releaseLock(i->second.second);
			transactionLockMap.erase(i);
			processAndSendNotifications(lockOffset); // trigger pending locks
			return;
		}
	}

}

void TableWaitDie::releaseTransaction(hdb::messages::TransactionEnd* endTransaction) {

	uint64_t transactionId = endTransaction->transactionNumber;
	auto range = transactionLockMap.equal_range(transactionId);
	uint32_t numberOfAquiredLock = transactionLockMap.count(transactionId);
	aquiredLockCopy.reserve(numberOfAquiredLock);

	for (auto i = range.first; i != range.second; ++i) {
		aquiredLockCopy.push_back(i->second);
	}
	transactionLockMap.erase(transactionId);

	for (uint32_t i = 0; i < numberOfAquiredLock; ++i) {
		aquiredLockCopy[i].first->releaseLock(aquiredLockCopy[i].second);
		processAndSendNotifications(aquiredLockCopy[i].first->lockOffset); // trigger pending locks
	}
	aquiredLockCopy.clear();
}


void TableWaitDie::abortTransaction(hdb::messages::AbortTransaction *Transaction){

	uint64_t transactionId = Transaction->transactionNumber;
	auto range = transactionLockMap.equal_range(transactionId);
	uint32_t numberOfAquiredLock = transactionLockMap.count(transactionId);
	aquiredLockCopy.reserve(numberOfAquiredLock);

	for (auto i = range.first; i != range.second; ++i) {
		aquiredLockCopy.push_back(i->second);
	}
	transactionLockMap.erase(transactionId);

	for (uint32_t i = 0; i < numberOfAquiredLock; ++i) {
		aquiredLockCopy[i].first->releaseLock(aquiredLockCopy[i].second);
		processAndSendNotifications(aquiredLockCopy[i].first->lockOffset); // trigger pending locks
	}
	aquiredLockCopy.clear();
}

void TableWaitDie::processAndSendNotifications(uint32_t lockOffset) {

	while (locks[lockOffset].processNextElementFromQueue(grantMessageBuffer)) {

		uint64_t transactionId = grantMessageBuffer->transactionNumber;
		DLOG("LockTable", "Insert into map TX %lu (%d, %d) with %d mode %d", transactionId, grantMessageBuffer->clientGlobalRank,
				grantMessageBuffer->transactionNumber, lockOffset, (uint32_t) grantMessageBuffer->mode);
		transactionLockMap.insert( { transactionId, { locks + lockOffset, grantMessageBuffer->mode } });
#ifdef USE_LOGGING
		grantMessageBuffer->serverSendTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
		communicator->sendMessage(grantMessageBuffer, grantMessageBuffer->clientGlobalRank);

	}

	if (locks[lockOffset].queueHasPendingElements()) {
		locksWithPendingRequests.insert(locks + lockOffset);
	}

}

void TableWaitDie::handleExpiredRequests() {

	for (auto it = locksWithPendingRequests.begin(); it != locksWithPendingRequests.end(); ++it) {

		while ((*it)->processExpiredElementsFromQueue(grantMessageBuffer)) {

#ifdef USE_LOGGING
			grantMessageBuffer->serverSendTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
			communicator->sendMessage(grantMessageBuffer, grantMessageBuffer->clientGlobalRank);

		}
	}

	for (auto it = locksWithPendingRequests.begin(); it != locksWithPendingRequests.end();) {
		if ((*it)->queueHasPendingElements()) {
			++it;
		} else {
			locksWithPendingRequests.erase(it++);
		}

	}

}





/**********************************************************************************/


TableNoWait::TableNoWait(uint64_t numberOfElements, hdb::communication::Communicator *communicator){

	this->numberOfElements = numberOfElements;
	this->locks = new hdb::locktable::LockNoWait[numberOfElements];
    if(this->locks==NULL){
                 DLOG_ALWAYS("TABLE","Memory allocation error of locks %lu",numberOfElements);
	} else{
	        for (uint64_t i = 0; i < numberOfElements; ++i) {
                     this->locks[i].lockOffset = i;
        	}
	}
	this->communicator = communicator;
	this->grantMessageBuffer = (hdb::messages::LockGrant *) calloc(1, sizeof(hdb::messages::LockGrant));

	this->aquiredLockCopy.reserve(512);
}

TableNoWait::~TableNoWait() {

	delete[] locks;

}

void TableNoWait::insertNewRequest(hdb::messages::LockRequest* request) {

	uint32_t lockOffset = request->lockId % numberOfElements;


   	bool insertstatus = locks[lockOffset].insertNewRequest(*(request), grantMessageBuffer);

	DLOG("LockTable", "Lock %d mode %d inserted", lockOffset, (uint32_t) request->mode);

	if (insertstatus) {

		uint64_t transactionId = grantMessageBuffer->transactionNumber;
		transactionLockMap.insert( { transactionId, { locks + lockOffset, grantMessageBuffer->mode } });
#ifdef USE_LOGGING
		grantMessageBuffer->serverSendTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
		communicator->sendMessage(grantMessageBuffer, grantMessageBuffer->clientGlobalRank);

	} else {

#ifdef USE_LOGGING
		grantMessageBuffer->serverEnqueueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
		grantMessageBuffer->serverSendTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif
		communicator->sendMessage(grantMessageBuffer, grantMessageBuffer->clientGlobalRank);

	}  
}

void TableNoWait::releaseLock(hdb::messages::LockRelease* release) {

	uint32_t lockOffset = release->lockId % numberOfElements;

	uint64_t transactionId = release->transactionNumber;
	auto range = transactionLockMap.equal_range(transactionId);
	for (auto i = range.first; i != range.second; ++i) {
		if (i->second.first->lockOffset == lockOffset) {
			locks[lockOffset].releaseLock(i->second.second);
			transactionLockMap.erase(i);
			return;
		}
	}

}


void TableNoWait::abortTransaction(hdb::messages::AbortTransaction *Transaction){
	uint64_t transactionId = Transaction->transactionNumber;
	auto range = transactionLockMap.equal_range(transactionId);
	uint32_t numberOfAquiredLock = transactionLockMap.count(transactionId);
	aquiredLockCopy.reserve(numberOfAquiredLock);

	for (auto i = range.first; i != range.second; ++i) {
		aquiredLockCopy.push_back(i->second);
	}
	transactionLockMap.erase(transactionId);

	for (uint32_t i = 0; i < numberOfAquiredLock; ++i) {
		aquiredLockCopy[i].first->releaseLock(aquiredLockCopy[i].second);
	}
	aquiredLockCopy.clear();
}


void TableNoWait::releaseTransaction(hdb::messages::TransactionEnd* endTransaction) {

	uint64_t transactionId = endTransaction->transactionNumber;
	auto range = transactionLockMap.equal_range(transactionId);
	uint32_t numberOfAquiredLock = transactionLockMap.count(transactionId);
	aquiredLockCopy.reserve(numberOfAquiredLock);

	for (auto i = range.first; i != range.second; ++i) {
		aquiredLockCopy.push_back(i->second);
	}
	transactionLockMap.erase(transactionId);

	for (uint32_t i = 0; i < numberOfAquiredLock; ++i) {
		aquiredLockCopy[i].first->releaseLock(aquiredLockCopy[i].second);
	}
	aquiredLockCopy.clear();

}



void TableNoWait::handleExpiredRequests() {
 	// EMPTY
}



/**********************************************************************************/



TableTimestamp::TableTimestamp(uint64_t numberOfElements, hdb::communication::Communicator *communicator){

	this->numberOfElements = numberOfElements;
	this->communicator = communicator;
	this->grantMessageBuffer = (hdb::messages::LockGrant *) calloc(1, sizeof(hdb::messages::LockGrant));
	this->rows = new hdb::locktable::Row[numberOfElements];
	if(this->rows==NULL){
            DLOG_ALWAYS("TableTimestamp","Memory allocation error of locks %lu",numberOfElements);
	}  
}

TableTimestamp::~TableTimestamp() {
	delete[] rows;
}

void TableTimestamp::insertNewRequest(hdb::messages::LockRequest* request) {
	uint32_t rowid = request->lockId % numberOfElements;

#ifdef USE_LOGGING
	request->serverEnqueueTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif

	bool granted = true;


	auto row = &rows[rowid];
   	uint64_t TS = request->transactionNumber;
        if (!LOCK_READ_OR_WRITE(request->mode)) {
                goto sendreply;
        }

    if(request->mode == LockMode::S){
        DLOG("TableTimestamp", "[%d] read", TS);

    	if(TS < row->wts){
                DLOG("TableTimestamp", "[%d] read rejected", TS);
    		granted = false;
    		//instantReject(request, grant);
 			goto sendreply;
			//RowReturn::Reject
    	}
    	if(!row->prewrites.empty() && TS > (*row->prewrites.begin())  ){
                DLOG("TableTimestamp", "[%d] read buffered", TS);
    		row->reads.insert(TS);
    		assert( row->pendingreq.find(TS) == row->pendingreq.end() && "READ TS was already in pending requests");
    		row->pendingreq.insert( {TS, *request });
    		//transactionLockMap.insert( { TS, row }); i think we don't need as we can commit/abort only after all replies received
    		return  ; //RowReturn::Wait
    	}
        DLOG("TableTimestamp", "[%d] read granted", TS);
    	row->rts = std::max(row->rts, TS);
    	//instantGrant(request, grant);
    	goto sendreply;
		//RowReturn::Success
    }
    else{ // for writes
        DLOG("TableTimestamp", "[%d] write", TS)
    	if(TS < row->wts ||  TS < row->rts ){
                DLOG("TableTimestamp", "[%d] write rejected", TS);
    		granted = false;
    		goto sendreply;
			//return RowReturn::Reject;
    	}
               DLOG("TableTimestamp", "[%d] prewrite granted", TS);
  		transactionLockMap.insert( { TS, row });
		row->prewrites.insert(TS);
  		goto sendreply;
		// RowReturn::Success;
    }

sendreply:
    DLOG("TableTimestamp", "[%d] send reply", TS);
	new (grantMessageBuffer) hdb::messages::LockGrant(request->clientGlobalRank, TS, request->lockId, request->mode,granted);
	#ifdef USE_LOGGING
		grantMessageBuffer->clientSendTime 	 = request->clientSendTime;
		grantMessageBuffer->serverReceiveTime = request->serverReceiveTime;
		grantMessageBuffer->serverSendTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
		grantMessageBuffer->serverEnqueueTime = request->serverEnqueueTime;
		grantMessageBuffer->serverDequeueTime = request->serverEnqueueTime;
	#endif
	communicator->sendMessage(grantMessageBuffer, grantMessageBuffer->clientGlobalRank);
}

void TableTimestamp::releaseLock(hdb::messages::LockRelease* release) {
 // EMPTY

	 assert(0 && "Unused"); // I think it is unused
}



// abort
void TableTimestamp::abortTransaction(hdb::messages::AbortTransaction *Transaction){
       
	uint64_t transactionId = Transaction->transactionNumber;
	auto range = transactionLockMap.equal_range(transactionId);
	uint32_t numberOfAquiredLock = transactionLockMap.count(transactionId);
	aquiredLockCopy.reserve(numberOfAquiredLock);

        DLOG("TableTimestamp", "[%d] abort", transactionId);

	for (auto i = range.first; i != range.second; ++i) {
		aquiredLockCopy.push_back(i->second);
	}
	transactionLockMap.erase(transactionId);

	for (uint32_t i = 0; i < numberOfAquiredLock; ++i) {
		auto row = aquiredLockCopy[i];
		uint64_t TS = transactionId;
		
/*
		// the imposible scenario. If we abort, we know that our read was rejected or granted
		size_t removed = row->reads.erase(TS);
		if(removed){
			row->pendingreq.erase(TS);
		}*/
		assert(!row->prewrites.empty());
		//if(!row->prewrites.empty()){
		uint64_t minpts = *row->prewrites.begin();
		size_t removed = row->prewrites.erase(TS);
		assert(removed != 0 && "The prewrite was not in the prewrite queue");
		
		trigger(minpts,row);
		//} 

	}

	aquiredLockCopy.clear();

}

// commit
void TableTimestamp::releaseTransaction(hdb::messages::TransactionEnd* endTransaction) {
  
	uint64_t transactionId = endTransaction->transactionNumber;
	auto range = transactionLockMap.equal_range(transactionId);
	uint32_t numberOfAquiredLock = transactionLockMap.count(transactionId);
	aquiredLockCopy.reserve(numberOfAquiredLock);
        DLOG("TableTimestamp", "[%d] commit", transactionId);

	for (auto i = range.first; i != range.second; ++i) {
		aquiredLockCopy.push_back(i->second);
	}
	transactionLockMap.erase(transactionId);

	for (uint32_t i = 0; i < numberOfAquiredLock; ++i) {

		auto row = aquiredLockCopy[i];
		uint64_t TS = transactionId;

		// prewrites cannot be empty as we try to finish write
	  	uint64_t minpts = *row->prewrites.begin();

		if(TS > minpts ){
			row->writes.insert(TS);
			continue ;// return RowReturn::Wait;
		}
		if(!row->reads.empty() && TS > (*row->reads.begin())  ){
			row->writes.insert(TS);
			continue;// return RowReturn::Wait;
		}
	    
		size_t removed = row->prewrites.erase(TS);
		assert(removed && "The TS must be in prewrites");
		// update write timestamp;
        row->wts = std::max(row->wts,TS);
 
		trigger(minpts,row);
	}

	aquiredLockCopy.clear();
}

void TableTimestamp::trigger(uint64_t oldminpts,hdb::locktable::Row* row) {
    // check whether minpts has been updated
        DLOG("TableTimestamp", "Trigger a buffered requests");
    uint64_t currentminpts = row->prewrites.empty() ?  std::numeric_limits<uint64_t>::max()  : *row->prewrites.begin();

	while(oldminpts != currentminpts){

		assert( oldminpts < currentminpts && "minpts must be smaller");

		oldminpts = currentminpts;
		//minpts = *row->prewrites.begin();
		//trigger reads
		for (auto it = row->reads.begin(); it != row->reads.end();  ){
			if(*it > currentminpts){
				break;
			}else{
                DLOG("TableTimestamp", "[%lu] Read has been  triggered", *it);
				row->rts = std::max(row->rts,*it);

				hdb::messages::LockRequest request = row->pendingreq[*it];
				new (grantMessageBuffer) hdb::messages::LockGrant(request.clientGlobalRank, *it, request.lockId, request.mode, true);
				#ifdef USE_LOGGING
					grantMessageBuffer->clientSendTime 	 = request.clientSendTime;
					grantMessageBuffer->serverReceiveTime = request.serverReceiveTime;
					grantMessageBuffer->serverSendTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
					grantMessageBuffer->serverEnqueueTime = request.serverEnqueueTime;
					grantMessageBuffer->serverDequeueTime = grantMessageBuffer->serverSendTime;
				#endif
				communicator->sendMessage(grantMessageBuffer, grantMessageBuffer->clientGlobalRank);
                       
				row->pendingreq.erase(*it);
				it = row->reads.erase(it);
			}
		}

		uint64_t minrts = row->reads.empty() ?  std::numeric_limits<uint64_t>::max()  : *row->reads.begin();

		//trigger writes
		for (auto it = row->writes.begin(); it != row->writes.end();  ){
			if(*it > currentminpts || *it > minrts){
				break;
			}else{
				size_t removed = row->prewrites.erase(*it);
				assert(removed && "The TS must be in prewrites");
				row->wts = std::max(row->wts,*it);
				  DLOG("TableTimestamp", "[%lu] Write has been  triggered", *it);

				it = row->writes.erase(it);
			}
		}

		currentminpts = row->prewrites.empty() ?  std::numeric_limits<uint64_t>::max()  : *row->prewrites.begin();
	} // end of while loop 
 
}
 

void TableTimestamp::handleExpiredRequests() {
 	// EMPTY
}



} /* namespace locktable */
} /* namespace hdb */

