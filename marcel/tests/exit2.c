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

#define N 10


static
void *wait_prev_thr(void *arg)
{
	marcel_join((marcel_t)arg, NULL);
	return NULL;
}


int marcel_main(int argc, char *argv[])
{
	int i;
	marcel_t prev, cur;

        marcel_init(&argc, argv);

	prev = marcel_self();
	for (i = 0; i < N; i++) {
		if (-1 == marcel_create(&cur, NULL, wait_prev_thr, (void *)prev))
			return EXIT_FAILURE;

		prev = cur;
	}
	marcel_exit(NULL);

        return EXIT_FAILURE;
}
