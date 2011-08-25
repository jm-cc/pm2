/* Copyright (C) 2003, 2004 Free Software Foundation, Inc.
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

#include <marcel.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>


#if (defined(MA__IFACE_PMARCEL) || defined(MARCEL_LIBPTHREAD)) && defined(MARCEL_ONCE_ENABLED)


static pmarcel_once_t once = PMARCEL_ONCE_INIT;

static pmarcel_cond_t cond = PMARCEL_COND_INITIALIZER;
static pmarcel_mutex_t mut = PMARCEL_MUTEX_INITIALIZER;

static pmarcel_barrier_t bar;

static int global;
static int cl_called;

static void once_handler1(void)
{
	if (pmarcel_mutex_lock(&mut) != 0) {
		puts("once_handler1: mutex_lock failed");
		exit(1);
	}

	int r = pmarcel_barrier_wait(&bar);
	if (r != 0 && r != PMARCEL_BARRIER_SERIAL_THREAD) {
		puts("once_handler1: barrier_wait failed");
		exit(1);
	}

	pmarcel_cond_wait(&cond, &mut);

	/* We should never get here.  */
}


static void once_handler2(void)
{
	global = 1;
}


static void cl(void *arg TBX_UNUSED)
{
	++cl_called;
}


static void *tf1(void *arg TBX_UNUSED)
{
	pmarcel_cleanup_push(cl, NULL);

	pmarcel_once(&once, once_handler1);

	pmarcel_cleanup_pop(0);

	/* We should never get here.  */
	puts("pmarcel_once in tf returned");
	exit(1);
}


static void *tf2(void *arg TBX_UNUSED)
{
	pmarcel_cleanup_push(cl, NULL);

	int r = pmarcel_barrier_wait(&bar);
	if (r != 0 && r != PMARCEL_BARRIER_SERIAL_THREAD) {
		puts("once_handler2: barrier_wait failed");
		exit(1);
	}

	pmarcel_cleanup_pop(0);

	pmarcel_once(&once, once_handler2);

	return NULL;
}


static int do_test(void)
{
	pmarcel_t th[2];

	if (pmarcel_barrier_init(&bar, NULL, 2) != 0) {
		puts("barrier_init failed");
		return 1;
	}

	if (pmarcel_create(&th[0], NULL, tf1, NULL) != 0) {
		puts("first create failed");
		return 1;
	}

	int r = pmarcel_barrier_wait(&bar);
	if (r != 0 && r != PMARCEL_BARRIER_SERIAL_THREAD) {
		puts("first barrier_wait failed");
		return 1;
	}

	if (pmarcel_mutex_lock(&mut) != 0) {
		puts("mutex_lock failed");
		return 1;
	}
	/* We unlock the mutex so that we catch the case where the pmarcel_cond_wait
	   call incorrectly resumes and tries to get the mutex.  */
	if (pmarcel_mutex_unlock(&mut) != 0) {
		puts("mutex_unlock failed");
		return 1;
	}

	if (pmarcel_create(&th[1], NULL, tf2, NULL) != 0) {
		puts("second create failed");
		return 1;
	}

	r = pmarcel_barrier_wait(&bar);
	if (r != 0 && r != PMARCEL_BARRIER_SERIAL_THREAD) {
		puts("second barrier_wait failed");
		return 1;
	}

	/* Give the second thread a chance to reach the pmarcel_once call.  */
	sleep(2);

	/* Cancel the thread.  */
	if (pmarcel_cancel(th[0]) != 0) {
		puts("cancel failed");
		return 1;
	}

	void *result;
	pmarcel_join(th[0], &result);
	if (result != PMARCEL_CANCELED) {
		puts("first join didn't return PMARCEL_CANCELED");
		return 1;
	}
#ifdef VERBOSE_TESTS
	puts("joined first thread");
#endif

	pmarcel_join(th[1], &result);
	if (result != NULL) {
		puts("second join didn't return PMARCEL_CANCELED");
		return 1;
	}

	if (global !=1) {
		puts("global still 0");
		return 1;
	}

	if (cl_called != 1) {
		printf("cl_called = %d\n", cl_called);
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
