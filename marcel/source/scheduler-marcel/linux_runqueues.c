
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

ma_runqueue_t ma_dontsched_runqueue;

void ma_init_rq(ma_runqueue_t *rq, const char *name)
{
	int j, k;
	ma_prio_array_t *array;

	LOG_IN();

	ma_holder_init(&rq->as_holder, MA_RUNQUEUE_HOLDER);
	snprintf((rq)->as_holder.name,MARCEL_MAXNAMESIZE,"rq %s",name);
	for (j = 0; j < 1; j++) {
		array = rq->active + j;
		array->nr_active = 0;
		memset(&array->bitmap, 0, sizeof(array->bitmap));
		for (k = 0; k < MA_MAX_PRIO; k++)
			INIT_LIST_HEAD(ma_array_queue(array, k));
		// delimiter for bitsearch
		__ma_set_bit(MA_MAX_PRIO, array->bitmap);
	}
#ifdef MA__LWPS
	rq->father = NULL;
	// level set by topology
	// cpuset set by topology
#endif
	rq->topolevel = NULL; /* will be set by the caller if needed */

	LOG_OUT();
}
