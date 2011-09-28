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
#include <semaphore.h>
#include "main.c"

static sem_t s;

static void *lock_unlock(void *arg)
{
	while (! isend) {
		if (0 == sem_trywait(&s)) {
			count ++;
			sem_post(&s);
		}
	}

	return arg;
}

static void test_exec(void)
{
	pthread_t t;

	sem_init(&s, 0, 1);

	count = 0;
	pthread_create(&t, NULL, lock_unlock, NULL);

	pthread_join(t, NULL);
	sem_destroy(&s);
}

static void test_print_results(int sig)
{
	isend = 1;
	print_results();
}
