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


/* This header provides an automatic test framework for bubble schedulers.
   The idea is to generate topologies and bubble hierarchies, submit them to
   a bubble scheduler, and check whether the resulting scheduling entity
   distribution matches our expectations.  */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/* Ask for the complete definition of `marcel_bubble_sched_t'.  */
#define MARCEL_INTERNAL_INCLUDE

#include <marcel.h>

#ifndef MA__BUBBLES
# error "You need bubble scheduling support for this test."
#endif

#ifndef MA__NUMA
# error "Synthetic topology support depends on NUMA support."
#endif


/* Logging.  */

/* The stream where verbose debugging output should be sent.  */
static FILE *verbose_output = NULL;


static void
indent (FILE *out, unsigned count)
{
	unsigned i;

	for (i = 0; i < count; i++)
		fprintf (out, " ");
}

/* Debugging message buffering.  */

#define MAX_MESSAGE_COUNT  256
#define MAX_MESSAGE_LENGTH 128

static char messages[MAX_MESSAGE_COUNT][MAX_MESSAGE_LENGTH];
static unsigned message_count = 0;

static void level_log (FILE *, const struct marcel_topo_level *,
											 const char *, ...)
#ifdef __GNUC__
	__attribute__ ((__format__ (__printf__, 3, 4)))
#endif
	;

/* A nice level-sensitive logging facility.  */
static void
level_log (FILE *out, const struct marcel_topo_level *level,
					 const char *format, ...)
{
	va_list args;
	char prefix[100];
	char *long_format;
	unsigned indent;

	indent = level->level * 3;
	memset (prefix, ' ', indent);

	snprintf (prefix + indent, sizeof (prefix) - indent - 1,
						"[%2u] ", level->number);
	long_format = alloca (strlen (format) + strlen (prefix) + 2);
	strcpy (long_format, prefix);
	strcat (long_format, format);
	strcat (long_format, "\n");

	va_start (args, format);
	if (out == NULL)
		{
			/* Buffer the message.  */
			assert (message_count < MAX_MESSAGE_COUNT);
			vsnprintf (messages[message_count++], sizeof (messages[0]),
								 long_format, args);
		}
	else
		vfprintf (out, long_format, args);
	va_end (args);
}

static void
dump_log (FILE *out)
{
	unsigned i;

	for (i = 0; i < message_count; i++)
		fprintf (out, "%s", messages[i]);
}


/* Topology pretty printing.  */

/* Print the topology rooted at LEVEL to OUTPUT in a human-readable way.  */
static void
print_topology (struct marcel_topo_level *level, FILE *output,
								unsigned indent_level)
{
  unsigned i;

	indent (output, indent_level);
  marcel_print_level (level, output, 1, 0, " ", "#", ":", "");

  for (i = 0; i < level->arity; i++)
    print_topology (level->children[i], output, indent_level + 2);
}


/* Bubble hierarchies.  */

static void *
phony_thread_entry_point (void *arg)
{
	ma_atomic_t *exit_signal;

	/* We have to poll here rather than wait on a condition variable or a
		 barrier to make sure that the thread remains on a runqueue.  */
	exit_signal = (ma_atomic_t *) arg;
	while (!ma_atomic_read (exit_signal));

	return NULL;
}

static void
populate_bubble_hierarchy (marcel_bubble_t *bubble, const unsigned *level_breadth,
													 int use_main_thread,
													 ma_atomic_t *thread_exit_signal)
{
  unsigned i;

  /* Recursion ends when *LEVEL_BREADTH is zero, meaning that BUBBLE has no
     children.  */
  if (*level_breadth > 0)
    {
      if (* (level_breadth + 1) == 0)
				{
					/* Creating threads as leaves of the hierarchy.  */
					marcel_t thread;
					marcel_attr_t attr;
					unsigned thread_count;

					marcel_attr_init (&attr);
					marcel_attr_setinitbubble (&attr, bubble);

					thread_count = *level_breadth;

					if (use_main_thread)
						{
							/* We must account for the main thread, thus we put it in the
								 first bubble we create and we're left with fewer threads to
								 create.  */
							marcel_bubble_inserttask (bubble, marcel_self ());
							thread_count -= 1;
						}

					if (verbose_output)
						printf ("creating %u threads\n", thread_count);

					for (i = 0; i < thread_count; i++)
						/* Note: We can't use `dontsched' since THREAD would not appear
							 on the runqueue.  */
						marcel_create (&thread, &attr, phony_thread_entry_point,
													 (void *) thread_exit_signal);
				}
      else
				{
					/* Creating child bubbles.  */
					marcel_bubble_t *child;

					if (verbose_output)
						printf ("creating %u bubbles\n", *level_breadth);

					for (i = 0; i < *level_breadth; i++)
						{
							child = malloc (sizeof (*child));
							if (child != NULL)
								{
									marcel_bubble_init (child);
									marcel_bubble_insertbubble (bubble, child);

									/* Recurse into CHILD.  */
									populate_bubble_hierarchy (child, level_breadth + 1,
																						 use_main_thread,
																						 thread_exit_signal);

									/* The main thread is no longer available.  */
									use_main_thread = 0;
								}
							else
								abort ();
						}
				}
    }
}

/* Create a simple bubble hierarchy according to LEVEL_BREADTH.  As a
   side-effect, update `marcel_root_bubble'.  All newly created threads will
   poll *THREAD_EXIT_SIGNAL and exit when it's non-zero.  */
static marcel_bubble_t *
make_simple_bubble_hierarchy (const unsigned *level_breadth,
															ma_atomic_t *thread_exit_signal)
{
  populate_bubble_hierarchy (&marcel_root_bubble, level_breadth, 1,
														 thread_exit_signal);

  return &marcel_root_bubble;
}


/* Poor man's pattern matching.  This provides the tools that allow us to
   check whether the scheduling entity distribution produced by a bubble
   scheduler corresponds to what we expect.  */

#define MAX_TREE_BREADTH 16

/* The node of a tree representing scheduling entity distributions over a
   topology.  */
struct tree_item
{
  /* Child nodes.  */
  unsigned                children_count;
  const struct tree_item *children[MAX_TREE_BREADTH];

  /* Entities found on this node of the topology.  */
  unsigned                entity_count;
  enum marcel_entity      entities[MAX_TREE_BREADTH];
};

typedef struct tree_item tree_t;


/* Return true (non-zero) if the distribution of scheduling entities (threads
   and bubbles) rooted at LEVEL matches EXPECTED.  */
static int
topology_matches_tree_p (const struct marcel_topo_level *level,
												 const tree_t *expected)
{
  marcel_entity_t *e;
  unsigned entities, child;

  /* Make sure the list of entities matches.  */
  entities = 0;
  for_each_entity_scheduled_on_runqueue (e, &level->rq)
    {
      if (entities >= expected->entity_count)
				{
					level_log (stderr, level,"more entities than expected (%lu, expected %u)",
										 level->rq.as_holder.nr_ready,
										 expected->entity_count);
					return 0;
				}

      if (expected->entities[entities] != e->type)
				{
					level_log (stderr, level, "entity %u has type %i (expected %i)",
										 entities, e->type, expected->entities[entities]);
					return 0;
				}

      entities++;
    }

  level_log (verbose_output, level, "%u entities (expected %u)",
						 entities, expected->entity_count);
  if (entities != expected->entity_count)
    return 0;

  /* Make sure children match, recursively.  */
  level_log (verbose_output, level, "%u children (expected %u)",
						 level->arity, expected->children_count);
  if (expected->children_count != level->arity)
    return 0;

  for (child = 0; child < level->arity; child++)
    {
      if (!topology_matches_tree_p (level->children[child],
																		expected->children[child]))
				return 0;
    }

  return 1;
}


/* High-level test framework.  */

/* Run a test program that checks whether distributing the bubble hierarchy
	 described by BUBBLE_HIERARCHY_DESCRIPTION over the topology described by
	 TOPOLOGY_DESCRIPTION yields the entity distribution described by
	 EXPECTED_RESULT.  Return zero on success.  */
static int
test_marcel_bubble_scheduler (int argc, char *argv[],
															marcel_bubble_sched_t *bubble_scheduler,
															const char *topology_description,
															const unsigned *bubble_hierarchy_description,
															const tree_t *expected_result)
{
  int ret, matches_p, i;
  char **new_argv;
  marcel_bubble_t *root_bubble;
	ma_atomic_t thread_exit_signal;

	for (i = 1; i < argc; i++)
		{
			if (!strcmp ("--verbose", argv[i]))
				verbose_output = stdout;
		}

  /* Pass the topology description to Marcel.  Yes, it looks hackish to
     communicate with the library via command-line arguments.  */
  new_argv = alloca ((argc + 2) * sizeof (*new_argv));
  new_argv[0] = argv[0];
  new_argv[1] = (char *) "--marcel-synthetic-topology";
  new_argv[2] = (char *) topology_description;
  memcpy (&new_argv[3], &argv[1], argc * sizeof (*argv));
	argc += 2;

  /* Initialize Marcel.  It should pick use the "fake" topology described in
		 TOPOLOGY_DESCRIPTION.  */
  marcel_init (&argc, new_argv);
	marcel_ensure_abi_compatibility (MARCEL_HEADER_HASH);

	if (verbose_output)
		print_topology (&marcel_machine_level[0], stdout, 0);

 	/* Before creating any threads, initialize the variable that they'll
 		 poll.  */
 	ma_atomic_init (&thread_exit_signal, 0);

	/* Create a bubble hierarchy.  */
  root_bubble = make_simple_bubble_hierarchy (bubble_hierarchy_description,
																							&thread_exit_signal);

  marcel_bubble_change_sched (bubble_scheduler);

	/* Submit the generated bubble hierarchy to Affinity.  */
  marcel_bubble_sched_begin ();

	/* Did we get what we expected?  We need to lock the whole topology and
		 bubble hiearchy so that the entity distribution doesn't change while we
		 examine it.  */
	marcel_thread_preemption_disable ();
	ma_bubble_lock_all (&marcel_root_bubble, marcel_machine_level);

	matches_p = topology_matches_tree_p (marcel_machine_level, expected_result);

	ma_bubble_unlock_all (&marcel_root_bubble, marcel_machine_level);
	marcel_thread_preemption_enable ();

	if (matches_p)
		printf ("PASS: scheduling entities were distributed as expected\n");
	else
		printf ("FAIL: scheduling entities were NOT distributed as expected\n");

	if ((!verbose_output) && (!matches_p))
		/* Give feedback as to what failed.  */
		dump_log (stderr);

	ret = (matches_p ? 0 : 1);


	/* Tell threads to leave.  */
	ma_atomic_inc (&thread_exit_signal);

	marcel_end ();

  return ret;
}

/*
	 Local Variables:
	 tab-width: 2
	 End:

	 vim:ts=2
 */
