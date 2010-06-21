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


#ifndef __INLINEFUNCTIONS_MARCEL_HOLDER_H__
#define __INLINEFUNCTIONS_MARCEL_HOLDER_H__


#include "scheduler-marcel/marcel_holder.h"
#include "marcel_debug.h"
#include "marcel_sched_generic.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal inline functions **/
/** \brief Dynamic initializer for ::ma_holder structures. */
static __tbx_inline__ void ma_holder_init(ma_holder_t * h, enum marcel_holder type)
{
	h->type = type;
	ma_spin_lock_init(&h->lock);
	TBX_INIT_FAST_LIST_HEAD(&h->ready_entities);
	h->nb_ready_entities = 0;
}

#ifdef MA__BUBBLES
static __tbx_inline__ marcel_bubble_t *ma_bubble_holder(ma_holder_t * h)
{
	return tbx_container_of(h, marcel_bubble_t, as_holder);
}
#endif
static __tbx_inline__ ma_runqueue_t *ma_rq_holder(ma_holder_t * h)
{
	return tbx_container_of(h, ma_runqueue_t, as_holder);
}

static __tbx_inline__ marcel_task_t *ma_task_entity(marcel_entity_t * e)
{
	MA_BUG_ON(e->type != MA_THREAD_ENTITY && e->type != MA_THREAD_SEED_ENTITY);
	return tbx_container_of(e, marcel_task_t, as_entity);
}

#ifdef MA__BUBBLES
static __tbx_inline__ marcel_bubble_t *ma_bubble_entity(marcel_entity_t * e)
{
	MA_BUG_ON(e->type != MA_BUBBLE_ENTITY);
	return tbx_container_of(e, marcel_bubble_t, as_entity);
}
#endif

static __tbx_inline__ ma_holder_t *ma_entity_active_holder(marcel_entity_t * e)
{
	ma_holder_t *holder;
	if ((holder = e->ready_holder))
		/* currently ready */
		return holder;
	/* not ready, current runqueue */
	MARCEL_SCHED_LOG("using current queue for blocked %p\n", e);
#ifdef MA__BUBBLES
	if ((holder = e->sched_holder))
		return holder;
	MARCEL_SCHED_LOG("using default queue for %p\n", e);
	return e->natural_holder;
#else
	return e->sched_holder;
#endif
}

static inline ma_holder_t *ma_entity_holder_rawlock(marcel_entity_t * e)
{
	ma_holder_t *h, *h2;
	h = ma_entity_active_holder(e);
      again:
	if (!h)
		return NULL;
	MARCEL_SCHED_LOG("ma_entity_holder_rawlocking(%p)\n", h);
	ma_holder_rawlock(h);
	if (tbx_unlikely(h != (h2 = ma_entity_active_holder(e)))) {
		MARCEL_SCHED_LOG("ma_entity_holder_rawunlocking(%p)\n", h);
		ma_holder_rawunlock(h);
		h = h2;
		goto again;
	}
	MARCEL_SCHED_LOG("ma_entity_holder_rawlocked(%p)\n", h);
	return h;
}

static inline ma_holder_t *ma_entity_holder_lock(marcel_entity_t * e)
{
	ma_holder_t *h, *h2;
	ma_preempt_disable();
	h = ma_entity_active_holder(e);
      again:
	if (!h)
		return NULL;
	MARCEL_SCHED_LOG("ma_entity_holder_locking(%p)\n", h);
	if (tbx_unlikely(!ma_holder_trylock(h))) {
#ifdef MA__LWPS
		ma_holder_preempt_lock(h);
#endif
	}

	if (tbx_unlikely(h != (h2 = ma_entity_active_holder(e)))) {
		MARCEL_SCHED_LOG("ma_entity_holder_unlocking(%p)\n", h);
		ma_holder_rawunlock(h);
		h = h2;
		goto again;
	}
	MARCEL_SCHED_LOG("ma_entity_holder_locked(%p)\n", h);
	return h;
}

static inline ma_holder_t *ma_entity_holder_lock_softirq(marcel_entity_t * e)
{
	ma_local_bh_disable();
	return ma_entity_holder_lock(e);
}

static inline ma_holder_t *ma_entity_holder_lock_softirq_async(marcel_entity_t * e)
{
	ma_holder_t *h, *h2;
	ma_local_bh_disable();
	ma_preempt_disable();
	h = ma_entity_active_holder(e);
      again:
	if (!h)
		return NULL;
	MARCEL_SCHED_LOG("ma_entity_holder_locking_async(%p)\n", h);
	if (tbx_unlikely(!ma_holder_trylock(h)))
		return NULL;
	if (tbx_unlikely(h != (h2 = ma_entity_active_holder(e)))) {
		MARCEL_SCHED_LOG("ma_entity_holder_unlocking_async(%p)\n", h);
		ma_holder_rawunlock(h);
		h = h2;
		goto again;
	}
	MARCEL_SCHED_LOG("ma_entity_holder_locked_async(%p)\n", h);
	return h;
}

static __tbx_inline__ void ma_entity_holder_rawunlock(ma_holder_t * h)
{
	MARCEL_SCHED_LOG("ma_entity_holder_rawunlock(%p)\n", h);
	if (h)
		ma_holder_rawunlock(h);
}

static __tbx_inline__ void ma_entity_holder_unlock(ma_holder_t * h)
{
	MARCEL_SCHED_LOG("ma_entity_holder_unlock(%p)\n", h);
	if (h)
		ma_holder_unlock(h);
	else
		ma_preempt_enable();
}

static __tbx_inline__ void ma_entity_holder_unlock_softirq(ma_holder_t * h)
{
	MARCEL_SCHED_LOG("ma_entity_holder_unlock_softirq(%p)\n", h);
	if (h)
		ma_holder_unlock_softirq(h);
	else {
		ma_preempt_enable_no_resched();
		ma_local_bh_enable();
	}
}

/* topology */

/* ma_holder_rq - go up through holders and get to a rq */
static __tbx_inline__ ma_runqueue_t *ma_to_rq_holder(ma_holder_t * h)
{
	ma_holder_t *hh;
#ifdef MA__BUBBLES
	for (hh = h; hh && ma_holder_type(hh) != MA_RUNQUEUE_HOLDER;) {
		/* TODO: n'a plus de sens, enlever ready_holder */
		if (ma_bubble_holder(hh)->as_entity.ready_holder != hh)
			hh = ma_bubble_holder(hh)->as_entity.ready_holder;
		else if (ma_bubble_holder(hh)->as_entity.sched_holder != hh)
			hh = ma_bubble_holder(hh)->as_entity.sched_holder;
		else
			break;
	}
#else
	hh = h;
#endif
	return hh && ma_holder_type(hh) == MA_RUNQUEUE_HOLDER ? ma_rq_holder(hh) : NULL;
}

/* Activations et désactivations nécessitent que le holder soit verrouillé. */

/* Attention, lorsque le holder est une bulle, il se peut qu'ils déverrouillent
 * le holder pour pouvoir verrouiller celui qui le contient immédiatement */

/* activation */

static __tbx_inline__ void ma_set_ready_holder(marcel_entity_t * e, ma_holder_t * h)
{
	MARCEL_SCHED_LOG("holder %p [%s]: accounting entity %p [%s]\n", h, h->name, e,
			 e->name);
	MA_BUG_ON(e->ready_holder_data);
	MA_BUG_ON(e->ready_holder);
	MA_BUG_ON(e->sched_holder
		  && ma_holder_type(h) != ma_holder_type(e->sched_holder));
	MA_BUG_ON(!ma_holder_check_locked(h));
	e->ready_holder = h;
	if ((e->prio >= MA_BATCH_PRIO) && (e->prio != MA_LOWBATCH_PRIO))
		tbx_fast_list_add(&e->ready_entities_item, &h->ready_entities);
	else
		tbx_fast_list_add_tail(&e->ready_entities_item, &h->ready_entities);
	h->nb_ready_entities++;
}

static __tbx_inline__ void ma_rq_enqueue_entity(marcel_entity_t * e, ma_runqueue_t * rq)
{
	MA_BUG_ON(e->ready_holder != &rq->as_holder);
	MA_BUG_ON(!ma_holder_check_locked(&rq->as_holder));
	ma_array_enqueue_entity(e, rq->active);
}

/*
 * Note: if h may be a bubble the caller has to call
 * ma_bubble_try_to_wake_and_unlock() instead of unlocking it when it is done
 * with it (else the bubble may be left asleep).
 */
static __tbx_inline__ void ma_enqueue_entity(marcel_entity_t * e, ma_holder_t * h)
{
	MARCEL_SCHED_LOG("holder %p [%s]: enqueuing entity %p [%s]\n", h, h->name, e,
			 e->name);
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		ma_rq_enqueue_entity(e, ma_rq_holder(h));
	else
		ma_bubble_enqueue_entity(e, ma_bubble_holder(h));
}

/*
 * Call this instead of unlocking a holder if ma_activate_*() or ma_enqueue_*()
 * was called on it, unless it is known to be a runqueue.
 */
static __tbx_inline__ void ma_holder_try_to_wake_up_and_rawunlock(ma_holder_t * h)
{
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		ma_holder_rawunlock(h);
	else
		ma_bubble_try_to_wake_up_and_rawunlock(ma_bubble_holder(h));
}

static __tbx_inline__ void ma_holder_try_to_wake_up_and_unlock(ma_holder_t * h)
{
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		ma_holder_unlock(h);
	else
		ma_bubble_try_to_wake_up_and_unlock(ma_bubble_holder(h));
}

static __tbx_inline__ void ma_holder_try_to_wake_up_and_unlock_softirq(ma_holder_t * h)
{
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		ma_holder_unlock_softirq(h);
	else
		ma_bubble_try_to_wake_up_and_unlock_softirq(ma_bubble_holder(h));
}

/* deactivation */

static __tbx_inline__ void ma_clear_ready_holder(marcel_entity_t * e, ma_holder_t * h)
{
	MARCEL_SCHED_LOG("holder %p [%s]: unaccounting entity %p [%s]\n", h, h->name, e,
			 e->name);
	MA_BUG_ON(e->ready_holder_data);
	MA_BUG_ON(h->nb_ready_entities <= 0);
	MA_BUG_ON(!ma_holder_check_locked(h));
	h->nb_ready_entities--;
	tbx_fast_list_del(&e->ready_entities_item);
	MA_BUG_ON(e->ready_holder != h);
	e->ready_holder = NULL;
}

static __tbx_inline__ void ma_rq_dequeue_entity(marcel_entity_t * e,
						ma_runqueue_t * rq TBX_UNUSED)
{
	MA_BUG_ON(!ma_holder_check_locked(&rq->as_holder));
	ma_array_dequeue_entity(e, (ma_prio_array_t *) e->ready_holder_data);
	MA_BUG_ON(e->ready_holder != &rq->as_holder);
}

static __tbx_inline__ void ma_dequeue_entity(marcel_entity_t * e, ma_holder_t * h)
{
	MARCEL_SCHED_LOG("holder %p [%s]: dequeuing entity %p [%s]\n", h, h->name, e, e->name);
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		ma_rq_dequeue_entity(e, ma_rq_holder(h));
	else
		ma_bubble_dequeue_entity(e, ma_bubble_holder(h));
}

static __tbx_inline__ void ma_task_check(marcel_task_t * t TBX_UNUSED)
{
#ifdef PM2_BUG_ON
	if (MA_TASK_IS_READY(t)) {
		/* check that it is reachable from some runqueue */
		ma_holder_t *h = ma_task_ready_holder(t);
		MA_BUG_ON(!h);
#ifdef MA__BUBBLES
		if (h->type == MA_BUBBLE_HOLDER) {
			marcel_bubble_t *b = ma_bubble_holder(h);
			MA_BUG_ON(!b->as_entity.ready_holder);
			MA_BUG_ON(!b->as_entity.ready_holder_data);
			MA_BUG_ON(tbx_fast_list_empty(&b->cached_entities));
		}
#endif
	}
#endif
}

static __tbx_inline__ const char *ma_entity_state_msg(int state)
{
	const char *r;

	switch (state) {
	case MA_ENTITY_SLEEPING:
		r = "sleeping";
		break;

	case MA_ENTITY_READY:
		r = "ready";
		break;

	case MA_ENTITY_RUNNING:
		r = "running";
		break;

	default:
		r = "unknown";
		break;
	}

	return r;
}

static __tbx_inline__ int __tbx_warn_unused_result__ ma_get_entity(marcel_entity_t * e)
{
	int state;
	ma_holder_t *h;

	state = MA_ENTITY_SLEEPING;
	if ((h = e->ready_holder)) {

		if (e->ready_holder_data) {
			state = MA_ENTITY_READY;
			ma_dequeue_entity(e, h);
		} else
			state = MA_ENTITY_RUNNING;
		ma_clear_ready_holder(e, h);
	}
#ifdef MA__BUBBLES
	if (e->type == MA_BUBBLE_ENTITY) {
		/* detach bubble */
		state = MA_ENTITY_READY;
		ma_set_sched_holder(e, ma_bubble_entity(e), 0);
	}
#endif
	MARCEL_SCHED_LOG("holder %p [%s]: getting entity %p [%s] with state %d [%s]\n", h,
			 h->name, e, e->name, state, ma_entity_state_msg(state));
	return state;
}

static __tbx_inline__ void _ma_put_entity_check(marcel_entity_t * e BUBBLE_VAR_UNUSED,
						ma_holder_t * h TBX_UNUSED)
{
#ifdef MA__BUBBLES
	if (h->type == MA_BUBBLE_HOLDER) {
		MA_BUG_ON(h != e->natural_holder);
		if (e->type == MA_BUBBLE_ENTITY)
			PROF_EVENT2(bubble_sched_bubble_goingback, ma_bubble_entity(e),
				    ma_bubble_holder(h));
		else
			PROF_EVENT2(bubble_sched_goingback, ma_task_entity(e),
				    ma_bubble_holder(h));
	} else
#endif
	{
		MA_BUG_ON(h->type != MA_RUNQUEUE_HOLDER);
		PROF_EVENT2(bubble_sched_switchrq,
#ifdef MA__BUBBLES
			    e->type == MA_BUBBLE_ENTITY ? (void *) ma_bubble_entity(e) :
#endif
			    (void *) ma_task_entity(e), ma_rq_holder(h));
	}
}

static __tbx_inline__ void ma_put_entity(marcel_entity_t * e, ma_holder_t * h, int state)
{

	MARCEL_SCHED_LOG("holder %p [%s]: putting entity %p [%s] with state %d [%s]\n", h,
			 h->name, e, e->name, state, ma_entity_state_msg(state));
	_ma_put_entity_check(e, h);
#ifdef MA__BUBBLES
	if (h->type == MA_BUBBLE_HOLDER) {
		/* Don't directly enqueue in holding bubble, but in the thread cache. */
		marcel_bubble_t *b = ma_bubble_holder(h);
		while (b->as_entity.sched_holder
		       && b->as_entity.sched_holder->type == MA_BUBBLE_HOLDER) {
			h = b->as_entity.sched_holder;
			b = ma_bubble_holder(h);
		}
	}
#endif

	e->sched_holder = h;

	if (state == MA_ENTITY_SLEEPING)
		return;

#ifdef MA__BUBBLES
	if (!(e->type == MA_BUBBLE_ENTITY && h->type == MA_BUBBLE_HOLDER))
#endif
		ma_set_ready_holder(e, h);

	if (state == MA_ENTITY_READY) {
#ifdef MA__BUBBLES
		if (h->type == MA_BUBBLE_HOLDER) {
			marcel_bubble_t *b = ma_bubble_holder(h);
			if (e->type == MA_BUBBLE_ENTITY)
				/* Recursively set the new holder. */
				ma_set_sched_holder(e, b, 0);
			else
				/* Just enqueue, corresponding runqueue must be locked */
				__ma_bubble_enqueue_entity(e, b);
		} else
#endif
			ma_rq_enqueue_entity(e, ma_rq_holder(h));
	}
}

static __tbx_inline__ void ma_move_entity(marcel_entity_t * e, ma_holder_t * h)
{
	/* TODO: optimiser ! */
	int state = ma_get_entity(e);
	ma_put_entity(e, h, state);
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_MARCEL_HOLDER_H__ **/
