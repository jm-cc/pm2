/* MaMI --- NUMA Memory Interface
 *
 * Copyright (C) 2008, 2009, 2010, 2011  Centre National de la Recherche Scientifique
 *
 * MaMI is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * MaMI is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU Lesser General Public License in COPYING.LGPL for more details.
 */
#define MARCEL_INTERNAL_INCLUDE

#include <marcel.h>
#include <mami.h>
#include <mami_helper.h>
#include <mami_private.h>
#include <numa.h>
#include <numaif.h>
#include <errno.h>
#include <malloc.h>
#include <time.h>
#include "marcel_stream.h"

#if defined(MAMI_ENABLED)

#define TAB_SIZE 1024*1024*16
#define NB_TIMES 20

enum mbind_policy {
  BIND_POL,
  INTERLEAVE_POL,
  MAMI,
  MAMI_NEXT_TOUCH
};

static double **a, **b, **c;
static marcel_barrier_t barrier;

static void usage (void);
static int parse_command_line_arguments (unsigned int nb_args,
					 char **args,
					 enum mbind_policy *mpol,
					 unsigned int *nb_memory_nodes,
					 unsigned int *memory_nodes,
                                         int nb_teams);


static void print_welcoming_message (unsigned int nb_teams,
				     unsigned int nb_threads,
				     enum mbind_policy mpol,
				     unsigned int nb_memory_nodes,
				     unsigned int *memory_nodes);

static
double my_delay (struct timespec *t1, struct timespec *t2) {
  return ((double) t2->tv_sec + (double) t2->tv_nsec * 1.E-09) - ((double) t1->tv_sec + (double) t1->tv_nsec * 1.E-09);
}

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
  unsigned int i, team;
  struct timespec t1, t2;
  mami_manager_t *memory_manager;

  mami_init(&memory_manager, argc, argv);

  /* Variables to be filled when parsing command line arguments */
  unsigned int nb_teams = memory_manager->max_node + 1;
  unsigned int nb_threads = marcel_nbvps();
  enum mbind_policy mpol;
  unsigned int nb_memory_nodes = 0;
  unsigned int memory_nodes[nb_teams + 1];

  if (argc < 2) {
    usage ();
    marcel_fprintf (stderr, "No parameters specified. Using <interleave> policy ...\n\n");
    mpol = INTERLEAVE_POL;
  }
  else {
    int err_parse = parse_command_line_arguments (argc, argv, &mpol, &nb_memory_nodes, memory_nodes, nb_teams);
    if (err_parse < 0) {
      usage ();
      exit (1);
    }
  }

 /* Print a pretty and welcoming message */
  print_welcoming_message (nb_teams, nb_threads, mpol, nb_memory_nodes, memory_nodes);

  marcel_t working_threads[nb_teams][nb_threads];
  marcel_attr_t thread_attr[nb_teams][nb_threads];
  marcel_bubble_t bubbles[nb_teams];
  marcel_bubble_t main_bubble;
  unsigned long nodemasks[nb_teams];
  marcel_barrier_init (&barrier, NULL, (nb_threads * nb_teams));

  /* The main thread is thread 0 of team 0. */
  working_threads[0][0] = marcel_self ();

  /* Set the nodemask according to the memory policy passed in
     argument. */
  if (mpol ==  INTERLEAVE_POL) {
    nodemasks[0] = 0;
    for (i = 0; i < nb_teams ; i++) {
      nodemasks[0] |= (1 << i);
    }
    for (team = 1; team < nb_teams; team++) {
      nodemasks[team] = nodemasks[0];
    }
  } else if (mpol == BIND_POL) {
    for (team = 0; team < nb_teams; team++) {
      nodemasks[team] = 0;
      nodemasks[team] |= (1 << memory_nodes[team]);
    }
  }

  /* Bind the accessed data to the nodes. */
  if (mpol == MAMI_NEXT_TOUCH || mpol == MAMI) {
    a = mami_malloc (memory_manager, nb_teams * sizeof (double *), MAMI_MEMBIND_POLICY_DEFAULT, 0);
    b = mami_malloc (memory_manager, nb_teams * sizeof (double *), MAMI_MEMBIND_POLICY_DEFAULT, 0);
    c = mami_malloc (memory_manager, nb_teams * sizeof (double *), MAMI_MEMBIND_POLICY_DEFAULT, 0);
  }
  else {
    a = marcel_malloc (nb_teams * sizeof (double *));
    b = marcel_malloc (nb_teams * sizeof (double *));
    c = marcel_malloc (nb_teams * sizeof (double *));
  }

  for (i = 0; i < nb_teams; i++) {
    if (mpol == MAMI_NEXT_TOUCH) {
      a[i] = mami_malloc(memory_manager, tab_len, MAMI_MEMBIND_POLICY_DEFAULT, 0);
      b[i] = mami_malloc(memory_manager, tab_len, MAMI_MEMBIND_POLICY_DEFAULT, 0);
      c[i] = mami_malloc(memory_manager, tab_len, MAMI_MEMBIND_POLICY_DEFAULT, 0);
    }
    else if (mpol == MAMI) {
      a[i] = mami_malloc(memory_manager, tab_len, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, memory_nodes[i]);
      b[i] = mami_malloc(memory_manager, tab_len, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, memory_nodes[i]);
      c[i] = mami_malloc(memory_manager, tab_len, MAMI_MEMBIND_POLICY_SPECIFIC_NODE, memory_nodes[i]);
    }
    else {
      a[i] = memalign (getpagesize(), tab_len);
      b[i] = memalign (getpagesize(), tab_len);
      c[i] = memalign (getpagesize(), tab_len);

      int err_mbind;
      err_mbind = _mami_mbind (a[i], tab_len, mpol == BIND_POL ? MPOL_BIND : MPOL_INTERLEAVE, nodemasks + i, nb_teams + 1, MPOL_MF_MOVE);
      err_mbind += _mami_mbind (b[i], tab_len, mpol == BIND_POL ? MPOL_BIND : MPOL_INTERLEAVE, nodemasks + i, nb_teams + 1, MPOL_MF_MOVE);
      err_mbind += _mami_mbind (c[i], tab_len, mpol == BIND_POL ? MPOL_BIND : MPOL_INTERLEAVE, nodemasks + i, nb_teams + 1, MPOL_MF_MOVE);

      if (err_mbind < 0) {
        perror ("mbind");
        exit (1);
      }
    }
  }
  marcel_thread_preemption_disable ();

  stream_struct_t stream_struct[nb_teams];

  marcel_bubble_init (&main_bubble);
  marcel_bubble_insertbubble (&marcel_root_bubble, &main_bubble);

  /* Create the working threads. */
  for (team = 0; team < nb_teams; team++) {
    marcel_bubble_init (&bubbles[team]);
    marcel_bubble_insertbubble (&main_bubble, &bubbles[team]);
    /* Make the main thread part of team 0. */
    if (team == 0) {
      marcel_thread_setid(marcel_self (), 0);
      marcel_bubble_inserttask (&bubbles[team], marcel_self ());
    }
    STREAM_init (&stream_struct[team], nb_threads, TAB_SIZE, a[team], b[team], c[team]);

    if (mpol == MAMI_NEXT_TOUCH) {
      mami_migrate_on_next_touch(memory_manager, a[team]);
      mami_migrate_on_next_touch(memory_manager, b[team]);
      mami_migrate_on_next_touch(memory_manager, c[team]);
    }

    for (i = 0; i < nb_threads; i++) {
      int node;
      if (team == 0 && i == 0) {
	/* Avoid creating the main thread again :-) */
	goto attach_mem;
      }
      marcel_attr_init (&thread_attr[team][i]);
      marcel_attr_setpreemptible (&thread_attr[team][i], tbx_false);
      marcel_attr_setnaturalbubble (&thread_attr[team][i], &bubbles[team]);
      marcel_attr_setid (&thread_attr[team][i], i);
      marcel_create (&working_threads[team][i], &thread_attr[team][i], f, &stream_struct[team]);
    attach_mem:
      if (mpol == MAMI || mpol == MAMI_NEXT_TOUCH) {
	/* FIXME: mami_attach () can't be called more than once on the same data. */
        mami_task_attach(memory_manager, a[team], tab_len, working_threads[team][i], &node);
        mami_task_attach(memory_manager, b[team], tab_len, working_threads[team][i], &node);
        mami_task_attach(memory_manager, c[team], tab_len, working_threads[team][i], &node);
      }
      else {
        for (node = 0; node < marcel_nbnodes; node++) {
          ((long *) marcel_task_stats_get (working_threads[team][i], MEMNODE))[node] = (node == memory_nodes[team]) ? tab_len : 0;
        }
      }
    }
  }

  marcel_bubble_sched_begin ();

  marcel_printf ("Threads created and ready to work!\n");

  clock_gettime (CLOCK_MONOTONIC, &t1);
  /* The main thread has to do his job like everyone else. */
  f (&stream_struct[0]);
  clock_gettime (CLOCK_MONOTONIC, &t2);

  /* Put the main thread back to the root bubble before joining team 0. */
  marcel_bubble_inserttask (&marcel_root_bubble, marcel_self ());

  /* Wait for the working threads to finish. */
  for (team = 0; team < nb_teams; team++) {
    marcel_bubble_join (&bubbles[team]);
  }
  marcel_bubble_join (&main_bubble);

  if (mpol == MAMI_NEXT_TOUCH || mpol == MAMI) {
    for (i = 0; i < nb_teams; i++) {
      mami_free(memory_manager, a[i]);
      mami_free(memory_manager, b[i]);
      mami_free(memory_manager, c[i]);
    }
    mami_free(memory_manager, a);
    mami_free(memory_manager, b);
    mami_free(memory_manager, c);
  }
  else {
    for (i = 0; i < nb_teams; i++) {
      free (a[i]);
      free (b[i]);
      free (c[i]);
    }
    marcel_free (a);
    marcel_free (b);
    marcel_free (c);
  }

  double average_time = my_delay (&t1, &t2) / (double)(NB_TIMES);
  marcel_printf ("Test computed in %lfs!\n", average_time);
  marcel_printf ("Estimated rate (MB/s): %11.4f!\n", (double)(10 * tab_len * 1E-06)/ average_time);

  mami_exit(&memory_manager);
  marcel_end ();
  return 0;
}

static void
usage () {
  marcel_fprintf (stderr, "Usage: ./memteams <mbind policy> node1 node2 .. nodeN\n");
  marcel_fprintf (stderr, "            -<mbind policy>: The mbind policy used to bind accessed data\n");
  marcel_fprintf (stderr, "               'bind' to bind each array on each node\n");
  marcel_fprintf (stderr, "               'interleave' to distribute them over the nodes\n");
  marcel_fprintf (stderr, "               'mami' to use mami to allocate the memory\n");
  marcel_fprintf (stderr, "               'mami_next_touch' to use mami and the policy migrate on next touch\n");
  marcel_fprintf (stderr, "            -<node1 node2 .. nodeN>: The list of nodes you want the arrays to be bound to (arrayi is bound to nodei, if the memory policy is 'interleave', this list is ignored).\n");
}

static void
print_welcoming_message (unsigned int nb_teams,
			 unsigned int nb_threads,
			 enum mbind_policy mpol,
			 unsigned int nb_memory_nodes,
			 unsigned int *memory_nodes) {
  unsigned int i;
  marcel_printf ("Launching memteams-bubbles with %u %s of %u %s.\n",
		 nb_teams,
		 nb_teams > 1 ? "bubbles" : "bubble",
		 nb_threads,
		 nb_threads > 1 ? "threads" : "thread");
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
			      enum mbind_policy *mpol,
			      unsigned int *nb_memory_nodes,
			      unsigned int *memory_nodes,
                              int nb_teams) {
  unsigned int i;

  /* First remove the application's name from the arguments */
  if (!nb_args)
    return -1;
  nb_args--;
  args++;

  /* Fill the mbind policy */
  if (!nb_args)
    return -1;
  if (!strcmp (args[0], "bind")) {
    *mpol = BIND_POL;
  } else if (!strcmp (args[0], "interleave")) {
    *mpol = INTERLEAVE_POL;
  } else if (!strcmp (args[0], "mami_next_touch")) {
    *mpol = MAMI_NEXT_TOUCH;
  } else if (!strcmp (args[0], "mami")) {
    *mpol = MAMI;
  } else {
    return -1;
  }
  nb_args--;
  args++;

  if (*mpol == INTERLEAVE_POL || *mpol == MAMI_NEXT_TOUCH) {
    if (nb_args) {
      marcel_printf ("'interleave' or 'mami_next_touch' option detected, ignoring node list.\n");
    }
    return 0;
  }

  /* Eventually fill the nodes array */
  *nb_memory_nodes = 0;
  for (i = 0; (i < nb_args) && (*nb_memory_nodes < nb_teams + 1); i++) {
    memory_nodes[i] = atoi(args[i]);
    *nb_memory_nodes = *nb_memory_nodes + 1;
  }

  if (*nb_memory_nodes != nb_teams) {
    marcel_printf ("Error: you specified %i nodes, but %i were expected.\n", *nb_memory_nodes, nb_teams);
    return -1;
  }

  return 0;
}

#else
int main(int argc, char * argv[]) {
  fprintf(stderr, "This application needs MaMI to be enabled\n");
}
#endif
