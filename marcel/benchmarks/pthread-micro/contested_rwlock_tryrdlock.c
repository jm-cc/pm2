/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2010, 2011 "the PM2 team" (see AUTHORS file)
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

#include <pthread.h>
#include "main.c"

static unsigned long *count;
static pthread_rwlock_t r;

static void *lock_unlock(void *arg)
{
	long rank = (long)arg;

	while (! isend) {
		if (0 == pthread_rwlock_tryrdlock(&r)) {
			count[rank]++;
			pthread_rwlock_unlock(&r);
		}
	}

	return NULL;
}

static void test_exec(void)
{
	int i;
	pthread_t *t;

	printf("-- Contested rwlock_tryrdlock test (duration: %ds) --\n", 
	       TEST_TIME);

	pthread_rwlock_init(&r, NULL);

	count = malloc(sizeof(*count) * nproc);
	t = malloc(sizeof(*t) * nproc);
	for (i = 0; i < nproc; i++) {
		count[i] = 0;
		pthread_create(&t[i], NULL, lock_unlock, (void *)((long)i));
	}

	for (i = 0; i < nproc; i++)
		pthread_join(t[i], NULL);
	free(t);
	free(count);

	pthread_rwlock_destroy(&r);
}

static void test_print_results(int sig)
{
	int i;
	unsigned long res;

	res = 0;
	for (i = 0; i < nproc; i++)
		res += count[i];
			
	isend = 1;
	printf("%ld rwlock_tryrdlock taken in %d seconds [%ld rwlock_tryrdlock/s]\n",
	       res, TEST_TIME, res/TEST_TIME);
}
