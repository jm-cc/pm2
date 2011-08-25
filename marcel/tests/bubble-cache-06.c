/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008, 2009 "the PM2 team" (see AUTHORS file)
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

/* This test checks Cache's greedy distribution behaviour. */

#include "bubble-testing.h"


#if !defined(MARCEL_LIBPTHREAD) && defined(MARCEL_BUBBLES) && defined(WHITE_BOX)


#define NB_THREADS  4
#define BIG_LOAD    100
#define LITTLE_LOAD 1
#define NB_VPS      NB_THREADS

static unsigned int vp_levels[NB_VPS], expected_result[NB_VPS];
static marcel_mutex_t vp_level_mutx;

static void *thread_entry_point(void *arg)
{
	ma_atomic_t *start;
	unsigned long current_vp;

	start = (ma_atomic_t *) arg;
	do { marcel_barrier(); } while (!ma_atomic_read(start));

	current_vp = *(unsigned long *) marcel_task_stats_get(marcel_self(), LAST_VP);
	marcel_mutex_lock(&vp_level_mutx);
	vp_levels[current_vp] += ma_entity_load(&marcel_self()->as_entity);
	marcel_mutex_unlock(&vp_level_mutx);

	return NULL;
}

int main(int argc, char *argv[])
{
	int ret;
	unsigned int i;
	ma_atomic_t start_signal = MA_ATOMIC_INIT(0);
	marcel_bubble_sched_t *scheduler;

	/* Pass the topology description to Marcel: a bi-dual-core computer */
	char *pm2_args = "PM2_ARGS=--synthetic-topology '2 2'";
	ret = putenv(pm2_args);
	assert(0 == ret);

	/* check nbvp */
	marcel_init(&argc, argv);
	if (marcel_nbvps() > NB_VPS)
		exit(MARCEL_TEST_SKIPPED);

	scheduler = alloca(marcel_bubble_sched_instance_size(&marcel_bubble_cache_sched_class));
	ret = marcel_bubble_cache_sched_init((marcel_bubble_cache_sched_t *)scheduler,
					     marcel_topo_level(0, 0), tbx_false);
	assert(0 == ret);

	marcel_bubble_change_sched(scheduler);

	/* Init data structures */
	marcel_mutex_init(&vp_level_mutx, NULL);
	memset(vp_levels, 0, sizeof(vp_levels));

	/* Creating threads as leaves of the hierarchy.  */
	marcel_t threads[NB_THREADS];
	marcel_attr_t attrs[NB_THREADS];

	/* The main thread is one of the _heavy_ threads. */
	marcel_task_stats_set(unsigned long, marcel_self(), LOAD, BIG_LOAD);
	/* The main thread is thread 0. */
	marcel_thread_setid(marcel_self(), 0);
	threads[0] = marcel_self();

	for (i = 1; i < NB_THREADS; i++) {
		/* Note: We can't use `dontsched' since THREAD would not appear
		   on the runqueue.  */
		marcel_attr_init(&attrs[i]);
		marcel_attr_setnaturalbubble(&attrs[i], &marcel_root_bubble);
		marcel_attr_setid(&attrs[i], i);
		marcel_create(threads + i, &attrs[i], thread_entry_point, &start_signal);
		marcel_task_stats_set(unsigned long, threads[i], LOAD,
				      (i == NB_THREADS - 1) ? BIG_LOAD : LITTLE_LOAD);
	}

	/* Threads have been created, let's distribute them. */
	marcel_bubble_sched_begin();

	ma_atomic_inc(&start_signal);

	/* The main has to do its part of the job like everyone else. */
	thread_entry_point(&start_signal);

	/* Wait for other threads to end. */
	for (i = 1; i < NB_THREADS; i++) {
		marcel_join(threads[i], NULL);
	}

#ifdef MARCEL_SMT_ENABLED
	/* The expected result, i.e., the scheduling entity distribution
	   that we expect from Cache. Most loaded threads have to be
	   distributed first, that's why we expect to see them scheduled on
	   vps 0 and 2. */
	expected_result[0] = expected_result[2] = BIG_LOAD;
	expected_result[1] = expected_result[3] = LITTLE_LOAD;
#else
	expected_result[0] = expected_result[1] = BIG_LOAD + LITTLE_LOAD;
	expected_result[2] = expected_result[3] = 0;
#endif

	ret = 0;
	for (i = 0; i < NB_VPS; i++) {
		if (vp_levels[i] != expected_result[i]) {
			marcel_printf("Bad distribution [VP #%d has %d threads, but %d were expected].\n",
				      i, vp_levels[i], expected_result[i]);
			ret = 1;
		}
	}
#ifdef VERBOSE_TESTS
	if (!ret)
		marcel_printf("scheduling entities were distributed as expected\n");
#endif

	for (i = 1; i < NB_THREADS; i++) {
		if (0 != marcel_attr_destroy(&attrs[i]))
			ret = 1;
	}
	marcel_end();

	return ret;
}


#else


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
