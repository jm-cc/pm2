
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
 * \defgroup marcel_holders Marcel holders
 *
 * Holders (runqueues or bubbles) contain entities (threads or bubbles).
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

#section marcel_types
/** \brief Holder types: runqueue or bubble */
enum marcel_holder {
	/** \brief holder */
	MA_RUNQUEUE_HOLDER,
#ifdef MA__BUBBLES
	/** \brief bubble */
	MA_BUBBLE_HOLDER,
#endif
};

/** \brief Entity types: bubble or thread */
enum marcel_entity {
#ifdef MA__BUBBLES
	/** \brief bubble */
	MA_BUBBLE_ENTITY,
#endif
	/** \brief thread */
	MA_THREAD_ENTITY,
	/** \brief thread seed */
	MA_THREAD_SEED_ENTITY,
};

#section structures
#depend "linux_spinlock.h[types]"
#depend "[marcel_types]"
#depend "marcel_barrier.h[types]"
#depend "marcel_stats.h[marcel_types]"

/** Holder information */
struct ma_holder {
	/** \brief Type of holder */
	enum marcel_holder type;
	/** \brief Lock of holder */
	ma_spinlock_t lock;
	/** \brief List of entities placed for schedule in this holder */
	struct list_head sched_list;
	/** \brief Number of entities in the list above */
	unsigned long nr_ready;
	/** \brief Number of interruptibly blocked held entities (probably broken) */
	unsigned long nr_uninterruptible;

#ifdef MARCEL_STATS_ENABLED
	/** \brief Synthesis of statistics of contained entities */
	ma_stats_t stats;
#endif /* MARCEL_STATS_ENABLED */
};

#section types
/* \brief Holder type */
typedef struct ma_holder ma_holder_t;

#section marcel_functions
#ifdef MA__BUBBLES
/** \brief Returns type of holder \e h */
enum marcel_holder ma_holder_type(ma_holder_t *h);
#define ma_holder_type(h) ((h)->type)
#else
#define ma_holder_type(h) MA_RUNQUEUE_HOLDER
#endif

#section marcel_macros
#define MA_HOLDER_INITIALIZER(h, t) { \
	.type = t, \
	.lock = MA_SPIN_LOCK_UNLOCKED, \
	.sched_list = LIST_HEAD_INIT((h).sched_list), \
	.nr_ready = 0, \
	.nr_uninterruptible = 0, \
}

#section marcel_functions
static __tbx_inline__ void ma_holder_init(ma_holder_t *h, enum marcel_holder type);
#section marcel_inline
static __tbx_inline__ void ma_holder_init(ma_holder_t *h, enum marcel_holder type) {
	h->type = type;
	ma_spin_lock_init(&h->lock);
	INIT_LIST_HEAD(&h->sched_list);
	h->nr_ready = h->nr_uninterruptible = 0;
}

#section marcel_functions
#ifdef MA__BUBBLES
/** \brief Converts ma_holder_t *h into marcel_bubble_t * (assumes that \e h is a bubble) */
static __tbx_inline__ marcel_bubble_t *ma_bubble_holder(ma_holder_t *h);
#else
#define ma_bubble_holder(h) NULL
#endif
/** \brief Converts ma_holder_t *h into ma_runqueue_t * (assumes that \e h is a runqueue) */
static __tbx_inline__ ma_runqueue_t *ma_rq_holder(ma_holder_t *h);
/** \brief Converts marcel_bubble_t *b into ma_holder_t * */
ma_holder_t *ma_holder_bubble(marcel_bubble_t *b);
#define ma_holder_bubble(b) (&(b)->hold)
/** \brief Converts ma_runqueue_t *b into ma_holder_t * */
ma_holder_t *ma_holder_rq(ma_runqueue_t *rq);
#define ma_holder_rq(rq) (&(rq)->hold)

/* \brief Find the number of the numa node where the entity is */ 
int ma_node_entity(marcel_entity_t *entity);

#section marcel_inline
#ifdef MA__BUBBLES
static __tbx_inline__ marcel_bubble_t *ma_bubble_holder(ma_holder_t *h) {
	return tbx_container_of(h, marcel_bubble_t, hold);
}
#endif
static __tbx_inline__ ma_runqueue_t *ma_rq_holder(ma_holder_t *h) {
	return tbx_container_of(h, ma_runqueue_t, hold);
}

#section structures
/* Une entité a un type (bulle/thread), a un conteneur initial (bulle ou
 * directement runqueue), un conteneur d'ordonnancement, et un conteneur où
 * elle est active (prête à être ordonnancé).  Si holder_data est NULL, c'est
 * qu'elle est en cours d'exécution. Sinon, c'est qu'elle est préemptée, prête
 * pour l'exécution dans la liste run_list.
 *
 * On peut ainsi avoir un thread contenu dans une bulle (init_holder), endormi
 * (donc run_holder == NULL), mais qui sera réveillé sur une certaine runqueue
 * (sched_holder).
 * */
#depend "[types]"
#depend "pm2_list.h"
#depend "asm/linux_atomic.h[marcel_types]"
#depend "marcel_stats.h[marcel_types]"
/**
 * An entity is held in some initial holder (::init_holder), is put on some
 * scheduling holder (::sched_holder) by bubble schedulers, and is actually
 * still running in some running holder (::run_holder).
 *
 * ::holder_data is NULL when the entity is currently running. Else, it was
 * preempted and put in ::run_list.
 */
struct ma_sched_entity {
	/** \brief Entity type */
	enum marcel_entity type;
	/** \brief Initial holder, i.e. the one that the programmer provides (the holding bubble, typically). */
	ma_holder_t *init_holder;
	/** \brief Scheduling holder */
	ma_holder_t *sched_holder;
	/** \brief Running holder */
	ma_holder_t *run_holder;
	/** \brief Data for holder */
	void *holder_data;
	/** \brief List link of ready entities */
	struct list_head run_list;
	/** \brief scheduling policy */
	int sched_policy;
	/** \brief priority */
	int prio;
	/** \brief remaining time slice */
	ma_atomic_t time_slice;
#ifdef MA__BUBBLES
	/** \brief List link of entities held in the containing bubble */
	struct list_head bubble_entity_list;
#endif
	/** \brief List of entities placed for schedule in this holder */
	struct list_head sched_list;

   /** \brief List of entities on the fake vp */
	struct list_head next_fake_sched;

	/** \brief Last VP used */
		int last_vp;

#ifdef MA__LWPS
	/** \brief Nesting level */
	int sched_level;
#endif

#ifdef MARCEL_STATS_ENABLED
	/** \brief Statistics for the entity */
	ma_stats_t stats;
#endif /* MARCEL_STATS_ENABLED */

#ifdef MA__NUMA
	/** \brief List of attached memory areas */
	struct list_head memory_areas;
	/** \brief Lock for ::memory_areas */
	ma_spinlock_t memory_areas_lock;

	/* heap allocator */
	ma_heap_t *heap;
#endif

#ifdef MA__BUBBLES
	/** \brief General-purpose list link for bubble schedulers */
	struct list_head next;
#endif
};

#section macros
#define MARCEL_ENTITY_SELF (&(MARCEL_SELF)->sched.internal.entity)

#section types
typedef struct ma_sched_entity marcel_entity_t;

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
#define ma_entity_task(t) (&(t)->sched.internal.entity)
#section marcel_inline
static __tbx_inline__ marcel_task_t *ma_task_entity(marcel_entity_t *e) {
	MA_BUG_ON(e->type != MA_THREAD_ENTITY && e->type != MA_THREAD_SEED_ENTITY);
	return tbx_container_of(e, marcel_task_t, sched.internal.entity);
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
#define MA_BUBBLE_SCHED_ENTITY_INITIALIZER(e) .bubble_entity_list = LIST_HEAD_INIT((e).bubble_entity_list),
#else
#define MA_BUBBLE_SCHED_ENTITY_INITIALIZER(e)
#endif
#ifdef MA__NUMA
#define MA_SCHED_MEMORY_AREA_INIT(e) \
	.memory_areas_lock = MA_SPIN_LOCK_UNLOCKED, \
	.memory_areas = LIST_HEAD_INIT((e).memory_areas),
#else
#define MA_SCHED_MEMORY_AREA_INIT(e)
#endif

#define MA_SCHED_ENTITY_INITIALIZER(e,t,p) { \
	.type = t, \
	.init_holder = NULL, .sched_holder = NULL, .run_holder = NULL, \
	.holder_data = NULL, \
	.run_list = LIST_HEAD_INIT((e).run_list), \
	/*.sched_policy = */ \
	.prio = p, \
	.time_slice = MA_ATOMIC_INIT(0), \
	MA_BUBBLE_SCHED_ENTITY_INITIALIZER(e) \
	MA_SCHED_LEVEL_INIT \
	MA_SCHED_MEMORY_AREA_INIT(e) \
   .heap = NULL, \
   .last_vp = -1, \
   /* .heap_lock = MA_SPIN_LOCK_UNLOCKED,*/ \
}

#section marcel_macros
#define ma_task_init_holder(p)	(THREAD_GETMEM(p,sched.internal.entity.init_holder))
#define ma_task_sched_holder(p)	(THREAD_GETMEM(p,sched.internal.entity.sched_holder))
#define ma_task_run_holder(p)	(THREAD_GETMEM(p,sched.internal.entity.run_holder))
#define ma_this_holder()	(ma_task_run_holder(MARCEL_SELF))
#define ma_task_holder_data(p)  (THREAD_GETMEM(p,sched.internal.entity.holder_data))

#define MA_TASK_IS_RUNNING(tsk) (ma_task_run_holder(tsk)&&!ma_task_holder_data(tsk))
#define MA_TASK_IS_READY(tsk) (ma_task_run_holder(tsk)&&ma_task_holder_data(tsk))
#define MA_TASK_IS_BLOCKED(tsk) (!ma_task_run_holder(tsk))

#define ma_holder_preempt_lock(h) __ma_preempt_spin_lock(&(h)->lock)
#define ma_holder_trylock(h) _ma_raw_spin_trylock(&(h)->lock)
#define ma_holder_rawlock(h) _ma_raw_spin_lock(&(h)->lock)
#define ma_holder_lock(h) ma_spin_lock(&(h)->lock)
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
static __tbx_inline__ ma_holder_t *ma_entity_some_holder(marcel_entity_t *e);
ma_holder_t *ma_task_some_holder(marcel_task_t *e);
#define ma_task_some_holder(t) ma_entity_some_holder(&(t)->sched.internal.entity)
#section marcel_inline
static __tbx_inline__ ma_holder_t *ma_entity_some_holder(marcel_entity_t *e)
{
        ma_holder_t *holder;
        if ((holder = e->run_holder))
		/* currently ready */
                return holder;
	/* not ready, current runqueue */
        sched_debug("using current queue for blocked %p\n",e);
#ifdef MA__BUBBLES
	if ((holder = e->sched_holder))
		return holder;
        sched_debug("using default queue for %p\n",e);
        return e->init_holder;
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
#define ma_task_holder_rawlock(t) ma_entity_holder_rawlock(&(t)->sched.internal.entity)
#define ma_task_holder_lock(t) ma_entity_holder_lock(&(t)->sched.internal.entity)
#define ma_task_holder_lock_softirq(t) ma_entity_holder_lock_softirq(&(t)->sched.internal.entity)
#define ma_task_holder_lock_softirq_async(t) ma_entity_holder_lock_softirq_async(&(t)->sched.internal.entity)
#define ma_bubble_holder_rawlock(b) ma_entity_holder_rawlock(&(b)->as_entity)
#define ma_bubble_holder_lock(b) ma_entity_holder_lock(&(b)->as_entity)
#define ma_bubble_holder_lock_softirq(b) ma_entity_holder_lock_softirq(&(b)->as_entity)
#section marcel_inline
static inline ma_holder_t *ma_entity_holder_rawlock(marcel_entity_t *e) {
	ma_holder_t *h, *h2;
	h = ma_entity_some_holder(e);
again:
	if (!h)
		return NULL;
	sched_debug("ma_entity_holder_rawlocking(%p)\n",h);
	ma_holder_rawlock(h);
	if (tbx_unlikely(h != (h2 = ma_entity_some_holder(e)))) {
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
	h = ma_entity_some_holder(e);
again:
	if (!h)
		return NULL;
	sched_debug("ma_entity_holder_locking(%p)\n",h);
	if (tbx_unlikely(!ma_holder_trylock(h)))
#ifdef MA__LWPS
		ma_holder_preempt_lock(h)
#endif
		;
	if (tbx_unlikely(h != (h2 = ma_entity_some_holder(e)))) {
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
	h = ma_entity_some_holder(e);
again:
	if (!h)
		return NULL;
	sched_debug("ma_entity_holder_locking_async(%p)\n",h);
	if (tbx_unlikely(!ma_holder_trylock(h)))
		return NULL;
	if (tbx_unlikely(h != (h2 = ma_entity_some_holder(e)))) {
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
		/* TODO: n'a plus de sens, enlever run_holder */
		if (ma_bubble_holder(hh)->as_entity.run_holder != hh)
			hh = ma_bubble_holder(hh)->as_entity.run_holder;
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
static __tbx_inline__ void ma_activate_running_entity(marcel_entity_t *e, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_activate_running_entity(marcel_entity_t *e, ma_holder_t *h) {
	MA_BUG_ON(e->holder_data);
	MA_BUG_ON(e->run_holder);
	MA_BUG_ON(e->sched_holder && ma_holder_type(h) != ma_holder_type(e->sched_holder));
	e->run_holder = h;
	if ((e->prio >= MA_BATCH_PRIO) && (e->prio != MA_LOWBATCH_PRIO))
		list_add(&e->sched_list, &h->sched_list);
	else
		list_add_tail(&e->sched_list, &h->sched_list);
	h->nr_ready++;
}

#section marcel_functions
static __tbx_inline__ void ma_rq_enqueue_entity(marcel_entity_t *e, ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void ma_rq_enqueue_entity(marcel_entity_t *e, ma_runqueue_t *rq) {
	MA_BUG_ON(e->run_holder != &rq->hold);
	ma_array_enqueue_entity(e, rq->active);
}

#section marcel_functions
static __tbx_inline__ void ma_enqueue_entity(marcel_entity_t *e, ma_holder_t *h);
void ma_enqueue_task(marcel_entity_t *e, ma_holder_t *h);
#define ma_enqueue_task(t, h) ma_enqueue_entity(&(t)->sched.internal.entity, (h))
#section marcel_inline
static __tbx_inline__ void ma_enqueue_entity(marcel_entity_t *e, ma_holder_t *h) {
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		ma_rq_enqueue_entity(e, ma_rq_holder(h));
	else
		ma_bubble_enqueue_entity(e, ma_bubble_holder(h));
}

/*
 * ma_activate_entity - move an entity to the holder
 */
#section marcel_functions
static __tbx_inline__ void ma_activate_entity(marcel_entity_t *e, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_activate_entity(marcel_entity_t *e, ma_holder_t *h) {
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		sched_debug("activating %p in %s\n",e,ma_rq_holder(h)->name);
	else
		bubble_sched_debugl(7,"activating %p in bubble %p\n",e,ma_bubble_holder(h));
	ma_activate_running_entity(e,h);
	ma_enqueue_entity(e,h);
}

/*
 * ma_activate_running_task - move a task to the holder, but don't enqueue it
 * because it is already running
 */
#section marcel_functions
static __tbx_inline__ void ma_activate_running_task(marcel_task_t *p, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_activate_running_task(marcel_task_t *p, ma_holder_t *h) {
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		sched_debug("activating running %d:%s in %s\n",p->number,p->name,ma_rq_holder(h)->name);
	else
		bubble_sched_debugl(7,"activating running %d:%s in bubble %p\n",p->number,p->name,ma_bubble_holder(h));
	ma_activate_running_entity(&p->sched.internal.entity,h);
}

/*
 * ma_activate_task - move a task to the holder
 */
#section marcel_functions
static __tbx_inline__ void ma_activate_task(marcel_task_t *p, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_activate_task(marcel_task_t *p, ma_holder_t *h) {
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		sched_debug("activating %d:%s in %s\n",p->number,p->name,ma_rq_holder(h)->name);
	else
		bubble_sched_debugl(7,"activating %d:%s in bubble %p\n",p->number,p->name,ma_bubble_holder(h));
	ma_activate_running_task(p,h);
	ma_enqueue_task(p,h);
}

/* activating whole lists of tasks */
#section marcel_functions
void ma_holder_task_list_add(struct list_head *head, marcel_task_t *p, int prio, ma_holder_t *h, void *data);
static __tbx_inline__ void ma_holder_entity_list_add(struct list_head *head, marcel_entity_t *e, int prio, ma_holder_t *h, void *data);
#define ma_holder_task_list_add(head,t,p,h,data) ma_holder_entity_list_add(head,&(t)->sched.internal.entity,p,h,data)
#section marcel_inline
static __tbx_inline__ void ma_holder_entity_list_add(struct list_head *head, marcel_entity_t *e, int prio, ma_holder_t *h, void *data)
{
	MA_BUG_ON(ma_holder_type(h) != MA_RUNQUEUE_HOLDER);
	ma_array_entity_list_add(head, e, (ma_prio_array_t *) data, ma_rq_holder(h));
}

#section marcel_functions
static __tbx_inline__ void ma_activate_running_task_list(struct list_head *head, int num, int prio, ma_holder_t *h, void *data);
#section marcel_inline
static __tbx_inline__ void ma_activate_running_task_list(struct list_head *head, int num, int prio, ma_holder_t *h, void *data)
{
	h->nr_ready += num;
}

#section marcel_functions
static __tbx_inline__ void ma_enqueue_task_list(struct list_head *head, int num, int prio, ma_holder_t *h, void *data);
#section marcel_inline
static __tbx_inline__ void ma_enqueue_task_list(struct list_head *head, int num, int prio, ma_holder_t *h, void *data)
{
	MA_BUG_ON(ma_holder_type(h) != MA_RUNQUEUE_HOLDER);
	ma_array_enqueue_entity_list(head, num, prio, (ma_prio_array_t *) data, ma_rq_holder(h));
}

#section marcel_functions
static __tbx_inline__ void ma_activate_task_list(struct list_head *head, int num, int prio, ma_holder_t *h, void *data);
#section marcel_inline
static __tbx_inline__ void ma_activate_task_list(struct list_head *head, int num, int prio, ma_holder_t *h, void *data)
{
	ma_activate_running_task_list(head, num, prio, h, data);
	ma_enqueue_task_list(head, num, prio, h, data);
}

/* deactivation */

#section marcel_functions
static __tbx_inline__ void ma_deactivate_running_entity(marcel_entity_t *e, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_deactivate_running_entity(marcel_entity_t *e, ma_holder_t *h) {
	MA_BUG_ON(e->holder_data);
	h->nr_ready--;
	list_del(&e->sched_list);
	MA_BUG_ON(e->run_holder != h);
	e->run_holder = NULL;
}

#section marcel_functions
static __tbx_inline__ void ma_rq_dequeue_entity(marcel_entity_t *e, ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void ma_rq_dequeue_entity(marcel_entity_t *e, ma_runqueue_t *rq) {
	ma_array_dequeue_entity(e, (ma_prio_array_t *) e->holder_data);
	MA_BUG_ON(e->run_holder != &rq->hold);
}

#section marcel_functions
static __tbx_inline__ void ma_dequeue_entity(marcel_entity_t *e, ma_holder_t *h);
void ma_dequeue_task(marcel_task_t *t, ma_holder_t *h);
#define ma_dequeue_task(t,h) ma_dequeue_entity(&(t)->sched.internal.entity, (h))
#section marcel_inline
static __tbx_inline__ void ma_dequeue_entity(marcel_entity_t *e, ma_holder_t *h) {
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		ma_rq_dequeue_entity(e, ma_rq_holder(h));
	else
		ma_bubble_dequeue_entity(e, ma_bubble_holder(h));
}
/*
 * ma_deactivate_entity - move an entity to the holder
 */
#section marcel_functions
static __tbx_inline__ void ma_deactivate_entity(marcel_entity_t *e, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_deactivate_entity(marcel_entity_t *e, ma_holder_t *h) {
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		sched_debug("deactivating %p from %s\n",e,ma_rq_holder(h)->name);
	else
		bubble_sched_debugl(7,"deactivating %p from bubble %p\n",e,ma_bubble_holder(h));
	ma_dequeue_entity(e,h);
	ma_deactivate_running_entity(e,h);
}

/*
 * ma_deactivate_running_task - move a task to the holder, but don't enqueue it
 * because it is already running
 */
#section marcel_functions
static __tbx_inline__ void ma_deactivate_running_task(marcel_task_t *p, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_deactivate_running_task(marcel_task_t *p, ma_holder_t *h) {
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		sched_debug("deactivating running %d:%s from %s\n",p->number,p->name,ma_rq_holder(h)->name);
	else
		bubble_sched_debugl(7,"deactivating running %d:%s from bubble %p\n",p->number,p->name,ma_bubble_holder(h));
	ma_deactivate_running_entity(&p->sched.internal.entity,h);
	if (p->sched.state == MA_TASK_INTERRUPTIBLE)
		h->nr_uninterruptible++;
}

/*
 * ma_deactivate_task - move a task to the holder
 */
#section marcel_functions
static __tbx_inline__ void ma_deactivate_task(marcel_task_t *p, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_deactivate_task(marcel_task_t *p, ma_holder_t *h) {
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		sched_debug("deactivating %d:%s from %s\n",p->number,p->name,ma_rq_holder(h)->name);
	else
		bubble_sched_debugl(7,"deactivating %d:%s from bubble %p\n",p->number,p->name,ma_bubble_holder(h));
	ma_dequeue_task(p,h);
	ma_deactivate_running_task(p,h);
}

#section marcel_functions
static __tbx_inline__ void ma_task_check(marcel_task_t *t);
#section marcel_inline
static __tbx_inline__ void ma_task_check(marcel_task_t *t) {
	if (MA_TASK_IS_READY(t)) {
		/* check that it is reachable from some runqueue */
		ma_holder_t *h = ma_task_run_holder(t);
		MA_BUG_ON(!h);
#ifdef MA__BUBBLES
		if (h->type == MA_BUBBLE_HOLDER) {
			marcel_bubble_t *b = ma_bubble_holder(h);
			MA_BUG_ON(!b->as_entity.run_holder);
			MA_BUG_ON(!b->as_entity.holder_data);
			MA_BUG_ON(list_empty(&b->queuedentities));
		}
#endif
	}
}

#section marcel_macros
#define MA_ENTITY_RUNNING 2
#define MA_ENTITY_READY 1
#define MA_ENTITY_SLEEPING 0
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
	int ret;
	ma_holder_t *h;

	ret = MA_ENTITY_SLEEPING;
	if ((h = e->run_holder)) {
		sched_debug("getting entity %p from holder %p\n", e, h);

		if (e->holder_data) {
			ret = MA_ENTITY_READY;
			ma_dequeue_entity(e, h);
		} else
			ret = MA_ENTITY_RUNNING;
		ma_deactivate_running_entity(e, h);
	}

#ifdef MA__BUBBLES
	if (e->type == MA_BUBBLE_ENTITY) {
		/* detach bubble */
		ret = MA_ENTITY_READY;
		ma_set_sched_holder(e, ma_bubble_entity(e), 0);
	}
#endif
	return ret;
}

#section marcel_functions
/** \brief Puts entity \e e into holder \e h (which must be already locked) in
 * state \e state.   If e is a bubble, its hierarchy must be already locked
 * (since it all needs moved). If h is a bubble, the corresponding top-bubble
 * and runqueue must be already locked. */
static __tbx_inline__ void ma_put_entity(marcel_entity_t *e, ma_holder_t *h, int state);
#section marcel_inline
#depend "[marcel_types]"
static __tbx_inline__ void ma_put_entity(marcel_entity_t *e, ma_holder_t *h, int state) {
#ifdef MA__BUBBLES
	if (h->type == MA_BUBBLE_HOLDER) {
		MA_BUG_ON(h != e->init_holder);
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
		ma_activate_running_entity(e, h);

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
