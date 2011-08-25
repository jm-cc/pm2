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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#if (defined(MARCEL_POSIX) || defined(MARCEL_LIBPTHREAD)) && defined(MARCEL_DEVIATION)


static pthread_barrier_t bar;
static int fd[2];


static void cleanup(void *arg TBX_UNUSED)
{
	static int ncall;

	if (++ncall != 1) {
		puts("second call to cleanup");
		exit(1);
	}

	printf("cleanup call #%d\n", ncall);
}


static void *tf(void *arg TBX_UNUSED)
{
	pthread_cleanup_push(cleanup, NULL);

	int e = pthread_barrier_wait(&bar);
	if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD) {
		puts("tf: 1st barrier_wait failed");
		exit(1);
	}

	/* This call should block and be cancelable.  */
	char buf[20];
	read(fd[0], buf, sizeof(buf));

	pthread_cleanup_pop(0);

	return NULL;
}


static int do_test(void)
{
	pthread_t th;

	if (pthread_barrier_init(&bar, NULL, 2) != 0) {
		puts("barrier_init failed");
		exit(1);
	}

	if (pipe(fd) != 0) {
		puts("pipe failed");
		exit(1);
	}

	if (pthread_create(&th, NULL, tf, NULL) != 0) {
		puts("create failed");
		exit(1);
	}

	int e = pthread_barrier_wait(&bar);
	if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD) {
		puts("1st barrier_wait failed");
		exit(1);
	}

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
