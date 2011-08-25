/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008, 2009 "the PM2 team" (see AUTHORS file)
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

/* Test Marcel's libpthread ABI compatibility layer.  */


#include "pthread-abi.h"


#if defined(MARCEL_LIBPTHREAD)


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>


#define THREADS 123


static void *thread_entry_point(void *arg)
{
	/* Make sure we have a working `pthread_self ()'.  */
	if (*(pthread_t *) arg != pthread_self())
		abort();

	usleep(1000);

	return arg;
}


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	int err;
	unsigned i;
	pthread_t threads[THREADS];

	assert_running_marcel();

	for (i = 0; i < THREADS; i++) {
		err = pthread_create(&threads[i], NULL, thread_entry_point, &threads[i]);
		if (err) {
			fprintf(stderr, "pthread_create: %s\n", strerror(err));
			exit(1);
		}
	}

	for (i = 0; i < THREADS; i++) {
		void *result;

		err = pthread_join(threads[i], &result);
		if (err) {
			fprintf(stderr, "pthread_join: %s\n", strerror(err));
			exit(1);
		}

		assert(result == &threads[i]);
	}

	return MARCEL_TEST_SUCCEEDED;
}


#else


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
