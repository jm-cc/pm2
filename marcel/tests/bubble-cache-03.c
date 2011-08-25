/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008, 2009 "the PM2 team" (see AUTHORS file)
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


#if !defined(MARCEL_LIBPTHREAD) && defined(MARCEL_BUBBLES) && defined(WHITE_BOX)


int main(int argc, char *argv[])
{
	/* An unrealistic topology: 5 nodes, 
	   each one has 1 CPU, each one has 5 threads...  */
	static const char topology_description[] = "5 1 3";

#ifdef MARCEL_SMT_ENABLED
	/* A flat bubble hierarchy.  */
	static const unsigned bubble_hierarchy_description[] = { 15, 0 };
#else
	static const unsigned bubble_hierarchy_description[] = { 5, 0 };
#endif


	/* The expected result, i.e., the scheduling entity distribution that we
	   expect from Cache.  */

#define BUBBLE MA_BUBBLE_ENTITY
#define THREAD MA_THREAD_ENTITY

	static const tree_t result_vp = {
		.children_count = 0,
		.entity_count = 1,
		.entities = {THREAD}
	};

	static const tree_t result_logical_core = {
		.children_count = 1,
		.children = {&result_vp},
		.entity_count = 0
	};

	static const tree_t result_cpu = {
#ifdef MARCEL_SMT_ENABLED
		.children_count = 3,
		.children = {&result_logical_core, &result_logical_core,
			     &result_logical_core, &result_logical_core},
#else
		.children_count = 1,
		.children = {&result_logical_core},
#endif
		.entity_count = 0
	};

	static const tree_t result_node = {
		.children_count = 1,
		.children = {&result_cpu},
		.entity_count = 0
	};

	static const tree_t result_root = {	/* root of the expected result */
		.children_count = 5,
		.children = {&result_node, &result_node,
			     &result_node, &result_node,
			     &result_node},
		.entity_count = 1,
		.entities = {BUBBLE}
	};

#undef THREAD
#undef BUBBLE

	return test_marcel_bubble_scheduler(argc, argv,
					    &marcel_bubble_cache_sched_class,
					    topology_description,
					    bubble_hierarchy_description, &result_root);
}


#else


int main(int argc TBX_UNUSED, char *argv[]TBX_UNUSED)
{
	return MARCEL_TEST_SKIPPED;
}


#endif
