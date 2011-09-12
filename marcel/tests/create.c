/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2010, 2011 "the PM2 team" (see AUTHORS file)
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

static void *do_nothing(void *arg)
{
	return arg;
}

int main(int argc, char *argv[])
{
	int i;
	pmarcel_t t;

	i = 0;
	marcel_init(&argc, argv);
	while (i < 800) {
		pmarcel_create(&t, NULL, do_nothing, NULL);
		pmarcel_join(t, NULL);
		//printf("%d\n", i);
		i ++;
	}
	marcel_end();
}
