/* Copyright (C) 2002 Free Software Foundation, Inc.
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

#include <marcel.h>
#include <stdio.h>


#if defined(MA__IFACE_PMARCEL) || defined(MARCEL_LIBPTHREAD)


static int do_test(void)
{
	pmarcel_spinlock_t s;

	if (pmarcel_spin_init(&s, PMARCEL_PROCESS_PRIVATE) != 0) {
		puts("spin_init failed");
		return 1;
	}

	if (pmarcel_spin_lock(&s) != 0) {
		puts("spin_lock failed");
		return 1;
	}

	if (pmarcel_spin_unlock(&s) != 0) {
		puts("spin_unlock failed");
		return 1;
	}

	if (pmarcel_spin_destroy(&s) != 0) {
		puts("spin_destroy failed");
		return 1;
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
