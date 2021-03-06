
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

/* heavyalloc.c */


#include "marcel.h"


#if defined(MARCEL_NUMA) && defined(MARCEL_STATS_ENABLED)


#define LOOPS  10000L


static marcel_sem_t sem;

static any_t thread_func(any_t arg TBX_UNUSED)
{
	unsigned i;
#ifdef VERBOSE_TESTS
	unsigned long num = (unsigned long) arg;
#endif

	marcel_detach(marcel_self());

#ifdef VERBOSE_TESTS
	marcel_fprintf(stderr, "Hi! I'm thread %ld on VP %d\n", num, marcel_current_vp());
#endif

	for (i = 0; i < LOOPS; i++) {
		char *ptr = tmalloc(1024);
		tfree(ptr);
	}

	marcel_sem_V(&sem);

	return NULL;
}

tbx_tick_t t1, t2;

int marcel_main(int argc, char *argv[])
{
	marcel_attr_t attr;
	unsigned long i;
	unsigned long temps;
	unsigned int NB;


	marcel_init(&argc, argv);

	if (argc > 1)
		NB = atoi(argv[1]);
	else
		NB = 10;

#ifdef VERBOSE_TESTS
	marcel_fprintf(stderr, "Using %d threads\n", NB);
#endif

	marcel_attr_init(&attr);

	marcel_sem_init(&sem, 0);

	TBX_GET_TICK(t1);

	for (i = 0; i < NB; i++) {
		marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(i % marcel_nbvps()));

		marcel_create(NULL, &attr, thread_func, (any_t) i);
	}

	for (i = 0; i < NB; i++)
		marcel_sem_P(&sem);

	TBX_GET_TICK(t2);
	temps = TBX_TIMING_DELAY(t1, t2);
	marcel_printf("time = %ld.%03ldms\n", temps / 1000, temps % 1000);

	marcel_end();
	return 0;
}


#else


int marcel_main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif				/* NUMA & STATS */
