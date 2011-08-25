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


#ifdef FREEBSD_SYS
static char *semname = "/tmp/semaphoretest";
#else
static char *semname = "/tmpsemaphoretest";
#endif
static marcel_barrier_t b;
static marcel_sem_t *father_s;


static void *thread_func(void *arg TBX_UNUSED)
{
	marcel_sem_t *s;

	s = marcel_sem_open(semname, 0);
	if (! s) {
		puts("child: init failed");
		exit(1);
	}

	if (s != father_s) {
		puts("child: same semaphore name but memory regions are different");
		exit(1);
	}

	/** 2-way sync **/
	marcel_barrier_wait(&b);
	if (marcel_sem_post(s) == -1) {
		puts("child: post failed");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	marcel_printf("child: sync 1: done \n");
#endif

	marcel_barrier_wait(&b);
	if (marcel_sem_wait(s) == -1) {
		puts("child: wait failed");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	marcel_printf("child: sync 2: done \n");
#endif

	marcel_sem_close(s);
	return NULL;
}

static int do_test(void)
{
	marcel_t t;

	marcel_barrier_init(&b, NULL, 2);

	marcel_sem_unlink(semname);
 	father_s = marcel_sem_open(semname, O_CREAT, S_IWUSR | S_IRUSR, 0);
	if (! father_s) {
		puts("father: init failed");
		return 1;
	}

	marcel_create(&t, NULL, thread_func, NULL);

	/** 2-way sync **/
	marcel_barrier_wait(&b);
	if (marcel_sem_wait(father_s) == -1) {
		puts("father: wait failed");
		return 1;
	}
#ifdef VERBOSE_TESTS
	marcel_printf("father: sync 1: done \n");
#endif

	marcel_barrier_wait(&b);
	if (marcel_sem_post(father_s) == -1) {
		puts("father: post failed");
		return 1;
	}
#ifdef VERBOSE_TESTS
	marcel_printf("father: sync 2: done \n");
#endif

	marcel_join(t, NULL);
	marcel_sem_close(father_s);
	marcel_sem_unlink(semname);

	return MARCEL_TEST_SUCCEEDED;
}


int marcel_main(int argc, char *argv[])
{
	int result;

	marcel_init(&argc, argv);
	result = do_test();
	marcel_end();

	return result;
}
