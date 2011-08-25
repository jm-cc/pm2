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

#include <errno.h>
#include <marcel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>


#if !defined(MARCEL_MONO) && defined(MARCEL_POSIX) && defined(MARCEL_FORK_ENABLED)


static int do_test(void)
{
	size_t ps = sysconf(_SC_PAGESIZE);
	char tmpfname[] = "/tmp/tst-spin2.XXXXXX";
	char data[ps];
	void *mem;
	int fd;
	pmarcel_spinlock_t *s;
	pid_t pid;
	char *p;
	int err;

	fd = mkstemp(tmpfname);
	if (fd == -1) {
		printf("cannot open temporary file: %m\n");
		return 1;
	}

	/* Make sure it is always removed.  */
	unlink(tmpfname);

	/* Create one page of data.  */
	memset(data, '\0', ps);

	/* Write the data to the file.  */
	if (write(fd, data, ps) != (ssize_t) ps) {
		marcel_puts("short write");
		return 1;
	}

	mem = marcel_mmap(NULL, ps, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (mem == MAP_FAILED) {
		marcel_printf("mmap failed: %m\n");
		return 1;
	}

	s = (pmarcel_spinlock_t *) (((uintptr_t) mem + __alignof(pmarcel_spinlock_t))
				    & ~(__alignof(pmarcel_spinlock_t) - 1));
	p = (char *) (s + 1);

	if (pmarcel_spin_init(s, PMARCEL_PROCESS_SHARED) != 0) {
		marcel_puts("spin_init failed");
		return 1;
	}

	if (pmarcel_spin_lock(s) != 0) {
		marcel_puts("spin_lock failed");
		return 1;
	}

	err = pmarcel_spin_trylock(s);
	if (err == 0) {
		marcel_puts("1st spin_trylock succeeded");
		return 1;
	} else if (err != EBUSY) {
		marcel_puts("1st spin_trylock didn't return EBUSY");
		return 1;
	}

	err = pmarcel_spin_unlock(s);
	if (err != 0) {
		marcel_puts("parent: spin_unlock failed");
		return 1;
	}

	err = pmarcel_spin_trylock(s);
	if (err != 0) {
		marcel_puts("2nd spin_trylock failed");
		return 1;
	}

	*p = 0;

#ifdef VERBOSE_TESTS
	marcel_puts("going to fork now");
#endif
	pid = pmarcel_fork();
	if (pid == -1) {
		marcel_puts("fork failed");
		return 1;
	} else if (pid == 0) {
		/* Play some lock ping-pong.  It's our turn to unlock first.  */
		if ((*p)++ != 0) {
			marcel_puts("child: *p != 0");
			return 1;
		}

		if (pmarcel_spin_unlock(s) != 0) {
			marcel_puts("child: 1st spin_unlock failed");
			return 1;
		}
#ifdef VERBOSE_TESTS
		marcel_puts("child done");
#endif
		exit(0);	// child must not call marcel_end()
	} else {
		if (pmarcel_spin_lock(s) != 0) {
			marcel_puts("parent: 2nd spin_lock failed");
			return 1;
		}
#ifdef VERBOSE_TESTS
		marcel_puts("waiting for child");
#endif
		marcel_waitpid(pid, NULL, 0);

#ifdef VERBOSE_TESTS
		marcel_puts("parent done");
#endif
	}

	return MARCEL_TEST_SUCCEEDED;
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
