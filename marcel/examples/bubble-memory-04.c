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

   This test behaves the same than bubble-memory-03, but let the
   underlying bubble scheduler migrate the accessed data
   automatically. We check whether every piece of memory is on the
   right NUMA node at the end of the application.

   Note that we attach a distinct memory area to each thread in this
   example. */

#define MARCEL_INTERNAL_INCLUDE

#include "bubble-testing.h"

#ifdef MM_MAMI_ENABLED

#include "mm_mami.h"
#define NB_BUBBLES 4
#define THREADS_PER_BUBBLE 4

#define TAB_LEN 4096

struct thread_args {
  ma_atomic_t *signal;
  unsigned int team;
};

static void *
thread_entry_point (void *arg)
{
  struct thread_args *ta = arg;
  ma_atomic_t *start = ta->signal;
  unsigned long current_vp = *(unsigned long *) ma_task_stats_get (marcel_self (), ma_stats_last_vp_offset);
  unsigned int current_node;

  current_node = current_vp / 4;

  start = (ma_atomic_t *) arg;
  while (!ma_atomic_read (start));

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
  mami_manager_t *memory_manager;

  /* A quad-socket quad-core computer ("aka kwak" (tm)) */
  static const char topology_description[] = "4 4 1 1 1";

  /* Pass the topology description to Marcel.  Yes, it looks hackish to
     communicate with the library via command-line arguments.  */
  new_argv = alloca ((argc + 2) * sizeof (*new_argv));
  new_argv[0] = argv[0];
  new_argv[1] = (char *) "--synthetic-topology";
  new_argv[2] = (char *) topology_description;
  memcpy (&new_argv[3], &argv[1], argc * sizeof (*argv));
  argc += 2;

  marcel_init (&argc, new_argv);
  mami_init (&memory_manager);

  /* Make sure we're currently testing the memory scheduler. */
  scheduler =
    alloca (marcel_bubble_sched_instance_size (&marcel_bubble_memory_sched_class));
  ret = marcel_bubble_memory_sched_init ((struct marcel_bubble_memory_sched *) scheduler,
					 memory_manager, tbx_false);
  MA_BUG_ON (ret != 0);

  marcel_bubble_change_sched (scheduler);


  /* Creating threads and bubbles hierarchy.  */
  marcel_bubble_t bubbles[NB_BUBBLES];
  marcel_t threads[NB_BUBBLES * THREADS_PER_BUBBLE];
  marcel_attr_t attr;
  unsigned int team;

  /* Allocate the data the threads are supposed to work on. */
  void *array[NB_BUBBLES];

  for (i = 0; i < NB_BUBBLES; i++)
    /* Team t will work on array[t]. */
    array[i] = mami_malloc (memory_manager, TAB_LEN * (1 << i), MAMI_MEMBIND_POLICY_SPECIFIC_NODE, 0);

  /* The main thread is thread 0 of team 0. */
  marcel_self ()->id = 0;
  threads[0] = marcel_self ();

  marcel_attr_init (&attr);

  struct thread_args ta[NB_BUBBLES];
  int node;

  for (team = 0; team < NB_BUBBLES; team++) {
    marcel_bubble_init (bubbles + team);
    marcel_bubble_setid (bubbles + team, team);
    marcel_bubble_insertbubble (&marcel_root_bubble, bubbles + team);
    if (team == 0)
      marcel_bubble_inserttask (bubbles + team, marcel_self ());

    marcel_attr_setnaturalbubble (&attr, bubbles + team);

    ta[team].signal = &start_signal;
    ta[team].team = team;

    mami_bubble_attach (memory_manager,
			array[team],
			TAB_LEN * (1 << team),
			&bubbles[team],
			&node);
    MA_BUG_ON (node != 0);

    for (i = team * THREADS_PER_BUBBLE; i < (team + 1) * THREADS_PER_BUBBLE; i++) {
      /* Note: We can't use `dontsched' since THREAD would not appear
	 on the runqueue.  */
      if ((team == 0) && (i == 0)) {
	continue; /* The main thread has already been created. */
      }

      marcel_create (threads + i, &attr, thread_entry_point, &ta[team]);
    }
  }

  /* Threads have been created, let's distribute them. */
  marcel_bubble_sched_begin ();

  ma_atomic_inc (&start_signal);

  /* The main has to do its part of the job like everyone else. */
  thread_entry_point (&ta[0]);

  /* Wait for other threads to end. */
  for (team = 0; team < NB_BUBBLES; team++) {
    mami_bubble_unattach (memory_manager, array[team], &bubbles[team]);
    for (i = team * THREADS_PER_BUBBLE; i < (team + 1) * THREADS_PER_BUBBLE; i++) {
      if ((team == 0) && (i == 0)) {
	continue; /* Avoid the main thread */
      }
      marcel_join (threads[i], NULL);
    }
  }

  /* Now check the memory location. */
  ret = mami_check_pages_location (memory_manager, array[0], TAB_LEN * (1 << 0), 1) < 0 ? 1 : 0;
  ret |= mami_check_pages_location (memory_manager, array[1], TAB_LEN * (1 << 1), 2) < 0 ? 1 : 0;
  ret |= mami_check_pages_location (memory_manager, array[2], TAB_LEN * (1 << 2), 3) < 0 ? 1 : 0;
  ret |= mami_check_pages_location (memory_manager, array[3], TAB_LEN * (1 << 3), 0) < 0 ? 1 : 0;

  marcel_printf ("%s\n", ret == 1 ? "FAILED: Bad distribution" : "PASS: scheduling entities and accessed data were distributed as expected");

  for (i = 0; i < NB_BUBBLES; i++)
    mami_free (memory_manager, array[i]);

  marcel_attr_destroy (&attr);
  mami_exit (&memory_manager);
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
