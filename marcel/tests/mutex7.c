/* Copyright (C) 2002, 2004, 2006 Free Software Foundation, Inc.
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

#include <errno.h>
#include <marcel.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#if defined(MA__IFACE_PMARCEL) || defined(MARCEL_LIBPTHREAD)


#ifndef TYPE
# define TYPE PMARCEL_MUTEX_DEFAULT
#endif


static pmarcel_mutex_t lock;


#define ROUNDS 20
#define N 100


static void *tf(void *arg)
{
	int nr = (long int) arg;
	int cnt;
	struct timespec ts = {.tv_sec = 0,.tv_nsec = 11000 };

	for (cnt = 0; cnt < ROUNDS; ++cnt) {
		if (pmarcel_mutex_lock(&lock) != 0) {
			marcel_printf("thread %d: failed to get the lock\n", nr);
			return (void *) 1l;
		}

		if (pmarcel_mutex_unlock(&lock) != 0) {
			marcel_printf("thread %d: failed to release the lock\n", nr);
			return (void *) 1l;
		}

		pmarcel_nanosleep(&ts, NULL);
	}

	return NULL;
}


static int do_test(void)
{
	pmarcel_mutexattr_t a;

	if (pmarcel_mutexattr_init(&a) != 0) {
		marcel_puts("mutexattr_init failed");
		exit(1);
	}

	if (pmarcel_mutexattr_settype(&a, TYPE) != 0) {
		marcel_puts("mutexattr_settype failed");
		exit(1);
	}
#ifdef ENABLE_PI
	if (pmarcel_mutexattr_setprotocol(&a, PMARCEL_PRIO_INHERIT) != 0) {
		marcel_puts("pmarcel_mutexattr_setprotocol failed");
		return 1;
	}
#endif

	int e = pmarcel_mutex_init(&lock, &a);
	if (e != 0) {
#ifdef ENABLE_PI
		if (e == ENOTSUP) {
			marcel_puts("PI mutexes unsupported");
			return 0;
		}
#endif
		marcel_puts("mutex_init failed");
		return 1;
	}

	if (pmarcel_mutexattr_destroy(&a) != 0) {
		marcel_puts("mutexattr_destroy failed");
		return 1;
	}

	pmarcel_attr_t at;
	pmarcel_t th[N];
	int cnt;

	if (pmarcel_attr_init(&at) != 0) {
		marcel_puts("attr_init failed");
		return 1;
	}

	if (pmarcel_mutex_lock(&lock) != 0) {
		marcel_puts("locking in parent failed");
		return 1;
	}

	for (cnt = 0; cnt < N; ++cnt)
		if (pmarcel_create(&th[cnt], &at, tf, (void *) (long int) cnt) != 0) {
			marcel_printf("creating thread %d failed\n", cnt);
			return 1;
		}

	if (pmarcel_attr_destroy(&at) != 0) {
		marcel_puts("attr_destroy failed");
		return 1;
	}

	if (pmarcel_mutex_unlock(&lock) != 0) {
		marcel_puts("unlocking in parent failed");
		return 1;
	}

	for (cnt = 0; cnt < N; ++cnt)
		if (pmarcel_join(th[cnt], NULL) != 0) {
			marcel_printf("joining thread %d failed\n", cnt);
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
