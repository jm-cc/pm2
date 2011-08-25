/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */


#include "marcel.h"


static marcel_recursivemutex_t m;


static void *thr_func(void *arg TBX_UNUSED)
{
	int lockcount;

	lockcount = marcel_recursivemutex_trylock(&m);
	if (lockcount == 0) {
		/* mutex was acquired by another thread */
		lockcount = marcel_recursivemutex_lock(&m);
	}

	lockcount = marcel_recursivemutex_lock(&m);
	if (2 != lockcount) {
		puts("recursive trylock+lock failed");
		return (void *)1;
	}

	lockcount = marcel_recursivemutex_unlock(&m);
	if (1 != lockcount) {
		puts("first unlock failed");
		return (void *)1;
	}

	lockcount = marcel_recursivemutex_trylock(&m);
	if (2 != lockcount) {
		puts("recursive trylock+trylock failed");
		return (void *)1;
	}

	marcel_recursivemutex_unlock(&m);
	lockcount = marcel_recursivemutex_unlock(&m);
	if (0 != lockcount) {
		puts("second unlock failed");
		return (void *)1;
	}

	return NULL;
}


static int do_test()
{
	void *retval;
	marcel_t t1, t2;
	marcel_recursivemutexattr_t m_attr;

	if (0 != marcel_recursivemutexattr_init(&m_attr)) {
		puts("marcel_recursivemutexattr_init failed");
		return -1;
	}
	if (0 != marcel_recursivemutex_init(&m, &m_attr)) {
		puts("marcel_recursivemutexattr_init failed");
		return -1;
	}
	
	if (0 != marcel_create(&t1, NULL, thr_func, NULL)) {
		puts("1 - marcel_create failed");
		return -1;
	}
	if (0 != marcel_create(&t2, NULL, thr_func, NULL)) {
		puts("2 - marcel_create failed");
		return -1;
	}

	if (0 != marcel_join(t1, &retval)) {
		puts("first marcel_join failed");
		return -1;
	}
	if (NULL != retval) {
		puts("first thread failed");
		return -1;
	}

	if (0 != marcel_join(t2, &retval)) {
		puts("second marcel_join failed");
		return -1;
	}
	if (NULL != retval) {
		puts("first thread failed");
		return -1;
	}

	if (0 != marcel_recursivemutex_destroy(&m)) {
		puts("marcel_recursivemutex_destroy failed");
		return -1;
	}
	if (0 != marcel_recursivemutexattr_destroy(&m_attr)) {
		puts("marcel_recursivemutexattr_destroy failed");
		return -1;
	}
	
	return MARCEL_TEST_SUCCEEDED;
}


int marcel_main(int argc, char *argv[])
{
	int result;

	marcel_init(&argc, argv);
	result = do_test();
	marcel_end();

	return result;
}
