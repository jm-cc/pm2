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


#ifndef __MARCEL_HOLDER_H__
#define __MARCEL_HOLDER_H__


#include "tbx_fast_list.h"
#include "sys/marcel_flags.h"
#ifdef MM_HEAP_ENABLED
#include "mm_heap_alloc.h"
#endif /* MM_HEAP_ENABLED */
#ifdef __MARCEL_KERNEL__
#endif


/** Public macros **/
#define MARCEL_ENTITY_SELF (&(MARCEL_SELF)->as_entity)

#ifdef MA__LWPS
#define MA_SCHED_LEVEL_INIT .sched_level = MARCEL_LEVEL_DEFAULT,
#else
#define MA_SCHED_LEVEL_INIT
#endif
#ifdef MA__BUBBLES
#define MA_BUBBLE_SCHED_ENTITY_INITIALIZER(e) .natural_entities_item = TBX_FAST_LIST_HEAD_INIT((e).natural_entities_item),
#else
#define MA_BUBBLE_SCHED_ENTITY_INITIALIZER(e)
#endif
#ifdef MM_MAMI_ENABLED
#define MA_SCHED_MEMORY_AREA_INIT(e) \
	.memory_areas_lock = MARCEL_SPINLOCK_INITIALIZER, \
	.memory_areas = TBX_FAST_LIST_HEAD_INIT((e).memory_areas),
#else
#define MA_SCHED_MEMORY_AREA_INIT(e)
#endif
#ifdef MM_HEAP_ENABLED
#define MA_SCHED_ENTITY_HEAP_INIT(e) \
	.heap = NULL,
#else
#define MA_SCHED_ENTITY_HEAP_INIT(e)
#endif

#define MA_SCHED_ENTITY_INITIALIZER(e,t,p) { \
	.type = t, \
	.natural_holder = NULL, .sched_holder = NULL, .ready_holder = NULL, \
	.ready_holder_data = NULL, \
	.cached_entities_item = TBX_FAST_LIST_HEAD_INIT((e).cached_entities_item), \
	/*.sched_policy = */ \
	.prio = p, \
	.time_slice = MA_ATOMIC_INIT(0), \
	MA_BUBBLE_SCHED_ENTITY_INITIALIZER(e) \
	MA_SCHED_LEVEL_INIT \
	MA_SCHED_MEMORY_AREA_INIT(e) \
	MA_SCHED_ENTITY_HEAP_INIT(e) \
   /* .heap_lock = MA_SPIN_LOCK_UNLOCKED,*/ \
}


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#ifdef MA__BUBBLES
/** \brief Get effective dynamic type of \e h.
 * Returns ::marcel_holder type of holder \e h */
enum marcel_holder ma_holder_type(ma_holder_t *h);
#define ma_holder_type(h) ((h)->type)
#else
#define ma_holder_type(h) MA_RUNQUEUE_HOLDER
#endif

/** \brief Static initializer for ::ma_holder structures. */
#define MA_HOLDER_INITIALIZER(h, t) { \
	.type = t, \
	.lock = MA_SPIN_LOCK_UNLOCKED, \
	.ready_entities = TBX_FAST_LIST_HEAD_INIT((h).ready_entities), \
	.nb_ready_entities = 0, \
}

/** \brief Returns type of entity \e e */

#ifdef DOXYGEN
enum marcel_holder ma_entity_type(marcel_entity_t *e);
#endif
#define ma_entity_type(e) ((e)->type)

#define ma_task_natural_holder(p)	(THREAD_GETMEM(p,as_entity.natural_holder))
#define ma_task_sched_holder(p)	(THREAD_GETMEM(p,as_entity.sched_holder))
#define ma_task_ready_holder(p)	(THREAD_GETMEM(p,as_entity.ready_holder))
#define ma_this_ready_holder()	(ma_task_ready_holder(MARCEL_SELF))
#define ma_task_ready_holder_data(p)  (THREAD_GETMEM(p,as_entity.ready_holder_data))

#define MA_TASK_IS_RUNNING(tsk) (ma_task_ready_holder(tsk)&&!ma_task_ready_holder_data(tsk))
#define MA_TASK_IS_READY(tsk) (ma_task_ready_holder(tsk)&&ma_task_ready_holder_data(tsk))
#define MA_TASK_IS_BLOCKED(tsk) (!ma_task_ready_holder(tsk))

#define ma_holder_preempt_lock(h) __ma_preempt_spin_lock(&(h)->lock)
#define ma_holder_trylock(h) _ma_raw_spin_trylock(&(h)->lock)
#define ma_holder_rawlock(h) _ma_raw_spin_lock(&(h)->lock)
#define ma_holder_lock(h) ma_spin_lock(&(h)->lock)
#define ma_holder_check_locked(h) ma_spin_check_locked(&(h)->lock)
/** \brief Locks holder \e h */
void ma_holder_lock_softirq(ma_holder_t *h);
#define ma_holder_lock_softirq(h) ma_spin_lock_softirq(&(h)->lock)
#define ma_holder_rawunlock(h) _ma_raw_spin_unlock(&(h)->lock)
#define ma_holder_unlock(h) ma_spin_unlock(&(h)->lock)
/** \brief Unlocks holder \e h */
void ma_holder_unlock_softirq(ma_holder_t *h);
#define ma_holder_unlock_softirq(h) ma_spin_unlock_softirq(&(h)->lock)

/* locking */
#define MA_ENTITY_RUNNING 2
#define MA_ENTITY_READY 1
#define MA_ENTITY_SLEEPING 0


/** Internal functions **/
static __tbx_inline__ void ma_holder_init(ma_holder_t *h, enum marcel_holder type);
#ifdef MA__BUBBLES
/** \brief Cast virtual class ::ma_holder \e h into subtype ::marcel_bubble.
 * \attention Semantics assumes that \e h dynamic type is a bubble.
 * Programmer should not assume that returned pointer is equal to \e h. */
static __tbx_inline__ marcel_bubble_t *ma_bubble_holder(ma_holder_t *h);
#else
#define ma_bubble_holder(h) NULL
#endif
/** \brief Cast virtual class ::ma_holder \e h into subtype ::ma_runqueue.
 * \attention Semantics assumes that \e h dynamic type is a runqueue.
 * Programmer should not assume that returned pointer is equal to \e h. */
static __tbx_inline__ ma_runqueue_t *ma_rq_holder(ma_holder_t *h);
/** \brief Cast subtype ::marcel_bubble \e b into virtual class ::ma_holder.
 * \attention Programmer should not assume that returned pointer is equal to \e b. */
ma_holder_t *ma_holder_bubble(marcel_bubble_t *b);
#define ma_holder_bubble(b) (&(b)->as_holder)
/** \brief Cast subtype ::ma_runqueue \e rq into virtual class ::ma_holder.
 * \attention Programmer should not assume that returned pointer is equal to \e rq. */
ma_holder_t *ma_holder_rq(ma_runqueue_t *rq);
#define ma_holder_rq(rq) (&(rq)->as_holder)

#ifdef MM_HEAP_ENABLED
/* \brief Get the number of the numa node where the entity is.
 * \return a node number */
int ma_node_entity(marcel_entity_t *entity);
#endif

/** \brief Converts ma_entity_t * \e e into marcel_task_t * (assumes that \e e is a thread) */
static __tbx_inline__ marcel_task_t *ma_task_entity(marcel_entity_t *e);
#ifdef MA__BUBBLES
/** \brief Converts ma_entity_t * \e e into marcel_bubble_t * (assumes that \e e is a bubble) */
static __tbx_inline__ marcel_bubble_t *ma_bubble_entity(marcel_entity_t *e);
#endif
/** \brief Converts marcel_bubble_t *b into marcel_entity_t * */
marcel_entity_t *ma_entity_bubble(marcel_bubble_t *b);
#define ma_entity_bubble(b) (&(b)->as_entity)
/** \brief Converts marcel_task_t *b into marcel_entity_t * */
marcel_entity_t *ma_entity_task(marcel_task_t *t);
#define ma_entity_task(t) (&(t)->as_entity)
static __tbx_inline__ ma_holder_t *ma_entity_active_holder(marcel_entity_t *e);
ma_holder_t *ma_task_active_holder(marcel_task_t *e);
#define ma_task_active_holder(t) ma_entity_active_holder(&(t)->as_entity)
static inline ma_holder_t *ma_entity_holder_rawlock(marcel_entity_t *e);
static inline ma_holder_t *ma_entity_holder_lock(marcel_entity_t *e);
/** \brief Locks the holder of entity \e e and return it */
static inline ma_holder_t *ma_entity_holder_lock_softirq(marcel_entity_t *e);
ma_holder_t *ma_task_holder_rawlock(marcel_task_t *e);
ma_holder_t *ma_task_holder_lock(marcel_task_t *e);
/** \brief Locks the holder of task \e t and return it */
ma_holder_t *ma_task_holder_lock_softirq(marcel_task_t *e);
/* TODO FIXME XXX: ma_task_holder must also lock the runqueue holding the scheduling bubble, if any */
#define ma_task_holder_rawlock(t) ma_entity_holder_rawlock(&(t)->as_entity)
#define ma_task_holder_lock(t) ma_entity_holder_lock(&(t)->as_entity)
#define ma_task_holder_lock_softirq(t) ma_entity_holder_lock_softirq(&(t)->as_entity)
#define ma_task_holder_lock_softirq_async(t) ma_entity_holder_lock_softirq_async(&(t)->as_entity)
#define ma_bubble_holder_rawlock(b) ma_entity_holder_rawlock(&(b)->as_entity)
#define ma_bubble_holder_lock(b) ma_entity_holder_lock(&(b)->as_entity)
#define ma_bubble_holder_lock_softirq(b) ma_entity_holder_lock_softirq(&(b)->as_entity)
static __tbx_inline__ void ma_entity_holder_rawunlock(ma_holder_t *h);
static __tbx_inline__ void ma_entity_holder_unlock(ma_holder_t *h);
/** \brief Unlocks holder \e h */
static __tbx_inline__ void ma_entity_holder_unlock_softirq(ma_holder_t *h);
void ma_task_holder_rawunlock(ma_holder_t *h);
void ma_task_holder_unlock(ma_holder_t *h);
/** \brief Unlocks holder \e h */
void ma_task_holder_unlock_softirq(ma_holder_t *h);
#define ma_task_holder_rawunlock(h) ma_entity_holder_rawunlock(h)
#define ma_task_holder_unlock(h) ma_entity_holder_unlock(h)
#define ma_task_holder_unlock_softirq(h) ma_entity_holder_unlock_softirq(h)
#define ma_bubble_holder_rawunlock(h) ma_entity_holder_rawunlock(h)
#define ma_bubble_holder_unlock(h) ma_entity_holder_unlock(h)
#define ma_bubble_holder_unlock_softirq(h) ma_entity_holder_unlock_softirq(h)
static __tbx_inline__ ma_runqueue_t *ma_to_rq_holder(ma_holder_t *h);
static __tbx_inline__ void ma_set_ready_holder(marcel_entity_t *e, ma_holder_t *h);
static __tbx_inline__ void ma_rq_enqueue_entity(marcel_entity_t *e, ma_runqueue_t *rq);
static __tbx_inline__ void ma_enqueue_entity(marcel_entity_t *e, ma_holder_t *h);
static __tbx_inline__ void ma_holder_try_to_wake_up_and_rawunlock(ma_holder_t *h);
static __tbx_inline__ void ma_holder_try_to_wake_up_and_unlock(ma_holder_t *h);
static __tbx_inline__ void ma_holder_try_to_wake_up_and_unlock_softirq(ma_holder_t *h);
static __tbx_inline__ void ma_clear_ready_holder(marcel_entity_t *e, ma_holder_t *h);
static __tbx_inline__ void ma_rq_dequeue_entity(marcel_entity_t *e, ma_runqueue_t *rq);
static __tbx_inline__ void ma_dequeue_entity(marcel_entity_t *e, ma_holder_t *h);
static __tbx_inline__ void ma_task_check(marcel_task_t *t);
static __tbx_inline__ const char *ma_entity_state_msg(int state);
/* Sets the sched holder of an entity to bubble (which is supposed to be
 * locked).  If that entity is a bubble, its hierarchy is supposed to be
 * already locked. */
void TBX_EXTERN ma_set_sched_holder(marcel_entity_t *e, marcel_bubble_t *bubble, int do_lock);
/** \brief Gets entity \e e out from its holder (which must be already locked),
 * returns its state.  If entity is a bubble, its hierarchy is supposed to be
 * already locked.  */
static __tbx_inline__ int __tbx_warn_unused_result__ ma_get_entity(marcel_entity_t *e);
/** \brief Puts entity \e e into holder \e h (which must be already locked) in
 * state \e state.   If e is a bubble, its hierarchy must be already locked
 * (since it all needs moved). If h is a bubble, the corresponding top-bubble
 * and runqueue must be already locked. */
static __tbx_inline__ void ma_put_entity(marcel_entity_t *e, ma_holder_t *h, int state);
/** \brief Moves entity \e e out from its holder (which must be already locked)
 * into holder \e h (which must be also already locked). If entity is a bubble,
 * its hierarchy is supposed to be already locked.  */
static __tbx_inline__ void ma_move_entity(marcel_entity_t *e, ma_holder_t *h);


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_HOLDER_H__ **/
