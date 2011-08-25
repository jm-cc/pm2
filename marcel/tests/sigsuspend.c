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
#include <errno.h>


#if defined(MARCEL_SIGNALS_ENABLED) && defined(MARCEL_LIBPTHREAD)


#define SIG1 SIGUSR1
#define SIG2 SIGUSR2

static sem_t semsync;


static void sigh(int sig TBX_UNUSED)
{
}

static void *thr_func(void *arg TBX_UNUSED)
{
	struct sigaction sa;
	sigset_t s;

	sa.sa_flags = 0;
	sa.sa_handler = sigh;
	sigemptyset(&sa.sa_mask);
	if (0 != sigaction(SIG1, &sa, NULL)) {
		puts("first sigaction failed");
		return (void *)1;
	}

	if (0 != sigaction(SIG2, &sa, NULL)) {
		puts("second sigaction failed");
		return (void *)1;
	}


	sigemptyset(&s);
	sigaddset(&s, SIG2);

	sem_post(&semsync);
	if (-1 != sigpause(SIG2) || EINTR != errno)
		return (void *)1;
	
	sem_post(&semsync);
	if (-1 == sigsuspend(&s) && EINTR == errno)
		return NULL;

	return (void *)1;
}


static int do_test()
{
	pthread_t t;
	void *retval;

	if (0 != sem_init(&semsync, 0, 0)) {
		puts("sem_init failed");
		return -1;
	}

	if (0 != pthread_create(&t, NULL, thr_func, NULL)) {
		puts("pthread_create failed");
		return -1;
	}

	// sigpause
	sem_wait(&semsync);
	sleep(1);
	pthread_kill(t, SIG1);

	// sigsuspend
	sem_wait(&semsync);
	sleep(1);
	pthread_kill(t, SIG1);

	if (0 != pthread_join(t, &retval)) {
		puts("pthread_join failed");
		return -1;
	}

	sem_destroy(&semsync);

	return (retval == NULL) ? MARCEL_TEST_SUCCEEDED : -1;
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
