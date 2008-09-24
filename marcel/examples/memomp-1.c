
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

#include "marcel.h"
#include <numa.h>

#define NUM_THREADS 16
#define TAB_SIZE 1024*1024*64
#define NB_TIMES 1024*1024

static int **tabs;

static int
random_read (int *tab, unsigned length) {
  return tab[marcel_random () % length];
}

static void
random_write (int *tab, unsigned length) {
  tab[marcel_random () % length] = marcel_random () % 1000;
}

static 
void * f (void *arg) {
  unsigned int i;

  for (i = 0; i < NB_TIMES; i++) {
    random_write (tabs[marcel_self ()->id], TAB_SIZE);
    random_read (tabs[marcel_self ()->id], TAB_SIZE);
  }

  return 0;
}

int
main (int argc, char **argv) {
  marcel_init (&argc, argv);

  unsigned int i, j;
  marcel_bubble_t main_bubble;
  marcel_t threads[NUM_THREADS];
  marcel_attr_t thread_attr;

  marcel_bubble_init (&main_bubble);
  marcel_bubble_insertbubble (&marcel_root_bubble, &main_bubble);
  
  tabs = marcel_malloc (NUM_THREADS * sizeof (int *), __FILE__, __LINE__);
  for (i = 0; i < NUM_THREADS; i++) {
    unsigned int node = i % (numa_max_node () + 1);
    tabs[i] = numa_alloc_onnode (TAB_SIZE * sizeof (int), node);
  }
  
  marcel_attr_init(&thread_attr);
  marcel_attr_setpreemptible (&thread_attr, tbx_false);
  marcel_thread_preemption_disable();
  marcel_attr_setinitbubble (&thread_attr, &main_bubble);

  for (i = 0; i < NUM_THREADS; i++) {
    marcel_attr_setid (&thread_attr, i);
    marcel_create (threads + i, &thread_attr, f, NULL);
    for (j = 0; j < numa_max_node() + 1; j++) {
      ((long *) ma_task_stats_get (threads[i], ma_stats_memnode_offset))[j] = 
	(j == i % (numa_max_node () + 1)) ? TAB_SIZE : 0;
    }
  }

  marcel_bubble_sched_begin ();
  
  marcel_bubble_join (&main_bubble);

  for (i = 0; i < NUM_THREADS; i++)
    numa_free (tabs[i], TAB_SIZE);
  free (tabs);

  marcel_end ();
}
