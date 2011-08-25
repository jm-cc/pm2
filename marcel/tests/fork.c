/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2009 "the PM2 team" (see AUTHORS file)
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

/* Test the behavior of Marcel wrt. fork(2).  */

#include <marcel.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#undef   NDEBUG
#include <assert.h>


#ifdef MARCEL_FORK_ENABLED


#define THREADS 123

static void *thread_entry_point(void *arg)
{
	/* Make sure we have a working `marcel_self ()'.  */
	if (*(marcel_t *) arg != marcel_self())
		abort();

	return arg;
}


int main(int argc, char *argv[])
{
	int err;
	pid_t kid;
	unsigned i;
	marcel_t threads[THREADS];

	marcel_init(&argc, argv);

	for (i = 0; i < THREADS; i++) {
		err = marcel_create(&threads[i], NULL, thread_entry_point, &threads[i]);
		if (err) {
			perror("marcel_create");
			return EXIT_FAILURE;
		}
	}

	/* Fork while LWPs and user-level threads are still alive.  */
	kid = marcel_fork();
	if (kid < 0) {
		perror("fork");
		return EXIT_FAILURE;
	} else if (kid == 0) {
		/* Child process.  It should start with a single thread.  */
		int alive_count;
		marcel_t alive[THREADS];

		marcel_threadslist(THREADS, alive, &alive_count, 0);

		assert(alive_count == 1);
		assert(alive[0] == __main_thread);
		assert(alive[0] == marcel_self());

		/* The `extlib_protect' mutex should still be usable.  */
		marcel_free(marcel_malloc(123));

		/* This call invokes the scheduler under the hood.  Thus, Marcel must
		   still be running at this point.  */
		marcel_usleep(345);

		exit(0);
	} else {
		/* Parent process.  */
		int kid_status;

		/* Join our threads.  */
		for (i = 0; i < THREADS; i++) {
			void *result;

			err = marcel_join(threads[i], &result);
			if (err) {
				perror("marcel_join");
				return EXIT_FAILURE;
			}

			assert(result == &threads[i]);
		}

		err = marcel_waitpid(kid, &kid_status, 0);
		if (err == -1) {
			perror("wait");
			return EXIT_FAILURE;
		} else
			assert(err == kid);

		if (!(WIFEXITED(kid_status)
		      && WEXITSTATUS(kid_status) == EXIT_SUCCESS)) {
			if (!WIFEXITED(kid_status))
				marcel_fprintf(stderr, "child terminated abnormally\n");
			else
				marcel_fprintf(stderr, "child returned %i\n",
					       WEXITSTATUS(kid_status));

			if (WIFSIGNALED(kid_status))
				marcel_fprintf(stderr,
					       "child terminated with signal %i\n",
					       WTERMSIG(kid_status));

			abort();
		}
	}

	marcel_end();

	return EXIT_SUCCESS;
}


#else


int main(int argc TBX_UNUSED, char *argv[] TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
