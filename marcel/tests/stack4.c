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

#include <limits.h>
#include <marcel.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <unistd.h>


#if defined(MARCEL_POSIX) && !defined(DARWIN_SYS)


static void *thr_stack;
static size_t size;


static void *tf(void *a TBX_UNUSED)
{
	int result = 0;

#ifdef VERBOSE_TESTS
	marcel_puts("child start");
#endif

	pmarcel_attr_t attr;
	if (pmarcel_getattr_np(pmarcel_self(), &attr) != 0) {
		marcel_puts("getattr_np failed");
		exit(1);
	}

	size_t test_size;
	void *test_thr_stack;
	if (pmarcel_attr_getstack(&attr, &test_thr_stack, &test_size) != 0) {
		marcel_puts("attr_getstack failed");
		exit(1);
	}

	if (test_size != size) {
		marcel_printf("child: reported size differs: is %zu, expected %zu\n",
		       test_size, size);
		result = 1;
	}

	if (test_thr_stack != thr_stack) {
		marcel_printf("child: reported thr_stack address differs: is %p, expected %p\n",
			      test_thr_stack, thr_stack);
		result = 1;
	}
#ifdef VERBOSE_TESTS
	marcel_puts("child OK");
#endif

	return result ? (void *) 1l : NULL;
}


static int do_test(void)
{
	int result = 0;
	void *stack;

	if (posix_memalign(&stack, THREAD_SLOT_SIZE, THREAD_SLOT_SIZE) != 0) {
		marcel_puts("out of memory while allocating the thr_stack memory");
		exit(1);
	}
#if defined(MA__PROVIDE_TLS) && defined(X86_64_ARCH)
	/* marcel_alloc.c (marcel_tls_attach()): because else we can't use ldt */
	if ((unsigned long) stack >= (1ul << 32))
		return MARCEL_TEST_SKIPPED;
#endif

	size = PMARCEL_STACK_MIN;
	thr_stack = (char *)stack + THREAD_SLOT_SIZE - size;
#ifdef VERBOSE_TESTS
	marcel_printf("stack region: %p - %p\n", thr_stack, (char *)thr_stack + size);
#endif

	pmarcel_attr_t attr;
	if (pmarcel_attr_init(&attr) != 0) {
		marcel_puts("attr_init failed");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	marcel_puts("attr_stack");
#endif
	if (pmarcel_attr_setstack(&attr, thr_stack, size) != 0) {
		marcel_puts("attr_stack failed");
		exit(1);
	}

	size_t test_size;
	void *test_thr_stack;
#ifdef VERBOSE_TESTS
	marcel_puts("attr_getstack");
#endif
	if (pmarcel_attr_getstack(&attr, &test_thr_stack, &test_size) != 0) {
		marcel_puts("attr_getstack failed");
		exit(1);
	}

	if (test_size != size) {
		marcel_printf("reported size differs: is %zu, expected %zu\n", test_size, size);
		result = 1;
	}

	if (test_thr_stack != thr_stack) {
		marcel_printf("reported thr_stack address differs: is %p, expected %p\n",
			      test_thr_stack, thr_stack);
		result = 1;
	}
#ifdef VERBOSE_TESTS
	marcel_puts("create");
#endif
	pmarcel_t th;
	if (pmarcel_create(&th, &attr, tf, NULL) != 0) {
		marcel_puts("failed to create thread");
		exit(1);
	}

	void *status;
	if (pmarcel_join(th, &status) != 0) {
		marcel_puts("join failed");
		exit(1);
	}

	result |= status != NULL;

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
