/*
 * VoteRequest.cpp
 *
 *  Created on: Apr 17, 2018
 *      Author: claudeb
 */

#include "VoteRequest.h"

namespace hdb {
namespace messages {

VoteRequest::VoteRequest(uint32_t clientGlobalRank, uint64_t transactionNumber) : Message(VOTE_REQUEST_MESSAGE, sizeof(VoteRequest), clientGlobalRank, transactionNumber) {

}

VoteRequest::~VoteRequest() {

}

} /* namespace messages */
} /* namespace hdb */
