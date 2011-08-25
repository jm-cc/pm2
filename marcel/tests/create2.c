
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
#include <sys/time.h>


#ifdef X86_64_ARCH
#define LOOP_MAX     4
#define CHILDREN_MAX 100
#else
#define LOOP_MAX     1
#define CHILDREN_MAX 30
#endif


#ifdef MA__LWPS

double     delay;
tbx_bool_t finished;

static
any_t looper(any_t arg TBX_UNUSED)
{
#ifdef VERBOSE_TESTS
	printf("Looper lauched on LWP %d\n", marcel_current_vp());
#endif

	while(! finished)
		marcel_yield();

	return NULL;
}

static
any_t f(any_t arg)
{
	marcel_sem_V((marcel_sem_t *) arg);
#ifdef VERBOSE_TESTS
	marcel_fprintf(stderr, "Hi! I'm on LWP %d\n", marcel_current_vp());
#endif
	return NULL;
}

static
any_t main_thread(void *arg)
{
	unsigned int   i;
	tbx_tick_t     t1, t2;
	marcel_vpset_t children_vpset;
	marcel_attr_t  attr;
	marcel_sem_t   sem;

#ifdef VERBOSE_TESTS
	printf("Main thread launched on LWP %d\n", marcel_current_vp());
#endif

	children_vpset = *((marcel_vpset_t *)arg),
	marcel_attr_init(&attr);
	marcel_attr_setvpset(&attr, children_vpset);
	marcel_sem_init(&sem, 0);
	TBX_GET_TICK(t1);
	for (i = 0; i < CHILDREN_MAX; i++) {
		marcel_create(NULL, &attr, f, (any_t) &sem);
		marcel_sem_P(&sem);
	}
	TBX_GET_TICK(t2);
	delay = TBX_TIMING_DELAY(t1, t2);

	return NULL;
}



int marcel_main(int argc, char *argv[])
{
	unsigned int i;
	marcel_t gen;
	marcel_attr_t attr;
	marcel_vpset_t gen_vpset, children_vpset;
	double total;

	finished = tbx_false;
	marcel_init(&argc, argv);

	// create looper thread on VPSET(0)
	marcel_attr_init(&attr);
	marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(0));
	marcel_create(NULL, &attr, looper, NULL);
	marcel_attr_destroy(&attr);
	
	// create threads on same vp
	total = 0;
	for (i = 0; i < LOOP_MAX; i++) {
		gen_vpset = children_vpset = MARCEL_VPSET_VP(0);
		marcel_attr_init(&attr);
		marcel_attr_setvpset(&attr, gen_vpset);
		marcel_create(&gen, &attr, main_thread, (void *)(&children_vpset));
		marcel_join(gen, NULL);
		marcel_attr_destroy(&attr);
		total += delay;
	}
	printf("on same vp: creation of %d threads took %lfus (one thread: %lfus)\n", LOOP_MAX * CHILDREN_MAX, total, total / (LOOP_MAX * CHILDREN_MAX));

	if (marcel_nbvps() > 1) {
		// create threads on different vp
		total = 0;
		for (i = 0; i < LOOP_MAX; i++) {
			children_vpset = MARCEL_VPSET_VP(0);
			gen_vpset = MARCEL_VPSET_VP(1);
			marcel_attr_init(&attr);
			marcel_attr_setvpset(&attr, gen_vpset);
			marcel_create(&gen, &attr, main_thread, (void *)(&children_vpset));
			marcel_join(gen, NULL);
			marcel_attr_destroy(&attr);
			total += delay;
		}
		printf("on different vp: creation of %d threads took %fus (one thread: %fus)\n", LOOP_MAX * CHILDREN_MAX, total, total / (LOOP_MAX * CHILDREN_MAX));
	}

	finished = tbx_true;
	marcel_end();
	return 0;
}

#else

int marcel_main(int argc TBX_UNUSED, char *argv[] TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}

#endif
