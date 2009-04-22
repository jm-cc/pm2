
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHOR file)
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

/** \file
 * \brief Holders
 *
 * \defgroup marcel_holders Entities and holders
 *
 * Holders (runqueues [::ma_runqueue] or bubbles [::marcel_bubble]) contain
 * entities (tasks [::marcel_task] or bubbles [::marcel_bubble]).
 * They must be locked before adding/removing entities.
 *
 * Locking convention is top-down, i.e. holder then holdee: the runqueue, then
 * the bubble on the runqueue, then bubbles held in the bubble (and not put on
 * another runqueue), then lower-leveled runqueue, then bubble on it, then
 * bubbles in the bubble, etc.
 *
 * If you know that bottom halves are already disabled, you can replace
 * ma_holder_lock_softirq() by ma_holder_lock() and ma_holder_unlock_softirq()
 * by ma_holder_unlock().
 *
 * If you know that you already hold a lock on a holder, you can even use
 * ma_holder_rawlock() and ma_holder_rawunlock()
 *
 * @{
 */

#section types
/** \brief Holder types: runqueue or bubble */
enum marcel_holder {
	/** \brief runqueue [::ma_runqueue] type holder. */
	MA_RUNQUEUE_HOLDER,
#ifdef MA__BUBBLES
	/** \brief bubble [::marcel_bubble] type holder. */
	MA_BUBBLE_HOLDER,
#endif
};

/** \brief Entity types: bubble or thread */
enum marcel_entity {
#ifdef MA__BUBBLES
	/** \brief bubble [::marcel_bubble] type entity. */
	MA_BUBBLE_ENTITY,
#endif
	/** \brief thread type entity (task [::marcel_task] subtype). */
	MA_THREAD_ENTITY,
	/** \brief thread seed (task [::marcel_task] subtype)*/
	MA_THREAD_SEED_ENTITY,
};

#section structures
#depend "linux_spinlock.h[types]"
#depend "[marcel_types]"
#depend "marcel_barrier.h[types]"
#depend "marcel_stats.h[marcel_types]"

/** \brief Virtual class of an entity holder. An \e holder is a container
 * dedicated to contain \e entities of type ::ma_entity.  Examples of holder
 * subclasses include runqueues (::ma_runqueue) and bubbles (::marcel_bubble). */
struct ma_holder {
	/** \brief Dynamic type of the effective holder subclass. */
	enum marcel_holder type;
	/** \brief Lock of holder */
	ma_spinlock_t lock;
	/** \brief List of entities running or ready to run in this holder */
	struct list_head ready_entities;
	/** \brief Number of entities in the ma_holder::ready_entities list above */
	unsigned long nb_ready_entities;

	/** \brief Name of the holder */
	char name[MARCEL_MAXNAMESIZE];

#ifdef MARCEL_STATS_ENABLED
	/** \brief Synthesis of statistics of contained entities */
	ma_stats_t stats;
#endif /* MARCEL_STATS_ENABLED */
};

#section types
/* \brief Holder type wrapper. */
typedef struct ma_holder ma_holder_t;

#section marcel_macros
#ifdef MA__BUBBLES
/** \brief Get effective dynamic type of \e h.
 * Returns ::marcel_holder type of holder \e h */
enum marcel_holder ma_holder_type(ma_holder_t *h);
#define ma_holder_type(h) ((h)->type)
#else
#define ma_holder_type(h) MA_RUNQUEUE_HOLDER
#endif

#section marcel_macros
/** \brief Static initializer for ::ma_holder structures. */
#define MA_HOLDER_INITIALIZER(h, t) { \
	.type = t, \
	.lock = MA_SPIN_LOCK_UNLOCKED, \
	.ready_entities = LIST_HEAD_INIT((h).ready_entities), \
	.nb_ready_entities = 0, \
}

#section marcel_functions
static __tbx_inline__ void ma_holder_init(ma_holder_t *h, enum marcel_holder type);
#section marcel_inline
/** \brief Dynamic initializer for ::ma_holder structures. */
static __tbx_inline__ void ma_holder_init(ma_holder_t *h, enum marcel_holder type) {
	h->type = type;
	ma_spin_lock_init(&h->lock);
	INIT_LIST_HEAD(&h->ready_entities);
	h->nb_ready_entities = 0;
}

#section marcel_functions
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

#ifdef MA__NUMA_MEMORY
/* \brief Get the number of the numa node where the entity is.
 * \return a node number */ 
int ma_node_entity(marcel_entity_t *entity);
#endif

#section marcel_inline
#ifdef MA__BUBBLES
static __tbx_inline__ marcel_bubble_t *ma_bubble_holder(ma_holder_t *h) {
	return tbx_container_of(h, marcel_bubble_t, as_holder);
}
#endif
static __tbx_inline__ ma_runqueue_t *ma_rq_holder(ma_holder_t *h) {
	return tbx_container_of(h, ma_runqueue_t, as_holder);
}

#section structures
#depend "[types]"
#depend "pm2_list.h"
#depend "asm/linux_atomic.h[marcel_types]"
#depend "marcel_stats.h[marcel_types]"
/** \brief Virtual class of a holder entity. An \e entity is a containee of a \e holder (::ma_holder).
 * Examples of entity subclasses include tasks (::marcel_task) and bubbles (::marcel_bubble).
 *
 * An entity is (potentially):
 * - held in some natural, application-structure-dependent, holder
 *   (#natural_holder)
 * - put on some application-state-dependent scheduling holder (#sched_holder)
 *   by bubble scheduler algorithms, indicating a target onto which the entity 
 *   should migrate as soon as
 *   _possible_ (but not necessarily immediately) if it is not already running
 *   there
 * - queued or running on some ready holder (#ready_holder).
 *
 * That is, the #sched_holder and the #ready_holder work as a dampening team in a
 * way similar to double buffering schemes where the #ready_holder represents the
 * last instantiated scheduling decision while the #sched_holder represents the
 * upcoming scheduling decision that the scheduling algorithm is currently busy
 * figuring out.
 *
 * If #ready_holder_data is \p NULL then the entity is currently running. Otherwise, it is 
 * currently preempted out and can be found in #cached_entities_item.
 */
struct ma_entity {
	/** \brief Dynamic type of the effective entity subclass. */
	enum marcel_entity type;
	/** \brief Natural holder, i.e. the one that the programmer hinted out of the application structure
	 * (the holding bubble, typically). */
	ma_holder_t *natural_holder;
	/** \brief Scheduling holder, i.e. the holder currently selected by the scheduling algorithms out
	 * of application activity state. It may be modified independently from the ready_holder, in which
	 * case it constitutes a target toward which the entity should migrate in due time. */
	ma_holder_t *sched_holder;
	/** \brief Ready holder, i.e. the transient holder on which the entity is currently running or
	 * queued as eligible for running once its turn comes. */
	ma_holder_t *ready_holder;
	/** \brief Data for the running holder. If the ma_entity#ready_holder is a runqueue \e and if the entity
	 * is \e not running, then the field is pointer to the runqueue's priority queue in which the entity
	 * is currently enqueued */
	void *ready_holder_data;
	/** \brief Item linker to the list of cached/enqueued entities sorted for schedule in this holder.
	 * The list is either marcel_bubble#cached_entities if the
	 * ma_entity#ready_holder is a bubble or ma_runqueue#active if the holder is
	 * a runqueue. */
	struct list_head cached_entities_item;
	/** \brief Scheduling policy code (see #__MARCEL_SCHED_AVAILABLE). */
	int sched_policy;
	/** \brief Current priority given to the entity. */
	int prio;
	/** \brief Remaining time slice until the entity gets preempted out. */
	ma_atomic_t time_slice;
#ifdef MA__BUBBLES
	/** \brief Item linker to the list of natural entities
	 * (marcel_bubble#natural_entities)  in the entity's natural holding bubble. */
	struct list_head natural_entities_item;
#endif
	/** \brief Item linker to the cumulative list of ready and running entities
	 * (ma_holder#ready_entities) in the entity's ma_entity#ready_holder. */
	struct list_head ready_entities_item;

#ifdef MA__LWPS
	/** \brief Depth of the (direct or indirect) runqueue onto which the entity is currently scheduled. */
	int sched_level;
#endif

	/** \brief Name of the entity, for debugging purpose. */
	char name[MARCEL_MAXNAMESIZE];

#ifdef MARCEL_STATS_ENABLED
	/** \brief Statistics for the entity */
	ma_stats_t stats;
#endif /* MARCEL_STATS_ENABLED */

#ifdef MM_MAMI_ENABLED
	/** \brief List of memory areas attached to the entity.*/
	struct list_head memory_areas;
	/** \brief Lock for serializing access to ma_entity#memory_areas */
	ma_spinlock_t memory_areas_lock;

#endif /* MM_MAMI_ENABLED */

#ifdef MA__NUMA_MEMORY
	/** \brief Back pointer to the NUMA heap allocator used to allocated this object */
	ma_heap_t *heap;
#endif /* MA__NUMA_MEMORY */

#ifdef MA__BUBBLES
	/** \brief General-purpose list item linker for bubble schedulers */
	struct list_head next;
#endif
};

#section macros
#define MARCEL_ENTITY_SELF (&(MARCEL_SELF)->as_entity)

#section types
typedef struct ma_entity marcel_entity_t;

#section marcel_macros
/** \brief Returns type of entity \e e */

#ifdef DOXYGEN
enum marcel_holder ma_entity_type(marcel_entity_t *e);
#endif
#define ma_entity_type(e) ((e)->type)

#section marcel_functions
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
#section marcel_inline
static __tbx_inline__ marcel_task_t *ma_task_entity(marcel_entity_t *e) {
	MA_BUG_ON(e->type != MA_THREAD_ENTITY && e->type != MA_THREAD_SEED_ENTITY);
	return tbx_container_of(e, marcel_task_t, as_entity);
}
#ifdef MA__BUBBLES
static __tbx_inline__ marcel_bubble_t *ma_bubble_entity(marcel_entity_t *e) {
	MA_BUG_ON(e->type != MA_BUBBLE_ENTITY);
	return tbx_container_of(e, marcel_bubble_t, as_entity);
}
#endif

#section macros
#ifdef MA__LWPS
#define MA_SCHED_LEVEL_INIT .sched_level = MARCEL_LEVEL_DEFAULT,
#else
#define MA_SCHED_LEVEL_INIT
#endif
#ifdef MA__BUBBLES
#define MA_BUBBLE_SCHED_ENTITY_INITIALIZER(e) .natural_entities_item = LIST_HEAD_INIT((e).natural_entities_item),
#else
#define MA_BUBBLE_SCHED_ENTITY_INITIALIZER(e)
#endif
#ifdef MM_MAMI_ENABLED
#define MA_SCHED_MEMORY_AREA_INIT(e) \
	.memory_areas_lock = MA_SPIN_LOCK_UNLOCKED, \
	.memory_areas = LIST_HEAD_INIT((e).memory_areas),
#else
#define MA_SCHED_MEMORY_AREA_INIT(e)
#endif
#ifdef MA__NUMA_MEMORY
#define MA_SCHED_ENTITY_HEAP_INIT(e) \
	.heap = NULL,
#else
#define MA_SCHED_ENTITY_HEAP_INIT(e)
#endif

#define MA_SCHED_ENTITY_INITIALIZER(e,t,p) { \
	.type = t, \
	.natural_holder = NULL, .sched_holder = NULL, .ready_holder = NULL, \
	.ready_holder_data = NULL, \
	.cached_entities_item = LIST_HEAD_INIT((e).cached_entities_item), \
	/*.sched_policy = */ \
	.prio = p, \
	.time_slice = MA_ATOMIC_INIT(0), \
	MA_BUBBLE_SCHED_ENTITY_INITIALIZER(e) \
	MA_SCHED_LEVEL_INIT \
	MA_SCHED_MEMORY_AREA_INIT(e) \
	MA_SCHED_ENTITY_HEAP_INIT(e) \
   /* .heap_lock = MA_SPIN_LOCK_UNLOCKED,*/ \
}

#section marcel_macros
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

#section marcel_functions
static __tbx_inline__ ma_holder_t *ma_entity_active_holder(marcel_entity_t *e);
ma_holder_t *ma_task_active_holder(marcel_task_t *e);
#define ma_task_active_holder(t) ma_entity_active_holder(&(t)->as_entity)
#section marcel_inline
static __tbx_inline__ ma_holder_t *ma_entity_active_holder(marcel_entity_t *e)
{
        ma_holder_t *holder;
        if ((holder = e->ready_holder))
		/* currently ready */
                return holder;
	/* not ready, current runqueue */
        sched_debug("using current queue for blocked %p\n",e);
#ifdef MA__BUBBLES
	if ((holder = e->sched_holder))
		return holder;
        sched_debug("using default queue for %p\n",e);
        return e->natural_holder;
#else
        return e->sched_holder;
#endif
}

#section marcel_functions
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
#section marcel_inline
static inline ma_holder_t *ma_entity_holder_rawlock(marcel_entity_t *e) {
	ma_holder_t *h, *h2;
	h = ma_entity_active_holder(e);
again:
	if (!h)
		return NULL;
	sched_debug("ma_entity_holder_rawlocking(%p)\n",h);
	ma_holder_rawlock(h);
	if (tbx_unlikely(h != (h2 = ma_entity_active_holder(e)))) {
		sched_debug("ma_entity_holder_rawunlocking(%p)\n", h);
		ma_holder_rawunlock(h);
		h = h2;
		goto again;
	}
	sched_debug("ma_entity_holder_rawlocked(%p)\n",h);
	return h;
}
static inline ma_holder_t *ma_entity_holder_lock(marcel_entity_t *e) {
	ma_holder_t *h, *h2;
	ma_preempt_disable();
	h = ma_entity_active_holder(e);
again:
	if (!h)
		return NULL;
	sched_debug("ma_entity_holder_locking(%p)\n",h);
	if (tbx_unlikely(!ma_holder_trylock(h)))
#ifdef MA__LWPS
		ma_holder_preempt_lock(h)
#endif
		;
	if (tbx_unlikely(h != (h2 = ma_entity_active_holder(e)))) {
		sched_debug("ma_entity_holder_unlocking(%p)\n",h);
		ma_holder_rawunlock(h);
		h = h2;
		goto again;
	}
	sched_debug("ma_entity_holder_locked(%p)\n",h);
	return h;
}

static inline ma_holder_t *ma_entity_holder_lock_softirq(marcel_entity_t *e) {
	ma_local_bh_disable();
	return ma_entity_holder_lock(e);
}

static inline ma_holder_t *ma_entity_holder_lock_softirq_async(marcel_entity_t *e) {
	ma_holder_t *h, *h2;
	ma_local_bh_disable();
	ma_preempt_disable();
	h = ma_entity_active_holder(e);
again:
	if (!h)
		return NULL;
	sched_debug("ma_entity_holder_locking_async(%p)\n",h);
	if (tbx_unlikely(!ma_holder_trylock(h)))
		return NULL;
	if (tbx_unlikely(h != (h2 = ma_entity_active_holder(e)))) {
		sched_debug("ma_entity_holder_unlocking_async(%p)\n",h);
		ma_holder_rawunlock(h);
		h = h2;
		goto again;
	}
	sched_debug("ma_entity_holder_locked_async(%p)\n",h);
	return h;
}

#section marcel_functions
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
#section marcel_inline
static __tbx_inline__ void ma_entity_holder_rawunlock(ma_holder_t *h) {
	sched_debug("ma_entity_holder_rawunlock(%p)\n",h);
	if (h)
		ma_holder_rawunlock(h);
	}
static __tbx_inline__ void ma_entity_holder_unlock(ma_holder_t *h) {
	sched_debug("ma_entity_holder_unlock(%p)\n",h);
	if (h)
		ma_holder_unlock(h);
	else
		ma_preempt_enable();
}
static __tbx_inline__ void ma_entity_holder_unlock_softirq(ma_holder_t *h) {
	sched_debug("ma_entity_holder_unlock_softirq(%p)\n",h);
	if (h)
		ma_holder_unlock_softirq(h);
	else {
		ma_preempt_enable_no_resched();
		ma_local_bh_enable();
	}
}

/* topology */

/* ma_holder_rq - go up through holders and get to a rq */
#section marcel_functions
static __tbx_inline__ ma_runqueue_t *ma_to_rq_holder(ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ ma_runqueue_t *ma_to_rq_holder(ma_holder_t *h) {
	ma_holder_t *hh;
#ifdef MA__BUBBLES
	for (hh=h; hh && ma_holder_type(hh) != MA_RUNQUEUE_HOLDER; ) {
		/* TODO: n'a plus de sens, enlever ready_holder */
		if (ma_bubble_holder(hh)->as_entity.ready_holder != hh)
			hh = ma_bubble_holder(hh)->as_entity.ready_holder;
		else if (ma_bubble_holder(hh)->as_entity.sched_holder != hh)
			hh = ma_bubble_holder(hh)->as_entity.sched_holder;
		else break;
	}
#else
	hh = h;
#endif
	return hh && ma_holder_type(hh) == MA_RUNQUEUE_HOLDER?ma_rq_holder(hh):NULL;
}

/* Activations et désactivations nécessitent que le holder soit verrouillé. */

/* Attention, lorsque le holder est une bulle, il se peut qu'ils déverrouillent
 * le holder pour pouvoir verrouiller celui qui le contient immédiatement */

/* activation */

#section marcel_functions
static __tbx_inline__ void ma_set_ready_holder(marcel_entity_t *e, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_set_ready_holder(marcel_entity_t *e, ma_holder_t *h) {
	sched_debug("holder %p [%s]: accounting entity %p [%s]\n", h, h->name, e, e->name);
	MA_BUG_ON(e->ready_holder_data);
	MA_BUG_ON(e->ready_holder);
	MA_BUG_ON(e->sched_holder && ma_holder_type(h) != ma_holder_type(e->sched_holder));
	MA_BUG_ON(!ma_holder_check_locked(h));
	e->ready_holder = h;
	if ((e->prio >= MA_BATCH_PRIO) && (e->prio != MA_LOWBATCH_PRIO))
		list_add(&e->ready_entities_item, &h->ready_entities);
	else
		list_add_tail(&e->ready_entities_item, &h->ready_entities);
	h->nb_ready_entities++;
}

#section marcel_functions
static __tbx_inline__ void ma_rq_enqueue_entity(marcel_entity_t *e, ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void ma_rq_enqueue_entity(marcel_entity_t *e, ma_runqueue_t *rq) {
	MA_BUG_ON(e->ready_holder != &rq->as_holder);
	MA_BUG_ON(!ma_holder_check_locked(&rq->as_holder));
	ma_array_enqueue_entity(e, rq->active);
}

/*
 * Note: if h may be a bubble the caller has to call
 * ma_bubble_try_to_wake_and_unlock() instead of unlocking it when it is done
 * with it (else the bubble may be left asleep).
 */
#section marcel_functions
static __tbx_inline__ void ma_enqueue_entity(marcel_entity_t *e, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_enqueue_entity(marcel_entity_t *e, ma_holder_t *h) {
	sched_debug("holder %p [%s]: enqueuing entity %p [%s]\n", h, h->name, e, e->name);
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		ma_rq_enqueue_entity(e, ma_rq_holder(h));
	else
		ma_bubble_enqueue_entity(e, ma_bubble_holder(h));
}

/*
 * Call this instead of unlocking a holder if ma_activate_*() or ma_enqueue_*()
 * was called on it, unless it is known to be a runqueue.
 */
#section marcel_functions
static __tbx_inline__ void ma_holder_try_to_wake_up_and_rawunlock(ma_holder_t *h);
static __tbx_inline__ void ma_holder_try_to_wake_up_and_unlock(ma_holder_t *h);
static __tbx_inline__ void ma_holder_try_to_wake_up_and_unlock_softirq(ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_holder_try_to_wake_up_and_rawunlock(ma_holder_t *h) {
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		ma_holder_rawunlock(h);
	else
		ma_bubble_try_to_wake_up_and_rawunlock(ma_bubble_holder(h));
}
static __tbx_inline__ void ma_holder_try_to_wake_up_and_unlock(ma_holder_t *h) {
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		ma_holder_unlock(h);
	else
		ma_bubble_try_to_wake_up_and_unlock(ma_bubble_holder(h));
}
static __tbx_inline__ void ma_holder_try_to_wake_up_and_unlock_softirq(ma_holder_t *h) {
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		ma_holder_unlock_softirq(h);
	else
		ma_bubble_try_to_wake_up_and_unlock_softirq(ma_bubble_holder(h));
}

/* deactivation */

#section marcel_functions
static __tbx_inline__ void ma_clear_ready_holder(marcel_entity_t *e, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_clear_ready_holder(marcel_entity_t *e, ma_holder_t *h) {
	sched_debug("holder %p [%s]: unaccounting entity %p [%s]\n", h, h->name, e, e->name);
	MA_BUG_ON(e->ready_holder_data);
	MA_BUG_ON(h->nb_ready_entities <= 0);
	MA_BUG_ON(!ma_holder_check_locked(h));
	h->nb_ready_entities--;
	list_del(&e->ready_entities_item);
	MA_BUG_ON(e->ready_holder != h);
	e->ready_holder = NULL;
}

#section marcel_functions
static __tbx_inline__ void ma_rq_dequeue_entity(marcel_entity_t *e, ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void ma_rq_dequeue_entity(marcel_entity_t *e, ma_runqueue_t *rq TBX_UNUSED) {
	MA_BUG_ON(!ma_holder_check_locked(&rq->as_holder));
	ma_array_dequeue_entity(e, (ma_prio_array_t *) e->ready_holder_data);
	MA_BUG_ON(e->ready_holder != &rq->as_holder);
}

#section marcel_functions
static __tbx_inline__ void ma_dequeue_entity(marcel_entity_t *e, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_dequeue_entity(marcel_entity_t *e, ma_holder_t *h) {
	sched_debug("holder %p [%s]: dequeuing entity %p [%s]\n", h, h->name, e, e->name);
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		ma_rq_dequeue_entity(e, ma_rq_holder(h));
	else
		ma_bubble_dequeue_entity(e, ma_bubble_holder(h));
}

#section marcel_functions
static __tbx_inline__ void ma_task_check(marcel_task_t *t);
#section marcel_inline
static __tbx_inline__ void ma_task_check(marcel_task_t *t TBX_UNUSED) {
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
			MA_BUG_ON(list_empty(&b->cached_entities));
		}
#endif
	}
#endif
}

#section marcel_macros
#define MA_ENTITY_RUNNING 2
#define MA_ENTITY_READY 1
#define MA_ENTITY_SLEEPING 0

#section marcel_functions
static __tbx_inline__ const char *ma_entity_state_msg(int state);
#section marcel_inline
static __tbx_inline__ const char *ma_entity_state_msg(int state) {
	const char * r;

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

#section marcel_functions
/* Sets the sched holder of an entity to bubble (which is supposed to be
 * locked).  If that entity is a bubble, its hierarchy is supposed to be
 * already locked. */
void TBX_EXTERN ma_set_sched_holder(marcel_entity_t *e, marcel_bubble_t *bubble, int do_lock);
/** \brief Gets entity \e e out from its holder (which must be already locked),
 * returns its state.  If entity is a bubble, its hierarchy is supposed to be
 * already locked.  */
static __tbx_inline__ int __tbx_warn_unused_result__ ma_get_entity(marcel_entity_t *e);
#section marcel_inline
static __tbx_inline__ int __tbx_warn_unused_result__ ma_get_entity(marcel_entity_t *e) {
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
	sched_debug("holder %p [%s]: getting entity %p [%s] with state %d [%s]\n", h, h->name, e, e->name, state, 
			ma_entity_state_msg(state));
	return state;
}

#section marcel_functions
/** \brief Puts entity \e e into holder \e h (which must be already locked) in
 * state \e state.   If e is a bubble, its hierarchy must be already locked
 * (since it all needs moved). If h is a bubble, the corresponding top-bubble
 * and runqueue must be already locked. */
static __tbx_inline__ void ma_put_entity(marcel_entity_t *e, ma_holder_t *h, int state);
#section marcel_inline
#depend "[marcel_types]"
static __tbx_inline__ void _ma_put_entity_check(marcel_entity_t *e, ma_holder_t *h) {
#ifdef MA__BUBBLES
	if (h->type == MA_BUBBLE_HOLDER) {
		MA_BUG_ON(h != e->natural_holder);
		if (e->type == MA_BUBBLE_ENTITY)
			PROF_EVENT2(bubble_sched_bubble_goingback, ma_bubble_entity(e), ma_bubble_holder(h));
		else
			PROF_EVENT2(bubble_sched_goingback, ma_task_entity(e), ma_bubble_holder(h));
	} else
#endif
	{
		MA_BUG_ON(h->type != MA_RUNQUEUE_HOLDER);
		PROF_EVENT2(bubble_sched_switchrq,
#ifdef MA__BUBBLES
			e->type == MA_BUBBLE_ENTITY?
				(void*) ma_bubble_entity(e):
#endif
				(void*) ma_task_entity(e),
				ma_rq_holder(h));
	}
}
static __tbx_inline__ void ma_put_entity(marcel_entity_t *e, ma_holder_t *h, int state) {
	
	sched_debug("holder %p [%s]: putting entity %p [%s] with state %d [%s]\n", h, h->name, e, e->name, state,
			ma_entity_state_msg(state));
	_ma_put_entity_check(e, h);
#ifdef MA__BUBBLES
	if (h->type == MA_BUBBLE_HOLDER) {
		/* Don't directly enqueue in holding bubble, but in the thread cache. */
		marcel_bubble_t *b = ma_bubble_holder(h);
		while (b->as_entity.sched_holder && b->as_entity.sched_holder->type == MA_BUBBLE_HOLDER) {
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
#section marcel_functions
/** \brief Moves entity \e e out from its holder (which must be already locked)
 * into holder \e h (which must be also already locked). If entity is a bubble,
 * its hierarchy is supposed to be already locked.  */
static __tbx_inline__ void ma_move_entity(marcel_entity_t *e, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_move_entity(marcel_entity_t *e, ma_holder_t *h) {
	/* TODO: optimiser ! */
	int state = ma_get_entity(e);
	ma_put_entity(e, h, state);
}

#section marcel_structures

/* @} */
