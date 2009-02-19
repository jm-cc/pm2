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

/* This test checks the Memory bubble scheduler's distribution
   algorithm.

   Here, all the threads are drawned to node 0. This test checks
   whether the Memory bubble scheduler manages to occupy every core
   anyway (see "ma_memory_global_balance ()" in
   "marcel_bubble_memory.c"). */

#define MARCEL_INTERNAL_INCLUDE

#include "bubble-testing.h"

#ifdef MARCEL_MAMI_ENABLED

#define NB_BUBBLES 4
#define THREADS_PER_BUBBLE 4

static unsigned int node_levels[4], expected_result[4]; 
static marcel_mutex_t write_lock;

static void *
thread_entry_point (void *arg) {
  ma_atomic_t *start;
  unsigned long current_vp = *(unsigned long *) ma_task_stats_get (marcel_self (), ma_stats_last_vp_offset);
  unsigned int current_node;

  current_node = current_vp / 4;

  start = (ma_atomic_t *) arg;
  while (!ma_atomic_read (start));

  marcel_mutex_lock (&write_lock);
  node_levels[current_node] += 1; 
  marcel_mutex_unlock (&write_lock);

  return NULL;
}

int
main (int argc, char *argv[]) {
  int ret;
  unsigned int i;
  char **new_argv;
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
  marcel_bubble_change_sched (&marcel_bubble_memory_sched);

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
	
  for (team = 0; team < NB_BUBBLES; team++) {
    marcel_bubble_init (bubbles + team);
    marcel_bubble_insertbubble (&marcel_root_bubble, bubbles + team);
    if (team == 0) {
      marcel_bubble_inserttask (bubbles + team, marcel_self ());
    }
    marcel_attr_setinitbubble (&attr, bubbles + team);
	
    for (i = team * THREADS_PER_BUBBLE; i < (team + 1) * THREADS_PER_BUBBLE; i++) {
      /* Note: We can't use `dontsched' since THREAD would not appear
	 on the runqueue.  */
      if ((team == 0) && (i == 0)) {
	continue; /* The main thread has already been created. */
      }
      marcel_create (threads + i, &attr, thread_entry_point, &start_signal);
      ((long *) ma_task_stats_get (threads[i], ma_stats_memnode_offset))[0] = 1024;
    }
  }

  /* Threads have been created, let's distribute them. */
  marcel_bubble_sched_begin ();

  ma_atomic_inc (&start_signal);

  /* The main has to do its part of the job like everyone else. */
  thread_entry_point (&start_signal);

  /* Wait for other threads to end. */
  for (team = 0; team < NB_BUBBLES; team++) {
    for (i = team * THREADS_PER_BUBBLE; i < (team + 1) * THREADS_PER_BUBBLE; i++) {
      if ((team == 0) && (i == 0)) {
	continue; /* Avoid the main thread */
      }
      marcel_join (threads[i], NULL);
    }
  }

  /* We expect each team to be schedule on a different node. */
  expected_result[0] = expected_result[1] = expected_result[2] = expected_result[3] = 4;

  for (i = 0; i < 4; i++) {
    if (node_levels[i] != expected_result[i]) {
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

#else /* MARCEL_MAMI_ENABLED */
#  warning MaMI must be enabled for this program
int main(int argc, char *argv[])
{
  fprintf(stderr, "'MaMI' disabled in the flavor\n");

  return 0;
}
#endif /* MARCEL_MAMI_ENABLED */