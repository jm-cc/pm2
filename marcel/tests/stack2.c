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

/* Test whether it is possible to create a thread with PTHREAD_STACK_MIN
   stack size.  */

#include <marcel.h>
#include <stdlib.h>
#include <stdio.h>


#if defined(MA__IFACE_PMARCEL) || defined(MARCEL_LIBPTHREAD)


static int seen;

static void *tf(void *p TBX_UNUSED)
{
	++seen;
	return NULL;
}

static int do_test(void)
{
	pmarcel_attr_t attr;
	pmarcel_attr_init(&attr);

	int result = 0;
	int res = pmarcel_attr_setstacksize(&attr, PMARCEL_STACK_MIN);
	if (res) {
		printf("pmarcel_attr_setstacksize failed %d\n", res);
		result = 1;
	}

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

	if (seen != 1) {
		printf("seen %d != 1\n", seen);
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
