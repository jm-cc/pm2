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

static unsigned long count;

static void *do_nothing(void *arg)
{
	return arg;
}

static void test_exec(void)
{
	pthread_t t;

	printf("-- Thread creation test (duration: %ds) --\n", 
	       TEST_TIME);

	count = 0;
	while (!isend) {
		pthread_create(&t, NULL, do_nothing, NULL);
		pthread_join(t, NULL);
		count++;
	}
}

static void test_print_results(int sig)
{
	isend = 1;
	printf("%ld thread created in %d seconds [%ld threads/s]\n",
	       count, TEST_TIME, count/TEST_TIME);
}
