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

#define NB_THREADS  4
#define BIG_LOAD    100
#define LITTLE_LOAD 1
#define NB_VPS      NB_THREADS

static unsigned int vp_levels[NB_VPS], expected_result[NB_VPS];

static void *
thread_entry_point (void *arg) {
	ma_atomic_t *start;
	unsigned long current_vp = *(unsigned long *) ma_task_stats_get (marcel_self (), ma_stats_last_vp_offset);

	start = (ma_atomic_t *) arg;
	while (!ma_atomic_read (start));

	vp_levels[current_vp] += ma_entity_load (&marcel_self ()->as_entity);

	return NULL;
}

int
main (int argc, char *argv[])
{
	int ret;
	unsigned int i;
	char **new_argv;
	ma_atomic_t start_signal = MA_ATOMIC_INIT (0);
	marcel_bubble_sched_t *scheduler;

	/* A bi-dual-core computer */
  static const char topology_description[] = "2 2";

 /* Pass the topology description to Marcel.  Yes, it looks hackish to
     communicate with the library via command-line arguments.  */
  new_argv = alloca ((argc + 2) * sizeof (*new_argv));
  new_argv[0] = argv[0];
  new_argv[1] = (char *) "--synthetic-topology";
  new_argv[2] = (char *) topology_description;
  memcpy (&new_argv[3], &argv[1], argc * sizeof (*argv));
	argc += 2;

	marcel_init (&argc, new_argv);

	scheduler =
		alloca (marcel_bubble_sched_instance_size (&marcel_bubble_cache_sched_class));
	ret = marcel_bubble_cache_sched_init ((marcel_bubble_cache_sched_t *) scheduler,
																				marcel_topo_level (0, 0),
																				tbx_false);
	MA_BUG_ON (ret != 0);

	marcel_bubble_change_sched (scheduler);

	/* Creating threads as leaves of the hierarchy.  */
	marcel_t threads[NB_THREADS];
	marcel_attr_t attrs[NB_THREADS];

	/* The main thread is one of the _heavy_ threads. */
	ma_task_stats_set (unsigned long, marcel_self(), marcel_stats_load_offset, BIG_LOAD);
	/* The main thread is thread 0. */
	marcel_self ()->id = 0;
	threads[0] = marcel_self ();

	for (i = 1; i < NB_THREADS; i++) {
		/* Note: We can't use `dontsched' since THREAD would not appear
			 on the runqueue.  */
		marcel_attr_init (&attrs[i]);
		marcel_attr_setnaturalbubble (&attrs[i], &marcel_root_bubble);
		marcel_attr_setid (&attrs[i], i);
		marcel_create (threads + i, &attrs[i], thread_entry_point, &start_signal);
		ma_task_stats_set (unsigned long,
											 threads[i],
											 marcel_stats_load_offset,
											 (i == NB_THREADS - 1) ? BIG_LOAD : LITTLE_LOAD);
	}

	/* Threads have been created, let's distribute them. */
	marcel_bubble_sched_begin ();

	ma_atomic_inc (&start_signal);

	/* The main has to do its part of the job like everyone else. */
	thread_entry_point (&start_signal);

	/* Wait for other threads to end. */
	for (i = 1; i < NB_THREADS; i++) {
		marcel_join (threads[i], NULL);
	}

  /* The expected result, i.e., the scheduling entity distribution
     that we expect from Cache. Most loaded threads have to be
     distributed first, that's why we expect to see them scheduled on
     vps 0 and 2. */
	expected_result[0] = expected_result[2] = BIG_LOAD;
	expected_result[1] = expected_result[3] = LITTLE_LOAD;

	ret = 0;
	for (i = 0; i < NB_VPS; i++) {
		if (vp_levels[i] != expected_result[i]) {
			printf ("FAILED: Bad distribution [VP #%d has %d threads, but %d were expected].\n",
							i, vp_levels[i], expected_result[i]);
			ret = 1;
			goto end;
		}
	}
	if (!ret)
		printf ("PASS: scheduling entities were distributed as expected\n");

end:
	for (i = 1; i < NB_THREADS; i++) {
		marcel_attr_destroy (&attrs[i]);
	}
	marcel_end ();

	return ret;
}

/*
	Local Variables:
	tab-width: 2
	End:
 */
