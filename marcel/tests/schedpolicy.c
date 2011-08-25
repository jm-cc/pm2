
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (pm2-dev@listes.ens-lyon.fr)
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


#ifdef MA__BUBBLES


#define SCHED_POLICY MARCEL_SCHED_AFFINITY
#define NB    3

any_t ALL_IS_OK = (any_t)123456789L;
char *mess[NB] = { "boys", "girls", "people" };

static any_t writer(any_t arg TBX_UNUSED)
{
	int i, j;

	for(i=0;i<10;i++) {
#ifdef VERBOSE_TESTS
		tfprintf(stderr, "Hi %s! (I'm %p on vp %d)\n", (char*)arg, marcel_self(), marcel_current_vp());
#endif
		j = 20000000; while(j--);
	}

	return ALL_IS_OK;
}

int marcel_main(int argc, char *argv[])
{
	any_t status;
	marcel_t pid[NB];
	marcel_attr_t attr;
	int i;

	marcel_init(&argc, argv);

	marcel_attr_init(&attr);

	for(i=0; i<NB; i++) {
		marcel_attr_setschedpolicy(&attr, SCHED_POLICY);

		marcel_create(&pid[i], &attr, writer, (any_t)mess[i]);
	}

	for(i=0; i<NB; i++) {
		marcel_join(pid[i], &status);
		if(status != ALL_IS_OK)
			exit(1);
	}

	marcel_end();
	return MARCEL_TEST_SUCCEEDED;
}


#else


int marcel_main(int argc TBX_UNUSED, char *argv[] TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
