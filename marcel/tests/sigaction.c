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


#include <signal.h>
#include "marcel.h"


#ifdef MARCEL_LIBPTHREAD


#define ITER_MAX 100000
#define SIG1 SIGUSR1
#define SIG2 SIGUSR2

static tbx_bool_t sigh1_called = tbx_false;
static tbx_bool_t sigh2_called = tbx_false;

static void sigh1(int sig)
{
	if (sig != SIG1)
		exit(1);

	sigh1_called = tbx_true;
}

static void sigh2(int sig, siginfo_t *info TBX_UNUSED, void *context TBX_UNUSED)
{
	if (sig != SIG2)
		exit(1);

	sigh2_called = tbx_true;
}

static int do_test()
{
	int i;
	struct sigaction sa;
	
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = sigh1;
	if (sigaction(SIG1, &sa, NULL)) {
		fprintf(stderr, "Cannot set handler for signal %d\n", SIG1);
		return 1;
	}

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = sigh2;
	if (sigaction(SIG2, &sa, NULL)) {
		fprintf(stderr, "Cannot set handler for signal %d\n", SIG2);
		return 1;
	}

	for (i = 0; i < ITER_MAX; i++) {
		raise(SIG1);
		raise(SIG2);
	}
       
	return (sigh1_called == tbx_true && sigh2_called == tbx_true) ? MARCEL_TEST_SUCCEEDED : EXIT_FAILURE;
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
