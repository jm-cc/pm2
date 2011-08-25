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

#include "pthread-abi.h"


#if defined(MARCEL_LIBPTHREAD)


#include <stdio.h>


static int do_test(void)
{
	pthread_rwlock_t r;

	if (pthread_rwlock_init(&r, NULL) != 0) {
		puts("rwlock_init failed");
		return 1;
	}
#ifdef VERBOSE_TESTS
	puts("rwlock_init succeeded");
#endif

	if (pthread_rwlock_rdlock(&r) != 0) {
		puts("1st rwlock_rdlock failed");
		return 1;
	}
#ifdef VERBOSE_TESTS
	puts("1st rwlock_rdlock succeeded");
#endif

	if (pthread_rwlock_rdlock(&r) != 0) {
		puts("2nd rwlock_rdlock failed");
		return 1;
	}
#ifdef VERBOSE_TESTS
	puts("2nd rwlock_rdlock succeeded");
#endif

	if (pthread_rwlock_unlock(&r) != 0) {
		puts("1st rwlock_unlock failed");
		return 1;
	}
#ifdef VERBOSE_TESTS
	puts("1st rwlock_unlock succeeded");
#endif

	if (pthread_rwlock_unlock(&r) != 0) {
		puts("2nd rwlock_unlock failed");
		return 1;
	}
#ifdef VERBOSE_TESTS
	puts("2nd rwlock_unlock succeeded");
#endif

	if (pthread_rwlock_wrlock(&r) != 0) {
		puts("1st rwlock_wrlock failed");
		return 1;
	}
#ifdef VERBOSE_TESTS
	puts("1st rwlock_wrlock succeeded");
#endif

	if (pthread_rwlock_unlock(&r) != 0) {
		puts("3rd rwlock_unlock failed");
		return 1;
	}
#ifdef VERBOSE_TESTS
	puts("3rd rwlock_unlock succeeded");
#endif

	if (pthread_rwlock_wrlock(&r) != 0) {
		puts("2nd rwlock_wrlock failed");
		return 1;
	}
#ifdef VERBOSE_TESTS
	puts("2nd rwlock_wrlock succeeded");
#endif

	if (pthread_rwlock_unlock(&r) != 0) {
		puts("4th rwlock_unlock failed");
		return 1;
	}
#ifdef VERBOSE_TESTS
	puts("4th rwlock_unlock succeeded");
#endif

	if (pthread_rwlock_rdlock(&r) != 0) {
		puts("3rd rwlock_rdlock failed");
		return 1;
	}
#ifdef VERBOSE_TESTS
	puts("3rd rwlock_rdlock succeeded");
#endif

	if (pthread_rwlock_unlock(&r) != 0) {
		puts("5th rwlock_unlock failed");
		return 1;
	}
#ifdef VERBOSE_TESTS
	puts("5th rwlock_unlock succeeded");
#endif

	if (pthread_rwlock_destroy(&r) != 0) {
		puts("rwlock_destroy failed");
		return 1;
	}
#ifdef VERBOSE_TESTS
	puts("rwlock_destroy succeeded");
#endif

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
