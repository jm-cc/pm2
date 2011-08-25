/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


/* This example program tests whether threads and bubbles are created
   at the right location.

   The marcel_root_bubble is initially located on core 0. We create
   bubbles in it, containing threads. These entities are NEVER
   distributed by any bubble scheduler.

   That's why we could infer that their sched_holder is constant
   (actually, equal to the marcel_root_bubble, which is the lower node
   of the entities tree to be directly scheduled on a runqueue.
*/


#include <marcel.h>


#if defined(MA__BUBBLES) && defined(WHITE_BOX)


static void start_new_team(void *(*fn) (void *), void *data, unsigned nthreads);

static void *my_inner_job(void *arg TBX_UNUSED)
{
#ifdef VERBOSE_TESTS
	marcel_printf("Hi!\n");
#endif
	return NULL;
}

static void *my_job(void *arg)
{
	int nb_threads = *(int *) arg;

	/* Nested bubble creation. */
	start_new_team(my_inner_job, NULL, nb_threads - 1);

	return NULL;
}

static void start_new_team(void *(*fn) (void *), void *data, unsigned nthreads)
{
	/* Initialize the thread attributes. */
	marcel_attr_t thread_attr;
	marcel_attr_init(&thread_attr);
	marcel_attr_setdetachstate(&thread_attr, tbx_true);
	marcel_attr_setpreemptible(&thread_attr, 0);
	marcel_attr_setprio(&thread_attr, MA_DEF_PRIO);

	/* Initialize the borning bubble. */
	marcel_bubble_t team_bubble;
	marcel_bubble_init(&team_bubble);
	marcel_bubble_scheduleonthreadholder(&team_bubble);
	marcel_bubble_setprio(&team_bubble, MA_DEF_PRIO);
	marcel_bubble_insertbubble(marcel_bubble_holding_task
				   (marcel_self()), &team_bubble);
	marcel_bubble_inserttask(&team_bubble, marcel_self());

	marcel_attr_setnaturalbubble(&thread_attr, &team_bubble);

	unsigned int i;
	marcel_t threads[nthreads];

	/* Launch the threads. */
	for (i = 0; i < nthreads; i++)
		marcel_create(&threads[i], &thread_attr, fn, data);

	/* The main thread does its part of the job too. */
	fn(data);

	/* Check the created threads' location. */
	marcel_entity_t *e = NULL;
	for_each_entity_held_in_bubble(e, &team_bubble)
	    MA_BUG_ON(e->sched_holder != &marcel_root_bubble.as_holder);

	/* Close the current team. */
	marcel_bubble_t *holding_bubble = marcel_bubble_holding_bubble(&team_bubble);
	marcel_bubble_inserttask(holding_bubble, marcel_self());
	marcel_bubble_join(&team_bubble);
	marcel_bubble_destroy(&team_bubble);
}

int main(int argc, char **argv)
{
	int nb_teams;
	int threads_per_team;

	if (argc < 2) {
		nb_teams = 2;
		threads_per_team = 2;
	} else {
		nb_teams = atoi(argv[1]);
		threads_per_team = atoi(argv[2]);
	}

	if (!nb_teams || !threads_per_team) {
		marcel_fprintf(stderr,
			       "Bad argument: nb_teams == 0 or nb_threads_per_team == 0\n");
		return EXIT_FAILURE;
	}

	marcel_init(&argc, argv);

	start_new_team(my_job, &threads_per_team, nb_teams - 1);

	marcel_end();

	return MARCEL_TEST_SUCCEEDED;
}


#else


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
