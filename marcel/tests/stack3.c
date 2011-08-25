/* Copyright (C) 2003, 2004 Free Software Foundation, Inc.
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

/* Test whether pthread_create/pthread_join with user defined stacks
   doesn't leak memory.
   NOTE: this tests functionality beyond POSIX.  In POSIX user defined
   stacks cannot be ever freed once used by pthread_create nor they can
   be reused for other thread.  */

#include <limits.h>
#include <marcel.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifdef LINUX_SYS
#include <mcheck.h>
#endif


#ifdef MARCEL_LIBPTHREAD


static int seen;

static void *tf(void *p TBX_UNUSED)
{
	++seen;
	return NULL;
}

static int do_test(void)
{
	void *stack;
	int i;
	size_t size = PMARCEL_STACK_MIN;
	int res = posix_memalign(&stack, getpagesize(), size);

	mtrace();
	if (res) {
		printf("malloc failed %s\n", strerror(res));
		return 1;
	}
#if defined(MA__PROVIDE_TLS) && defined(X86_64_ARCH)
	/* marcel_alloc.c (marcel_tls_attach()): because else we can't use ldt */
	if ((unsigned long) stack >= (1ul << 32)) {
		free(stack);
		return MARCEL_TEST_SKIPPED;
	}
#endif

	pmarcel_attr_t attr;
	pmarcel_attr_init(&attr);

	int result = 0;
	res = pmarcel_attr_setstack(&attr, stack, size);
	if (res) {
		printf("pmarcel_attr_setstack failed %d\n", res);
		result = 1;
	}

	for (i = 0; i < 16; ++i) {
		/* Create the thread.  */
		pmarcel_t th;
		res = pmarcel_create(&th, &attr, tf, NULL);
		if (res) {
			printf("pmarcel_create failed %d\n", res);
			result = 1;
		} else {
			res = pmarcel_join(th, NULL);
			if (res) {
				printf("pmarcel_join failed %d\n", res);
				result = 1;
			}
		}
	}

	if (seen != 16) {
		printf("seen %d != 16\n", seen);
		result = 1;
	}

	free(stack);
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
