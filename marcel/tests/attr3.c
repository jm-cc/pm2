/* pthread_getattr_np test.
   Copyright (C) 2003, 2004 Free Software Foundation, Inc.
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


#include <marcel.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#ifdef MARCEL_POSIX


static void *tf(void *arg)
{
	pmarcel_attr_t a, *ap, a2;
	int err;
	void *result = NULL;

	if (arg == NULL) {
		ap = &a2;
		err = pmarcel_attr_init(ap);
		if (err) {
			marcel_fprintf(stderr, "pmarcel_attr_init failed\n");
			return tf;
		}
	} else
		ap = (pmarcel_attr_t *) arg;

	err = pmarcel_getattr_np(pmarcel_self(), &a);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_getattr_np failed\n");
		result = tf;
	}

	int detachstate1, detachstate2;
	err = pmarcel_attr_getdetachstate(&a, &detachstate1);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_attr_getdetachstate failed\n");
		result = tf;
	} else {
		err = pmarcel_attr_getdetachstate(ap, &detachstate2);
		if (err) {
			marcel_fprintf(stderr, "pmarcel_attr_getdetachstate failed\n");
			result = tf;
		} else if (detachstate1 != detachstate2) {
			marcel_fprintf(stderr, "detachstate differs %d != %d\n",
				       detachstate1, detachstate2);
			result = tf;
		}
	}

	void *stackaddr;
	size_t stacksize;
	err = pmarcel_attr_getstack(&a, &stackaddr, &stacksize);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_attr_getstack failed\n");
		result = tf;
	} else if ((char *) &a < (char *)stackaddr || (char *) &a >= (char *)stackaddr + stacksize) {
		marcel_fprintf(stderr,
			       "pmarcel_attr_getstack returned range does not cover thread's stack\n");
		result = tf;
	}
#ifdef VERBOSE_TESTS
	else
		printf("thread stack %p-%p (0x%zx)\n", stackaddr,
		       stackaddr + stacksize, stacksize);
#endif

	size_t guardsize1, guardsize2;
	err = pmarcel_attr_getguardsize(&a, &guardsize1);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_attr_getguardsize failed\n");
		result = tf;
	} else {
		err = pmarcel_attr_getguardsize(ap, &guardsize2);
		if (err) {
			marcel_fprintf(stderr, "pmarcel_attr_getguardsize failed\n");
			result = tf;
		} else if (guardsize1 != guardsize2) {
			marcel_fprintf(stderr, "guardsize differs %zd != %zd\n",
				       guardsize1, guardsize2);
			result = tf;
		}
#ifdef VERBOSE_TESTS
		else
			printf("thread guardsize %zd\n", guardsize1);
#endif
	}

	int scope1, scope2;
	err = pmarcel_attr_getscope(&a, &scope1);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_attr_getscope failed\n");
		result = tf;
	} else {
		err = pmarcel_attr_getscope(ap, &scope2);
		if (err) {
			marcel_fprintf(stderr, "pmarcel_attr_getscope failed\n");
			result = tf;
		} else if (scope1 != scope2) {
			marcel_fprintf(stderr, "scope differs %d != %d\n", scope1, scope2);
			result = tf;
		}
	}

	int inheritsched1, inheritsched2;
	err = pmarcel_attr_getinheritsched(&a, &inheritsched1);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_attr_getinheritsched failed\n");
		result = tf;
	} else {
		err = pmarcel_attr_getinheritsched(ap, &inheritsched2);
		if (err) {
			marcel_fprintf(stderr, "pmarcel_attr_getinheritsched failed\n");
			result = tf;
		} else if (inheritsched1 != inheritsched2) {
			marcel_fprintf(stderr, "inheritsched differs %d != %d\n",
				       inheritsched1, inheritsched2);
			result = tf;
		}
	}

	pmarcel_cpu_set_t c1, c2;
	err = pmarcel_getaffinity_np(pmarcel_self(), sizeof(c1), &c1);
	if (err == 0) {
		err = pmarcel_attr_getaffinity_np(&a, sizeof(c2), &c2);
		if (err) {
			marcel_fprintf(stderr, "pmarcel_attr_getaffinity_np failed\n");
			result = tf;
		} else if (memcmp(&c1, &c2, sizeof(c1))) {
			marcel_fprintf(stderr,
				       "pmarcel_attr_getaffinity_np returned different CPU mask than pmarcel_getattr_np\n");
			result = tf;
		}
	}

	err = pmarcel_attr_destroy(&a);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_attr_destroy failed\n");
		result = tf;
	}

	if (ap == &a2) {
		err = pmarcel_attr_destroy(ap);
		if (err) {
			marcel_fprintf(stderr, "pmarcel_attr_destroy failed\n");
			result = tf;
		}
	}

	return result;
}


static int do_test(void)
{
	int result = 0;
	pmarcel_attr_t a;
	pmarcel_cpu_set_t c1, c2;

	int err = pmarcel_attr_init(&a);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_attr_init failed\n");
		result = 1;
	}

#ifdef __GLIBC__
	err = pmarcel_attr_getaffinity_np(&a, sizeof(c1), &c1);
	if (err && err != ENOSYS) {
		marcel_fprintf(stderr, "pmarcel_attr_getaffinity_np failed\n");
		result = 1;
	}
#endif

	err = pmarcel_attr_destroy(&a);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_attr_destroy failed\n");
		result = 1;
	}

	err = pmarcel_getattr_np(pmarcel_self(), &a);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_getattr_np failed\n");
		result = 1;
	}

	int detachstate;
	err = pmarcel_attr_getdetachstate(&a, &detachstate);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_attr_getdetachstate failed\n");
		result = 1;
	} else if (detachstate != PMARCEL_CREATE_JOINABLE) {
		marcel_fprintf(stderr, "initial thread not joinable\n");
		result = 1;
	}

	void *stackaddr;
	size_t stacksize;
	err = pmarcel_attr_getstack(&a, &stackaddr, &stacksize);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_attr_getstack failed\n");
		result = 1;
	} else if ((char *) &a < (char *)stackaddr || (char *) &a >= (char *)stackaddr + stacksize) {
		printf("thread stack %p-%p %p (0x%zx)\n", stackaddr,
		       (char *)stackaddr + stacksize, &a, stacksize);
		marcel_fprintf(stderr,
			       "pmarcel_attr_getstack returned range does not cover main's stack\n");
		result = 1;
	}
#ifdef VERBOSE_TESTS
	else
		printf("initial thread stack %p-%p (0x%zx)\n", stackaddr,
		       (char *)stackaddr + stacksize, stacksize);
#endif

	size_t guardsize;
	err = pmarcel_attr_getguardsize(&a, &guardsize);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_attr_getguardsize failed\n");
		result = 1;
	} else if (guardsize != 0) {
		marcel_fprintf(stderr, "pmarcel_attr_getguardsize returned %zd != 0\n", guardsize);
		result = 1;
	}

	int inheritsched;
	err = pmarcel_attr_getinheritsched(&a, &inheritsched);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_attr_getinheritsched failed\n");
		result = 1;
	} else if (inheritsched != PMARCEL_INHERIT_SCHED) {
		marcel_fprintf(stderr, 
			       "pmarcel_attr_getinheritsched returned %d != PMARCEL_INHERIT_SCHED\n",
			       inheritsched);
		result = 1;
	}

	err = pmarcel_getaffinity_np(pmarcel_self(), sizeof(c1), &c1);
	if (err == 0) {
		err = pmarcel_attr_getaffinity_np(&a, sizeof(c2), &c2);
		if (err) {
			marcel_fprintf(stderr, "pmarcel_attr_getaffinity_np failed\n");
			result = 1;
		} else if (memcmp(&c1, &c2, sizeof(c1))) {
			marcel_fprintf(stderr,
				       "pmarcel_attr_getaffinity_np returned different CPU mask than pmarcel_getattr_np\n");
			result = 1;
		}
	}

	err = pmarcel_attr_destroy(&a);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_attr_destroy failed\n");
		result = 1;
	}

	pmarcel_t th;
	err = pmarcel_create(&th, NULL, tf, NULL);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_create #1 failed\n");
		result = 1;
	} else {
		void *ret;
		err = pmarcel_join(th, &ret);
		if (err) {
			marcel_fprintf(stderr, "pmarcel_join #1 failed\n");
			result = 1;
		} else if (ret != NULL)
			result = 1;
	}

	err = pmarcel_attr_init(&a);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_attr_init failed\n");
		result = 1;
	}

	err = pmarcel_create(&th, &a, tf, &a);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_create #2 failed\n");
		result = 1;
	} else {
		void *ret;
		err = pmarcel_join(th, &ret);
		if (err) {
			marcel_fprintf(stderr, "pmarcel_join #2 failed\n");
			result = 1;
		} else if (ret != NULL)
			result = 1;
	}

	err = pmarcel_create(&th, &a, tf, &a);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_create #3 failed\n");
		result = 1;
	} else {
		void *ret;
		err = pmarcel_join(th, &ret);
		if (err) {
			marcel_fprintf(stderr, "pmarcel_join #3 failed\n");
			result = 1;
		} else if (ret != NULL)
			result = 1;
	}

	err = pmarcel_attr_destroy(&a);
	if (err) {
		marcel_fprintf(stderr, "pmarcel_attr_destroy failed\n");
		result = 1;
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
