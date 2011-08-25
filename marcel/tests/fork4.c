/* Test of fork updating child universe's pthread structures.
   Copyright (C) 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>


#if defined(MARCEL_SIGNALS_ENABLED) && defined(MARCEL_FORK_ENABLED)


static int do_test(void)
{
	marcel_t me = marcel_self();
	pid_t pid = marcel_fork();

	if (pid < 0) {
		marcel_printf("fork: %m\n");
		return 1;
	}

	if (pid == 0) {
		int err = marcel_kill(me, SIGTERM);
		marcel_printf("marcel_kill returned: %s\n", strerror(err));
		return 3;
	}

	int status;
	errno = 0;
	if (marcel_wait(&status) != pid)
		marcel_printf("wait failed: %m\n");
	else if (WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM) {
#ifdef VERBOSE_TESTS
		marcel_printf("child correctly died with SIGTERM\n");
#endif
		return 0;
	} else
		marcel_printf("child died with bad status %#x\n", status);

	return 1;
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
