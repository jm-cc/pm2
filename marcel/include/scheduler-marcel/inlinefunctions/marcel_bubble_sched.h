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


#ifndef __INLINEFUNCTIONS_MARCEL_BUBBLE_SCHED_H__
#define __INLINEFUNCTIONS_MARCEL_BUBBLE_SCHED_H__


#include "scheduler-marcel/marcel_bubble_sched.h"
#include "marcel_sched_generic.h"


/** Public inline functions **/
/**
 * \brief Return the size of an instance of \param klass .  */
static __tbx_inline__ size_t
marcel_bubble_sched_instance_size(const marcel_bubble_sched_class_t *klass) {
        return klass->instance_size;
}

/**
 * \brief Return the name of \param klass .  */
static __tbx_inline__ const char *
marcel_bubble_sched_class_name(const marcel_bubble_sched_class_t *klass) {
        return klass->name;
}

/**
 * \brief Return the \param scheduler class. */
static __tbx_inline__ const marcel_bubble_sched_class_t *
marcel_bubble_sched_class(const marcel_bubble_sched_t *scheduler) {
	return scheduler->klass;
}


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
/* This version (with __) assumes that the runqueue holding b is already locked  */
static __tbx_inline__ void __ma_bubble_enqueue_entity(marcel_entity_t *e, marcel_bubble_t *b) {
#ifdef MA__BUBBLES
	bubble_sched_debugl(7,"enqueuing %p in bubble %p\n",e,b);
	MA_BUG_ON(e->ready_holder != &b->as_holder);
	MA_BUG_ON(!ma_holder_check_locked(&b->as_holder));
	if (tbx_fast_list_empty(&b->cached_entities)) {
		ma_holder_t *h = b->as_entity.ready_holder;
		bubble_sched_debugl(7,"first running entity in bubble %p\n",b);
		if (h) {
			MA_BUG_ON(ma_holder_type(h) != MA_RUNQUEUE_HOLDER);
			MA_BUG_ON(!ma_holder_check_locked(h));
			if (!b->as_entity.ready_holder_data)
				ma_rq_enqueue_entity(&b->as_entity, ma_rq_holder(h));
		}
	}
	if ((e->prio >= MA_BATCH_PRIO) && (e->prio != MA_LOWBATCH_PRIO))
		tbx_fast_list_add(&e->cached_entities_item, &b->cached_entities);
	else
		tbx_fast_list_add_tail(&e->cached_entities_item, &b->cached_entities);
	MA_BUG_ON(e->ready_holder_data);
	e->ready_holder_data = (void *)1;
#endif
}
static __tbx_inline__ void __ma_bubble_dequeue_entity(marcel_entity_t *e, marcel_bubble_t *b) {
#ifdef MA__BUBBLES
	bubble_sched_debugl(7,"dequeuing %p from bubble %p\n",e,b);
	MA_BUG_ON(!ma_holder_check_locked(&b->as_holder));
	tbx_fast_list_del(&e->cached_entities_item);
	if (tbx_fast_list_empty(&b->cached_entities)) {
		ma_holder_t *h = b->as_entity.ready_holder;
		bubble_sched_debugl(7,"last running entity in bubble %p\n",b);
		if (h && ma_holder_type(h) == MA_RUNQUEUE_HOLDER) {
			MA_BUG_ON(!ma_holder_check_locked(h));
			if (b->as_entity.ready_holder_data)
				ma_rq_dequeue_entity(&b->as_entity, ma_rq_holder(h));
		}
	}
	MA_BUG_ON(!e->ready_holder_data);
	e->ready_holder_data = NULL;
	MA_BUG_ON(e->ready_holder != &b->as_holder);
#endif
}

/* This version doesn't assume that the runqueue holding b is already locked.
 * It thus can not wake the bubble when needed, so the caller has to call
 * ma_bubble_try_to_wake_and_rawunlock() instead of unlocking it when it is done
 * with it.
 */
static __tbx_inline__ void ma_bubble_enqueue_entity(marcel_entity_t *e, marcel_bubble_t *b) {
#ifdef MA__BUBBLES
	bubble_sched_debugl(7,"enqueuing %p in bubble %p\n",e,b);
	MA_BUG_ON(!ma_holder_check_locked(&b->as_holder));
	if ((e->prio >= MA_BATCH_PRIO) && (e->prio != MA_LOWBATCH_PRIO))
		tbx_fast_list_add(&e->cached_entities_item, &b->cached_entities);
	else
		tbx_fast_list_add_tail(&e->cached_entities_item, &b->cached_entities);
	MA_BUG_ON(e->ready_holder_data);
	e->ready_holder_data = (void *)1;
#endif
}

/*
 * B, still locked, has seen entities being enqueued on it by the
 * ma_bubble_enqueue_entity() function above. We may have to wake b if it
 * wasn't awake and these entities are awake. In that case we have to unlock
 * the holding runqueue.
 *
 * Note: this function raw-unlocks b.
 */
static __tbx_inline__ void ma_bubble_try_to_wake_up_and_rawunlock(marcel_bubble_t *b)
{
#ifdef MA__BUBBLES
	ma_holder_t *h = b->as_entity.ready_holder;
	MA_BUG_ON(!ma_holder_check_locked(&b->as_holder));

	if (tbx_fast_list_empty(&b->cached_entities) || b->as_entity.ready_holder_data || !h) {
		/* No awake entity or B already awake, just unlock */
		ma_holder_rawunlock(&b->as_holder);
		return;
	}

	/* B holds awake entities but is not awake, we have to unlock it so as
	 * to be able to lock the runqueue because of the locking conventions.
	 */
	bubble_sched_debugl(7,"waking up bubble %p because of its containing awake entities\n",b);

	MA_BUG_ON(ma_holder_type(h) != MA_RUNQUEUE_HOLDER);
	ma_holder_rawunlock(&b->as_holder);
	h = ma_bubble_holder_rawlock(b);
	ma_holder_rawlock(&b->as_holder);
	if (!tbx_fast_list_empty(&b->cached_entities) && h) {
		MA_BUG_ON(ma_holder_type(h) != MA_RUNQUEUE_HOLDER);
		if (!b->as_entity.ready_holder_data)
			ma_rq_enqueue_entity(&b->as_entity, ma_rq_holder(h));
	}
	ma_bubble_holder_rawunlock(h);
	ma_holder_rawunlock(&b->as_holder);
#endif
}

static __tbx_inline__ void ma_bubble_try_to_wake_up_and_unlock(marcel_bubble_t *b) {
	ma_bubble_try_to_wake_up_and_rawunlock(b);
	ma_preempt_enable();
}

static __tbx_inline__ void ma_bubble_try_to_wake_up_and_unlock_softirq(marcel_bubble_t *b) {
	ma_bubble_try_to_wake_up_and_rawunlock(b);
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
}

static __tbx_inline__ void ma_bubble_dequeue_entity(marcel_entity_t *e, marcel_bubble_t *b TBX_UNUSED) {
#ifdef MA__BUBBLES
	MA_BUG_ON(!ma_holder_check_locked(&b->as_holder));
	bubble_sched_debugl(7,"dequeuing %p from bubble %p\n",e,b);
	tbx_fast_list_del(&e->cached_entities_item);
	MA_BUG_ON(!e->ready_holder_data);
	e->ready_holder_data = NULL;
	MA_BUG_ON(e->ready_holder != &b->as_holder);
#endif
}

/** \brief Tell whether bubble \e b is preemptible, i.e., whether one of its
 * threads can be preempted by a thread from another bubble.  */
static __tbx_inline__ void marcel_bubble_set_preemptible(marcel_bubble_t *b, tbx_bool_t preemptible) {
#ifdef MA__BUBBLES
	b->not_preemptible = !preemptible;
#endif
}

/** \brief Return true if bubble \e b is preemptible, i.e., if one of its
 * threads can be preempted by a thread from another bubble.  */
static __tbx_inline__ tbx_bool_t marcel_bubble_is_preemptible(const marcel_bubble_t *b) {
#ifdef MA__BUBBLES
	return !b->not_preemptible;
#else
	return tbx_true;
#endif
}


#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_MARCEL_BUBBLE_SCHED_H__ **/
