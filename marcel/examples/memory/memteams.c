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

#define MARCEL_INTERNAL_INCLUDE

#include <marcel.h>
#include <numa.h>
#include <numaif.h>
#include <errno.h>
#include <malloc.h>
#include <time.h>
# include <float.h>
#include "../marcel_stream.h"

#if defined(MARCEL_MAMI_ENABLED)

#define TAB_SIZE 1024*1024*4
#define NB_TIMES 20

enum mbind_policy {
  BIND_POL,
  INTERLEAVE_POL
};

enum sched_policy {
  LOCAL_POL,
  DISTRIBUTED_POL
};

# ifndef MIN
# define MIN(x,y) ((x)<(y)?(x):(y))
# endif
# ifndef MAX
# define MAX(x,y) ((x)>(y)?(x):(y))
# endif

# define HLINE "-------------------------------------------------------------\n"

static double **a, **b, **c;
static double	avgtime[4] = {0}, maxtime[4] = {0},
  mintime[4] = {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX};

static char	*label[4] = {"Copy:      ", "Scale:     ",
			     "Add:       ", "Triad:     "};

static double	bytes[4] = {
  2 * sizeof(double) * TAB_SIZE,
  2 * sizeof(double) * TAB_SIZE,
  3 * sizeof(double) * TAB_SIZE,
  3 * sizeof(double) * TAB_SIZE
};

static marcel_barrier_t barrier;

static void usage (void);
static int parse_command_line_arguments (unsigned int nb_args,
					 char **args,
					 enum sched_policy *spol,
					 enum mbind_policy *mpol,
					 unsigned int *nb_memory_nodes,
					 unsigned int *memory_nodes);


static void print_welcoming_message (unsigned int nb_teams,
				     unsigned int nb_threads,
				     enum sched_policy spol,
				     enum mbind_policy mpol,
				     unsigned int nb_memory_nodes,
				     unsigned int *memory_nodes);

double mysecond()
{
  struct timeval tp;
  struct timezone tzp;
  int i;

  i = gettimeofday(&tp,&tzp);
  return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

static
void * f (void *arg) {
  stream_struct_t *stream_struct = (stream_struct_t *)arg;
  unsigned int i;

  marcel_barrier_wait (&barrier);

  for (i = 0; i < NB_TIMES; i++) {
    /* Let's do the job. */
    STREAM_copy (stream_struct);
    marcel_barrier_wait (&barrier);
    STREAM_scale (stream_struct, 3.0);
    marcel_barrier_wait (&barrier);
    STREAM_add (stream_struct);
    marcel_barrier_wait (&barrier);
    STREAM_triad (stream_struct, 3.0);
    marcel_barrier_wait (&barrier);
  }

  return 0;
}

int
main (int argc, char **argv)
{
  unsigned long tab_len = TAB_SIZE * sizeof (double);
  unsigned int i, team;
  double times[4][NB_TIMES];

  marcel_init (&argc, argv);

  if (argc < 3) {
    usage ();
    return -1;
  }

  /* Variables to be filled when parsing command line arguments */
  unsigned int nb_teams = numa_max_node () + 1;
  unsigned int nb_threads = marcel_vpset_weight (&marcel_topo_node_level[0].vpset);
  enum sched_policy spol;
  enum mbind_policy mpol;
  unsigned int nb_memory_nodes = 0;
  unsigned int memory_nodes[numa_max_node () + 1];

  int err_parse = parse_command_line_arguments (argc, argv, &spol, &mpol, &nb_memory_nodes, memory_nodes);
  if (err_parse < 0) {
    usage ();
    exit (1);
  }

 /* Print a pretty and welcoming message */
  print_welcoming_message (nb_teams, nb_threads, spol, mpol, nb_memory_nodes, memory_nodes);

  marcel_t working_threads[nb_teams][nb_threads];
  marcel_attr_t thread_attr[nb_teams][nb_threads];
  unsigned long nodemasks[nb_teams];
  marcel_barrier_init (&barrier, NULL, (nb_threads * nb_teams));

  /* The main thread is thread 0 of team 0. */
  working_threads[0][0] = marcel_self ();
  marcel_self ()->id = 0;

  /* Set the nodemask according to the memory policy passed in
     argument. */
  if (mpol ==  INTERLEAVE_POL) {
    nodemasks[0] = 0;
    for (i = 0; i < numa_max_node () + 1; i++) {
      nodemasks[0] |= (1 << i);
    }
    for (team = 1; team < nb_teams; team++) {
      nodemasks[team] = nodemasks[0];
    }
  } else {
    for (team = 0; team < nb_teams; team++) {
      nodemasks[team] = 0;
      nodemasks[team] |= (1 << memory_nodes[team]);
    }
  }

  /* Bind the accessed data to the nodes. */
  a = marcel_malloc (nb_teams * sizeof (double *), __FILE__, __LINE__);
  b = marcel_malloc (nb_teams * sizeof (double *), __FILE__, __LINE__);
  c = marcel_malloc (nb_teams * sizeof (double *), __FILE__, __LINE__);

  for (i = 0; i < nb_teams; i++) {
    a[i] = memalign (getpagesize(), tab_len);
    b[i] = memalign (getpagesize(), tab_len);
    c[i] = memalign (getpagesize(), tab_len);

    int err_mbind;
    err_mbind = mbind (a[i], tab_len, mpol == BIND_POL ? MPOL_BIND : MPOL_INTERLEAVE, nodemasks + i, numa_max_node () + 2, MPOL_MF_MOVE);
    err_mbind += mbind (b[i], tab_len, mpol == BIND_POL ? MPOL_BIND : MPOL_INTERLEAVE, nodemasks + i, numa_max_node () + 2, MPOL_MF_MOVE);
    err_mbind += mbind (c[i], tab_len, mpol == BIND_POL ? MPOL_BIND : MPOL_INTERLEAVE, nodemasks + i, numa_max_node () + 2, MPOL_MF_MOVE);

    if (err_mbind < 0) {
      perror ("mbind");
      exit (1);
    }
  }
  marcel_thread_preemption_disable ();

  stream_struct_t stream_struct[nb_teams];

  /* Create the working threads. */
  for (team = 0; team < nb_teams; team++) {
    STREAM_init (&stream_struct[team], nb_threads, TAB_SIZE, a[team], b[team], c[team]);

    for (i = 0; i < nb_threads; i++) {
      if (team == 0 && i == 0) {
	/* Avoid creating the main thread again :-) */
	continue;
      }
      unsigned int dest_node = spol == LOCAL_POL ? team : ((team * nb_threads) + i) % (numa_max_node () + 1);
      marcel_attr_init (&thread_attr[team][i]);
      marcel_attr_setpreemptible (&thread_attr[team][i], tbx_false);
      marcel_attr_setid (&thread_attr[team][i], i);
      marcel_attr_settopo_level (&thread_attr[team][i],
				 &marcel_topo_vp_level[dest_node * nb_threads + i]);
      marcel_create (&working_threads[team][i], &thread_attr[team][i], f, &stream_struct[team]);
    }
  }
  marcel_printf ("Threads created and ready to work!\n");

  marcel_barrier_wait (&barrier);

  /* The main thread has to do his job like everyone else. */
  for (i = 0; i < NB_TIMES; i++) {
    times[0][i] = mysecond();
    STREAM_copy (&stream_struct[0]);
    marcel_barrier_wait (&barrier);
    times[0][i] = mysecond() - times[0][i];

    times[1][i] = mysecond();
    STREAM_scale (&stream_struct[0], 3.0);
    marcel_barrier_wait (&barrier);
    times[1][i] = mysecond() - times[1][i];

    times[2][i] = mysecond();
    STREAM_add (&stream_struct[0]);
    marcel_barrier_wait (&barrier);
    times[2][i] = mysecond() - times[2][i];

    times[3][i] = mysecond();
    STREAM_triad (&stream_struct[0], 3.0);
    marcel_barrier_wait (&barrier);
    times[3][i] = mysecond() - times[3][i];
 }

  /* Wait for the working threads to finish. */
  for (team = 0; team < nb_teams; team++) {
    for (i = 0; i < nb_threads; i++) {
      /* Avoid the main thread. */
      if (team == 0 && i == 0) {
	continue;
      }
      marcel_join (working_threads[team][i], NULL);
    }
  }

  for (i = 0; i < nb_teams; i++) {
    free (a[i]);
    free (b[i]);
    free (c[i]);
  }

  marcel_free (a);
  marcel_free (b);
  marcel_free (c);

 /*	--- SUMMARY --- */

  for (i = 1; i < NB_TIMES; i++)
    {
      int j;
      for (j=0; j<4; j++)
	{
	  avgtime[j] = avgtime[j] + times[j][i];
	  mintime[j] = MIN(mintime[j], times[j][i]);
	  maxtime[j] = MAX(maxtime[j], times[j][i]);
	}
    }

  /* First phase results */
  marcel_printf("Function      Rate (MB/s)   Avg time     Min time     Max time\n");
  int j;
  for (j=0; j<4; j++) {
    avgtime[j] = avgtime[j]/(double)(NB_TIMES-1);

    marcel_printf("%s%11.4f  %11.4f  %11.4f  %11.4f\n", label[j],
		  1.0E-06 * bytes[j]/mintime[j],
		  avgtime[j],
		  mintime[j],
		  maxtime[j]);
  }
  marcel_printf(HLINE);

  marcel_end ();
  return 0;
}

static void
usage () {
  marcel_fprintf (stderr, "Usage: ./memteams <sched policy> <mbind policy> node1 node2 .. nodeN\n");
  marcel_fprintf (stderr, "            -sched policy: The way the created teams will be scheduled (can be 'local' for moving the whole team of threads to the same numa node or 'distributed' to distribute the threads over the nodes).\n");
  marcel_fprintf (stderr, "            -mbind policy: The mbind policy used to bind accessed data (can be 'bind' to bind each array on each node or 'interleave' to distribute them over the nodes).\n");
  marcel_fprintf (stderr, "            node1 node2 .. nodeN: The list of nodes you want the arrays to be bound to (arrayi is bound to nodei, if the memory policy is 'interleave', this list is ignored).\n");
}

static void
print_welcoming_message (unsigned int nb_teams,
			 unsigned int nb_threads,
			 enum sched_policy spol,
			 enum mbind_policy mpol,
			 unsigned int nb_memory_nodes,
			 unsigned int *memory_nodes) {
  unsigned int i;
  marcel_printf ("Launching memteams with %u %s of %u %s %s.\n",
		 nb_teams,
		 nb_teams > 1 ? "teams" : "team",
		 nb_threads,
		 nb_threads > 1 ? "threads" : "thread",
		 spol == LOCAL_POL ? "scheduled together" : "spreaded over the machine");
   marcel_printf ("The global array is %s %s", (mpol == BIND_POL) ? "bound to" : "distributed over",
		 nb_memory_nodes > 0 ? "nodes " : "the whole set of nodes the target machine holds ");
  for (i = 0; i < nb_memory_nodes; i++) {
    marcel_printf ("%u ", memory_nodes[i]);
  }
  marcel_printf (".\n\n");

}

static int
parse_command_line_arguments (unsigned int nb_args,
			      char **args,
			      enum sched_policy *spol,
			      enum mbind_policy *mpol,
			      unsigned int *nb_memory_nodes,
			      unsigned int *memory_nodes) {
  unsigned int i;

  /* First remove the application's name from the arguments */
  if (!nb_args)
    return -1;
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

  if (*mpol == INTERLEAVE_POL) {
    if (nb_args) {
      marcel_printf ("'interleave' option detected, ignoring node list.\n");
    }
    return 0;
  }

  /* Eventually fill the nodes array */
  *nb_memory_nodes = 0;
  for (i = 0; (i < nb_args) && (*nb_memory_nodes < numa_max_node () + 2); i++) {
    memory_nodes[i] = atoi(args[i]);
    *nb_memory_nodes = *nb_memory_nodes + 1;
  }

  if (*nb_memory_nodes != numa_max_node () + 1) {
    marcel_printf ("Error: you specified %i nodes, but %i were expected.\n", *nb_memory_nodes, numa_max_node () + 1);
    return -1;
  }

  return 0;
}

#else
int marcel_main(int argc, char * argv[]) {
  marcel_fprintf(stderr, "This application needs MAMI to be enabled\n");
}
#endif
