/*
    Copyright (c) 2003, Intel Corporation. All rights reserved.
    Created by:  majid.awad REMOVE-THIS AT intel DOT com
    This file is licensed under the GPL license.  For the full content 
    of this license, see the COPYING file at the top level of this 
    source tree.
 */

/*
 * sem_post shall unlock the locked semaphore and increment the semaphore
 * value by one.
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "posixtest.h"


#define TEST "1-1"
#define FUNCTION "sem_post"
#define ERROR_PREFIX "unexpected error: " FUNCTION " " TEST ": "



int main() {
	sem_t *mysemp;
	int val;
   int ret;

	/* Initial value of Semaphore is 0 */
	ret = sem_init(mysemp,0,0);
	if( ret != 0 || mysemp == NULL ) {
		perror(ERROR_PREFIX "sem_init");
		return PTS_UNRESOLVED;
	}


	if( sem_post(mysemp) == -1 ) {
		perror(ERROR_PREFIX "sem_post");
		return PTS_UNRESOLVED; 
	}


	if( sem_getvalue(mysemp, &val) == -1 ) {
		perror(ERROR_PREFIX "sem_getvalue");
		return PTS_UNRESOLVED;

	/* Checking if the value of the Semaphore incremented by one */
	} else if( val == 1 ) {
		puts("TEST PASSED");
		sem_destroy(mysemp);
		return PTS_PASS;
	} else { 
		puts("TEST FAILED");
		return PTS_FAIL;
	}
}

