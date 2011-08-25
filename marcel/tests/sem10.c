/* Copyright (C) 2007 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jakub Jelinek <jakub@redhat.com>, 2007.

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

#include <errno.h>
#include <marcel.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include "tbx_compiler.h"


#if (defined(MA__IFACE_PMARCEL) || defined(MARCEL_LIBPTHREAD)) && defined(MARCEL_CLEANUP_ENABLED)


static int do_test(void)
{
	pmarcel_sem_t s;
	if (pmarcel_sem_init(&s, 0, 0) == -1) {
		puts("pmarcel_sem_init failed");
		return 1;
	}

	struct timeval tv;
	if (gettimeofday(&tv, NULL) != 0) {
		puts("gettimeofday failed");
		return 1;
	}

	struct timespec ts;
	TIMEVAL_TO_TIMESPEC(&tv, &ts);

	/* Set ts to yesterday.  */
	ts.tv_sec -= 86400;

	int type_before;
	if (pmarcel_setcanceltype(PMARCEL_CANCEL_DEFERRED, &type_before) != 0) {
		puts("first pmarcel_setcanceltype failed");
		return 1;
	}

	errno = 0;
	if (MA_FAILURE_RETRY(pmarcel_sem_timedwait(&s, &ts)) != -1) {
		puts("pmarcel_sem_timedwait succeeded");
		return 1;
	}
	if (errno != ETIMEDOUT) {
		printf("pmarcel_sem_timedwait return errno = %d instead of ETIMEDOUT\n", errno);
		return 1;
	}

	int type_after;
	if (pmarcel_setcanceltype(PMARCEL_CANCEL_DEFERRED, &type_after) != 0) {
		puts("second pmarcel_setcanceltype failed");
		return 1;
	}
	if (type_after != PMARCEL_CANCEL_DEFERRED) {
		puts("pmarcel_sem_timedwait changed cancellation type");
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
