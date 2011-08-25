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
#include <stdio.h>
#include <stdlib.h>


typedef struct {
	int inf, sup, res;
} job;

static void job_init(job * j, int inf, int sup)
{
	j->inf = inf;
	j->sup = sup;
}

static any_t sum(any_t arg)
{
	marcel_t chld1, chld2;
	job *j = (job *) arg;
	job j1, j2;

	if (j->inf == j->sup) {
		j->res = j->inf;
		return NULL;
	}

	job_init(&j1, j->inf, (j->inf + j->sup) / 2);
	job_init(&j2, (j->inf + j->sup) / 2 + 1, j->sup);

	marcel_create(&chld1, NULL, sum, (any_t) & j1);	/* Here! */
	marcel_create(&chld2, NULL, sum, (any_t) & j2);	/* Here! */

	marcel_join(chld1, NULL);
	marcel_join(chld2, NULL);

	j->res = j1.res + j2.res;
	return NULL;
}

int marcel_main(int argc, char **argv)
{				/* Here! */
	int inf = 1;
	int sup = 100;
	int expected = 5050;
	marcel_t chld;

	job j;

	marcel_init(&argc, argv);

	job_init(&j, inf, sup);

	marcel_create(&chld, NULL, sum, (any_t) & j);	/* Here! */
	marcel_join(chld, NULL);
	expected = (expected == j.res) ? MARCEL_TEST_SUCCEEDED : j.res;

	marcel_end();
	return expected;
}
