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

/* This test checks Cache's work stealing algorithm behaviour. */

#define MARCEL_INTERNAL_INCLUDE

#include "bubble-testing.h"

#define NB_BUBBLES 2
#define THREADS_PER_BUBBLE 2

static ma_bubble_sched_vp_is_idle aff_steal = NULL;
static ma_atomic_t first_team_is_dead = MA_ATOMIC_INIT (0);
static ma_atomic_t die_later_signal = MA_ATOMIC_INIT (0);

static unsigned int vp_levels[2], expected_result[2];
static marcel_mutex_t write_lock;

/* Arguments passed to marcel_create ().*/
struct thread_args {
	ma_atomic_t *start_signal;
	marcel_barrier_t *team_barrier;
};

static void *
thread_entry_point (void *arg) {
	struct thread_args *targs = (struct thread_args *)arg;
	unsigned long current_vp;

	while (!ma_atomic_read (targs->start_signal));

	/* Wait our teammates before leaving (helps with updating the
		 entities run_holder after a successful steal. */
	marcel_barrier_wait (targs->team_barrier);

	current_vp = *(unsigned long *) ma_task_stats_get (marcel_self (), ma_stats_last_vp_offset);

	marcel_mutex_lock (&write_lock);
	vp_levels[current_vp] += 1;
	marcel_mutex_unlock (&write_lock);

	return NULL;
}


/* Our own implementation of the `vp_is_idle' scheduler method.  */
static int
my_steal (marcel_bubble_sched_t *scheduler, unsigned int from_vp) {
	if (ma_atomic_read (&first_team_is_dead)) {
		aff_steal (scheduler, from_vp);
		/* Set the last threads free. */
		ma_atomic_inc (&die_later_signal);
	}

	return 0;
}


int
main (int argc, char *argv[]) {
	int ret;
	unsigned int i;
	char **new_argv;
	marcel_bubble_sched_t *scheduler;
	ma_atomic_t die_first_signal = MA_ATOMIC_INIT (0);

	/* A dual-core computer */
  static const char topology_description[] = "2";

 /* Pass the topology description to Marcel.  Yes, it looks hackish to
     communicate with the library via command-line arguments.  */
  new_argv = alloca ((argc + 2) * sizeof (*new_argv));
  new_argv[0] = argv[0];
  new_argv[1] = (char *) "--synthetic-topology";
  new_argv[2] = (char *) topology_description;
  memcpy (&new_argv[3], &argv[1], argc * sizeof (*argv));
	argc += 2;

	marcel_init (&argc, new_argv);
	marcel_mutex_init (&write_lock, NULL);

	/* Creating threads and bubbles hierarchy.  */
	marcel_bubble_t bubbles[NB_BUBBLES];
	marcel_t threads[NB_BUBBLES * THREADS_PER_BUBBLE];
	marcel_barrier_t team_barrier[NB_BUBBLES];
	marcel_attr_t attr;
	unsigned int team;
	int nb_threads;

	marcel_barrier_init (team_barrier + 0, NULL, THREADS_PER_BUBBLE);
	marcel_barrier_init (team_barrier + 1, NULL, THREADS_PER_BUBBLE);

	/* The main thread is thread 0 of team 0. */
	marcel_self ()->id = 0;
	threads[0] = marcel_self ();

	marcel_attr_init (&attr);

	struct thread_args ta[NB_BUBBLES];

	for (team = 0; team < NB_BUBBLES; team++) {
		marcel_bubble_init (bubbles + team);
		marcel_bubble_insertbubble (&marcel_root_bubble, bubbles + team);
		if (team == 0) {
			marcel_bubble_inserttask (bubbles + team, marcel_self ());
		}
		marcel_attr_setnaturalbubble (&attr, bubbles + team);

		ta[team].start_signal = (team == 0) ? &die_later_signal : &die_first_signal;
		ta[team].team_barrier = team_barrier + team;

		for (i = team * THREADS_PER_BUBBLE; i < (team + 1) * THREADS_PER_BUBBLE; i++) {
			if ((team == 0) && (i == 0)) {
				continue; /* The main thread has already been created. */
			}
			marcel_create (threads + i, &attr, thread_entry_point, ta + team);
		}
	}

	/* Make sure we're currently testing the Cache scheduler, with work
		 stealing enabled.  */
	scheduler =
		alloca (marcel_bubble_sched_instance_size (&marcel_bubble_cache_sched_class));
	ret = marcel_bubble_cache_sched_init ((struct marcel_bubble_cache_sched *) scheduler,
																				marcel_topo_level (0, 0),
																				tbx_true);
	MA_BUG_ON (ret != 0);

	/* We asked for work stealing, so it should be available.  */
	MA_BUG_ON (scheduler->vp_is_idle == NULL);

	marcel_bubble_change_sched (scheduler);

	/* Intercept Cache's work stealing algorithm to assure it's
		 called before all threads have died. */
	aff_steal = scheduler->vp_is_idle;
	scheduler->vp_is_idle = my_steal;

	/* Threads have been created, let's distribute them. */
	marcel_bubble_sched_begin ();

	/* Make the first team die */
	ma_atomic_inc (&die_first_signal);

	/* Wait for them to die. */
	marcel_threadslist (0, NULL, &nb_threads, ALL_THREADS);
	while (nb_threads > 2) {
		marcel_threadslist (0, NULL, &nb_threads, ALL_THREADS);
	}

	/* Inform our work stealing function the first team is dead. */
	ma_atomic_inc (&first_team_is_dead);

	/* The main has to do its work like everyone else. */
	thread_entry_point (ta);

	/* Wait for other threads to end. */
	for (team = 0; team < NB_BUBBLES; team++) {
		for (i = team * THREADS_PER_BUBBLE; i < (team + 1) * THREADS_PER_BUBBLE; i++) {
			if ((team == 0) && (i == 0)) {
				continue; /* Avoid the main thread */
			}
			marcel_join (threads[i], NULL);
		}
	}

  expected_result[0] = 1;
	expected_result[1] = 3;

	for (i = 0; i < 2; i++) {
		if (vp_levels[i] != expected_result[i]) {
			printf ("FAILED: Bad distribution.\n");
			ret = 1;
			goto end;
		}
	}
	printf ("PASS: scheduling entities were distributed as expected\n");
	ret = 0;

end:
	marcel_attr_destroy (&attr);
	marcel_end ();

	return ret;
}

/*
	 Local Variables:
	 tab-width: 2
	 End:
 */
