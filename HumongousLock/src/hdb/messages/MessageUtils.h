/*
 * MessageUtils.h
 *
 *  Created on: Mar 27, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_MESSAGES_MESSAGEUTILS_H_
#define SRC_HDB_MESSAGES_MESSAGEUTILS_H_

#include <hdb/messages/LockRequest.h>
#include <hdb/messages/LockGrant.h>
#include <hdb/messages/LockRelease.h>
#include <hdb/messages/VoteRequest.h>
#include <hdb/messages/TransactionEnd.h>
#include <hdb/messages/Shutdown.h>

namespace hdb {
namespace messages {

#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define MAX_MESSAGE_SIZE (MAX(																		\
							sizeof(hdb::messages::LockRequest),										\
							MAX (																	\
								sizeof(hdb::messages::LockGrant),									\
								MAX (																\
									sizeof(hdb::messages::LockRelease),								\
									MAX (															\
										sizeof(hdb::messages::VoteRequest),							\
										MAX (														\
											sizeof(hdb::messages::TransactionEnd),					\
											sizeof(hdb::messages::Shutdown)							\
											)														\
										)															\
									)																\
								)																	\
							)																		\
						)

} /* namespace messages */
} /* namespace hdb */

#endif /* SRC_HDB_MESSAGES_MESSAGEUTILS_H_ */
