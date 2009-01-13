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

#include <marcel.h>
#include <numa.h>
#include <numaif.h>
#include <errno.h>
#include <malloc.h>
#include <time.h>
#include "../marcel_stream.h"

#if defined(MARCEL_MAMI_ENABLED)

#define TAB_SIZE 1024*1024*16
#define NB_TIMES 100

enum mbind_policy {
  BIND_POL,
  INTERLEAVE_POL
};

enum sched_policy {
  LOCAL_POL,
  DISTRIBUTED_POL
};

static double *a, *b, *c;
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

static 
double my_delay (struct timespec *t1, struct timespec *t2) {
  return ((double) t2->tv_sec + (double) t2->tv_nsec * 1.E-09) - ((double) t1->tv_sec + (double) t1->tv_nsec * 1.E-09);
}

static 
void * f (void *arg) {
  stream_struct_t *stream_struct = (stream_struct_t *)arg;
  unsigned int i, j;
  
  for (i = 0; i < NB_TIMES; i++) {
    marcel_barrier_wait (&barrier);
    
    /* Let's do the job. */
    STREAM_copy (stream_struct);
    STREAM_scale (stream_struct, 3.0);
    STREAM_add (stream_struct);
    STREAM_triad (stream_struct, 3.0);
  }

  return 0;
}

int
main (int argc, char **argv) 
{
  unsigned long tab_len = TAB_SIZE * sizeof (double);
  unsigned long nodemask = 0;
  unsigned long maxnode = numa_max_node () + 1;
  unsigned int nb_nodes, i;
  struct timespec t1, t2;

  marcel_init (&argc, argv);
 
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
      marcel_fprintf (stderr, "Specified node out of bounds [0 - %u]\n", numa_max_node ());
      return -1;
    }
    nodemask |= (1 << memory_nodes[i]);
  }

  /* Bind the accessed data to the nodes. */
  a = memalign (getpagesize(), tab_len);
  b = memalign (getpagesize(), tab_len);
  c = memalign (getpagesize(), tab_len);

  int err_mbind;
  if (mpol == BIND_POL) {
    err_mbind = mbind (a, tab_len, MPOL_BIND, &nodemask, maxnode + 1, MPOL_MF_MOVE);
    err_mbind += mbind (b, tab_len, MPOL_BIND, &nodemask, maxnode + 1, MPOL_MF_MOVE);
    err_mbind += mbind (c, tab_len, MPOL_BIND, &nodemask, maxnode + 1, MPOL_MF_MOVE);
  } else {
    err_mbind = mbind (a, tab_len, MPOL_INTERLEAVE, &nodemask, maxnode + 1, MPOL_MF_MOVE);
    err_mbind += mbind (b, tab_len, MPOL_INTERLEAVE, &nodemask, maxnode + 1, MPOL_MF_MOVE);
    err_mbind += mbind (c, tab_len, MPOL_INTERLEAVE, &nodemask, maxnode + 1, MPOL_MF_MOVE);
  }
  if (err_mbind < 0) {
    perror ("mbind");
  }

  /* Initialize the STREAM library. */
  stream_struct_t stream_struct;
  STREAM_init (&stream_struct, nb_threads, TAB_SIZE, a, b, c);
  
  /* Disable preemption on the main thread. */
  marcel_thread_preemption_disable ();

  marcel_attr_t thread_attr[nb_threads];
  
  /* Create the working threads. */
  for (i = 0; i < nb_threads; i++) {
    marcel_attr_init (thread_attr + i);
    marcel_attr_setid (thread_attr + i, i);
    marcel_attr_setpreemptible (thread_attr + i, tbx_false);
    marcel_attr_settopo_level (thread_attr + i, &marcel_topo_node_level[spol == LOCAL_POL ? threads_nodes[0] : threads_nodes[i % nb_threads_nodes]]);
    marcel_create (working_threads + i, thread_attr + i, f, &stream_struct);
  }
  
  clock_gettime (CLOCK_MONOTONIC, &t1);
  for (i = 0; i < NB_TIMES; i++) {
    marcel_barrier_wait (&barrier);
  }
  clock_gettime (CLOCK_MONOTONIC, &t2);
   
  /* Wait for the working threads to finish. */
  for (i = 0; i < nb_threads; i++) {
    marcel_join (working_threads[i], NULL);
  }

  free (a);
  free (b);
  free (c);

  /* Avoid the first iteration, in which we only measure the time we
     need to cross the barrier. */
  double average_time = my_delay (&t1, &t2) / (double)(NB_TIMES-1);
  marcel_printf ("Test computed in %lfs!\n", average_time);
  marcel_printf ("Estimated rate (MB/s): %11.4f!\n", (double)(10 * tab_len * 1E-06)/ average_time);

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

#else
int marcel_main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MAMI to be enabled\n");
}
#endif
