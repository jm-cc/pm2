/* Copyright (C) 2003 Free Software Foundation, Inc.
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

#include <marcel.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#if defined(MA__IFACE_PMARCEL) || defined(MARCEL_LIBPTHREAD)


static void *tf(void *arg TBX_UNUSED)
{
	return NULL;
}


static int do_test(void)
{
	pmarcel_t th;
	if (pmarcel_create(&th, NULL, tf, (void *) pmarcel_self()) != 0) {
		puts("create failed");
		exit(1);
	}

	/* Give the child a chance to finish.  */
	sleep(1);

	if (pmarcel_detach(th) != 0) {
		puts("detach failed");
		exit(1);
	}

	return 0;
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
