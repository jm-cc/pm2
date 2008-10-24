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

/* Testing application that experiments crossing data transfers. */
/* Thread A's data is located on node B, thread B's data is located on
   node A.  A and B locations are passed in argument. */

#include <marcel.h>
#include <numa.h>
#include <numaif.h>
#include <errno.h>
#include <malloc.h>
#include "../marcel_stream.h"

#define TAB_SIZE 1024*1024*16
#define NB_TIMES 100

static double **a, **b, **c;
static marcel_barrier_t barrier;

static void usage (void);
static int parse_command_line_arguments (unsigned int nb_args, char **args, unsigned int *nodes);

static 
void * f (void *arg) {
  stream_struct_t *stream_struct = (stream_struct_t *)arg;
  unsigned int i;
  
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
  unsigned long nodemask[2] = {0, 0};
  unsigned int nodes[2], i;
  unsigned int maxnode = numa_max_node () + 1;
  tbx_tick_t t1, t2;
  
  marcel_init (&argc, argv);
  
  if (argc < 3) {
    usage ();
    marcel_end ();
    exit (1);
  }
  
  tbx_timing_init ();
  
  marcel_t working_threads[2];
  marcel_barrier_init (&barrier, NULL, 3);
  
  parse_command_line_arguments (argc, argv, nodes);
  
  /* Set the nodemask to contain the nodes passed in argument. */
  for (i = 0; i < 2; i++) {
    if (nodes[1 - i] > maxnode - 1) {
      marcel_printf ("Error: %i > maxnode (%i)\n", nodes[1 - i], maxnode - 1);
      marcel_end ();
      exit (1);
    }
    nodemask[i] |= (1 << nodes[1 - i]);
  }
  
  /* Bind the accessed data to the nodes. */
  a = marcel_malloc (2 * sizeof (double *), __FILE__, __LINE__);
  b = marcel_malloc (2 * sizeof (double *), __FILE__, __LINE__);
  c = marcel_malloc (2 * sizeof (double *), __FILE__, __LINE__);
  
  for (i = 0; i < 2; i++) {
    a[i] = memalign (getpagesize (), tab_len);
    b[i] = memalign (getpagesize (), tab_len);
    c[i] = memalign (getpagesize (), tab_len);
  }
  
  int err_mbind;
  err_mbind = mbind (a[0], tab_len, MPOL_BIND, nodemask, maxnode + 1, MPOL_MF_MOVE);
  err_mbind += mbind (b[0], tab_len, MPOL_BIND, nodemask, maxnode + 1, MPOL_MF_MOVE);
  err_mbind += mbind (c[0], tab_len, MPOL_BIND, nodemask, maxnode + 1, MPOL_MF_MOVE);
  
  err_mbind += mbind (a[1], tab_len, MPOL_BIND, nodemask + 1, maxnode + 1, MPOL_MF_MOVE);
  err_mbind += mbind (b[1], tab_len, MPOL_BIND, nodemask + 1, maxnode + 1, MPOL_MF_MOVE);
  err_mbind += mbind (c[1], tab_len, MPOL_BIND, nodemask + 1, maxnode + 1, MPOL_MF_MOVE);

  if (err_mbind < 0) {
    perror ("mbind");
  }
  
  /* Initialize the STREAM data structures. */
  stream_struct_t stream_struct[2];
  STREAM_init (stream_struct, 1, TAB_SIZE, a[0], b[0], c[0]);
  STREAM_init (stream_struct + 1, 1, TAB_SIZE, a[1], b[1], c[1]);
  
  /* Disable preemption on the main thread. */
  marcel_thread_preemption_disable ();

  marcel_attr_t thread_attr[2];
  
  /* Create the working threads. */
  for (i = 0; i < 2; i++) {
    marcel_attr_init (thread_attr + i);
    marcel_attr_setid (thread_attr + i, 0);
    marcel_attr_setpreemptible (thread_attr + i, tbx_false);
    marcel_attr_settopo_level (thread_attr + i, &marcel_topo_node_level[nodes[i]]);
    marcel_create (working_threads + i, thread_attr + i, f, stream_struct + i);
  }
  
  TBX_GET_TICK (t1);
  for (i = 0; i < NB_TIMES; i++) {
    marcel_barrier_wait (&barrier);
  }
  TBX_GET_TICK (t2); 
    
  /* Wait for the working threads to finish. */
  for (i = 0; i < 2; i++) {
    marcel_join (working_threads[i], NULL);
  }

  for (i = 0; i < 2; i++) {
    free (a[i]);
    free (b[i]);
    free (c[i]);
  }

  marcel_free (a);
  marcel_free (b);
  marcel_free (c);
  
  double average_time = (double)TBX_TIMING_DELAY(t1, t2) / (NB_TIMES * 1000000);
  marcel_printf ("Test computed in %lfs!\n", average_time);
  marcel_printf ("Estimated rate (MB/s): %11.4f!\n", (double)(10 * tab_len * 1E-06)/ average_time);

  marcel_end ();
  return 0;
}

static void
usage () {
  marcel_fprintf (stderr, "Usage: ./crossing_access node1 node2\n");
}

static int
parse_command_line_arguments (unsigned int nb_args, 
			      char **args, 
			      unsigned int *nodes) {
  unsigned int i;

  /* First remove the application's name from the arguments */
  if (!nb_args)
    return -1;
  nb_args--;
  args++;
  
  /* Fill the nodes array */
  for (i = 0; i < nb_args; i++) {
    nodes[i] = atoi(args[i]);
  }

  return 0;
}
