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
#ifdef MARCEL_POSIX
#include <pthread/pthread.h>
#else
#include <marcel.h>
#endif
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#if (defined(MARCEL_POSIX) || defined(MARCEL_LIBPTHREAD)) && defined(MARCEL_DEVIATION)


static pthread_barrier_t bar;
static sem_t sem;


static void cleanup(void *arg TBX_UNUSED)
{
	static int ncall;

	if (++ncall != 1) {
		puts("second call to cleanup");
		exit(1);
	}

	printf("cleanup call #%d\n", ncall);
}


static void *tf1(void *arg TBX_UNUSED)
{
	pthread_cleanup_push(cleanup, NULL);

	int e = pthread_barrier_wait(&bar);
	if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD) {
		puts("tf: 1st barrier_wait failed");
		exit(1);
	}

	/* This call should block and be cancelable.  */
	sem_wait(&sem);

	pthread_cleanup_pop(0);

#ifdef VERBOSE_TESTS
	puts("sem_wait returned");
#endif
	return NULL;
}

static void *tf2(void *arg TBX_UNUSED)
{
	pthread_cleanup_push(cleanup, NULL);

	int e = pthread_barrier_wait(&bar);
	if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD) {
		puts("tf: 1st barrier_wait failed");
		exit(1);
	}

	/* This call should block and be cancelable.  */
	sem_trywait(&sem);

	pthread_cleanup_pop(0);

#ifdef VERBOSE_TESTS
	puts("sem_trywait returned");
#endif
	return NULL;
}

static void *tf3(void *arg TBX_UNUSED)
{
	struct timespec ts;

	pthread_cleanup_push(cleanup, NULL);

	int e = pthread_barrier_wait(&bar);
	if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD) {
		puts("tf: 1st barrier_wait failed");
		exit(1);
	}

	ts.tv_nsec = ts.tv_sec = 1000;
	/* This call should block and be cancelable.  */
	sem_timedwait(&sem, &ts);

	pthread_cleanup_pop(0);

#ifdef VERBOSE_TESTS
	puts("sem_timedwait returned");
#endif
	return NULL;
}


static int launch_testfunc(void *(*test_routine) (void *))
{
	pthread_t th;

	if (pthread_barrier_init(&bar, NULL, 2) != 0) {
		puts("barrier_init failed");
		exit(1);
	}

	if (sem_init(&sem, 0, 0) != 0) {
		puts("sem_init failed");
		exit(1);
	}

	if (pthread_create(&th, NULL, test_routine, NULL) != 0) {
		puts("create failed");
		exit(1);
	}

	int e = pthread_barrier_wait(&bar);
	if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD) {
		puts("1st barrier_wait failed");
		exit(1);
	}

	/* Give the child a chance to go to sleep in sem_wait.  */
	sleep(1);

	/* Check whether cancellation is honored when waiting in sem_wait.  */
	if (pthread_cancel(th) != 0) {
		puts("1st cancel failed");
		exit(1);
	}

	void *r;
	if (pthread_join(th, &r) != 0) {
		puts("join failed");
		exit(1);
	}

	if (r != PTHREAD_CANCELED) {
		puts("thread not canceled");
		exit(1);
	}

	sem_destroy(&sem);
	return 0;
}


static int do_test(void)
{
	if (0 != launch_testfunc(tf1))
		exit(1);

	if (0 != launch_testfunc(tf2))
		exit(1);

	if (0 != launch_testfunc(tf3))
		exit(1);

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
