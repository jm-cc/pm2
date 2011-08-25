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


#define SIG SIGUSR1
#define MAGIC_CONSTANT 1234
#define SLEEP_TIME 10

static marcel_t threads[4];
static int      thread_retval[4];
static marcel_sem_t    semsync;


static int get_my_rank()
{
	int my_rank;
	marcel_t me;

	me = marcel_self();
	for (my_rank = 0; my_rank < 4; my_rank++) {
		if (threads[my_rank] == me)
			return my_rank;
	}

	return -1;
}

static void sigh(int sig TBX_UNUSED)
{
	int my_rank = get_my_rank();

#ifdef VERBOSE_TESTS
	marcel_fprintf(stderr, "threads(%p) number %d receive %d\n", marcel_self(), my_rank, sig);
#endif
	if (-1 != my_rank)
		thread_retval[my_rank] = 0;
}

/* thread1: SIG is not yet blocked by main thread 
 * thread2 inherit of main sigmask: SIG still blocked */
static void *thr_func12(void *arg TBX_UNUSED)
{
	struct marcel_sigaction sa;

	marcel_sigemptyset(&sa.marcel_sa_mask);
	sa.marcel_sa_handler = sigh;
	sa.marcel_sa_flags = 0;	
	if (marcel_sigaction(SIG, &sa, NULL)) {
		marcel_fprintf(stderr, "sigaction (1) failed\n");
		exit(1);
	}

	marcel_sem_post(&semsync);
	marcel_sleep(SLEEP_TIME);
	return NULL;
}

/* thread3 inherit of main sigmask: unblock SIG and check handler call */
static void *thr_func3(void *arg TBX_UNUSED)
{
	marcel_sigset_t s;
	struct marcel_sigaction sa;

	marcel_sigemptyset(&sa.marcel_sa_mask);
	sa.marcel_sa_handler = sigh;
	sa.marcel_sa_flags = 0;
	if (marcel_sigaction(SIG, &sa, NULL)) {
		marcel_fprintf(stderr, "sigaction (2) failed\n");
		exit(1);
	}

	marcel_sigemptyset(&s);
	marcel_sigaddset(&s, SIG);
	if (marcel_sigmask(SIG_UNBLOCK, &s, NULL)) {
		marcel_fprintf(stderr, "sigmask failed\n");
		exit(1);
	}

	marcel_sem_post(&semsync);
	marcel_sleep(SLEEP_TIME);
	return NULL;
}

static int do_test(void)
{
	int i;
	marcel_sigset_t s;

	marcel_sem_init(&semsync, 0);
	threads[0] = marcel_self();
	for (i = 0; i < 4; i ++)
		thread_retval[i] = MAGIC_CONSTANT;

	if (marcel_create(&threads[1], NULL, thr_func12, NULL)) {
		marcel_fprintf(stderr, "thread creation (1) failed\n");
		exit(1);
	}
	marcel_sem_wait(&semsync);

	marcel_sigemptyset(&s);
	marcel_sigaddset(&s, SIG);
	if (marcel_sigmask(SIG_BLOCK, &s, NULL)) {
		marcel_fprintf(stderr, "sigmask failed\n");
		exit(1);
	}

	if (marcel_create(&threads[2], NULL, thr_func12, NULL)) {
		marcel_fprintf(stderr, "thread creation (2) failed\n");
		exit(1);
	}
	marcel_sem_wait(&semsync);

	if (marcel_create(&threads[3], NULL, thr_func3, NULL)) {
		marcel_fprintf(stderr, "thread creation (3) failed\n");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	marcel_sem_wait(&semsync);
#endif

	for (i = 0; i < 4; i++)
		marcel_kill(threads[i], SIG);

	// expect join 0 for thread 1 and 3 and MAGIC_CONST for thread 2
	if (marcel_join(threads[1], NULL) || thread_retval[1] == MAGIC_CONSTANT) {
		marcel_fprintf(stderr, "thread 1: error %d\n", thread_retval[1]);
		exit(1);
	}
#ifdef VERBOSE_TESTS
	marcel_printf("thread 1: ok\n");
#endif

	if (marcel_join(threads[3], NULL) || thread_retval[3] == MAGIC_CONSTANT) {
		marcel_fprintf(stderr, "thread 3: error\n");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	marcel_printf("thread 3: ok\n");
#endif

	if (marcel_join(threads[2], NULL) || thread_retval[2] != MAGIC_CONSTANT) {
		marcel_fprintf(stderr, "thread 2: error\n");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	marcel_printf("thread 2: ok\n");
#endif

	if (thread_retval[0] != MAGIC_CONSTANT) {
		marcel_fprintf(stderr, "main thread: error\n");
		exit(1);
	}

	marcel_sem_destroy(&semsync);
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
