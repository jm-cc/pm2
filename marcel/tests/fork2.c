/* Copyright (C) 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Roland McGrath <roland@redhat.com>, 2002.

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
#include <unistd.h>
#include <sys/wait.h>


#ifdef MARCEL_FORK_ENABLED


static pid_t initial_pid;


static void *tf(void *arg TBX_UNUSED)
{
	if (getppid() != initial_pid) {
		marcel_printf("getppid in thread returned %ld, expected %ld\n",
			       (long int) getppid(), (long int) initial_pid);
		return (void *) -1;
	}

	return NULL;
}


static int do_test()
{
	initial_pid = getpid();

	pid_t child = marcel_fork();
	if (child == 0) {
		if (getppid() != initial_pid) {
			marcel_printf("first getppid returned %ld, expected %ld\n",
				      (long int) getppid(), (long int) initial_pid);
			exit(1);
		}

		marcel_t th;
		if (marcel_create(&th, NULL, tf, NULL) != 0) {
			marcel_puts("marcel_create failed");
			exit(1);
		}

		void *result;
		if (marcel_join(th, &result) != 0) {
			marcel_puts("marcel_join failed");
			exit(1);
		}

		exit(result == NULL ? 0 : 1);
	} else if (child == -1) {
		marcel_puts("initial fork failed");
		return 1;
	}

	int status;
	if (MA_FAILURE_RETRY(marcel_waitpid(child, &status, 0)) != child) {
		marcel_printf("waitpid failed: %m\n");
		return 1;
	}

	return status;
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
