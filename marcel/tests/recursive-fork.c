/* Copyright (C) 2002, 2004 Free Software Foundation, Inc.
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
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


#if (defined(MA__IFACE_PMARCEL) || defined(MARCEL_LIBPTHREAD)) && defined(MARCEL_FORK_ENABLED)


static void *thread_function(void *arg)
{
	int i = (intptr_t) arg;
	int status;
	pid_t pid, pid2, pid3;
	struct timespec ts;

	pid = marcel_fork();
	switch (pid) {
	case 0:
		pid = getpid();
#ifdef VERBOSE_TESTS
		marcel_printf("child: %ld for %d\n", pid, i);
#endif
		ts.tv_sec = 0;
		ts.tv_nsec = 100000000 * i;
		marcel_nanosleep(&ts, NULL);

		pid2 = marcel_fork();
		if (0 == pid2) {
#ifdef VERBOSE_TESTS
			marcel_printf("child of %ld: %ld for %d\n", pid, (long int) getpid(), i);
#endif
			exit(i);
		}

		pid3 = MA_FAILURE_RETRY(marcel_waitpid(pid2, &status, 0));
		if (pid2 == pid3)
			exit(WEXITSTATUS(status));

		exit(-1);
		break;
	case -1:
		marcel_printf("fork: %m\n");
		return (void *) 1l;
		break;
	}

	pid2 = MA_FAILURE_RETRY(marcel_waitpid(pid, &status, 0));
	if (pid2 != pid) {
		marcel_printf("waitpid returned %ld, expected %ld\n",
			      (long int) pid2, (long int) pid);
		return (void *) 1l;
	}
#ifdef VERBOSE_TESTS
	marcel_printf("%ld with %d, expected %d\n", (long int) pid, WEXITSTATUS(status), i);
#endif

	return WEXITSTATUS(status) == i ? NULL : (void *) 1l;
}

#define N 50
static int t[N];

static int do_test()
{
	pmarcel_t th[N];
	int i;
	int result = 0;

	for (i = 0; i < N; i ++)
		t[i] = i;

	for (i = 0; i < N; ++i)
		if (pmarcel_create(&th[i], NULL, thread_function,
				   (void *) (intptr_t) t[i]) != 0) {
			marcel_printf("creation of thread %d failed\n", i);
			exit(1);
		}

	for (i = 0; i < N; ++i) {
		void *v;
		if (pmarcel_join(th[i], &v) != 0) {
			marcel_printf("join of thread %d failed\n", i);
			result = 1;
		} else if (v != NULL) {
			marcel_printf("join %d successful, but child failed\n", i);
			result = 1;
		}
#ifdef VERBOSE_TESTS
		else
			marcel_printf("join %d successful\n", i);
#endif
	}

	return result;
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
