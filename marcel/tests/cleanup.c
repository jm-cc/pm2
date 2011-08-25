
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

/* cleanup.c */

#include "marcel.h"


#if defined(MARCEL_CLEANUP_ENABLED)


#define DELAY 50

static
const char *mess[] = { "boys", "girls", "everybody", "all" };

static
void bye(any_t arg TBX_UNUSED)
{
#ifdef VERBOSE_TESTS
	tfprintf(stderr, "Bye bye %s!\n", (char *) arg);
#endif
}

static
any_t writer(any_t arg)
{
	int n = (intptr_t) arg;
	const char *message = mess[n];
	int i;


	marcel_cleanup_push(bye, (any_t) message);

	for (i = 0; i < 10; i++) {
		if (n == 2 && i == 5)
			marcel_exit(NULL);
		marcel_delay(DELAY);
#ifdef VERBOSE_TESTS
		tfprintf(stderr, "Hi %s!\n", message);
#endif
	}
	marcel_cleanup_pop(n == 3);
	return (any_t) NULL;
}

int marcel_main(int argc, char *argv[])
{
	any_t status;
	marcel_t writer1_pid, writer2_pid, writer3_pid, writer4_pid;
	marcel_attr_t writer_attr;

	marcel_init(&argc, argv);

	marcel_attr_init(&writer_attr);

	marcel_create(&writer1_pid, &writer_attr, writer, (any_t) (uintptr_t) 0);
	marcel_create(&writer2_pid, &writer_attr, writer, (any_t) (uintptr_t) 1);
	marcel_create(&writer3_pid, &writer_attr, writer, (any_t) (uintptr_t) 2);
	marcel_create(&writer4_pid, &writer_attr, writer, (any_t) (uintptr_t) 3);

	marcel_delay(3 * DELAY + DELAY / 2);
	marcel_cancel(writer1_pid);

	marcel_join(writer1_pid, &status);
	marcel_join(writer2_pid, &status);
	marcel_join(writer3_pid, &status);
	marcel_join(writer4_pid, &status);

	marcel_end();
	return MARCEL_TEST_SUCCEEDED;
}


#else


int marcel_main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif				/* MARCEL_CLEANUP_ENABLED */
