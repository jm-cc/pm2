/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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


#include "marcel.h"
#include <signal.h>


#ifdef MARCEL_SIGNALS_ENABLED


#define N 4
#define ITER_MAX 2000
#define SIG SIGUSR1


static marcel_barrier_t b;


static void *siggen(void *arg TBX_UNUSED)
{
	int i;
	marcel_sigset_t s;

	marcel_sigemptyset(&s);
	marcel_sigaddset(&s, SIG);
	marcel_sigmask(SIG_BLOCK, &s, NULL);

	marcel_barrier_wait(&b);

	for (i = 0; i < ITER_MAX; i++)
		kill(getpid(), SIG);

	return NULL;
}

static void sigh(int sig)
{
	if (sig != SIG)
		exit(1);
}

static int do_test()
{
	int i;
	marcel_t tasks[N];
	struct marcel_sigaction sa;

	marcel_barrier_init(&b, NULL, N);

	marcel_sigemptyset(&sa.marcel_sa_mask);
	sa.marcel_sa_flags = 0;
	sa.marcel_sa_handler = sigh;
	if (marcel_sigaction(SIG, &sa, NULL)) {
		fprintf(stderr, "Cannot set handler for signal %d\n", SIG);
		return 1;
	}
       
	for (i = 0; i < N; i++) {
		if (marcel_create(&tasks[i], NULL, siggen, NULL)) {
			fprintf(stderr, "Cannot create thread num %d\n", i);
			return 1;
		}
	}

	for (i = 0; i < N; i++) {
		if (marcel_join(tasks[i], NULL)) {
			fprintf(stderr, "Cannot join thread num %d\n", i);
			return 1;
		}
	}

	marcel_barrier_destroy(&b);

	return MARCEL_TEST_SUCCEEDED;
}

int marcel_main(int argc, char *argv[])
{
	int result;

	marcel_init(&argc, argv);
	result = do_test();
	marcel_end();

	return result;
}


#else


int marcel_main(int argc TBX_UNUSED, char *argv[] TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
