/*
 * Shutdown.h
 *
 *  Created on: Apr 17, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_MESSAGES_SHUTDOWN_H_
#define SRC_HDB_MESSAGES_SHUTDOWN_H_

#include <stdint.h>

#include <hdb/messages/Message.h>

namespace hdb {
namespace messages {

class Shutdown : public Message {

public:

	Shutdown(uint32_t clientGlobalRank);
	virtual ~Shutdown();
};

} /* namespace messages */
} /* namespace hdb */

#endif /* SRC_HDB_MESSAGES_SHUTDOWN_H_ */
