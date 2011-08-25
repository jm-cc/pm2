/* Copyright (C) 2003, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2003.

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


#if defined(MA__IFACE_PMARCEL) || defined(MARCEL_LIBPTHREAD)


static pmarcel_barrier_t bar;


static void *tf(void *arg TBX_UNUSED)
{
	int barrier_status;

	barrier_status = pmarcel_barrier_wait(&bar);
	if (0 != barrier_status && PMARCEL_BARRIER_SERIAL_THREAD != barrier_status) {
		puts("tf: barrier_wait failed");
		exit(1);
	}

	return (void *) 1l;
}


static int do_test(void)
{
	if (pmarcel_barrier_init(&bar, NULL, 3) != 0) {
		puts("barrier_init failed");
		exit(1);
	}

	pmarcel_attr_t a;

	if (pmarcel_attr_init(&a) != 0) {
		puts("attr_init failed");
		exit(1);
	}

	pmarcel_t th[2];

	if (pmarcel_create(&th[0], &a, tf, NULL) != 0) {
		puts("1st create failed");
		exit(1);
	}

	if (pmarcel_attr_setdetachstate(&a, PMARCEL_CREATE_DETACHED) != 0) {
		puts("attr_setdetachstate failed");
		exit(1);
	}

	if (pmarcel_create(&th[1], &a, tf, NULL) != 0) {
		puts("1st create failed");
		exit(1);
	}

	if (pmarcel_attr_destroy(&a) != 0) {
		puts("attr_destroy failed");
		exit(1);
	}

	if (pmarcel_detach(th[0]) != 0) {
		puts("could not detach 1st thread");
		exit(1);
	}

	int err = pmarcel_detach(th[0]);
	if (err == 0) {
		puts("second detach of 1st thread succeeded");
		exit(1);
	}
	if (err != EINVAL) {
		printf("second detach of 1st thread returned %d, not EINVAL\n", err);
		exit(1);
	}

	err = pmarcel_detach(th[1]);
	if (err == 0) {
		puts("detach of 2nd thread succeeded");
		exit(1);
	}
	if (err != EINVAL) {
		printf("detach of 2nd thread returned %d, not EINVAL\n", err);
		exit(1);
	}

	/** unlock threads and quit **/
	tf(NULL);

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
