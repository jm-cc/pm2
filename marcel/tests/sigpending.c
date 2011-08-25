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


#define ITER_MAX 1000
#define SIG SIGUSR1


static tbx_bool_t sigh_called = tbx_false;


static void sigh(int sig)
{
	if (SIG == sig)
		sigh_called = tbx_true;
}	


static int do_test()
{
	int i;
	sigset_t s;
	struct sigaction sa;

	sigemptyset(&s);
	sigaddset(&s, SIG);
	if (0 != sigpending(&s)) {
		puts("sigpending failed");
		return -1;
	}

	sa.sa_flags = 0;
	sa.sa_handler = sigh;
	sigemptyset(&sa.sa_mask);
	if (0 != sigaction(SIG, &sa, NULL)) {
		puts("sigaction failed");
		return -1;
	}

	for (i = 0; i < ITER_MAX; i++)
		kill(getpid(), SIG);

	sigemptyset(&s);
	sigpending(&s);
	sleep(5);

	return (tbx_true == sigh_called) ? MARCEL_TEST_SUCCEEDED : EXIT_FAILURE;
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
