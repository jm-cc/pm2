/* Copyright (C) 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2004.

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

/* This is a test for behavior not guaranteed by POSIX.  */
#include "marcel.h"


#ifdef MARCEL_POSIX


#define N 20


static pmarcel_barrier_t b1;
static pmarcel_barrier_t b2;


static void *tf(void *arg TBX_UNUSED)
{
	int round = 0;

	while (round++ < 30) {
		if (pmarcel_barrier_wait(&b1) == PMARCEL_BARRIER_SERIAL_THREAD) {
			pmarcel_barrier_destroy(&b1);
			if (pmarcel_barrier_init(&b1, NULL, N) != 0) {
				marcel_fprintf(stderr, "tf: 1st barrier_init failed\n");
				pmarcel_exit((void *) 1);
			}
		}

		if (pmarcel_barrier_wait(&b2) == PMARCEL_BARRIER_SERIAL_THREAD) {
			pmarcel_barrier_destroy(&b2);
			if (pmarcel_barrier_init(&b2, NULL, N) != 0) {
				marcel_fprintf(stderr, "tf: 2nd barrier_init failed\n");
				pmarcel_exit((void *) 1);
			}
		}
	}

	return NULL;
}


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	pmarcel_attr_t at;
	int cnt;

	marcel_init(&argc, argv);

	if (pmarcel_attr_init(&at) != 0) {
		marcel_fprintf(stderr, "attr_init failed\n");
		return 1;
	}

#if !defined(PM2VALGRIND) && !defined(DARWIN_SYS)
	if (pmarcel_attr_setstacksize(&at, PMARCEL_STACK_MIN) != 0) {
		marcel_fprintf(stderr, "attr_setstacksize failed\n");
		return 1;
	}
#endif

	if (pmarcel_barrier_init(&b1, NULL, N) != 0) {
		marcel_fprintf(stderr, "1st barrier_init failed\n");
		return 1;
	}

	if (pmarcel_barrier_init(&b2, NULL, N) != 0) {
		marcel_fprintf(stderr, "2nd barrier_init failed\n");
		return 1;
	}

	pmarcel_t th[N - 1];
	for (cnt = 0; cnt < N - 1; ++cnt)
		if (pmarcel_create(&th[cnt], &at, tf, NULL) != 0) {
			marcel_fprintf(stderr, "pmarcel_create failed\n");
			return 1;
		}

	if (pmarcel_attr_destroy(&at) != 0) {
		marcel_fprintf(stderr, "attr_destroy failed\n");
		return 1;
	}

	tf(NULL);

	for (cnt = 0; cnt < N - 1; ++cnt)
		if (pmarcel_join(th[cnt], NULL) != 0) {
			marcel_fprintf(stderr, "pmarcel_join failed\n");
			return 1;
		}

	marcel_end();

	return 0;
}


#else


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
