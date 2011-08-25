/* Copyright (C) 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2003.

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
#include <sys/time.h>


#if (defined(MA__IFACE_PMARCEL) || defined(MARCEL_LIBPTHREAD)) && defined(MARCEL_DEVIATION_ENABLED) && defined(MARCEL_CLEANUP_ENABLED)


static pmarcel_cond_t cond = PMARCEL_COND_INITIALIZER;
static pmarcel_mutex_t mut = PMARCEL_ERRORCHECK_MUTEX_INITIALIZER_NP;

static pmarcel_barrier_t bar;


static void ch(void *arg TBX_UNUSED)
{
	int e = pmarcel_mutex_lock(&mut);
	if (e == 0) {
		puts("mutex not locked at all by cond_wait");
		exit(1);
	}

	if (e != EDEADLK) {
		puts("no deadlock error signaled");
		exit(1);
	}

	if (pmarcel_mutex_unlock(&mut) != 0) {
		puts("ch: cannot unlock mutex");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	puts("ch done");
#endif
}


static void *tf1(void *p TBX_UNUSED)
{
	int err;

	if (pmarcel_setcancelstate(PMARCEL_CANCEL_ENABLE, NULL) != 0
	    || pmarcel_setcanceltype(PMARCEL_CANCEL_DEFERRED, NULL) != 0) {
		puts("cannot set cancellation options");
		exit(1);
	}

	err = pmarcel_mutex_lock(&mut);
	if (err != 0) {
		puts("child: cannot get mutex");
		exit(1);
	}

	err = pmarcel_barrier_wait(&bar);
	if (err != 0 && err != PMARCEL_BARRIER_SERIAL_THREAD) {
		printf("barrier_wait returned %d\n", err);
		exit(1);
	}
#ifdef VERBOSE_TESTS
	puts("child: got mutex; waiting");
#endif

	pmarcel_cleanup_push(ch, NULL);

	pmarcel_cond_wait(&cond, &mut);

	pmarcel_cleanup_pop(0);

#ifdef VERBOSE_TESTS
	puts("child: cond_wait should not have returned");
#endif
	return NULL;
}


static void *tf2(void *p TBX_UNUSED)
{
	int err;

	if (pmarcel_setcancelstate(PMARCEL_CANCEL_ENABLE, NULL) != 0
	    || pmarcel_setcanceltype(PMARCEL_CANCEL_DEFERRED, NULL) != 0) {
		puts("cannot set cancellation options");
		exit(1);
	}

	err = pmarcel_mutex_lock(&mut);
	if (err != 0) {
		puts("child: cannot get mutex");
		exit(1);
	}

	err = pmarcel_barrier_wait(&bar);
	if (err != 0 && err != PMARCEL_BARRIER_SERIAL_THREAD) {
		printf("barrier_wait returned %d\n", err);
		exit(1);
	}
#ifdef VERBOSE_TESTS
	puts("child: got mutex; waiting");
#endif

	pmarcel_cleanup_push(ch, NULL);

	/* Current time.  */
	struct timeval tv;
	(void) gettimeofday(&tv, NULL);
	/* +1000 seconds in correct format.  */
	struct timespec ts;
	TIMEVAL_TO_TIMESPEC(&tv, &ts);
	ts.tv_sec += 1000;

	pmarcel_cond_timedwait(&cond, &mut, &ts);

	pmarcel_cleanup_pop(0);

#ifdef VERBOSE_TESTS
	puts("child: cond_wait should not have returned");
#endif
	return NULL;
}


static int do_test(void)
{
	pmarcel_t th;
	int err;

#ifdef VERBOSE_TESTS
	printf("&cond = %p\n&mut = %p\n", &cond, &mut);
	puts("parent: get mutex");
#endif

	err = pmarcel_barrier_init(&bar, NULL, 2);
	if (err != 0) {
		puts("parent: cannot init barrier");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	puts("parent: create child");
#endif

	err = pmarcel_create(&th, NULL, tf1, NULL);
	if (err != 0) {
		puts("parent: cannot create thread");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	puts("parent: wait for child to lock mutex");
#endif

	err = pmarcel_barrier_wait(&bar);
	if (err != 0 && err != PMARCEL_BARRIER_SERIAL_THREAD) {
		puts("parent: cannot wait for barrier");
		exit(1);
	}

	err = pmarcel_mutex_lock(&mut);
	if (err != 0) {
		puts("parent: mutex_lock failed");
		exit(1);
	}

	err = pmarcel_mutex_unlock(&mut);
	if (err != 0) {
		puts("parent: mutex_unlock failed");
		exit(1);
	}

	if (pmarcel_cancel(th) != 0) {
		puts("cannot cancel thread");
		exit(1);
	}

	void *r;
	err = pmarcel_join(th, &r);
	if (err != 0) {
		puts("parent: failed to join");
		exit(1);
	}

	if (r != PMARCEL_CANCELED) {
		puts("child hasn't been canceled");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	puts("parent: create 2nd child");
#endif
	err = pmarcel_create(&th, NULL, tf2, NULL);
	if (err != 0) {
		puts("parent: cannot create thread");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	puts("parent: wait for child to lock mutex");
#endif
	err = pmarcel_barrier_wait(&bar);
	if (err != 0 && err != PMARCEL_BARRIER_SERIAL_THREAD) {
		puts("parent: cannot wait for barrier");
		exit(1);
	}

	err = pmarcel_mutex_lock(&mut);
	if (err != 0) {
		puts("parent: mutex_lock failed");
		exit(1);
	}

	err = pmarcel_mutex_unlock(&mut);
	if (err != 0) {
		puts("parent: mutex_unlock failed");
		exit(1);
	}

	if (pmarcel_cancel(th) != 0) {
		puts("cannot cancel thread");
		exit(1);
	}

	err = pmarcel_join(th, &r);
	if (err != 0) {
		puts("parent: failed to join");
		exit(1);
	}

	if (r != PMARCEL_CANCELED) {
		puts("child hasn't been canceled");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	puts("done");
#endif
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
