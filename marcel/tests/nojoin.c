
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


#if !defined(MARCEL_LIBPTHREAD)
#define NB    3


static
char *mess[NB] = { "boys", "girls", "people" };

static
any_t writer(any_t arg TBX_UNUSED)
{
	int i, j;

	marcel_detach(marcel_self());

	for (i = 0; i < 10; i++) {
#ifdef VERBOSE_TESTS
		tfprintf(stderr, "Hi %s! (I'm %p on vp %d)\n",
			 (char *) arg, marcel_self(), marcel_current_vp());
#endif
		j = 20000000;
		while (j--);
	}

	return NULL;
}

int marcel_main(int argc, char *argv[])
{
	int i;

	marcel_init(&argc, argv);

	for (i = 0; i < NB; i++)
		marcel_create(NULL, NULL, writer, (any_t) mess[i]);

	marcel_end();

	return 0;
}


#else


int marcel_main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
