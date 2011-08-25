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


#ifdef MARCEL_LIBPTHREAD


static void *protected_stack;
static size_t size;


static void *tf(void *a TBX_UNUSED)
{
	int result = 0;

#ifdef VERBOSE_TESTS
	puts("child start");
#endif

	pmarcel_attr_t attr;
	if (pmarcel_getattr_np(pmarcel_self(), &attr) != 0) {
		puts("getattr_np failed");
		exit(1);
	}

	size_t test_size;
	void *test_protected_stack;
	if (pmarcel_attr_getstack(&attr, &test_protected_stack, &test_size) != 0) {
		puts("attr_getstack failed");
		exit(1);
	}

	if (test_size != size) {
		printf
		    ("child: reported size differs: is %zu, expected %zu\n",
		     test_size, size);
		result = 1;
	}

	if (test_protected_stack != protected_stack) {
		printf
		    ("child: reported protected_stack address differs: is %p, expected %p\n",
		     test_protected_stack, protected_stack);
		result = 1;
	}
#ifdef VERBOSE_TESTS
	puts("child OK");
#endif

	return result ? (void *) 1l : NULL;
}


static int do_test(void)
{
	int result = 0;
	void *stack;

	size = MAX((size_t) (4 * getpagesize()), PMARCEL_STACK_MIN);
	if (posix_memalign(&stack, getpagesize(), 3 * size) != 0) {
		puts("out of memory while allocating the protected_stack memory");
		exit(1);
	}
#if defined(MA__PROVIDE_TLS) && defined(X86_64_ARCH)
	/* marcel_alloc.c (marcel_tls_attach()): because else we can't use ldt */
	if ((unsigned long) stack >= (1ul << 32))
		return MARCEL_TEST_SKIPPED;
#endif

	/* protect arround protected_stack */
	if (-1 == mprotect(stack, size, PROT_NONE)) {
		perror("mprotect 1");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	else
		printf("region %p - %p is protected\n", stack, stack + size);
#endif
	if (-1 == mprotect((char *)stack + 2 * size, size, PROT_NONE)) {
		perror("mprotect 2");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	else
		printf("region %p - %p is protected\n", (char *)stack + 2 * size,
		       (char *)stack + 3 * size);
#endif

	protected_stack = (char *)stack + size;
#ifdef VERBOSE_TESTS
	printf("provide protected_stack %p of %lu bytes\n", protected_stack, size);
#endif

	pmarcel_attr_t attr;
	if (pmarcel_attr_init(&attr) != 0) {
		puts("attr_init failed");
		exit(1);
	}
#ifdef VERBOSE_TESTS
	puts("attr_stack");
#endif
	if (pmarcel_attr_setstack(&attr, protected_stack, size) != 0) {
		puts("attr_stack failed");
		exit(1);
	}

	size_t test_size;
	void *test_protected_stack;
#ifdef VERBOSE_TESTS
	puts("attr_getstack");
#endif
	if (pmarcel_attr_getstack(&attr, &test_protected_stack, &test_size) != 0) {
		puts("attr_getstack failed");
		exit(1);
	}

	if (test_size != size) {
		printf("reported size differs: is %zu, expected %zu\n", test_size, size);
		result = 1;
	}

	if (test_protected_stack != protected_stack) {
		printf
		    ("reported protected_stack address differs: is %p, expected %p\n",
		     test_protected_stack, protected_stack);
		result = 1;
	}
#ifdef VERBOSE_TESTS
	puts("create");
#endif
	pmarcel_t th;
	if (pmarcel_create(&th, &attr, tf, NULL) != 0) {
		puts("failed to create thread");
		exit(1);
	}

	void *status;
	if (pmarcel_join(th, &status) != 0) {
		puts("join failed");
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
