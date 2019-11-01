/*
 * LockMode.h
 *
 *  Created on: Mar 16, 2017
 *      Author: claudeb
 */

#ifndef HDB_LOCKTABLE_LOCKMODE_H_
#define HDB_LOCKTABLE_LOCKMODE_H_

#include <string.h>

namespace hdb {
namespace locktable {

enum LockMode {
	NL = 0, IS = 1, IX = 2, S = 3, SIX = 4 , X = 5, MODE_COUNT = 6
};

static const bool COMPATIBILITY_MATRIX [LockMode::MODE_COUNT][LockMode::MODE_COUNT] =
	{
			{true, 	true, 	true, 	true, 	true, 	true},
			{true, 	true, 	true, 	true, 	true, 	false},
			{true, 	true, 	true, 	false, 	false, 	false},
			{true, 	true, 	false, 	true, 	false, 	false},
			{true, 	true, 	false, 	false, 	false, 	false},
			{true, 	false, 	false, 	false, 	false, 	false}
	};

static const bool LOCK_MODE_COMPATIBLE(LockMode a, LockMode b) {
	return COMPATIBILITY_MATRIX[a][b];
}

static const bool LOCK_READ_OR_WRITE(LockMode a) {
	return a == LockMode::S || a == LockMode::X;
}

static const bool WRITE_MATRIX [LockMode::MODE_COUNT] =
			{false, false, false, false, false, true};
		
 
static const bool WRITE_LOCK_MODE(LockMode a) {
	return WRITE_MATRIX[a];
}


static LockMode LOCK_MODE_FROM_STRING(char *lockMode) {

	if(strcmp(lockMode, "IS") == 0) {
		return LockMode::IS;
	} else if(strcmp(lockMode, "IX") == 0) {
		return LockMode::IX;
	} else if(strcmp(lockMode, "S") == 0) {
		return LockMode::S;
	} else if(strcmp(lockMode, "SIX") == 0) {
		return LockMode::SIX;
	} else if(strcmp(lockMode, "X") == 0) {
		return LockMode::X;
	} else {
		return LockMode::NL;
	}

}

} /* namespace locktable */
} /* namespace hdb */

#endif /* HDB_LOCKTABLE_LOCKMODE_H_ */
