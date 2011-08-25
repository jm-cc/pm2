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
#include <stdlib.h>


static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;


static void *tf(void *p TBX_UNUSED)
{
	int err;

	err = pthread_mutex_lock(&mut);
	if (err != 0)
		error(EXIT_FAILURE, err, "child: cannot get mutex");
#ifdef VERBOSE_TESTS
	puts("child: got mutex; signalling");
#endif

	pthread_cond_signal(&cond);
#ifdef VERBOSE_TESTS
	puts("child: unlock");
#endif

	err = pthread_mutex_unlock(&mut);
	if (err != 0)
		error(EXIT_FAILURE, err, "child: cannot unlock");
#ifdef VERBOSE_TESTS
	puts("child: done");
#endif

	return NULL;
}


static int do_test(void)
{
	pthread_t th;
	int err;

	//  printf ("&cond = %p\n&mut = %p\n", &cond, &mut);
#ifdef VERBOSE_TESTS
	puts("parent: get mutex");
#endif

	err = pthread_mutex_lock(&mut);
	if (err != 0)
		error(EXIT_FAILURE, err, "parent: cannot get mutex");
#ifdef VERBOSE_TESTS
	puts("parent: create child");
#endif

	err = pthread_create(&th, NULL, tf, NULL);
	if (err != 0)
		error(EXIT_FAILURE, err, "parent: cannot create thread");
#ifdef VERBOSE_TESTS
	puts("parent: wait for condition");
#endif

	err = pthread_cond_wait(&cond, &mut);
	if (err != 0)
		error(EXIT_FAILURE, err, "parent: cannot wait fir signal");
#ifdef VERBOSE_TESTS
	puts("parent: got signal");
#endif

	err = pthread_join(th, NULL);
	if (err != 0)
		error(EXIT_FAILURE, err, "parent: failed to join");
#ifdef VERBOSE_TESTS
	puts("done");
#endif

	return 0;
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
