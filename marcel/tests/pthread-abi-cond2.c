/* Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
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

#include "pthread-abi.h"


#if defined(MARCEL_LIBPTHREAD)


#include <stdio.h>
#include <stdlib.h>


static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static pthread_barrier_t bar;


static void *tf(void *a)
{
	int i = (long int) a;
	int err;
#ifdef VERBOSE_TESTS
	printf("child %d: lock\n", i);
#endif

	err = pthread_mutex_lock(&mut);
	if (err != 0)
		error(EXIT_FAILURE, err, "locking in child failed");
#ifdef VERBOSE_TESTS
	printf("child %d: sync\n", i);
#endif

	int e = pthread_barrier_wait(&bar);
	if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD) {
		puts("child: barrier_wait failed");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	printf("child %d: wait\n", i);
#endif

	err = pthread_cond_wait(&cond, &mut);
	if (err != 0)
		error(EXIT_FAILURE, err, "child %d: failed to wait", i);
#ifdef VERBOSE_TESTS
	printf("child %d: woken up\n", i);
#endif

	err = pthread_mutex_unlock(&mut);
	if (err != 0)
		error(EXIT_FAILURE, err, "child %d: unlock[2] failed", i);
#ifdef VERBOSE_TESTS
	printf("child %d: done\n", i);
#endif

	return NULL;
}


#define N 10


static int do_test(void)
{
	pthread_t th[N];
	int i;
	int err;

	//  printf ("&cond = %p\n&mut = %p\n", &cond, &mut);

	if (pthread_barrier_init(&bar, NULL, 2) != 0) {
		puts("barrier_init failed");
		exit(1);
	}

	for (i = 0; i < N; ++i) {
#ifdef VERBOSE_TESTS
		printf("create thread %d\n", i);
#endif

		err = pthread_create(&th[i], NULL, tf, (void *) (long int) i);
		if (err != 0)
			error(EXIT_FAILURE, err, "cannot create thread %d", i);
#ifdef VERBOSE_TESTS
		printf("wait for child %d\n", i);
#endif

		/* Wait for the child to start up and get the mutex for the
		   conditional variable.  */
		int e = pthread_barrier_wait(&bar);
		if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD) {
			puts("barrier_wait failed");
			exit(1);
		}
	}

#ifdef VERBOSE_TESTS
	puts("get lock outselves");
#endif
	err = pthread_mutex_lock(&mut);
	if (err != 0)
		error(EXIT_FAILURE, err, "mut locking failed");

#ifdef VERBOSE_TESTS
	puts("broadcast");
#endif
	/* Wake up all threads.  */
	err = pthread_cond_broadcast(&cond);
	if (err != 0)
		error(EXIT_FAILURE, err, "parent: broadcast failed");

	err = pthread_mutex_unlock(&mut);
	if (err != 0)
		error(EXIT_FAILURE, err, "mut unlocking failed");

	/* Join all threads.  */
	for (i = 0; i < N; ++i) {
#ifdef VERBOSE_TESTS
		printf("join thread %d\n", i);
#endif

		err = pthread_join(th[i], NULL);
		if (err != 0)
			error(EXIT_FAILURE, err, "join of child %d failed", i);
	}

#ifdef VERBOSE_TESTS
	puts("done");
#endif
	return 0;
}



int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	assert_running_marcel();
	return do_test();
}


#else


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
