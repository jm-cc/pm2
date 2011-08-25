/* Copyright (C) 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Jakub Jelinek <jakub@redhat.com>, 2003.

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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>


#if defined(MARCEL_POSIX) && defined(MARCEL_DEVIATION)


static void *tf(void *arg TBX_UNUSED)
{
	return NULL;
}

static void handler(int sig TBX_UNUSED)
{
}

static void __attribute__ ((noinline))
clobber_lots_of_regs(void)
{
#define X1(n) long r##n = 10##n; __asm __volatile ("" : "+r" (r##n));
#define X2(n) X1(n##0) X1(n##1) X1(n##2) X1(n##3) X1(n##4)
#define X3(n) X2(n##0) X2(n##1) X2(n##2) X2(n##3) X2(n##4)
	X3(0) X3(1) X3(2) X3(3) X3(4)
#undef X1
#define X1(n) __asm __volatile ("" : : "r" (r##n));
		X3(0) X3(1) X3(2) X3(3) X3(4)
#undef X1
#undef X2
#undef X3
			}

static int do_test(void)
{
	pmarcel_t th;
	int old, rc;
	int ret = 0;
	int fd[2];

	rc = pipe(fd);
	if (rc < 0) {
		marcel_fprintf(stderr, "couldn't create pipe\n");
		return EXIT_FAILURE;
	}

	rc = pmarcel_create(&th, NULL, tf, NULL);
	if (rc) {
		marcel_fprintf(stderr, "couldn't create thread");
		return EXIT_FAILURE;
	}

	rc = pmarcel_setcanceltype(PMARCEL_CANCEL_DEFERRED, &old);
	if (rc) {
		error(0, rc, "1st pmarcel_setcanceltype failed");
		ret = 1;
	}
	if (old != PMARCEL_CANCEL_DEFERRED && old != PMARCEL_CANCEL_ASYNCHRONOUS) {
		marcel_fprintf(stderr, "1st pmarcel_setcanceltype returned invalid value %d\n", old);
		ret = 1;
	}

	clobber_lots_of_regs();
	close(fd[0]);

	rc = pmarcel_setcanceltype(PMARCEL_CANCEL_ASYNCHRONOUS, &old);
	if (rc) {
		marcel_fprintf(stderr, "pmarcel_setcanceltype after close failed\n");
		ret = 1;
	}
	if (old != PMARCEL_CANCEL_DEFERRED) {
		marcel_fprintf(stderr,
			       "pmarcel_setcanceltype after close returned invalid value %d\n", old);
		ret = 1;
	}

	clobber_lots_of_regs();
	close(fd[1]);

	rc = pmarcel_setcanceltype(PMARCEL_CANCEL_DEFERRED, &old);
	if (rc) {
		marcel_fprintf(stderr, "pmarcel_setcanceltype after 2nd close failed\n");
		ret = 1;
	}
	if (old != PMARCEL_CANCEL_ASYNCHRONOUS) {
		marcel_fprintf(stderr,
			       "pmarcel_setcanceltype after 2nd close returned invalid value %d\n",
			       old);
		ret = 1;
	}

	struct sigaction sa = {.sa_handler = handler,.sa_flags = 0 };
	sigemptyset(&sa.sa_mask);
	sigaction(SIGALRM, &sa, NULL);

	struct itimerval it;
	it.it_value.tv_sec = 1;
	it.it_value.tv_usec = 0;
	it.it_interval = it.it_value;
	setitimer(ITIMER_REAL, &it, NULL);

	clobber_lots_of_regs();
	pause();

	memset(&it, 0, sizeof(it));
	setitimer(ITIMER_REAL, &it, NULL);

	rc = pmarcel_setcanceltype(PMARCEL_CANCEL_ASYNCHRONOUS, &old);
	if (rc) {
		marcel_fprintf(stderr, "pmarcel_setcanceltype after pause failed\n");
		ret = 1;
	}
	if (old != PMARCEL_CANCEL_DEFERRED) {
		marcel_fprintf(stderr,
			       "pmarcel_setcanceltype after pause returned invalid value %d\n", old);
		ret = 1;
	}

	it.it_value.tv_sec = 1;
	it.it_value.tv_usec = 0;
	it.it_interval = it.it_value;
	setitimer(ITIMER_REAL, &it, NULL);

	clobber_lots_of_regs();
	pause();

	memset(&it, 0, sizeof(it));
	setitimer(ITIMER_REAL, &it, NULL);

	rc = pmarcel_setcanceltype(PMARCEL_CANCEL_DEFERRED, &old);
	if (rc) {
		marcel_fprintf(stderr, "pmarcel_setcanceltype after 2nd pause failed\n");
		ret = 1;
	}
	if (old != PMARCEL_CANCEL_ASYNCHRONOUS) {
		marcel_fprintf(stderr,
			       "pmarcel_setcanceltype after 2nd pause returned invalid value %d\n",
			       old);
		ret = 1;
	}

	char fname[] = "/tmp/tst-cancel19-dir-XXXXXX\0foo/bar";
	char *enddir = strchr(fname, '\0');
	if (mkdtemp(fname) == NULL) {
		marcel_fprintf(stderr, "mkdtemp failed\n");
		ret = 1;
	}
	*enddir = '/';

	clobber_lots_of_regs();
	creat(fname, 0400);

	rc = pmarcel_setcanceltype(PMARCEL_CANCEL_ASYNCHRONOUS, &old);
	if (rc) {
		marcel_fprintf(stderr, "pmarcel_setcanceltype after creat failed\n");
		ret = 1;
	}
	if (old != PMARCEL_CANCEL_DEFERRED) {
		marcel_fprintf(stderr,
			       "pmarcel_setcanceltype after creat returned invalid value %d\n", old);
		ret = 1;
	}

	clobber_lots_of_regs();
	creat(fname, 0400);

	rc = pmarcel_setcanceltype(PMARCEL_CANCEL_DEFERRED, &old);
	if (rc) {
		marcel_fprintf(stderr, "pmarcel_setcanceltype after 2nd creat failed\n");
		ret = 1;
	}
	if (old != PMARCEL_CANCEL_ASYNCHRONOUS) {
		marcel_fprintf(stderr,
			       "pmarcel_setcanceltype after 2nd creat returned invalid value %d\n",
			       old);
		ret = 1;
	}

	clobber_lots_of_regs();
	open(fname, O_CREAT, 0400);

	rc = pmarcel_setcanceltype(PMARCEL_CANCEL_ASYNCHRONOUS, &old);
	if (rc) {
		fprintf(stderr,"pmarcel_setcanceltype after open failed\n");
		ret = 1;
	}
	if (old != PMARCEL_CANCEL_DEFERRED) {
		fprintf(stderr,
			"pmarcel_setcanceltype after open returned invalid value %d\n", old);
		ret = 1;
	}

	clobber_lots_of_regs();
	open(fname, O_CREAT, 0400);

	rc = pmarcel_setcanceltype(PMARCEL_CANCEL_DEFERRED, &old);
	if (rc) {
		fprintf(stderr, "pmarcel_setcanceltype after 2nd open failed\n");
		ret = 1;
	}
	if (old != PMARCEL_CANCEL_ASYNCHRONOUS) {
		marcel_fprintf(stderr,
			       "pmarcel_setcanceltype after 2nd open returned invalid value %d\n",
			       old);
		ret = 1;
	}

	*enddir = '\0';
	rmdir(fname);

	clobber_lots_of_regs();
	select(-1, NULL, NULL, NULL, NULL);

	rc = pmarcel_setcanceltype(PMARCEL_CANCEL_ASYNCHRONOUS, &old);
	if (rc) {
		marcel_fprintf(stderr, "pmarcel_setcanceltype after select failed\n");
		ret = 1;
	}
	if (old != PMARCEL_CANCEL_DEFERRED) {
		marcel_fprintf(stderr,
			       "pmarcel_setcanceltype after select returned invalid value %d\n",
			       old);
		ret = 1;
	}

	clobber_lots_of_regs();
	select(-1, NULL, NULL, NULL, NULL);

	rc = pmarcel_setcanceltype(PMARCEL_CANCEL_DEFERRED, &old);
	if (rc) {
		marcel_fprintf(stderr, "pmarcel_setcanceltype after 2nd select failed\n");
		ret = 1;
	}
	if (old != PMARCEL_CANCEL_ASYNCHRONOUS) {
		marcel_fprintf(stderr,
			       "pmarcel_setcanceltype after 2nd select returned invalid value %d\n",
			       old);
		ret = 1;
	}

	pmarcel_join(th, NULL);

	return ret;
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
