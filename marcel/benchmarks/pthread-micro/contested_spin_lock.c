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

static pthread_spinlock_t s;

static void *lock_unlock(void *arg)
{
	while (! isend) {
		pthread_spin_lock(&s);
		count ++;
		pthread_spin_unlock(&s);
	}

	return arg;
}

static void test_exec(void)
{
	int i;
	pthread_t *t;

	count = 0;
	pthread_spin_init(&s, 0);
	t = malloc(sizeof(*t) * nproc);
	for (i = 0; i < nproc; i++)
		pthread_create(&t[i], NULL, lock_unlock, NULL);

	for (i = 0; i < nproc; i++)
		pthread_join(t[i], NULL);
	pthread_spin_destroy(&s);
	free(t);
}

static void test_print_results(int sig)
{
	isend = 1;
	print_results();
}
