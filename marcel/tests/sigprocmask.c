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


#if defined(MARCEL_SIGNALS_ENABLED) && defined(MARCEL_LIBPTHREAD)


#define ITER_MAX 100000
#define SIG SIGUSR1


static int do_test()
{
	int i;
	sigset_t s;

	sigemptyset(&s);
	sigaddset(&s, SIG);
	sigprocmask(SIG_BLOCK, &s, NULL);

	for (i = 0; i < ITER_MAX; i++)
		kill(getpid(), SIG);

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
