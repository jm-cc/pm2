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

static pthread_barrier_t b;

static void *lock_unlock(void *arg)
{
	while (! isend) {
		pthread_barrier_wait(&b);
		count++;
	}

	return arg;
}

static void test_exec(void)
{
	int i;
	pthread_t *t;

	pthread_barrier_init(&b, NULL, nproc);

	count = 0;
	t = malloc(sizeof(*t) * nproc);
	for (i = 0; i < nproc; i++)
		pthread_create(&t[i], NULL, lock_unlock, NULL);

	for (i = 0; i < nproc; i++)
		pthread_join(t[i], NULL);
	free(t);

	pthread_barrier_destroy(&b);
}

static void test_print_results(int sig)
{
	isend = 1;
	print_results();
	exit(0); // erk ... some threads are still waiting in the barrier
}
