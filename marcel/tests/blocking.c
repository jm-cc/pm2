
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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>


#if defined(SYS_nanosleep) && !defined(MARCEL_LIBPTHREAD) && defined(MA__LWPS) && defined(MARCEL_BLOCKING_ENABLED) && defined(MARCEL_DEVIATION_ENABLED)
#define N 100


static int is_valid_timespec(struct timespec *ts)
{
	/** On Darwin, nanosleep can fill ts with junk values **/
	if ((int32_t)ts->tv_sec >= 0 && (int32_t)ts->tv_nsec >= 0 && ts->tv_nsec <= 999999999)
		return 1;

	return 0;
}


static any_t loop(any_t arg TBX_UNUSED)
{
	unsigned long start;
	while (1) {
#ifdef VERBOSE_TESTS
		marcel_printf("busy looping 1s\n");
#endif
		start = marcel_clock();
		while (marcel_clock() < start + 1000);
	}
	return NULL;
}

static any_t block(any_t arg)
{
	struct timespec ts;
	int n = (int) (intptr_t) arg;
	char name[16];
	int  ret;

	marcel_getname(marcel_self(), name, 16);
	while (n--) {
#ifdef VERBOSE_TESTS
		marcel_fprintf(stderr, "%s sleeping (VP %d)\n", name,
			       marcel_current_vp());
#endif
		marcel_enter_blocking_section();
#ifdef VERBOSE_TESTS
		marcel_fprintf(stderr, "%s really sleeping (VP %d)\n",
			       name, marcel_current_vp());
#endif
		ts.tv_sec = 0;
		ts.tv_nsec = 10000;

		/* darwin: update structure can be invalid for the next call of nanosleep 
		*  check ts before calling nanosleep */
		do {
			ret = syscall(SYS_nanosleep, &ts, &ts);
		} while (0 != ret && is_valid_timespec(&ts));

#ifdef VERBOSE_TESTS
		marcel_fprintf(stderr, "%s really slept (VP %d)\n", name,
			       marcel_current_vp());
#endif
		marcel_leave_blocking_section();
#ifdef VERBOSE_TESTS
		marcel_fprintf(stderr, "%s slept (VP %d)\n", name, marcel_current_vp());
#endif
	}
#ifdef VERBOSE_TESTS
	marcel_fprintf(stderr, "%s finished sleeping\n", name);
#endif
	return NULL;
}


int marcel_main(int argc, char **argv)
{
	marcel_attr_t attr;
	marcel_t loopt;
	marcel_t blockt[N];
	int n = 3, i;
	char blockname[] = "block/XXX";

	marcel_init(&argc, argv);

	if (argc > 1)
		n = atoi(argv[2]);

	marcel_setname(marcel_self(), "main");
	marcel_attr_init(&attr);
	marcel_attr_setname(&attr, "loop");
	marcel_attr_setdetachstate(&attr, tbx_true);
	marcel_create(&loopt, &attr, loop, NULL);
	marcel_attr_setdetachstate(&attr, tbx_false);
	for (i = 0; i < N; i++) {
		marcel_snprintf(blockname, sizeof(blockname), "block/%d", i);
		marcel_attr_setname(&attr, blockname);
		marcel_create(&blockt[i], &attr, block, (any_t) (intptr_t) n);
	}
	for (i = 0; i < N; i++) {
		marcel_join(blockt[i], NULL);
#ifdef VERBOSE_TESTS
		marcel_fprintf(stderr, "%d joined\n", i);
#endif
	}

#ifdef VERBOSE_TESTS
	marcel_fprintf(stderr, "cancelling loop thread\n");
#endif
	marcel_cancel(loopt);
	marcel_end();

	return MARCEL_TEST_SUCCEEDED;
}


#else


int marcel_main(int argc TBX_UNUSED, char **argv TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
