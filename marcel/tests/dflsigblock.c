
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

/* dflsigblock.c */


#include "marcel.h"


#ifdef MARCEL_SIGNALS_ENABLED


static const marcel_sigset_t set =
    (marcel_sigoneset(SIGUSR1) | marcel_sigoneset(SIGUSR2));
static int done;
static int sig_recv;


static void handler(int sig)
{
#ifdef VERBOSE_TESTS
	marcel_printf("got signal %d\n", sig);
#endif
	sig_recv = sig;
}

static any_t f(any_t foo TBX_UNUSED)
{
	marcel_sigmask(SIG_BLOCK, &set, NULL);
	marcel_kill(marcel_self(), SIGUSR1);
	marcel_kill(marcel_self(), SIGUSR2);
	marcel_signal(SIGUSR2, SIG_IGN);
	marcel_yield();
	marcel_yield();
	marcel_sigmask(SIG_UNBLOCK, &set, NULL);
	/* shouldn't have crashed, as we have reset to SIG_IGN */
	done = 1;
	return NULL;
}

int marcel_main(int argc, char *argv[])
{
	marcel_init(&argc, argv);

	marcel_signal(SIGUSR1, handler);
	marcel_signal(SIGUSR2, SIG_DFL);

	marcel_create(NULL, NULL, f, NULL);

	while (!done)
		marcel_yield();

	marcel_end();
	return (sig_recv == SIGUSR1) ? MARCEL_TEST_SUCCEEDED : sig_recv;
}


#else


int marcel_main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif				/* MARCEL_SIGNALS_ENABLED */
