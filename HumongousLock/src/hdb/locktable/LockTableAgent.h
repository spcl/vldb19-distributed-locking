/*
 * LockTableAgent.h
 *
 *  Created on: Mar 27, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_LOCKTABLE_LOCKTABLEAGENT_H_
#define SRC_HDB_LOCKTABLE_LOCKTABLEAGENT_H_

#include <stdint.h>

#include <hdb/configuration/SystemConfig.h>
#include <hdb/communication/Communicator.h>
#include <hdb/locktable/Table.h>
#include <hdb/messages/Message.h>
#include <hdb/messages/VoteRequest.h>

namespace hdb {
namespace locktable {

class LockTableAgent {

public:

	LockTableAgent(hdb::configuration::SystemConfig *config);
	virtual ~LockTableAgent();

public:

	void execute();

protected:

	hdb::configuration::SystemConfig *config;
	hdb::communication::Communicator *communicator;

protected:

	hdb::locktable::Table *table;

protected:

	void processMessage(hdb::messages::Message *message);

	uint32_t remainingClients;

};

} /* namespace locktable */
} /* namespace hdb */

#endif /* SRC_HDB_LOCKTABLE_LOCKTABLEAGENT_H_ */
