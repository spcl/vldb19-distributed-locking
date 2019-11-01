/*
 * SendReceiveCommunicator.h
 *
 *  Created on: Apr 30, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_COMMUNICATION_SENDRECEIVECOMMUNICATOR_H_
#define SRC_HDB_COMMUNICATION_SENDRECEIVECOMMUNICATOR_H_

#include <hdb/messages/Message.h>
#include <hdb/configuration/SystemConfig.h>

namespace hdb {
namespace communication {

class SendReceiveCommunicator {

public:

	SendReceiveCommunicator(hdb::configuration::SystemConfig *config);
	virtual ~SendReceiveCommunicator();

public:
	const uint32_t comdelay;
	bool sendMessage(hdb::messages::Message *message, uint32_t targetGlobalRank);
	hdb::messages::Message * getMessage();
	hdb::messages::Message * getMessageBlocking();

protected:

	void** receiveBuffer;

	uint32_t globalMachineId;
	uint32_t localMachineId;
	uint32_t numberOfProcessesPerMachine;

	uint32_t Slots;
    MPI_Request *requests;

};

} /* namespace communication */
} /* namespace hdb */

#endif /* SRC_HDB_COMMUNICATION_SENDRECEIVECOMMUNICATOR_H_ */
