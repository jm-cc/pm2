/* Thread with running thread calls exec.
   Copyright (C) 2002 Free Software Foundation, Inc.
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
#include <paths.h>
#include <marcel.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>


#if defined(MARCEL_LIBPTHREAD) && defined(MARCEL_FORK_ENABLED)


#ifndef CHILD_EXEC_CALL
#define CHILD_EXEC_CALL execl(_PATH_BSHELL, _PATH_BSHELL, "-c", "echo ok", NULL);
#endif


static void *tf(void *arg)
{
	pthread_t th = (pthread_t) arg;

	if (pthread_join(th, NULL) == 0) {
		puts("thread in parent joined!?");
		exit(1);
	}

	puts("join in thread in parent returned!?");
	exit(1);
}


static int do_test(void)
{
	int fd[2];
	if (pipe(fd) != 0) {
		puts("pipe failed");
		exit(1);
	}

	/* Not interested in knowing when the pipe is closed.  */
	if (sigignore(SIGPIPE) != 0) {
		puts("sigignore failed");
		exit(1);
	}

	pid_t pid = fork();
	if (pid == -1) {
		puts("fork failed");
		exit(1);
	}

	if (pid == 0) {
		/* Use the fd for stdout.  This is kind of ugly because it
		   substitutes the fd of stdout but we know what we are doing
		   here...  */
		if (dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO) {
			puts("dup2 failed");
			exit(1);
		}

		close(fd[0]);

		pthread_t th;
		if (pthread_create(&th, NULL, tf, (void *) pthread_self()) != 0) {
			puts("create failed");
			exit(1);
		}

		CHILD_EXEC_CALL

		puts("child exec failed");
		exit(1);
	}

	close(fd[1]);

	char buf[200];
	int result = EXIT_FAILURE;
	while (MA_FAILURE_RETRY(read(fd[0], buf, sizeof(buf))) > 0) {
		if (! strncmp(buf, "ok", 2))
			result = MARCEL_TEST_SUCCEEDED;
	}

	close(fd[0]);

	int status;
	int err = waitpid(pid, &status, 0);
	if (err != pid) {
		puts("waitpid failed");
		exit(1);
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
