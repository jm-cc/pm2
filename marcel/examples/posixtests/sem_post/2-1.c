/*
    Copyright (c) 2003, Intel Corporation. All rights reserved.
    Created by:  majid.awad REMOVE-THIS AT intel DOT com
    This file is licensed under the GPL license.  For the full content 
    of this license, see the COPYING file at the top level of this 
    source tree.
 */

/*
 * This test case verifies the semaphore value is 0, then shows a successful
 * call to release the unlock from mysemp.
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "posixtest.h"


#define TEST "2-1"
#define FUNCTION "sem_post"
#define ERROR_PREFIX "unexpected error: " FUNCTION " " TEST ": "



int main() {
	sem_t *mysemp;
	int ret;

	/* Initial value of Semaphore is 0 */
	ret = sem_init(mysemp,0,1);
	if( ret != 0 || mysemp == NULL ) {
		perror(ERROR_PREFIX "sem_init");
		return PTS_UNRESOLVED;
	}

	if( sem_wait(mysemp) == -1 ) {
		perror(ERROR_PREFIX "sem_post");
		return PTS_UNRESOLVED; 
	}

	if((  sem_post(mysemp)) == 0 ) {
		puts("TEST PASSED");
		sem_destroy(mysemp);
		return PTS_PASS;
	} else { 
		puts("TEST FAILED: value of sem_post is not zero");
		return PTS_FAIL;
	}

}

