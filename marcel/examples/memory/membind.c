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

#include <marcel.h>
#include <numa.h>
#include <numaif.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/uio.h>

#define TAB_SIZE 1024*1024*64
#define NB_ITERATIONS 1024*1024*256
#define NB_TIMES 10

enum mbind_policy {
  BIND_POL,
  INTERLEAVE_POL
};

enum sched_policy {
  LOCAL_POL,
  DISTRIBUTED_POL
};

static int *tab;
static int begin = 0;
static marcel_barrier_t barrier;

static void usage (void);
static int parse_command_line_arguments (unsigned int nb_args,
					 char **args,
					 unsigned int *nb_threads,
					 enum sched_policy *spol,
					 unsigned int *threads_nodes,
					 unsigned int *nb_threads_nodes,
					 enum mbind_policy *mpol,
					 unsigned int *memory_nodes,
					 unsigned int *nb_memory_nodes);

static void print_welcoming_message (unsigned int nb_threads,
				     enum sched_policy spol,
				     unsigned int *threads_nodes,
				     unsigned int nb_threads_nodes,
				     enum mbind_policy mpol,
				     unsigned int *memory_nodes,
				     unsigned int nb_memory_nodes);

static int
create_file (char *filename,
	     long **access_pattern, 
	     unsigned int nb_threads,
	     unsigned int access_pattern_len,
	     unsigned int tab_size) {
  unsigned int i, j;
  int fileno = open (filename, O_CREAT | O_WRONLY, 0666);
  if (fileno < 0) {
    perror (filename);
    exit (1);
  }

  for (i = 0; i < nb_threads; i++) {
    for (j = 0; j < access_pattern_len; j++) {
      access_pattern[i][j] = marcel_random () % tab_size;
    }
  }
  
  for (i = 0; i < nb_threads; i++) {
    write (fileno, access_pattern[i], access_pattern_len);
  }
  return fileno;
} 

static void 
initialize_access_pattern_vectors_from_file (char *filename,
					     long **access_pattern, 
					     unsigned int nb_threads,
					     unsigned int access_pattern_len,
					     unsigned int tab_size) {
  unsigned int i, j;
  int fileno = open (filename, O_RDONLY);

  if (fileno < 0) {
    fileno = create_file (filename, access_pattern, nb_threads, access_pattern_len, tab_size);
  } else {
    for (i = 0; i < nb_threads; i++) {
      read (fileno, access_pattern[i], access_pattern_len);
    }
  }

  close (fileno);
  marcel_printf ("Access pattern vectors initialized!\n");
}

static 
void * f (void *arg) {
  long *access_pattern = (long *)arg;
  unsigned int i, j;
  
  for (i = 0; i < NB_TIMES; i++) {
    marcel_barrier_wait (&barrier);
    
    /* Let's do the job. */
    for (j = 0; j < NB_ITERATIONS; j++) {
      int dummy = tab[access_pattern[j]];
      tab[access_pattern[NB_ITERATIONS - j - 1]] = dummy;
    }
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
  long **access_pattern;
  tbx_tick_t t1, t2;

  marcel_init (&argc, argv);
  tbx_timing_init ();

  if (argc < 5) {
    usage ();
    return -1;
  }

  /* Trust me, it's true! */
  nb_nodes = argc - 4;

  /* Variables to be filled when parsing command line arguments */
  unsigned int memory_nodes[nb_nodes];
  unsigned int threads_nodes[nb_nodes];
  unsigned int nb_threads, threads_location, nb_threads_nodes, nb_memory_nodes;
  enum sched_policy spol;
  enum mbind_policy mpol;

  int err_parse = parse_command_line_arguments (argc, 
						argv, 
						&nb_threads, 
						&spol, 
						threads_nodes, 
						&nb_threads_nodes, 
						&mpol, 
						memory_nodes, 
						&nb_memory_nodes);
  if (err_parse < 0) {
    usage();
    exit(1);
  }

 /* Print a pretty and welcoming message */
  print_welcoming_message (nb_threads, 
			   spol, 
			   threads_nodes, 
			   nb_threads_nodes, 
			   mpol, 
			   memory_nodes, 
			   nb_memory_nodes);

  marcel_t working_threads[nb_threads];
  marcel_barrier_init (&barrier, NULL, nb_threads + 1);

  /* Set the nodemask to contain the nodes passed in argument. */
  for (i = 0; i < nb_memory_nodes; i++) {
    if (memory_nodes[i] > numa_max_node ()) {
      fprintf (stderr, "Specified node out of bounds [0 - %u]\n", numa_max_node ());
      return -1;
    }
    nodemask |= (1 << memory_nodes[i]);
  }

  /* Bind the access pattern vectors next to the accessing threads. */
  access_pattern = numa_alloc_onnode (nb_threads * sizeof (long *), threads_nodes[0]);
  for (i = 0; i < nb_threads; i++) {
    access_pattern[i] = numa_alloc_onnode (NB_ITERATIONS * sizeof (long), spol == LOCAL_POL ? threads_nodes[0] : threads_nodes[i % nb_threads_nodes]);
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

  /* Build the access pattern vectors */
  initialize_access_pattern_vectors_from_file ("membind_pattern.dat", access_pattern, nb_threads, NB_ITERATIONS, TAB_SIZE);

  marcel_attr_init (&thread_attr);
  marcel_attr_setpreemptible (&thread_attr, tbx_false);
  marcel_thread_preemption_disable ();

  /* Create the working threads. */
  for (i = 0; i < nb_threads; i++) {
    marcel_attr_settopo_level (&thread_attr, &marcel_topo_node_level[spol == LOCAL_POL ? threads_nodes[0] : threads_nodes[i % nb_threads_nodes]]);
    marcel_create (working_threads + i, &thread_attr, f, access_pattern[i]);
  }
  
  TBX_GET_TICK (t1);
  for (i = 0; i < NB_TIMES; i++) {
    marcel_barrier_wait (&barrier);
  }
  TBX_GET_TICK (t2); 
    
  /* Wait for the working threads to finish. */
  for (i = 0; i < nb_threads; i++) {
    marcel_join (working_threads[i], NULL);
  }

  for (i = 0; i < nb_threads; i++) {
    numa_free (access_pattern[i], NB_ITERATIONS * sizeof (long));
  }
  numa_free (access_pattern, nb_threads * sizeof (long *));
  free (tab);

  marcel_printf ("Test computed in %lfs!\n", (double)TBX_TIMING_DELAY(t1, t2) / (NB_TIMES * 1000000));

  marcel_end ();
  return 0;
}

static void
usage () {
  marcel_fprintf (stderr, "Usage: ./membind <nb threads> <sched policy> node1 [node2 .. nodeN] <mbind policy> node1 [node2 .. nodeN]\n");
  marcel_fprintf (stderr, "                 -nb threads: The number of created threads.\n");
  marcel_fprintf (stderr, "                 -sched policy: The way the created threads will be scheduled (can be 'local' for moving the whole group of threads to the first node passed in argument, or 'distributed' to distribute the threads over the nodes).\n");
  marcel_fprintf (stderr, "                 -node1 node2 .. nodeN: The nodes the threads will be distributed over. If you specify more nodes than threads, the exceeding nodes will be ignored.\n");
  marcel_fprintf (stderr, "                 -mbind policy: The mbind policy used to bind accessed data (can be 'bind' or 'interleave').\n");
  marcel_fprintf (stderr, "                 -node1 node2 .. nodeN: The nodes the memory will be bound to (if the mbind policy is 'bind', only the first one will be taken into account.)\n");
}

static void
print_welcoming_message (unsigned int nb_threads,
			 enum sched_policy spol,
			 unsigned int *threads_nodes,
			 unsigned int nb_threads_nodes,
			 enum mbind_policy mpol,
			 unsigned int *memory_nodes,
			 unsigned int nb_memory_nodes) {
  unsigned int i;
  marcel_printf ("Launching membind with %u %s %s %s ", 
		 nb_threads,
		 nb_threads > 1 ? "threads" : "thread",
		 spol == LOCAL_POL ? "bound to" : "distributed over", 
		 spol == LOCAL_POL ? "node" : "nodes");
  for (i = 0; i < nb_threads_nodes; i++) {
    marcel_printf ("%u ", threads_nodes[i]);
  } 
  marcel_printf (".\n");
  marcel_printf ("The global array is %s %s", (mpol == BIND_POL) ? "bound to" : "distributed over",
		 nb_memory_nodes == 1 ? "node " : "nodes ");
  for (i = 0; i < nb_memory_nodes; i++) {
    marcel_printf ("%u ", memory_nodes[i]);
  }
  marcel_printf (".\n\n");
}

static int
parse_command_line_arguments (unsigned int nb_args, 
			      char **args, 
			      unsigned int *nb_threads,
			      enum sched_policy *spol,
			      unsigned int *threads_nodes,
			      unsigned int *nb_threads_nodes,
			      enum mbind_policy *mpol,
			      unsigned int *memory_nodes,
			      unsigned int *nb_memory_nodes) {
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
  
  /* Fill the scheduling policy */
  if (!nb_args)
    return -1;
  if (!strcmp (args[0], "local")) {
    *spol = LOCAL_POL;
  } else if (!strcmp (args[0], "distributed")) {
    *spol = DISTRIBUTED_POL;
  } else {
    return -1;
  }
  nb_args--;
  args++;

  /* Fill the threads location */
  long val;
  char *endptr;
  errno = 0;
  *nb_threads_nodes = 0;
  for (val = 0; endptr != args[0]; args++, nb_args--, val = strtol (args[0], &endptr, 10)) {
    if (!nb_args)
      return -1;
    threads_nodes[*nb_threads_nodes] = atoi(args[0]);
    *nb_threads_nodes = *nb_threads_nodes + 1;
  }

  /* Fill the mbind policy */
  if (!nb_args)
    return -1;
  if (!strcmp (args[0], "bind")) {
    *mpol = BIND_POL;
  } else if (!strcmp (args[0], "interleave")) {
    *mpol = INTERLEAVE_POL;
  } else {
    return -1;
  }
  nb_args--;
  args++;

  /* Eventually fill the nodes array */
  *nb_memory_nodes = 0;
  for (i = 0; i < nb_args; i++) {
    memory_nodes[i] = atoi(args[i]);
    *nb_memory_nodes = *nb_memory_nodes + 1;
  }

  return 0;
}
