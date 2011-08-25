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

#include <fcntl.h>
#include <marcel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#if defined(MARCEL_POSIX) && defined(MARCEL_DEVIATION)


static pmarcel_barrier_t b;


static void cleanup(void *arg TBX_UNUSED)
{
	fputs("in cleanup\n", stdout);
}


static void *tf(void *arg TBX_UNUSED)
{
	int fd = open("/dev/null", O_RDWR);
	if (fd == -1) {
		puts("cannot open /dev/null");
		exit(1);
	}
	FILE *fp = fdopen(fd, "w");
	if (fp == NULL) {
		puts("fdopen failed");
		exit(1);
	}

	pmarcel_cleanup_push(cleanup, NULL);

	int e = pmarcel_barrier_wait(&b);
	if (e != 0 && e != PMARCEL_BARRIER_SERIAL_THREAD) {
		puts("barrier_wait failed");
		exit(1);
	}

	while (1)
		/* fprintf() uses write() which is a cancallation point.  */
		fprintf(fp, "foo");

	pmarcel_cleanup_pop(0);

	return NULL;
}


static int do_test(void)
{
	if (pmarcel_barrier_init(&b, NULL, 2) != 0) {
		puts("barrier_init failed");
		exit(1);
	}

	pmarcel_t th;
	if (pmarcel_create(&th, NULL, tf, NULL) != 0) {
		puts("create failed");
		return 1;
	}

	int e = pmarcel_barrier_wait(&b);
	if (e != 0 && e != PMARCEL_BARRIER_SERIAL_THREAD) {
		puts("barrier_wait failed");
		exit(1);
	}

	sleep(1);

	puts("cancel now");

	if (pmarcel_cancel(th) != 0) {
		puts("cancel failed");
		exit(1);
	}

	puts("waiting for the child");

	void *r;
	if (pmarcel_join(th, &r) != 0) {
		puts("join failed");
		exit(1);
	}

	if (r != PMARCEL_CANCELED) {
		puts("thread wasn't canceled");
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
