
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

/* disable.c */

#include "bubble-testing.h"

#define N 2
#define DELAY 2

int marcel_main(int argc, char *argv[])
{
	int i;
	ma_atomic_t thread_exit_signal;
	marcel_vpset_t vpset = MARCEL_VPSET_ZERO;
	marcel_bubble_sched_t *scheduler;
	int ret;

#ifdef PROFILE
	profile_activate(FUT_ENABLE, MARCEL_PROF_MASK, 0);
#endif

	marcel_init(&argc, argv);

	for (i = 2; i < marcel_nbvps(); i++)
		if (marcel_random() % 2)
			marcel_vpset_set(&vpset, i);

	/* Force at least some enabled and disabled */
	marcel_vpset_set(&vpset, 0);
	marcel_vpset_clr(&vpset, 1);

	ma_atomic_init (&thread_exit_signal, 0);

	static const unsigned bubble_hierarchy_description1[] = { 2, 3, 2, 0 };
	static const unsigned bubble_hierarchy_description2[] = { 2, 2, 0 };
	static const unsigned bubble_hierarchy_description3[] = { 2, 0 };

	struct marcel_sched_param p = {.sched_priority = MA_DEF_PRIO - 1};
	marcel_sched_setparam(MARCEL_SELF, &p);

	make_simple_bubble_hierarchy (bubble_hierarchy_description1, 0, &thread_exit_signal);
	make_simple_bubble_hierarchy (bubble_hierarchy_description2, 0, &thread_exit_signal);
	//make_simple_bubble_hierarchy (bubble_hierarchy_description3, 0, &thread_exit_signal);

#if 1
	scheduler = alloca(marcel_bubble_sched_instance_size (&marcel_bubble_cache_sched_class));
	ret = marcel_bubble_cache_sched_init((marcel_bubble_cache_sched_t *) scheduler, tbx_false);
	MA_BUG_ON(ret);

	marcel_bubble_change_sched((void*)scheduler);

	/* Submit bubbles */
	marcel_bubble_sched_begin();
#endif

	marcel_start_playing();

	for (i = 0; i < N; i++) {
		marcel_disable_vps(&vpset);
		marcel_printf("nuit\n");
		marcel_sleep(DELAY);
		marcel_enable_vps(&vpset);
		marcel_printf("jour\n");
		marcel_sleep(DELAY);
	}
	marcel_disable_vps(&vpset);

	ma_atomic_inc (&thread_exit_signal);

	free_bubble_hierarchy();

	marcel_end();

#ifdef PROFILE
	profile_stop();
#endif

	return 0;
}



