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


#if defined(MARCEL_SIGNALS_ENABLED) && defined(MARCEL_LIBPTHREAD)


#define SIG SIGUSR1


static pthread_barrier_t barrier;


static void *thr_func(void *arg TBX_UNUSED)
{
	struct timespec timeout;
	struct sigaction sa;
	siginfo_t info;
	sigset_t set;
	int sig;

	sa.sa_flags = 0;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	if (0 != sigaction(SIG, &sa, NULL)) {
		puts("sigaction failed");
		return (void *)1;
	}

	sigemptyset(&set);
	sigaddset(&set, SIG);
	pthread_barrier_wait(&barrier);
	if (0 != sigwait(&set, &sig)) {
		puts("sigwait failed");
		return (void *)1;
	}
	if (sig != SIG) {
		fprintf(stderr, "sigwait: receive %d\n", sig);
		return (void *)1;
	}

	pthread_barrier_wait(&barrier);
	if (SIG != sigwaitinfo(&set, &info)) {
		puts("sigwaitinfo failed");
		return (void *)1;
	}
	if (info.si_signo != SIG) {
		fprintf(stderr, "sigwaitinfo: receive %d\n", info.si_signo);
		return (void *)1;
	}

	timeout.tv_sec = 2;
	timeout.tv_nsec = 0;
	pthread_barrier_wait(&barrier);
	if (SIG != sigtimedwait(&set, &info, &timeout)) {
		puts("sigtimedwait(child) failed");
		return (void *)1;
	}

	return NULL;
}


static int do_test()
{
	int i;
	sigset_t s;
	pthread_t t;
	void *retval;
	siginfo_t info;
	struct timespec timeout;

	if (0 != pthread_barrier_init(&barrier, NULL, 2)) {
		puts("barrier_init failed");
		return -1;
	}

	if (0 != pthread_create(&t, NULL, thr_func, NULL)) {
		puts("pthread_create failed");
		return -1;
	}

	for (i = 0; i < 3; i++) {
		pthread_barrier_wait(&barrier);
		sleep(1);
		pthread_kill(t, SIG);
	}

	if (0 != pthread_join(t, &retval)) {
		puts("pthread_join failed");
		return -1;
	}

	sigemptyset(&s);
	sigaddset(&s, SIGUSR2);
	timeout.tv_sec = 1;
	timeout.tv_nsec = 0;
	if (-1 != sigtimedwait(&s, &info, &timeout) || errno != EAGAIN) {
		puts("sigtimedwait(main) failed");
		return -1;
	}

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
