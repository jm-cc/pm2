
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

MA_DEFINE_RUNQUEUE(ma_main_runqueue);
MA_DEFINE_RUNQUEUE(ma_dontsched_runqueue);
#ifdef MA__NUMA
MA_DEFINE_RUNQUEUE(ma_node_runqueue)[MARCEL_NBMAXNODES];
MA_DEFINE_RUNQUEUE(ma_die_runqueue)[MARCEL_NBMAXDIES];
MA_DEFINE_RUNQUEUE(ma_core_runqueue)[MARCEL_NBMAXCORES];

ma_runqueue_t *ma_level_runqueues[] = {ma_node_runqueue, ma_core_runqueue};
#endif

#ifdef MA__LWPS
MA_DEFINE_PER_LWP(ma_runqueue_t, runqueue, {0});
MA_DEFINE_PER_LWP(ma_runqueue_t, dontsched_runqueue, {0});
#endif

#if 0
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

__marcel_init void node_nr_running_init(void)
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
#endif

void init_rq(ma_runqueue_t *rq, char *name, enum ma_rq_type type)
{
	int j, k;
	ma_prio_array_t *array;

	LOG_IN();

	strncpy(rq->name,name,sizeof(rq->name)-1);
	rq->name[sizeof(rq->name)-1]=0;
	rq->type = type;
	rq->active = rq->arrays;
	rq->expired = rq->arrays + 1;
//	rq->best_expired_prio = MA_MAX_PRIO;

	ma_spin_lock_init(&rq->lock);
//	INIT_LIST_HEAD(&rq->migration_queue);
//	ma_atomic_set(&rq->nr_iowait, 0);
//	nr_running_init(rq);

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
	rq->father = NULL;
#endif

	LOG_OUT();
}
