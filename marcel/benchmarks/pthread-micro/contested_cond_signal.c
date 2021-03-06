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
#include <errno.h>
#include "main.c"

static pthread_cond_t c = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

static void *th_waiters(void *arg)
{
	pthread_mutex_lock(&m);
	while (! isend) {
		pthread_cond_wait(&c, &m);
		count++;
	}
	pthread_mutex_unlock(&m);

	return arg;
}

static void *th_signal(void *arg)
{
	while (! isend)
		pthread_cond_signal(&c);
	
	return arg;
}

static void test_exec(void)
{
	int i;
	pthread_t *t;

	count = 0;
	t = malloc(sizeof(*t) * nproc);
	for (i = 0; i < nproc-1; i++)
		pthread_create(&t[i], NULL, th_waiters, NULL);
	pthread_create(&t[i], NULL, th_signal, NULL);

	for (i = 0; i < nproc; i++)
		pthread_join(t[i], NULL);

	free(t);
}

static void test_print_results(int sig)
{
	isend = 1;
	print_results();
	pthread_cond_broadcast(&c);
}
