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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "sys/marcel_utils.h"


#if defined(MARCEL_SIGNALS_ENABLED) && defined(MARCEL_LIBPTHREAD)


#define SIG SIGUSR1


static void sigh(int sig)
{
	if (SIG == sig)
		exit(MARCEL_TEST_SUCCEEDED);
}


static int do_test()
{
	if (-1 == sighold(SIG)) {
		puts("sighold failed");
		return -1;
	}
	kill(getpid(), SIG);

	if (SIG_ERR == signal(SIG, sigh)) {
		puts("signal failed");
		return -1;
	}

	if (-1 == sigrelse(SIG)) {
		puts("sigrelse failed");
		return -1;
	}
	kill(getpid(), SIG);

	sleep(3);

	return -1;
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
