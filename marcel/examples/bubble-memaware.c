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

#include "bubble-testing.h"

static void print_hierarchy(const struct marcel_topo_level *level) {
  marcel_entity_t *e;
  unsigned entities, child;

  entities = 0;
  for_each_entity_scheduled_on_runqueue (e, &level->rq) {
    if (e->type == MA_THREAD_ENTITY) {
      marcel_t thread;

      thread = ma_task_entity (e);
      if (MA_TASK_NOT_COUNTED_IN_RUNNING (thread))
        /* THREAD is an internal thread (e.g., `ksoftirqd') so we
           should not take it into account here.  */
        continue;
    }
    entities++;
  }
  level_log (stdout, level, "%u entit%s, %u child%s", entities, entities>1?"ies":"y", level->arity, level->arity?"ren":"");
  for (child = 0; child < level->arity; child++) {
    print_hierarchy(level->children[child]);
  }
}

int main (int argc, char *argv[]) {
  char **new_argv;
  marcel_bubble_t *root_bubble;
  marcel_bubble_sched_t *scheduler;
  ma_atomic_t thread_exit_signal;
  int ret;

  /* A simple topology: 2 nodes, each of which has 2 CPUs, each of which has 4 cores.  */
  static const char topology_description[] = "2 2 4";

  /* A flat bubble hierarchy.  */
  static const unsigned bubble_hierarchy_description[] = { 16, 0 };

  /* Pass the topology description to Marcel.  Yes, it looks hackish to
     communicate with the library via command-line arguments.  */
  new_argv = alloca ((argc + 2) * sizeof (*new_argv));
  new_argv[0] = argv[0];
  new_argv[1] = (char *) "--synthetic-topology";
  new_argv[2] = (char *) topology_description;
  memcpy (&new_argv[3], &argv[1], argc * sizeof (*argv));
  argc += 2;

  /* Initialize Marcel.  It should pick use the "fake" topology described in
		 TOPOLOGY_DESCRIPTION.  */
  marcel_init(argc, new_argv);
  marcel_ensure_abi_compatibility(MARCEL_HEADER_HASH);

  print_topology (&marcel_machine_level[0], stdout, 0);

  /* Before creating any threads, initialize the variable that they'll
     poll.  */
  ma_atomic_init (&thread_exit_signal, 0);

  /* Create a bubble hierarchy.  */
  root_bubble = make_simple_bubble_hierarchy (bubble_hierarchy_description, 1,
                                              &thread_exit_signal);

  scheduler =
    alloca (marcel_bubble_sched_instance_size (&marcel_bubble_memaware_sched_class));
  ret = marcel_bubble_memaware_sched_init ((marcel_bubble_memaware_sched_t * )scheduler, tbx_true);
  MA_BUG_ON (ret != 0);

  marcel_bubble_change_sched (scheduler);

  /* Submit the generated bubble hierarchy to the scheduler.  */
  marcel_bubble_sched_begin ();

  /* Print the hierarchy */
  marcel_thread_preemption_disable ();
  ma_bubble_lock_all (&marcel_root_bubble, marcel_machine_level);
  print_hierarchy(marcel_machine_level);
  ma_bubble_unlock_all (&marcel_root_bubble, marcel_machine_level);
  marcel_thread_preemption_enable ();

  /* Tell threads to leave.  */
  ma_atomic_inc (&thread_exit_signal);

  free_bubble_hierarchy ();
  marcel_end ();
}
