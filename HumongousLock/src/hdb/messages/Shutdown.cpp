/*
 * Shutdown.cpp
 *
 *  Created on: Apr 17, 2018
 *      Author: claudeb
 */

#include <hdb/messages/Shutdown.h>

namespace hdb {
namespace messages {

Shutdown::Shutdown(uint32_t clientGlobalRank) : Message(SHUTDOWN_MESSAGE, sizeof(Shutdown), clientGlobalRank, 0) {

}

Shutdown::~Shutdown() {

}

} /* namespace messages */
} /* namespace hdb */
