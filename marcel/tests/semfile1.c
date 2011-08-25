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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <marcel.h>


#ifdef FREEBSD_SYS
static char *semname = "/tmp/semaphoretest";
#else
static char *semname = "/tmpsemaphoretest";
#endif


static int do_test(void)
{
	marcel_sem_t *s;

	marcel_sem_unlink(semname);
	s = marcel_sem_open(semname, O_CREAT | O_EXCL, S_IWUSR | S_IRUSR, 1);
	if (! s) {
		puts("init failed");
		return 1;
	}

	if (MA_FAILURE_RETRY(marcel_sem_wait(s)) == -1) {
		puts("1st wait failed");
		return 1;
	}

	if (marcel_sem_post(s) == -1) {
		puts("1st post failed");
		return 1;
	}

	if (MA_FAILURE_RETRY(marcel_sem_trywait(s)) == -1) {
		puts("1st trywait failed");
		return 1;
	}

	errno = 0;
	if (MA_FAILURE_RETRY(marcel_sem_trywait(s)) != -1) {
		puts("2nd trywait succeeded");
		return 1;
	} else if (errno != EAGAIN) {
		puts("2nd trywait did not set errno to EAGAIN");
		return 1;
	}

	if (marcel_sem_post(s) == -1) {
		puts("2nd post failed");
		return 1;
	}

	if (MA_FAILURE_RETRY(marcel_sem_wait(s)) == -1) {
		puts("2nd wait failed");
		return 1;
	}

	if (marcel_sem_close(s) == -1) {
		puts("destroy failed");
		return 1;
	}

	if (marcel_sem_unlink(semname) == -1) {
		puts("unlink failed");
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
