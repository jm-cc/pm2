/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2011 "the PM2 team" (see AUTHORS file)
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


#define THRD_MAX 20


static void *work(void *arg)
{
	while (1)
		marcel_usleep(100);

	return arg;
}

static int do_test(void)
{
	int i;
	marcel_t t[THRD_MAX];

	for (i = 0; i < THRD_MAX; i++)
		marcel_create(&t[i], NULL, work, NULL);

	marcel_kill_other_threads_np();

	for (i = 0; i < THRD_MAX; i++) {
		if (! marcel_join(t[i], NULL)) {
#ifdef VERBOSE_TESTS
			marcel_printf("thread %p is alive\n", t[i]);
#endif
			return MARCEL_TEST_FAILED;
		}
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
