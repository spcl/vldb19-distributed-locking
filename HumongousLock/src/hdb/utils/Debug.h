/*
 * Debug.h
 *
 *  Created on: Apr 19, 2018
 *      Author: claudeb
 */

#ifndef SRC_HDB_UTILS_DEBUG_H_
#define SRC_HDB_UTILS_DEBUG_H_

#include <stdio.h>

#ifdef DEBUG 
	#define DLOG(A, B, ...) { \
		fprintf(stdout, "["); \
		fprintf(stdout, A); \
		fprintf(stdout, "] "); \
		fprintf(stdout, B, ##__VA_ARGS__); \
		fprintf(stdout, "\n"); \
		fflush(stdout); \
	}
#else
	#define DLOG(A, B, ...) {}
#endif

#define DLOG_ALWAYS(A, B, ...) { \
		fprintf(stdout, "["); \
		fprintf(stdout, A); \
		fprintf(stdout, "] "); \
		fprintf(stdout, B, ##__VA_ARGS__); \
		fprintf(stdout, "\n"); \
		fflush(stdout); \
}

#endif /* SRC_HDB_UTILS_DEBUG_H_ */
