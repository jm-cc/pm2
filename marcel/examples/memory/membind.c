
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

/* This test creates two threads that will work on the same chunk of
 * memory, previously distributed upon the closest nodes of the one
 * the first thread is running on. The second thread location is user
 * defined. */

#define MARCEL_INTERNAL_INCLUDE

#include <marcel.h>
#include <numa.h>
#include <numaif.h>
#include <errno.h>
#include <malloc.h>

#define TAB_SIZE 1024*1024*64
#define NB_TIMES 1024*1024*32

enum mbind_policy {
  BIND_POL,
  INTERLEAVE_POL
};

static int *tab;
static int begin = 0;

static void usage (void);
static int parse_command_line_arguments (unsigned int nb_args, 
					  char **args, 
					  unsigned int *nb_threads,
					  unsigned int *threads_location,
					  enum mbind_policy *mpol,
					  unsigned int *nodes);

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
  marcel_attr_t thread_attr;
  unsigned long tab_len = TAB_SIZE * sizeof (int);
  unsigned long nodemask = 0;
  unsigned long maxnode = numa_max_node () + 1;
  unsigned int nb_nodes, i;

  if (argc < 5) {
    usage ();
    return -1;
  }
  
  /* Trust me, it's true! */
  nb_nodes = argc - 4;

  /* Variables to be filled when parsing command line arguments */
  unsigned int nodes[nb_nodes];
  unsigned int nb_threads;
  unsigned int threads_location;
  enum mbind_policy mpol;

  int err_parse = parse_command_line_arguments (argc, argv, &nb_threads, &threads_location, &mpol, nodes);
  if (err_parse < 0) {
    perror ("parse_command_line_arguments");
  }

  marcel_init (&argc, argv);
  marcel_t working_threads[nb_threads];
  
  /* Set the nodemask to contain the nodes passed in argument. */
  for (i = 0; i < nb_nodes; i++) {
    if (nodes[i] > numa_max_node ()) {
      fprintf (stderr, "Specified node out of bounds [0 - %lu]\n", numa_max_node ());
      return -1;
    }
    nodemask |= (1 << nodes[i]);
  }

  /* Bind the accessed data to the nodes. */
  tab = memalign (getpagesize(), tab_len);
  int err_mbind;
  if (mpol == BIND_POL) {
    err_mbind = mbind (tab, tab_len, MPOL_BIND, &nodemask, maxnode + 1, MPOL_MF_MOVE);
  } else {
    err_mbind = mbind (tab, tab_len, MPOL_INTERLEAVE, &nodemask, maxnode + 1, MPOL_MF_MOVE);
  }
  if (err_mbind < 0) {
    perror ("mbind");
  }

  marcel_attr_init (&thread_attr);
  marcel_attr_setpreemptible (&thread_attr, tbx_false);
  marcel_thread_preemption_disable ();

  /* Create the working threads. */
  for (i = 0; i < nb_threads; i++) {
    marcel_create (working_threads + i, &thread_attr, f, NULL);
  }

  /* Move the working threads to the nodes passed in argument. */
  for (i = 0; i < nb_threads; i++) {
    ma_move_entity (&working_threads[i]->as_entity, &marcel_topo_node_level[threads_location].rq.as_holder);
  }

  /* Let the working threads start their work. */
  __sync_fetch_and_add (&begin, 1);

  /* Wait for the working threads to finish. */
  for (i = 0; i < nb_threads; i++) {
    marcel_join (working_threads[i], NULL);
  }

  free (tab);
  marcel_end ();
}

static void
usage () {
  fprintf (stderr, "Usage: ./membind <nb threads> <threads location> <mbind policy> node1 [node2 .. nodeN]\n");
  fprintf (stderr, "                 -nb threads: The number of created threads.\n");
  fprintf (stderr, "                 -threads location: The numa node the threads will be scheduled from.\n");
  fprintf (stderr, "                 -mbind policy: The mbind policy used to bind accessed data (can be 'bind' or 'interleave').\n");
  fprintf (stderr, "                 -node1 node2 .. nodeN: The nodes the memory will be bound to (if the mbind policy is 'bind', only the first one will be taken into account.).\n");
}

static int
parse_command_line_arguments (unsigned int nb_args, 
			      char **args, 
			      unsigned int *nb_threads,
			      unsigned int *threads_location,
			      enum mbind_policy *mpol,
			      unsigned int *nodes) {
  unsigned int i;

  /* First remove the application's name from the arguments */
  if (!nb_args)
    return -1;
  nb_args--;
  args++;
  
  /* Fill the number of threads */
  if (!nb_args)
    return -1;
  *nb_threads = atoi(args[0]);
  nb_args--;
  args++;
  
  /* Fill the threads location */
  if (!nb_args)
    return -1;
  *threads_location = atoi(args[0]);
  nb_args--;
  args++;

  /* Fill the mbind policy */
  if (!nb_args)
    return -1;
  if (strcmp (args[0], "bind")) {
    *mpol = BIND_POL;
  } else if (strcmp (args[0], "interleave")) {
    *mpol = INTERLEAVE_POL;
  } else {
    return -1;
  }
  nb_args--;
  args++;

  /* Eventually fill the nodes array */
  for (i = 0; i < nb_args; i++) {
    nodes[i] = atoi(args[i]);
  }

  return 0;
}
