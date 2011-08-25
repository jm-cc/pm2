/* Copyright (C) 2002, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <marcel.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#if (defined(MA__IFACE_PMARCEL) || defined(MARCEL_LIBPTHREAD)) && defined(MARCEL_ONCE_ENABLED)


#define N 100

static pmarcel_once_t once = PMARCEL_ONCE_INIT;

static int global;

static void once_handler(void)
{
	struct timespec ts;

	++global;

	ts.tv_sec = 2;
	ts.tv_nsec = 0;
	nanosleep(&ts, NULL);
}


static void *tf(void *arg)
{
	pmarcel_once(&once, once_handler);

	if (global !=1) {
		printf("thread %ld: global == %d\n", (long int) arg, global);
		exit(1);
	}

	return NULL;
}


static int do_test(void)
{
	pmarcel_attr_t at;
	pmarcel_t th[N];
	int cnt;

	if (pmarcel_attr_init(&at) != 0) {
		puts("attr_init failed");
		return 1;
	}

	for (cnt = 0; cnt < N; ++cnt)
		if (pmarcel_create(&th[cnt], &at, tf, (void *) (long int) cnt) != 0) {
			printf("creation of thread %d failed\n", cnt);
			return 1;
		}

	if (pmarcel_attr_destroy(&at) != 0) {
		puts("attr_destroy failed");
		return 1;
	}

	for (cnt = 0; cnt < N; ++cnt)
		if (pmarcel_join(th[cnt], NULL) != 0) {
			printf("join of thread %d failed\n", cnt);
			return 1;
		}

	return 0;
}


int marcel_main(int argc, char *argv[])
{
	int status;

	marcel_init(&argc, argv);
	status = do_test();
	marcel_end();
	return status;
}


#else


int marcel_main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
