/*
 * Communicator.cpp
 *
 *  Created on: Apr 17, 2018
 *      Author: claudeb
 */

#include <hdb/communication/Communicator.h>

namespace hdb {
namespace communication {

Communicator::Communicator(hdb::configuration::SystemConfig* config) {
	if (config->syntheticmode){
		dataLayer = nullptr;
	} else {
		dataLayer = new hdb::communication::DataLayer(config);
	}
	voteCommunictor = new hdb::communication::VoteCommunicator(config);
#ifdef USE_FOMPI
	notifiedCommunicator = new hdb::communication::NotifiedCommunicator(config);
#else
	notifiedCommunicator = new hdb::communication::SendReceiveCommunicator(config);
#endif

}

Communicator::~Communicator() {

	delete notifiedCommunicator;
	delete voteCommunictor;

}

bool Communicator::sendMessage(hdb::messages::Message* message, uint32_t targetRank) {

	return notifiedCommunicator->sendMessage(message, targetRank);

}

hdb::messages::Message* Communicator::getMessage() {

	return notifiedCommunicator->getMessage();
}

hdb::messages::Message* Communicator::getMessageBlocking() {
	return notifiedCommunicator->getMessageBlocking();
}

void Communicator::vote(uint32_t targetRank, bool outcome) {
	voteCommunictor->vote(targetRank, outcome);
}

bool Communicator::checkVoteReady(bool* outcome, uint32_t targetCount) {
	bool ready = voteCommunictor->checkVoteReady(outcome, targetCount);
	if(ready) {
		voteCommunictor->reset();
	}
	return ready;
}

void Communicator::signalTransactionEnd(uint32_t targetRank) {
	vote(targetRank, true);
}



void Communicator::waitForTransactionEndSignals(uint32_t targetCount) {
	bool outcome = false;
	while(!voteCommunictor->checkVoteReady(&outcome, targetCount));
	voteCommunictor->reset();
}

} /* namespace communication */
} /* namespace hdb */
