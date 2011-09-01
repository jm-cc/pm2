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

#ifdef MARCEL_POSIX
#include <pthread/pthread.h>
#else
#include <marcel.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#if (defined(MARCEL_POSIX) || defined(MARCEL_LIBPTHREAD)) && defined(MARCEL_DEVIATION)


static void *tf(void *arg)
{
	char buf[100];
	fgets(buf, sizeof(buf), arg);
	/* This call should never return.  */
	return NULL;
}


static int do_test(void)
{
	int fd[2];
	if (pipe(fd) != 0) {
		puts("pipe failed");
		return 1;
	}

	FILE *fp = fdopen(fd[0], "r");
	if (fp == NULL) {
		puts("fdopen failed");
		return 1;
	}

	pthread_t th;
	if (pthread_create(&th, NULL, tf, fp) != 0) {
		puts("pthread_create failed");
		return 1;
	}

	sleep(1);

	if (pthread_cancel(th) != 0) {
		puts("pthread_cancel failed");
		return 1;
	}

	void *r;
	if (pthread_join(th, &r) != 0) {
		puts("pthread_join failed");
		return 1;
	}

	return r != PTHREAD_CANCELED;
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