
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

/*
 * Similar to:
 *   kernel/sched.c
 */

#include "marcel.h"

#ifdef MA__LWPS
MA_DEFINE_PER_LWP(ma_runqueue_t, runqueue);
#endif
ma_runqueue_t ma_main_runqueue;

#ifdef MA__LWPS
MA_DEFINE_PER_LWP(ma_runqueue_t, norun_runqueue);
#endif
ma_runqueue_t ma_norun_runqueue;

#ifdef CONFIG_NUMA
/*
 * Keep track of running tasks.
 */

static atomic_t node_nr_running[MAX_NUMNODES] ____cacheline_maxaligned_in_smp =
	{[0 ...MAX_NUMNODES-1] = ATOMIC_INIT(0)};

static inline void nr_running_init(ma_runqueue_t *rq)
{
        rq->node_nr_running = &node_nr_running[0]; // corriger le numro de noeud
}

__init void node_nr_running_init(void)
{
	int i;

	for (i = 0; i < NR_CPUS; i++) {
		if (cpu_possible(i))
			cpu_rq(i)->node_nr_running =
				&node_nr_running[cpu_to_node(i)];
	}
}
#else
# define nr_running_init(rq)   do { } while (0)
#endif

void init_rq(ma_runqueue_t *rq)
{
	int j, k;
	ma_prio_array_t *array;

	LOG_IN();

	rq->active = rq->arrays;
	rq->expired = rq->arrays + 1;
//	rq->best_expired_prio = MA_MAX_PRIO;

	ma_spin_lock_init(&rq->lock);
	INIT_LIST_HEAD(&rq->migration_queue);
	atomic_set(&rq->nr_iowait, 0);
	nr_running_init(rq);

	for (j = 0; j < 2; j++) {
		array = rq->arrays + j;
		for (k = 0; k < MA_MAX_PRIO; k++) {
			INIT_LIST_HEAD(array->queue + k);
			__ma_clear_bit(k, array->bitmap);
		}
		// delimiter for bitsearch
		__ma_set_bit(MA_MAX_PRIO, array->bitmap);
	}

#ifdef MA__LWPS
	if (tbx_likely(rq != &ma_main_runqueue))
		rq->father = &ma_main_runqueue;
	else
		rq->father = NULL;
#endif

	LOG_OUT();
}
