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

static unsigned long *th_count;
static pthread_rwlock_t r;

static void *lock_unlock(void *arg)
{
	long rank = (long)arg;

	while (! isend) {
		pthread_rwlock_wrlock(&r);
		th_count[rank]++;
		pthread_rwlock_unlock(&r);
	}

	return NULL;
}

static void test_exec(void)
{
	int i;
	pthread_t *t;

	pthread_rwlock_init(&r, NULL);

	th_count = malloc(sizeof(*th_count) * nproc);
	t = malloc(sizeof(*t) * nproc);
	for (i = 0; i < nproc; i++) {
		th_count[i] = 0;
		pthread_create(&t[i], NULL, lock_unlock, (void *)((long)i));
	}

	for (i = 0; i < nproc; i++)
		pthread_join(t[i], NULL);
	free(t);
	free(th_count);

	pthread_rwlock_destroy(&r);
}

static void test_print_results(int sig)
{
	int i;

	isend = 1;

	count = 0;
	for (i = 0; i < nproc; i++)
		count += th_count[i];
	print_results();
}
