/* Copyright (C) 2002, 2003 Free Software Foundation, Inc.
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


#include "pthread-abi.h"


#if defined(MARCEL_LIBPTHREAD)


#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>


static int kind[] = {
	PTHREAD_RWLOCK_PREFER_READER_NP,
	PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP,
	PTHREAD_RWLOCK_PREFER_WRITER_NP,
};


static void *tf(void *arg)
{
	pthread_rwlock_t *r = arg;

	/* Timeout: 0.3 secs.  */
	struct timeval tv;
	(void) gettimeofday(&tv, NULL);

	struct timespec ts;
	TIMEVAL_TO_TIMESPEC(&tv, &ts);
	ts.tv_nsec += 300000000;
	if (ts.tv_nsec >= 1000000000) {
		ts.tv_nsec -= 1000000000;
		++ts.tv_sec;
	}
#ifdef VERBOSE_TESTS
	puts("child calling timedrdlock");
#endif
	int err = pthread_rwlock_timedrdlock(r, &ts);
	if (err == 0) {
		puts("rwlock_timedrdlock returned");
		pthread_exit((void *) 1l);
	}

	if (err != ETIMEDOUT) {
		printf("err = %s (%d), expected %s (%d)\n",
		       strerror(err), err, strerror(ETIMEDOUT), ETIMEDOUT);
		pthread_exit((void *) 1l);
	}
#ifdef VERBOSE_TESTS
	puts("1st child timedrdlock done");
#endif

	struct timeval tv2;
	(void) gettimeofday(&tv2, NULL);

	timersub(&tv2, &tv, &tv);

	if (tv.tv_usec < 200000) {
		printf
		    ("timeout %lu.%06lu too short (expected at least %lu.%06lu)",
		     (unsigned long) tv.tv_sec, (unsigned long) tv.tv_usec,
		     0UL, 200000UL);
		pthread_exit((void *) 1l);
	}

	(void) gettimeofday(&tv, NULL);
	TIMEVAL_TO_TIMESPEC(&tv, &ts);
	ts.tv_sec += 10;
	/* Note that the following operation makes ts invalid.  */
	ts.tv_nsec += 1000000000;

	err = pthread_rwlock_timedrdlock(r, &ts);
	if (err == 0) {
		puts("2nd timedrdlock succeeded");
		pthread_exit((void *) 1l);
	}
	if (err != EINVAL) {
		puts("2nd timedrdlock did not return EINVAL");
		pthread_exit((void *) 1l);
	}
#ifdef VERBOSE_TESTS
	puts("2nd child timedrdlock done");
#endif

	return NULL;
}


static int do_test(void)
{
	size_t cnt;
	for (cnt = 0; cnt < sizeof(kind) / sizeof(kind[0]); ++cnt) {
		pthread_rwlock_t r;
		pthread_rwlockattr_t a;

		if (pthread_rwlockattr_init(&a) != 0) {
			printf("round %Zu: rwlockattr_t failed\n", cnt);
			exit(1);
		}

		if (pthread_rwlockattr_setkind_np(&a, kind[cnt]) != 0) {
			printf("round %Zu: rwlockattr_setkind failed\n", cnt);
			exit(1);
		}

		if (pthread_rwlock_init(&r, &a) != 0) {
			printf("round %Zu: rwlock_init failed\n", cnt);
			exit(1);
		}

		if (pthread_rwlockattr_destroy(&a) != 0) {
			printf("round %Zu: rwlockattr_destroy failed\n", cnt);
			exit(1);
		}

		struct timeval tv;
		(void) gettimeofday(&tv, NULL);

		struct timespec ts;
		TIMEVAL_TO_TIMESPEC(&tv, &ts);

		++ts.tv_sec;

		/* Get a write lock.  */
		int e = pthread_rwlock_timedwrlock(&r, &ts);
		if (e != 0) {
			printf("round %Zu: rwlock_timedwrlock failed (%d)\n", cnt, e);
			exit(1);
		}
#ifdef VERBOSE_TESTS
		puts("1st timedwrlock done");
#endif

		(void) gettimeofday(&tv, NULL);
		TIMEVAL_TO_TIMESPEC(&tv, &ts);
		++ts.tv_sec;
		e = pthread_rwlock_timedrdlock(&r, &ts);
		if (e == 0) {
			puts("timedrdlock succeeded");
			exit(1);
		}
		if (e != EDEADLK) {
			puts("timedrdlock did not return EDEADLK");
			exit(1);
		}
#ifdef VERBOSE_TESTS
		puts("1st timedrdlock done");
#endif

		(void) gettimeofday(&tv, NULL);
		TIMEVAL_TO_TIMESPEC(&tv, &ts);
		++ts.tv_sec;
		e = pthread_rwlock_timedwrlock(&r, &ts);
		if (e == 0) {
			puts("2nd timedwrlock succeeded");
			exit(1);
		}
		if (e != EDEADLK) {
			puts("2nd timedwrlock did not return EDEADLK");
			exit(1);
		}
#ifdef VERBOSE_TESTS
		puts("2nd timedwrlock done");
#endif

		pthread_t th;
		if (pthread_create(&th, NULL, tf, &r) != 0) {
			printf("round %Zu: create failed\n", cnt);
			exit(1);
		}
#ifdef VERBOSE_TESTS
		puts("started thread");
#endif

		void *status;
		if (pthread_join(th, &status) != 0) {
			printf("round %Zu: join failed\n", cnt);
			exit(1);
		}
		if (status != NULL) {
			printf("failure in round %Zu\n", cnt);
			exit(1);
		}
#ifdef VERBOSE_TESTS
		puts("joined thread");
#endif

		if (pthread_rwlock_destroy(&r) != 0) {
			printf("round %Zu: rwlock_destroy failed\n", cnt);
			exit(1);
		}
	}

	return MARCEL_TEST_SUCCEEDED;
}


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	assert_running_marcel();
	return do_test();
}


#else


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
