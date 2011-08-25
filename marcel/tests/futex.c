
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2009 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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

/* futex.c */


#include "marcel.h"


#define THREADS 25
#define ITER_MAX 100000


typedef struct {
	unsigned long held;
	marcel_futex_t futex;
} lock_t;

static lock_t mylock = {.held = 0,.futex = MARCEL_FUTEX_INITIALIZER };
static int tot_retries;
static int val;


static void lock(lock_t * l)
{
	int retries = 0;
	while (__sync_lock_test_and_set(&l->held, 1)) {
		/* Already held, sleep */
		marcel_futex_wait(&l->futex, &l->held, 1, 1, MARCEL_MAX_SCHEDULE_TIMEOUT);
		retries++;
	}
	tot_retries += retries;
}

static void unlock(lock_t * l)
{
	l->held = 0;
	marcel_futex_wake(&l->futex, 1);
}

static void *f(void *arg)
{
	int n = ITER_MAX;

	while (n--) {
		lock(&mylock);
		val++;
		unlock(&mylock);
	}

	return arg;
}

int marcel_main(int argc, char *argv[])
{
	int i;

	marcel_init(&argc, argv);

	for (i = 0; i < THREADS; i++)
		marcel_create(NULL, NULL, f, NULL);

	marcel_end();

#ifdef VERBOSE_TESTS
	printf("%d total retries\n", tot_retries);
#endif

	if (val != THREADS*ITER_MAX) {
		printf("%d, should be %d\n", val, THREADS*ITER_MAX);
		exit(1);
	}

	return MARCEL_TEST_SUCCEEDED;
}
