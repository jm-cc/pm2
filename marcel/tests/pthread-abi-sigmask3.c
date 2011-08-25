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


#include <pthread.h>
#include <signal.h>
#include "marcel.h"


#ifdef MARCEL_LIBPTHREAD


#define N 2
#define ITER_MAX 2000
#define SIG SIGUSR1

static pthread_barrier_t b;

static void *siggen(void *arg TBX_UNUSED)
{
	int i;
	sigset_t s;
	
	sigaddset(&s, SIG);
	pthread_sigmask(SIG_BLOCK, &s, NULL);

	pthread_barrier_wait(&b);

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
	pthread_t tasks[N];
	struct sigaction sa;
	
	pthread_barrier_init(&b, NULL, N);

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = sigh;
	if (sigaction(SIG, &sa, NULL)) {
		fprintf(stderr, "Cannot set handler for signal %d\n", SIG);
		return 1;
	}
       
	for (i = 0; i < N; i++) {
		if (pthread_create(&tasks[i], NULL, siggen, NULL)) {
			fprintf(stderr, "Cannot create thread num %d\n", i);
			return 1;
		}
	}

	for (i = 0; i < N; i++) {
		if (pthread_join(tasks[i], NULL)) {
			fprintf(stderr, "Cannot join thread num %d\n", i);
			return 1;
		}
	}

	pthread_barrier_destroy(&b);

	return MARCEL_TEST_SUCCEEDED;
}

int main(int argc TBX_UNUSED, char *argv[] TBX_UNUSED)
{
	return do_test();
}


#else


int main(int argc TBX_UNUSED, char *argv[] TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
