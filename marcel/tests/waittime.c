/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */


#include "marcel.h"


#define SLEEP_TIME 1
#define SEC_TO_USEC(t) ((t) * 1000000)
#define SEC_TO_NSEC(t) ((t) * 1000000000)


#ifdef VERBOSE_TESTS
#  define PRINT_TESTNAME(name) marcel_printf("%s\n", name);
#else
#  define PRINT_TESTNAME(name)
#endif


// SLEEP_TIME = 1s => measure error = 1ms
#define CHECK_DELAY(start, end, reftime_sec) (TBX_TIMING_DELAY(start, end) < ((double)(999*reftime_sec))/1000)


#ifdef MARCEL_SIGNALS_ENABLED
static void alarm_handler(int sig TBX_UNUSED)
{
}
#endif


static int do_test(void)
{
	tbx_tick_t start, end;
	unsigned int intr;
	struct timespec wait;

	// sleep test
	PRINT_TESTNAME("marcel_sleep")
	TBX_GET_TICK(start);
	intr = marcel_sleep(SLEEP_TIME);
	TBX_GET_TICK(end);
	if ((0 == intr) && CHECK_DELAY(start, end, SLEEP_TIME)) {
		puts("marcel_sleep returns too early");
		return -1;
	}

	// usleep test
	PRINT_TESTNAME("marcel_usleep")
	TBX_GET_TICK(start);
	intr = marcel_usleep(SEC_TO_USEC(SLEEP_TIME));
	TBX_GET_TICK(end);
	if ((0 == intr) && CHECK_DELAY(start, end, SLEEP_TIME)) {
		puts("marcel_usleep returns too early");
		return -1;
	}

	// nanosleep test
	PRINT_TESTNAME("marcel_nanosleep")
	TBX_GET_TICK(start);
	wait.tv_sec = SLEEP_TIME;
	wait.tv_nsec = 0;
	intr = marcel_nanosleep(&wait, NULL);
	TBX_GET_TICK(end);
	if ((0 == intr) && CHECK_DELAY(start, end, SLEEP_TIME)) {
		puts("marcel_nanosleep returns too early");
		return -1;
	}

	// sem_timedwait test
	marcel_sem_t s;
	struct timeval tv;

	marcel_sem_init(&s, 0);

	PRINT_TESTNAME("marcel_sem_timedwait")
	TBX_GET_TICK(start);
	gettimeofday(&tv, NULL);
	wait.tv_sec = tv.tv_sec + 1;
	wait.tv_nsec = tv.tv_usec * 1000;
	marcel_sem_timedwait(&s, &wait);
	TBX_GET_TICK(end);
	if (CHECK_DELAY(start, end, SLEEP_TIME)) {
		puts("marcel_sem_timedwait returns too early");
		return -1;
	}

	// condtimedwait test
	marcel_cond_t c;
	marcel_mutex_t cm;

	marcel_cond_init(&c, NULL);
	marcel_mutex_init(&cm, NULL);
	marcel_mutex_lock(&cm);
	
	PRINT_TESTNAME("marcel_cond_timedwait")
	TBX_GET_TICK(start);
	gettimeofday(&tv, NULL);
	wait.tv_sec = tv.tv_sec + 1;
	wait.tv_nsec = tv.tv_usec * 1000;
	marcel_cond_timedwait(&c, &cm, &wait);
	TBX_GET_TICK(end);
	if (CHECK_DELAY(start, end, SLEEP_TIME)) {
		puts("marcel_cond_timedwait returns too early");
		return -1;
	}

#ifdef MARCEL_POSIX
	// mutex_timedlock test
	pmarcel_mutex_t m;

	pmarcel_mutex_init(&m, NULL);
	pmarcel_mutex_lock(&m);

	PRINT_TESTNAME("pmarcel_mutex_timedlock")
	TBX_GET_TICK(start);
	gettimeofday(&tv, NULL);
	wait.tv_sec = tv.tv_sec + 1;
	wait.tv_nsec = tv.tv_usec * 1000;
	pmarcel_mutex_timedlock(&m, &wait);
	TBX_GET_TICK(end);
	if (CHECK_DELAY(start, end, SLEEP_TIME)) {
		puts("pmarcel_mutex_timedlock returns too early");
		return -1;
	}
#endif

#ifdef MARCEL_SIGNALS_ENABLED
	// alarm test
	struct marcel_sigaction sa;

	sa.marcel_sa_flags = 0;
	sa.marcel_sa_handler = alarm_handler;
	marcel_sigemptyset(&sa.marcel_sa_mask);
	marcel_sigaction(SIGALRM, &sa, NULL);

	PRINT_TESTNAME("marcel_alarm")
	TBX_GET_TICK(start);
	marcel_alarm(SLEEP_TIME);
	marcel_sleep(2*SLEEP_TIME);
	TBX_GET_TICK(end);
	if (CHECK_DELAY(start, end, SLEEP_TIME)) {
		puts("marcel_alarm sends SIGALRM too early");
		return -1;
	}

	// sigtimedwait test
	marcel_sigset_t set;

	wait.tv_sec = SLEEP_TIME;
	wait.tv_nsec = 0;
	marcel_sigemptyset(&set);
	marcel_sigaddset(&set, SIGUSR1);

	PRINT_TESTNAME("marcel_sigtimedwait")
	TBX_GET_TICK(start);
	marcel_sigtimedwait(&set, NULL, &wait);
	TBX_GET_TICK(end);
	if (CHECK_DELAY(start, end, SLEEP_TIME)) {
		puts("marcel_sigtimedwait returns too early");
		return -1;
	}

#endif

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
