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


#if defined(MARCEL_LIBPTHREAD) && defined(MARCEL_SIGMASK_ENABLED)


#define SIG SIGUSR1
#define MAGIC_CONSTANT 1234

static pthread_t threads[4];
static int       thread_retval[4];
static sem_t     semsync;


static int get_my_rank()
{
	int my_rank;
	pthread_t me;

	me = pthread_self();
	for (my_rank = 0; my_rank < 4; my_rank++) {
		if (threads[my_rank] == me)
			return my_rank;
	}

	return -1;
}

static void sigh(int sig)
{
	int my_rank = get_my_rank();

#ifdef VERBOSE_TESTS
	fprintf(stderr, "threads(%p) number %d receive %d\n", pthread_self(), my_rank, sig);
#endif
	if (-1 != my_rank)
		thread_retval[my_rank] = 0;
}

/* thread1: SIG is not yet blocked by main thread 
 * thread2 inherit of main sigmask: SIG still blocked */
static void *thr_func12(void *arg)
{
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = sigh;
	sa.sa_flags = 0;	
	if (sigaction(SIG, &sa, NULL)) {
		fprintf(stderr, "sigaction (1) failed\n");
		exit(1);
	}

	sem_post(&semsync);
	sleep(5);
	return NULL;
}

/* thread3 inherit of main sigmask: unblock SIG and check handler call */
static void *thr_func3(void *arg)
{
	sigset_t s;
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = sigh;
	sa.sa_flags = 0;
	if (sigaction(SIG, &sa, NULL)) {
		fprintf(stderr, "sigaction (2) failed\n");
		exit(1);
	}

	sigemptyset(&s);
	sigaddset(&s, SIG);
	if (pthread_sigmask(SIG_UNBLOCK, &s, NULL)) {
		fprintf(stderr, "sigmask failed\n");
		exit(1);
	}

	sem_post(&semsync);
	sleep(5);
	return NULL;
}

static int do_test(void)
{
	int i;
	sigset_t s;

	sem_init(&semsync, 0, 0);
	threads[0] = pthread_self();
	for (i = 0; i < 4; i ++)
		thread_retval[i] = MAGIC_CONSTANT;

	if (pthread_create(&threads[1], NULL, thr_func12, NULL)) {
		fprintf(stderr, "thread creation (1) failed\n");
		exit(1);
	}
	sem_wait(&semsync);

	sigemptyset(&s);
	sigaddset(&s, SIG);
	if (pthread_sigmask(SIG_BLOCK, &s, NULL)) {
		fprintf(stderr, "sigmask failed\n");
		exit(1);
	}

	if (pthread_create(&threads[2], NULL, thr_func12, NULL)) {
		fprintf(stderr, "thread creation (2) failed\n");
		exit(1);
	}
	sem_wait(&semsync);

	if (pthread_create(&threads[3], NULL, thr_func3, NULL)) {
		fprintf(stderr, "thread creation (3) failed\n");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	sem_wait(&semsync);
#endif

	for (i = 1; i < 4; i++)
		pthread_kill(threads[i], SIG);

	// expect join 0 for thread 1 and 3 and MAGIC_CONST for thread 2
	if (pthread_join(threads[1], NULL) || thread_retval[1] == MAGIC_CONSTANT) {
		fprintf(stderr, "thread 1: error\n");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	printf("thread 1: ok\n");
#endif

	if (pthread_join(threads[3], NULL) || thread_retval[3] == MAGIC_CONSTANT) {
		fprintf(stderr, "thread 3: error\n");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	printf("thread 3: ok\n");
#endif

	if (pthread_join(threads[2], NULL) || thread_retval[2] != MAGIC_CONSTANT) {
		fprintf(stderr, "thread 2: error\n");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	printf("thread 2: ok\n");
#endif

	if (thread_retval[0] != MAGIC_CONSTANT) {
		fprintf(stderr, "main thread: error\n");
		exit(1);
	}

	sem_destroy(&semsync);
	return MARCEL_TEST_SUCCEEDED;
}

int main(int argc, char *argv[])
{
	return do_test();
}


#else


int main(int argc TBX_UNUSED, char *argv[] TBX_UNUSED)
{
        return MARCEL_TEST_SKIPPED;
}


#endif
