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
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <sys/time.h>
#include <sys/resource.h>

#define TAB_SIZE 1024*1024*64
#define NB_ITER 1024*1024
#define NB_TIMES 20
#define ACCESS_PATTERN_SIZE 1024*1024

enum mbind_policy {
  BIND_POL,
  INTERLEAVE_POL
};

enum sched_policy {
  LOCAL_POL,
  DISTRIBUTED_POL
};

static int **tab;
static int begin = 0;
static marcel_barrier_t barrier;

struct thread_args {
  unsigned int team;
  unsigned long *access_pattern_vector;
};

static void usage (void);
static int parse_command_line_arguments (unsigned int nb_args, 
					 char **args, 
					 enum sched_policy *spol, 
					 enum mbind_policy *mpol);
					 

static void print_welcoming_message (unsigned int nb_teams,
				     unsigned int nb_threads,
				     enum sched_policy spol,
				     enum mbind_policy mpol);

static void 
initialize_access_pattern_vector (unsigned long *access_pattern_vector, 
				   unsigned int access_pattern_len,
				   unsigned int tab_size) {
  unsigned int team, thread, i;
  for (i = 0; i < access_pattern_len; i++) {
    access_pattern_vector[i] = marcel_random () % tab_size;
  }
}

static 
void * f (void *arg) {
  struct thread_args *ta = (struct thread_args *)arg;
  unsigned long *access_pattern_vector = ta->access_pattern_vector;
  unsigned int team = ta->team;
  unsigned int i, j;

  for (i = 0; i < NB_TIMES; i++) {
    marcel_barrier_wait (&barrier);
    
    /* Let's do the job. */
    for (j = 0; j < NB_ITER; j++) {
      int dummy = tab[team][access_pattern_vector[i % ACCESS_PATTERN_SIZE]];
      tab[team][access_pattern_vector[(NB_ITER - i) % ACCESS_PATTERN_SIZE]] = dummy;
    }
  }

  return 0;
}

int
main (int argc, char **argv) 
{
  marcel_attr_t thread_attr;
  unsigned long tab_len = TAB_SIZE * sizeof (int);
  unsigned int i, team;
  tbx_tick_t t1, t2;

  marcel_init (&argc, argv);
  tbx_timing_init ();

  if (argc < 3) {
    usage ();
    return -1;
  }

  /* Variables to be filled when parsing command line arguments */
  unsigned int nb_teams = numa_max_node () + 1;
  unsigned int nb_threads = marcel_vpset_weight (&marcel_topo_node_level[0].vpset);
  enum sched_policy spol;
  enum mbind_policy mpol;

  int err_parse = parse_command_line_arguments (argc, argv, &spol, &mpol);
  if (err_parse < 0) {
    usage ();
    exit (1);
  }

 /* Print a pretty and welcoming message */
  print_welcoming_message (nb_teams, nb_threads, spol, mpol);

  marcel_t working_threads[nb_teams][nb_threads];
  unsigned long nodemasks[nb_teams];
  unsigned long ***access_pattern;
  marcel_barrier_init (&barrier, NULL, (nb_threads * nb_teams) + 1);

  /* Set the nodemask according to the memory policy passed in
     argument. */
  if (mpol ==  INTERLEAVE_POL) {
    for (team = 0; team < nb_teams; team++) {
      nodemasks[team] = 0;
      for (i = 0; i < numa_max_node () + 1; i++) {
	nodemasks[team] |= (1 << i);
      }
    }
  } else {
    for (team = 0; team < nb_teams; team++) {
      nodemasks[team] = 0;
      nodemasks[team] |= (1 << team);
    }
  }

  /* Bind the accessed data to the nodes. */
  tab = memalign (getpagesize (), nb_teams * sizeof (int *));
  for (team = 0; team < nb_teams; team++) {    
    tab[team] = memalign (getpagesize (), tab_len);
    int err_mbind;
    if (mpol == BIND_POL) {
      err_mbind = mbind (tab[team], 
			 tab_len, 
			 MPOL_BIND, 
			 nodemasks + team, 
			 numa_max_node () + 2, 
			 MPOL_MF_MOVE);
    } else {
      err_mbind = mbind (tab[team], 
			 tab_len, 
			 MPOL_INTERLEAVE, 
			 nodemasks + team, 
			 numa_max_node () + 2, 
			 MPOL_MF_MOVE);
    }
    if (err_mbind < 0) {
      perror ("mbind");
      exit (1);
    }
  }

  marcel_attr_init (&thread_attr);
  marcel_attr_setpreemptible (&thread_attr, tbx_false);
  marcel_thread_preemption_disable ();

  access_pattern = numa_alloc_onnode (nb_teams * sizeof (unsigned long **), 0);

  /* Create the working threads. */
  for (team = 0; team < nb_teams; team++) {
    access_pattern[team] = numa_alloc_onnode (nb_threads * sizeof (unsigned long *), 0);
    for (i = 0; i < nb_threads; i++) {
      unsigned int dest_node = spol == LOCAL_POL ? team : ((team * nb_threads) + i) % (numa_max_node () + 1);
      access_pattern[team][i] = numa_alloc_onnode (ACCESS_PATTERN_SIZE * sizeof (unsigned long), dest_node);
      initialize_access_pattern_vector (access_pattern[team][i], ACCESS_PATTERN_SIZE, TAB_SIZE);
      struct thread_args ta = {
	.team = team,
	.access_pattern_vector = access_pattern[team][i]
      };
      marcel_attr_settopo_level (&thread_attr, 
				 &marcel_topo_node_level[dest_node]);
      marcel_create (&working_threads[team][i], &thread_attr, f, &ta);
    }
  }
  marcel_printf ("Threads created and ready to work!\n");

  TBX_GET_TICK (t1);
  for (i = 0; i < NB_TIMES; i++) {
    marcel_barrier_wait (&barrier);
  }
  TBX_GET_TICK (t2); 
    
  /* Wait for the working threads to finish. */
  for (team = 0; team < nb_teams; team++) {
    for (i = 0; i < nb_threads; i++) {
      marcel_join (working_threads[team][i], NULL);
    }
  }

  for (team = 0; team < nb_teams; team++) {
    free (tab[team]);
  }
  free (tab);

  marcel_printf ("Test computed in %lfs!\n", (double)TBX_TIMING_DELAY(t1, t2) / (NB_TIMES * 1000000));

  marcel_end ();
  return 0;
}

static void
usage () {
  marcel_fprintf (stderr, "Usage: ./memteams <sched policy> <mbind policy>\n");
  marcel_fprintf (stderr, "                 -sched policy: The way the created teams will be scheduled (can be 'local' for moving the whole team of threads to the same numa node or 'distributed' to distribute the threads over the nodes).\n");
  marcel_fprintf (stderr, "                 -mbind policy: The mbind policy used to bind accessed data (can be 'bind' to bind each array on each node or 'interleave' to distribute them over the nodes).\n");
}

static void
print_welcoming_message (unsigned int nb_teams,
			 unsigned int nb_threads,
			 enum sched_policy spol,
			 enum mbind_policy mpol) {
  unsigned int i;
  marcel_printf ("Launching memteams with %u %s of %u %s %s.\n", 
		 nb_teams,
		 nb_teams > 1 ? "teams" : "team",
		 nb_threads,
		 nb_threads > 1 ? "threads" : "thread",
		 spol == LOCAL_POL ? "scheduled together" : "spreaded over the machine");
  marcel_printf ("The global arrays are %s.", (mpol == BIND_POL) ? "locally allocated" : "distributed over the nodes");
  marcel_printf ("\n\n");
}

static int
parse_command_line_arguments (unsigned int nb_args,
			      char **args,
			      enum sched_policy *spol,
			      enum mbind_policy *mpol) {
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

  return 0;
}
