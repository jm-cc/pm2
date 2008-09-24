
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

/* This test creates one working thread that will work on a chunk of
 * memory previously distributed upon the closest nodes to the node
 * the thread is running on. */

#define MARCEL_INTERNAL_INCLUDE

#include <marcel.h>
#include <numa.h>
#include <numaif.h>
#include <errno.h>
#include <malloc.h>

#define TAB_SIZE 1024*1024*64
#define NB_TIMES 1024*1024*32

static int *tab;
static int begin = 0;

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

  /* Wait for the master thread to ask us to start. */
  while (!__sync_fetch_and_add (&begin, 0))
    ;

  /* Let's do the job. */
  for (i = 0; i < NB_TIMES; i++) {
    random_write (tab, TAB_SIZE);
    random_read (tab, TAB_SIZE);
  }

  return 0;
}

int
main (int argc, char **argv) 
{
  marcel_t working_thread;
  marcel_attr_t thread_attr;
  unsigned long tab_len = TAB_SIZE * sizeof (int);
  unsigned long nodemask = 0;
  unsigned long maxnode = numa_max_node () +  1;

  if (numa_max_node () != 7) {
    fprintf (stderr, "This test targets Hagrid, or some 8-node computer. Please buy one!\n");
    return -1;
  }

  marcel_init (&argc, argv);
  
  /* Note that this example aims Hagrid only. */
  /* Set the nodemask to contain the closest nodes from node 7. */
  nodemask |= (1 << 4);
  nodemask |= (1 << 5);
  nodemask |= (1 << 6);

  /* Distribute the array to the closest numa nodes. */
  tab = memalign (getpagesize(), tab_len);
  int err = mbind (tab, tab_len, MPOL_INTERLEAVE, &nodemask, maxnode, MPOL_MF_MOVE);
  if (err < 0) {
    perror ("mbind");
  }
  marcel_attr_init (&thread_attr);
  marcel_attr_setpreemptible (&thread_attr, tbx_false);
  marcel_thread_preemption_disable ();

  /* Create the working thread. */
  marcel_create (&working_thread, &thread_attr, f, NULL);
  
  /* Move the working thread to node 7. */
  ma_move_entity (&working_thread->as_entity, &marcel_topo_node_level[7].rq.as_holder);

  /* Let the working thread start its work. */
  __sync_fetch_and_add (&begin, 1);

  /* Wait for the worker thread to finish. */
  marcel_join (working_thread, NULL);
  free (tab);

  marcel_end ();
}
