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

#include <errno.h>
#include <marcel.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>


#if defined(MARCEL_LIBPTHREAD)


static int do_test(void)
{
	sigset_t ss;

	sigemptyset(&ss);

	int i;
	for (i = 0; i < 10000; ++i) {
		long int r = random();

		if (r != SIG_BLOCK && r != SIG_SETMASK && r != SIG_UNBLOCK) {
			int e = pthread_sigmask(r, &ss, NULL);

			if (e == 0) {
				printf("pthread_sigmask succeeded for how = %ld\n", r);
				exit(1);
			}

			if (e != EINVAL) {
				puts("pthread_sigmask didn't return EINVAL");
				exit(1);
			}
		}
	}

	return 0;
}


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return do_test();
}


#else


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
