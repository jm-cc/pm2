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

#include "bubble-testing.h"


int
main (int argc, char *argv[])
{
	/* An opto-dual-core (e.g., `hagrid') with MARCEL_TOPO_MAX_ARITY >= 8.  */
  static const char topology_description[] = "8 2";

	/* A two-level bubble hierarchy.  */
  static const unsigned bubble_hierarchy_description[] =
    { 4, 4, 0 };


  /* The expected result, i.e., the scheduling entity distribution that we
     expect from Affinity.  */

#define BUBBLE MA_BUBBLE_ENTITY
#define THREAD MA_THREAD_ENTITY

  static const tree_t result_core =
    { .children_count = 0,
      .entity_count = 1,
      .entities = { THREAD } };

  static const tree_t result_cpu =
    { .children_count = 2,
      .children = { &result_core, &result_core },
      .entity_count = 0 };

  static const tree_t result_root = /* root of the expected result */
    { .children_count = 8,
      .children = { &result_cpu, &result_cpu,
			              &result_cpu, &result_cpu,
			              &result_cpu, &result_cpu,
			              &result_cpu, &result_cpu },
      .entity_count = 5,
      .entities = { BUBBLE, BUBBLE,
			              BUBBLE, BUBBLE,
			              BUBBLE } };

#undef THREAD
#undef BUBBLE

	return test_marcel_bubble_scheduler (argc, argv,
																			 &marcel_bubble_affinity_sched,
																			 topology_description,
																			 bubble_hierarchy_description,
																			 &result_root);
}
