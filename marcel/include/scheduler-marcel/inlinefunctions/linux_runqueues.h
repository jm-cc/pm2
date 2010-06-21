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


#ifndef __INLINEFUNCTIONS_LINUX_RUNQUEUES_H__
#define __INLINEFUNCTIONS_LINUX_RUNQUEUES_H__


#include "scheduler-marcel/linux_runqueues.h"
#include "marcel_debug.h"
#include "marcel_descr.h"
#include "linux_bitops.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal inline functions **/
static __tbx_inline__ void ma_array_dequeue_entity(marcel_entity_t * e,
						   ma_prio_array_t * array)
{
	MARCEL_SCHED_LOG("dequeueing %p (prio %d) from %p\n", e, e->prio, array);
	if (e->prio != MA_NOSCHED_PRIO)
		array->nr_active--;
	tbx_fast_list_del(&e->cached_entities_item);
	if (tbx_fast_list_empty(ma_array_queue(array, e->prio))) {
		MARCEL_SCHED_LOG("array %p (prio %d) empty\n", array, e->prio);
		__ma_clear_bit(e->prio, array->bitmap);
	}
	MA_BUG_ON(!e->ready_holder_data);
	e->ready_holder_data = NULL;
}

static __tbx_inline__ void ma_array_dequeue_task(marcel_task_t * p,
						 ma_prio_array_t * array)
{
	MARCEL_SCHED_LOG("dequeueing %d:%s (prio %d) from %p\n", p->number,
			 p->as_entity.name, p->as_entity.prio, array);
	ma_array_dequeue_entity(&p->as_entity, array);
}

static __tbx_inline__ void ma_array_enqueue_entity(marcel_entity_t * e,
						   ma_prio_array_t * array)
{
	MARCEL_SCHED_LOG("enqueueing %p (prio %d) in %p\n", e, e->prio, array);
	if ((e->prio >= MA_BATCH_PRIO) && (e->prio != MA_LOWBATCH_PRIO))
		tbx_fast_list_add(&e->cached_entities_item,
				  ma_array_queue(array, e->prio));
	else
		tbx_fast_list_add_tail(&e->cached_entities_item,
				       ma_array_queue(array, e->prio));
	__ma_set_bit(e->prio, array->bitmap);
	if (e->prio != MA_NOSCHED_PRIO)
		array->nr_active++;
	MA_BUG_ON(e->ready_holder_data);
	e->ready_holder_data = array;
}

static __tbx_inline__ void ma_array_enqueue_task(marcel_task_t * p,
						 ma_prio_array_t * array)
{
	MARCEL_SCHED_LOG("enqueueing %d:%s (prio %d) in %p\n", p->number,
			 p->as_entity.name, p->as_entity.prio, array);
	ma_array_enqueue_entity(&p->as_entity, array);
}

static __tbx_inline__ void ma_array_entity_list_add(struct tbx_fast_list_head *head,
						    marcel_entity_t * e,
						    ma_prio_array_t * array,
						    ma_runqueue_t * rq)
{
	MA_BUG_ON(e->ready_holder_data);
	tbx_fast_list_add_tail(&e->cached_entities_item, head);
	e->ready_holder_data = array;
	MA_BUG_ON(e->ready_holder);
	e->ready_holder = &rq->as_holder;
}

static __tbx_inline__ void ma_array_enqueue_entity_list(struct tbx_fast_list_head *head,
							int num, int prio,
							ma_prio_array_t * array,
							ma_runqueue_t * rq TBX_UNUSED)
{
	tbx_fast_list_splice(head, ma_array_queue(array, prio));
	__ma_set_bit(prio, array->bitmap);
	array->nr_active += num;
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_LINUX_RUNQUEUES_H__ **/
