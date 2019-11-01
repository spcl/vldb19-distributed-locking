/*
 * Communicator.h
 *
 *  Created on: Apr 17, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_COMMUNICATION_COMMUNICATOR_H_
#define SRC_HDB_COMMUNICATION_COMMUNICATOR_H_

#include <hdb/configuration/SystemConfig.h>
#include <hdb/communication/NotifiedCommunicator.h>
#include <hdb/communication/HybridCommunicator.h>
#include <hdb/communication/VoteCommunicator.h>
#include <hdb/communication/DataLayer.h>
#include <hdb/communication/SendReceiveCommunicator.h>

namespace hdb {
namespace communication {

class Communicator {

public:

	Communicator(hdb::configuration::SystemConfig *config);
	virtual ~Communicator();

public:

	hdb::communication::DataLayer *dataLayer;

protected:
#ifdef USE_FOMPI
	hdb::communication::NotifiedCommunicator *notifiedCommunicator;
	//hdb::communication::HybridCommunicator *notifiedCommunicator;
#else
	hdb::communication::SendReceiveCommunicator *notifiedCommunicator;
#endif
	hdb::communication::VoteCommunicator *voteCommunictor;

public:

	bool sendMessage(hdb::messages::Message *message, uint32_t targetRank);
	hdb::messages::Message * getMessage();
	hdb::messages::Message * getMessageBlocking();

public:

	void vote(uint32_t targetRank, bool outcome);
	bool checkVoteReady(bool *outcome, uint32_t targetCount);

public:

	void signalTransactionEnd(uint32_t targetRank);
	void waitForTransactionEndSignals(uint32_t targetCount);

};

} /* namespace communication */
} /* namespace hdb */

#endif /* SRC_HDB_COMMUNICATION_COMMUNICATOR_H_ */
