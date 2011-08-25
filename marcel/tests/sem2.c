/*
 * Copyright (c) 2004, Intel Corporation. All rights reserved.
 * This file is licensed under the GPL license.  For the full content 
 * of this license, see the COPYING file at the top level of this 
 * source tree.
 *
 * adam.li@intel.com
 * If the Timers option is supported, the timeout shall be based on 
 * the CLOCK_REALTIME clock. If the Timers option is not supported, 
 * the timeout shall be based on the system clock as returned by 
 * the time() function.
 * 
 */


#include "marcel.h"


#ifdef MARCEL_LIBPTHREAD


#define SLEEP_SEC 1
#ifdef WITH_TIME
#undef CLOCK_REALTIME
#endif


static int do_test(void)
{
	sem_t mysemp;
	struct timespec ts, ts_2;
	int rc;

	/* Init the value to 0 */
        if ( sem_init (&mysemp, 0, 0) == -1 ) {
                perror("sem_init");
                return -1;
        }

	/* Set the abs timeout */
#ifdef CLOCK_REALTIME
	if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
	{
		perror("clock_gettime");
		return -1;
	}
	ts.tv_sec += SLEEP_SEC;	
        ts.tv_nsec=0;
#else
	ts.tv_sec=time(NULL);
	ts.tv_sec += SLEEP_SEC;	
        ts.tv_nsec=0;
#endif
	/* Lock Semaphore */
	rc = sem_timedwait(&mysemp, &ts);
        if ( rc != -1 || (rc == -1 && errno != ETIMEDOUT)) 
	{
		perror("sem_timedwait");
		return -1; 
	}

	/* Check the time */
#ifdef CLOCK_REALTIME
	if (clock_gettime(CLOCK_REALTIME, &ts_2) != 0)
	{
		perror("clock_gettime");
		return -1;
	}
#else
	ts_2.tv_sec = time(NULL);
#endif
	if(ts_2.tv_sec == ts.tv_sec) {
		sem_destroy(&mysemp);
		return MARCEL_TEST_SUCCEEDED;
	}

	return -1;
}


int marcel_main(int argc, char *argv[])
{
	int result;

	marcel_init(&argc, argv);
	result = do_test();
	marcel_end();

	return result;
}


#else


int marcel_main(int argc TBX_UNUSED, char *argv[] TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
