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

/* Test the behavior of Marcel's libpthread ABI compatibility layer
   wrt. fork(2).  */


#include "pthread-abi.h"


#if defined(MARCEL_LIBPTHREAD) && defined(MARCEL_FORK_ENABLED)


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <dlfcn.h>


#define THREADS 123

static void *thread_entry_point(void *arg)
{
	/* Make sure we have a working `pthread_self ()'.  */
	if (*(pthread_t *) arg != pthread_self())
		abort();

	return arg;
}


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	int err;
	pid_t kid;
	unsigned i;
	pthread_t threads[THREADS];

	assert_running_marcel();

	for (i = 0; i < THREADS; i++) {
		err = pthread_create(&threads[i], NULL, thread_entry_point, &threads[i]);
		if (err) {
			perror("pthread_create");
			return EXIT_FAILURE;
		}
	}

	/* Fork while LWPs and user-level threads are still alive.  */
	kid = fork();
	if (kid < 0) {
		perror("fork");
		return EXIT_FAILURE;
	} else if (kid == 0) {
		/* Child process.  It should start with a single thread.  */
		FILE *out;

		out = fopen("/dev/null", "w");
		if (out != NULL) {
			fprintf(out, "hello\n");
			fclose(out);
		}

		/* This call invokes `marcel_usleep ()' under the hood, which invokes
		   the scheduler.  Thus, Marcel must still be running at this point.  */
		usleep(345);
	} else {
		/* Parent process.  */
		int kid_status;

		/* Join our threads.  */
		for (i = 0; i < THREADS; i++) {
			void *result;

			err = pthread_join(threads[i], &result);
			if (err) {
				perror("pthread_join");
				return EXIT_FAILURE;
			}

			assert(result == &threads[i]);
		}

		err = waitpid(kid, &kid_status, 0);
		if (err == -1) {
			perror("wait");
			return EXIT_FAILURE;
		} else
			assert(err == kid);

		if (!(WIFEXITED(kid_status) && WEXITSTATUS(kid_status) == 0)) {
			if (!WIFEXITED(kid_status))
				fprintf(stderr, "child terminated abnormally\n");
			else
				fprintf(stderr, "child returned %i\n",
					WEXITSTATUS(kid_status));

			if (WIFSIGNALED(kid_status))
				fprintf(stderr,
					"child terminated with signal %i\n",
					WTERMSIG(kid_status));

			abort();
		}
	}

	return EXIT_SUCCESS;
}


#else


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
