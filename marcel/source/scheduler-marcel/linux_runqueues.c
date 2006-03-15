
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
MA_DEFINE_RUNQUEUE(ma_cpu_runqueue)[MARCEL_NBMAXCPUS];

ma_runqueue_t *ma_level_runqueues[] = {ma_node_runqueue, ma_die_runqueue, ma_core_runqueue, ma_cpu_runqueue};
#endif

void init_rq(ma_runqueue_t *rq, char *name, enum ma_rq_type type)
{
	int j, k;
	ma_prio_array_t *array;

	LOG_IN();

	ma_holder_init(&rq->hold, MA_RUNQUEUE_HOLDER);
	rq->nr_switches = 0;
	strncpy(rq->name,name,sizeof(rq->name)-1);
	rq->name[sizeof(rq->name)-1]=0;
	//expired_timestamp, timestamp_last_tick
	rq->active = rq->arrays;
	rq->expired = rq->arrays + 1;
//	rq->best_expired_prio = MA_MAX_PRIO;

//	INIT_LIST_HEAD(&rq->migration_queue);
//	ma_atomic_set(&rq->nr_iowait, 0);

	for (j = 0; j < 2; j++) {
		array = rq->arrays + j;
		for (k = 0; k < MA_MAX_PRIO; k++) {
			INIT_LIST_HEAD(ma_array_queue(array, k));
			__ma_clear_bit(k, array->bitmap);
		}
		// delimiter for bitsearch
		__ma_set_bit(MA_MAX_PRIO, array->bitmap);
	}
#ifdef MA__LWPS
	rq->father = NULL;
	// level set by topology
	// cpuset set by topology
#endif
	rq->type = type;

	LOG_OUT();
}
