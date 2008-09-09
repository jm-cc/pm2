/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
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

/* This test checks Affinity's greedy distribution behaviour. */

#include "bubble-testing.h"

#define NB_THREADS  4
#define BIG_LOAD    100
#define LITTLE_LOAD 1

static unsigned int vp_levels[4], expected_result[4]; 
static int dummy = 0;
static marcel_mutex_t write_mutex;

static void *
thread_entry_point (void *arg) {
	unsigned int i, my_id = marcel_self ()->id;
	unsigned long current_vp = *(unsigned long *) ma_task_stats_get (marcel_self (), ma_stats_last_vp_offset);

	/* Dummy work */
	for (i = 0; i < 10000000; i++) {
		dummy += (i - my_id);
	}

	marcel_mutex_lock (&write_mutex);
	vp_levels[current_vp] += ma_entity_load (&marcel_self ()->as_entity); 
	marcel_mutex_unlock (&write_mutex);
	return NULL;
}

int
main (int argc, char *argv[])
{
	unsigned int i;
	char **new_argv;

	/* A bi-dual-core computer */
  static const char topology_description[] = "2 2";

 /* Pass the topology description to Marcel.  Yes, it looks hackish to
     communicate with the library via command-line arguments.  */
  new_argv = alloca ((argc + 2) * sizeof (*new_argv));
  new_argv[0] = argv[0];
  new_argv[1] = (char *) "--marcel-synthetic-topology";
  new_argv[2] = (char *) topology_description;
  memcpy (&new_argv[3], &argv[1], argc * sizeof (*argv));
	argc += 2;

	marcel_init (&argc, new_argv);
	marcel_mutex_init (&write_mutex, NULL);

	/* Creating threads as leaves of the hierarchy.  */
	marcel_t threads[NB_THREADS];
	marcel_attr_t attr;
	
	marcel_attr_init (&attr);
	marcel_attr_setinitbubble (&attr, &marcel_root_bubble);
	
	/* The main thread is one of the _heavy_ threads. */
	ma_task_stats_set (unsigned long, marcel_self(), marcel_stats_load_offset, BIG_LOAD);
	/* The main thread is thread 0. */
	marcel_self ()->id = 0;
	threads[0] = marcel_self ();

	for (i = 1; i < NB_THREADS; i++) {
		/* Note: We can't use `dontsched' since THREAD would not appear
			 on the runqueue.  */
		marcel_attr_setid (&attr, i);
		marcel_create (threads + i, &attr, thread_entry_point, NULL);
		ma_task_stats_set (unsigned long, 
											 threads[i], 
											 marcel_stats_load_offset, 
											 (i == NB_THREADS - 1) ? BIG_LOAD : LITTLE_LOAD);
	}
	
	/* Threads have been created, let's distribute them. */
	marcel_bubble_sched_begin ();

	/* The main has to do its part of the job like everyone else. */
	thread_entry_point (NULL);

	/* Wait for other threads to end. */
	for (i = 1; i < NB_THREADS; i++) {
		marcel_join (threads[i], NULL);
	}

  /* The expected result, i.e., the scheduling entity distribution
     that we expect from Affinity. Most loaded threads have to be
     distributed first, that's why we expect to see them scheduled on
     vps 0 and 2. */
	expected_result[0] = expected_result[2] = BIG_LOAD; 
	expected_result[1] = expected_result[3] = LITTLE_LOAD;

	for (i = 0; i < NB_THREADS; i++) {
		if (vp_levels[i] != expected_result[i]) {
			printf ("FAILED: Bad distribution.\n");
			return 1;
		}
	}
	printf ("PASS: scheduling entities were distributed as expected\n");
	
	marcel_end ();
	return 0;
}
