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

static pthread_rwlock_t r;

static void *lock_unlock(void *arg)
{
	while (! isend) {
		pthread_rwlock_rdlock(&r);
		count++;
		pthread_rwlock_unlock(&r);
	}

	return arg;
}

static void test_exec(void)
{
	pthread_t t;

	pthread_rwlock_init(&r, NULL);

	count = 0;
	pthread_create(&t, NULL, lock_unlock, NULL);
	pthread_join(t, NULL);

	pthread_rwlock_destroy(&r);
}

static void test_print_results(int sig)
{
	isend = 1;
	print_results();
}
