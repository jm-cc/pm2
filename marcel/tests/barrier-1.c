/* Copyright (C) 2002 Free Software Foundation, Inc.
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


#include "marcel.h"


#ifdef MARCEL_POSIX


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	pmarcel_barrier_t b;
	int e;
	int cnt;

	e = pmarcel_barrier_init(&b, NULL, 0);
	if (e == 0) {
		marcel_fprintf(stderr, "barrier_init with count 0 succeeded");
		return 1;
	}
	if (e != EINVAL) {
		marcel_fprintf(stderr, "barrier_init with count 0 didn't return EINVAL");
		return 1;
	}

	if (pmarcel_barrier_init(&b, NULL, 1) != 0) {
		marcel_fprintf(stderr, "real barrier_init failed");
		return 1;
	}

	for (cnt = 0; cnt < 10; ++cnt) {
		e = pmarcel_barrier_wait(&b);

		if (e != PMARCEL_BARRIER_SERIAL_THREAD) {
			marcel_fprintf(stderr,
				       "barrier_wait didn't return PMARCEL_BARRIER_SERIAL_THREAD\n");
			return 1;
		}
	}

	if (pmarcel_barrier_destroy(&b) != 0) {
		marcel_fprintf(stderr, "barrier_destroy failed");
		return 1;
	}

	return 0;
}


#else


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
