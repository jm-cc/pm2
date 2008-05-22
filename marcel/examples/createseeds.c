
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

#define MARCEL_INTERNAL_INCLUDE
#include "marcel.h"
#include <stdio.h>


marcel_cond_t c;
marcel_mutex_t m;
volatile int n = 3;

marcel_t seed[3];

any_t f(any_t arg)
{
	int me = (int) arg;
	int next = (me+1) % 3;
	int i;

	while (!seed[next])
		marcel_yield();

	for (i = 0; i < 10; i++) {
		marcel_fprintf(stderr, "%d yield to %d\n", me, next);
		marcel_yield_to(seed[next]);
	}
	marcel_fprintf(stderr, "%d exiting\n", me);
	marcel_mutex_lock(&m);
	n--;
	marcel_cond_signal(&c);
	marcel_mutex_unlock(&m);
	return NULL;
}


int marcel_main(int argc, char *argv[])
{
	marcel_attr_t attr;
	marcel_t t;

	marcel_init(&argc, argv);

	marcel_cond_init(&c, NULL);
	marcel_mutex_init(&m, NULL);

	marcel_attr_init(&attr);
	marcel_attr_setseed(&attr, tbx_true);
	marcel_attr_setdetachstate(&attr, tbx_true);
	marcel_attr_setprio(&attr, MA_BATCH_PRIO);

	marcel_create(seed + 0, &attr, f, (void *) 0);
	marcel_create(seed + 1, &attr, f, (void *) 1);
	marcel_create(seed + 2, &attr, f, (void *) 2);

	marcel_mutex_lock(&m);
	while (n)
		marcel_cond_wait(&c, &m);
	marcel_mutex_unlock(&m);
	marcel_fprintf(stderr, "main exiting\n");
	marcel_end();
	return 0;
}

