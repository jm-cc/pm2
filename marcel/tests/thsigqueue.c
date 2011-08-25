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


#if defined(MARCEL_SIGNALS_ENABLED) && defined(SA_SIGINFO)


#define SIG SIGUSR1
#define THRD_MAX 20


static volatile int isdone = 0;
static int sigrecv[THRD_MAX];


static void sigh(int sig TBX_UNUSED, siginfo_t *info, void *uc TBX_UNUSED)
{
	if (info->si_code != SI_QUEUE)
		return;

	int rank = info->si_value.sival_int;
	sigrecv[rank]++;
}

static void *work(void *arg)
{
	while (! isdone)
		marcel_usleep(100);

	return arg;
}

static int do_test(void)
{
	int i;
	struct marcel_sigaction sa;
	marcel_t t[THRD_MAX];
	union sigval value;

	marcel_sigemptyset(&sa.marcel_sa_mask);
	sa.marcel_sa_sigaction = sigh;
	sa.marcel_sa_flags = SA_SIGINFO;
	marcel_sigaction(SIG, &sa, NULL);

	for (i = 0; i < THRD_MAX; i++) {
		sigrecv[i] = 0;
		marcel_create(&t[i], NULL, work, NULL);
	}

	for (i = 0; i < THRD_MAX; i++) {
		value.sival_int = i;
		marcel_sigqueue(&t[i], SIG, value);
	}
	isdone = 1;

	for (i = 0; i < THRD_MAX; i++) {
		marcel_join(t[i], NULL);
#ifdef VERBOSE_TESTS
		marcel_printf("th %d receives %d signals\n", i, sigrecv[i]);
#endif
		if (sigrecv[i] == 0)
			return MARCEL_TEST_FAILED;
	}

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
