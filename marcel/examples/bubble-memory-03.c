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

/* This test checks the Memory bubble scheduler's distribution algorithm.

   This test creates 4 bubbles. Each bubble is drawn to the node 0,
   but have different amounts of data stored from there. We check
   whether the Memory scheduler is able to pick the bubble with the
   bigger amount of memory on node 0 to be scheduled from it. */

#define MARCEL_INTERNAL_INCLUDE

#include "bubble-testing.h"

#ifdef MM_MAMI_ENABLED

#define NB_BUBBLES 4
#define THREADS_PER_BUBBLE 4

static unsigned int node_levels[4], expected_result[4];
static marcel_mutex_t write_lock;

struct thread_args {
  ma_atomic_t *signal;
  unsigned int team;
};

static void *
thread_entry_point (void *arg)
{
  struct thread_args *ta = arg;
  ma_atomic_t *start = ta->signal;
  unsigned int team = ta->team;
  unsigned long current_vp = *(unsigned long *) ma_task_stats_get (marcel_self (), ma_stats_last_vp_offset);
  unsigned int current_node;

  current_node = current_vp / 4;

  start = (ma_atomic_t *) arg;
  while (!ma_atomic_read (start));

  marcel_mutex_lock (&write_lock);
  node_levels[current_node] = team;
  marcel_mutex_unlock (&write_lock);

  return NULL;
}

int
main (int argc, char *argv[])
{
  int ret = 0;
  unsigned int i;
  char **new_argv;
  marcel_bubble_sched_t *scheduler;
  ma_atomic_t start_signal = MA_ATOMIC_INIT (0);

  /* A quad-socket quad-core computer ("aka kwak" (tm)) */
  static const char topology_description[] = "4 4 1 1";

  /* Pass the topology description to Marcel.  Yes, it looks hackish to
     communicate with the library via command-line arguments.  */
  new_argv = alloca ((argc + 2) * sizeof (*new_argv));
  new_argv[0] = argv[0];
  new_argv[1] = (char *) "--marcel-synthetic-topology";
  new_argv[2] = (char *) topology_description;
  memcpy (&new_argv[3], &argv[1], argc * sizeof (*argv));
  argc += 2;

  marcel_init (&argc, new_argv);
  marcel_mutex_init (&write_lock, NULL);

  /* Make sure we're currently testing the memory scheduler. */
  scheduler =
    alloca (marcel_bubble_sched_instance_size (&marcel_bubble_memory_sched_class));
  ret = marcel_bubble_memory_sched_init ((struct marcel_bubble_memory_sched *) scheduler,
					 NULL, tbx_false);
  MA_BUG_ON (ret != 0);

  marcel_bubble_change_sched (scheduler);


  /* Creating threads and bubbles hierarchy.  */
  marcel_bubble_t bubbles[NB_BUBBLES];
  marcel_t threads[NB_BUBBLES * THREADS_PER_BUBBLE];
  marcel_attr_t attr;
  unsigned int team;

  /* The main thread is thread 0 of team 0. */
  marcel_self ()->id = 0;
  threads[0] = marcel_self ();

  marcel_attr_init (&attr);

  ((long *) ma_task_stats_get (marcel_self (), ma_stats_memnode_offset))[0] = 1024;

  struct thread_args ta[4];

  for (team = 0; team < NB_BUBBLES; team++) {
    marcel_bubble_init (bubbles + team);
    marcel_bubble_setid (bubbles + team, team);
    marcel_bubble_insertbubble (&marcel_root_bubble, bubbles + team);
    if (team == 0)
      marcel_bubble_inserttask (bubbles + team, marcel_self ());

    marcel_attr_setnaturalbubble (&attr, bubbles + team);

    ta[team].signal = &start_signal;
    ta[team].team = team;

    for (i = team * THREADS_PER_BUBBLE; i < (team + 1) * THREADS_PER_BUBBLE; i++) {
      /* Note: We can't use `dontsched' since THREAD would not appear
	 on the runqueue.  */
      if ((team == 0) && (i == 0)) {
	continue; /* The main thread has already been created. */
      }

      marcel_create (threads + i, &attr, thread_entry_point, &ta[team]);
      ((long *) ma_task_stats_get (threads[i], ma_stats_memnode_offset))[0] = (team + 1) * 1024;
    }
  }

  /* Threads have been created, let's distribute them. */
  marcel_bubble_sched_begin ();

  ma_atomic_inc (&start_signal);

  /* The main has to do its part of the job like everyone else. */
  thread_entry_point (&ta[0]);

  /* Wait for other threads to end. */
  for (team = 0; team < NB_BUBBLES; team++) {
    for (i = team * THREADS_PER_BUBBLE; i < (team + 1) * THREADS_PER_BUBBLE; i++) {
      if ((team == 0) && (i == 0)) {
	continue; /* Avoid the main thread */
      }
      marcel_join (threads[i], NULL);
    }
  }

  /* We expect the bubble with the bigger amount of data (bubble 3) to
     be scheduled from node 0. */
  expected_result[0] = 3;
  expected_result[1] = 0;
  expected_result[2] = 1;
  expected_result[3] = 2;

  for (i = 0; i < 4; i++) {
    if (node_levels[i] != expected_result[i]) {
      marcel_printf ("(expected_result[%i] : %i) != (result[%i] : %i)\n", i, expected_result[i], i, node_levels[i]);
      ret = 1;
    }
  }

  marcel_printf ("%s\n", ret == 1 ? "FAILED: Bad distribution" : "PASS: scheduling entities were distributed as expected");

  marcel_attr_destroy (&attr);
  marcel_end ();

  return ret;
}

#else /* MM_MAMI_ENABLED */
#  warning MaMI must be enabled for this program
int main (int argc, char *argv[])
{
  fprintf(stderr, "'MaMI' disabled in the flavor\n");

  return 0;
}
#endif /* MM_MAMI_ENABLED */
