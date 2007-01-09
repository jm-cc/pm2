
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

/** \file
 * \brief Marcel holders
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
 * If you know that you already hold a lock on a holder, you can even
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

/** \brief Entity types: thread or bubble */
enum marcel_entity {
	/** \brief task */
	MA_TASK_ENTITY,
#ifdef MA__BUBBLES
	/** \brief bubble */
	MA_BUBBLE_ENTITY,
#endif
};

#section structures
#depend "linux_spinlock.h[types]"
#depend "[marcel_types]"
#depend "marcel_barrier.h[types]"
#depend "marcel_stats.h[marcel_types]"

/** \brief Holder information */
struct ma_holder {
	/** \brief Type of holder */
	enum marcel_holder type;
	/** \brief Lock of holder */
	ma_spinlock_t lock;
	/** \brief Number of running held entities */
	unsigned long nr_running;
	/** \brief Number of interruptibly blocked held entities */
	unsigned long nr_uninterruptible;
	/** \brief Number of entities that are currently scheduled on processors */
	unsigned long nr_scheduled;
	/** \brief Synthesis of statistics of contained entities */
	ma_stats_t stats;
};

#section types
/** \brief Holder type */
typedef struct ma_holder ma_holder_t;

#section marcel_macros
#define MA_HOLDER_INITIALIZER(t) { \
	.type = t, \
	.lock = MA_SPIN_LOCK_UNLOCKED, \
	.nr_running = 0, \
	.nr_uninterruptible = 0, \
	.nr_scheduled = 0, \
}

#section marcel_functions
static __tbx_inline__ void ma_holder_init(ma_holder_t *h, enum marcel_holder type);
#section marcel_inline
static __tbx_inline__ void ma_holder_init(ma_holder_t *h, enum marcel_holder type) {
	h->type = type;
	ma_spin_lock_init(&h->lock);
	h->nr_running = h->nr_uninterruptible = 0;
	h->nr_scheduled = 1;
}

#section marcel_functions
/** \brief Convert ma_holder_t *h into marcel_bubble_t * (assumes that \e h is a bubble) */
static __tbx_inline__ marcel_bubble_t *ma_bubble_holder(ma_holder_t *h);
/** \brief Convert ma_holder_t *h into ma_runqueue_t * (assumes that \e h is a runqueue) */
static __tbx_inline__ ma_runqueue_t *ma_rq_holder(ma_holder_t *h);
/** \brief Convert marcel_bubble_t *b into ma_holder_t * */
ma_holder_t *ma_holder_bubble(marcel_bubble_t *b);
#define ma_holder_bubble(b) (&(b)->hold)
/** \brief Convert ma_runqueue_t *b into ma_holder_t * */
ma_holder_t *ma_holder_runqueue(ma_runqueue_t *rq);
#define ma_holder_runqueue(rq) (&(rq)->hold)

#section marcel_inline
static __tbx_inline__ marcel_bubble_t *ma_bubble_holder(ma_holder_t *h) {
	return tbx_container_of(h, marcel_bubble_t, hold);
}
static __tbx_inline__ ma_runqueue_t *ma_rq_holder(ma_holder_t *h) {
	return tbx_container_of(h, ma_runqueue_t, hold);
}

#section marcel_macros
#ifdef MA__BUBBLES
/** \brief Return type of holder \e h */
#define ma_holder_type(h) ((h)->type)
#else
#define ma_holder_type(h) MA_RUNQUEUE_HOLDER
#endif

#section structures
/* Une entit� a un type (bulle/thread), a un conteneur initial (bulle ou
 * directement runqueue), un conteneur d'ordonnancement, et un conteneur o�
 * elle est active (pr�te � �tre ordonnanc�).  Si holder_data est NULL, c'est
 * qu'elle est en cours d'ex�cution. Sinon, c'est qu'elle est pr�empt�e, pr�te
 * pour l'ex�cution dans la liste run_list.
 *
 * On peut ainsi avoir un thread contenue dans une bulle (init_holder), endormi
 * (donc run_holder == NULL), mais qui sera r�veill� sur une certaine runqueue
 * (sched_holder).
 * */
#depend "[types]"
#depend "pm2_list.h"
#depend "asm/linux_atomic.h[marcel_types]"
#depend "marcel_stats.h[marcel_types]"
/** \brief Entity information
 *
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
	/** \brief Initial holder */
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
#ifdef MARCEL_BUBBLE_EXPLODE
#ifdef MA__LWPS
	/** \brief scheduling level for the Burst Algorithm */
	int sched_level;
#endif
#endif
	/** \brief Statistics for the entity */
	ma_stats_t stats;

	/** \brief List of attached memory areas */
	struct list_head memory_areas;
	/** \brief Lock for ::memory_areas */
	ma_spinlock_t memory_areas_lock;
};

#section types
typedef struct ma_sched_entity marcel_entity_t;

#section marcel_functions
/** \brief Convert ma_entity_t * \e e into marcel_task_t * (assumes that \e e is a thread) */
static __tbx_inline__ marcel_task_t *ma_task_entity(marcel_entity_t *e);
#ifdef MA__BUBBLES
/** \brief Convert ma_entity_t * \e e into marcel_bubble_t * (assumes that \e e is a bubble) */
static __tbx_inline__ marcel_bubble_t *ma_bubble_entity(marcel_entity_t *e);
#endif
/** \brief Convert marcel_bubble_t *b into marcel_entity_t * */
marcel_entity_t *marcel_entity_bubble(marcel_bubble_t *b);
#define marcel_entity_bubble(b) (&(b)->sched)
/** \brief Convert marcel_task_t *b into marcel_entity_t * */
marcel_entity_t *marcel_entity_task(marcel_task_t *t);
#define marcel_entity_task(t) (&(t)->sched.internal.entity)
#section marcel_inline
static __tbx_inline__ marcel_task_t *ma_task_entity(marcel_entity_t *e) {
	MA_BUG_ON(e->type != MA_TASK_ENTITY);
	return tbx_container_of(e, marcel_task_t, sched.internal.entity);
}
#ifdef MA__BUBBLES
static __tbx_inline__ marcel_bubble_t *ma_bubble_entity(marcel_entity_t *e) {
	MA_BUG_ON(e->type != MA_BUBBLE_ENTITY);
	return tbx_container_of(e, marcel_bubble_t, sched);
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
	.memory_areas_lock = MA_SPIN_LOCK_UNLOCKED, \
	.memory_areas = LIST_HEAD_INIT((e).memory_areas), \
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
#define ma_holder_lock_softirq(h) ma_spin_lock_softirq(&(h)->lock)
#define ma_holder_rawunlock(h) _ma_raw_spin_unlock(&(h)->lock)
#define ma_holder_unlock(h) ma_spin_unlock(&(h)->lock)
/** \brief Unlocks holder \e h */
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
#define ma_task_holder_rawlock(t) ma_entity_holder_rawlock(&(t)->sched.internal.entity)
#define ma_task_holder_lock(t) ma_entity_holder_lock(&(t)->sched.internal.entity)
#define ma_task_holder_lock_softirq(t) ma_entity_holder_lock_softirq(&(t)->sched.internal.entity)
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
		sched_debug("ma_entity_holder_rawunlocking(%p)\n",h);
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
static __tbx_inline__ ma_holder_t *ma_this_holder_lock();
#section marcel_inline
static __tbx_inline__ ma_holder_t *ma_this_holder_lock() {
	ma_holder_t *h;
	ma_local_bh_disable();
	h = ma_this_holder();
	ma_holder_lock(h);

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
		if (ma_bubble_holder(hh)->sched.run_holder != hh)
			hh = ma_bubble_holder(hh)->sched.run_holder;
		else if (ma_bubble_holder(hh)->sched.sched_holder != hh)
			hh = ma_bubble_holder(hh)->sched.sched_holder;
		else break;
	}
#else
	hh = h;
#endif
	return hh && ma_holder_type(hh) == MA_RUNQUEUE_HOLDER?ma_rq_holder(hh):NULL;
}

/* Activations et d�sactivations n�cessitent que le holder soit verrouill�. */

/* Attention, lorsque le holder est une bulle, il se peut qu'ils d�verrouillent
 * le holder pour pouvoir verrouiller celui qui le contient imm�diatement */

/* activation */

#section marcel_functions
static __tbx_inline__ void ma_activate_running_entity(marcel_entity_t *e, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_activate_running_entity(marcel_entity_t *e, ma_holder_t *h) {
	MA_BUG_ON(e->run_holder);
	MA_BUG_ON(e->sched_holder && ma_holder_type(h) != ma_holder_type(e->sched_holder));
	e->run_holder = h;
	h->nr_running++;
}

#section marcel_functions
static __tbx_inline__ void ma_rq_enqueue_entity(marcel_entity_t *e, ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void ma_rq_enqueue_entity(marcel_entity_t *e, ma_runqueue_t *rq) {
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
	h->nr_running += num;
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
	h->nr_running--;
	MA_BUG_ON(!e->run_holder);
	e->run_holder = NULL;
}

#section marcel_functions
static __tbx_inline__ void ma_rq_dequeue_entity(marcel_entity_t *e, ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void ma_rq_dequeue_entity(marcel_entity_t *e, ma_runqueue_t *rq) {
	ma_array_dequeue_entity(e, (ma_prio_array_t *) e->holder_data);
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

#section common
#ifdef MARCEL_BUBBLE_STEAL
#section marcel_macros
#define MA_ENTITY_RUNNING 2
#define MA_ENTITY_BLOCKED 1
#define MA_ENTITY_SLEEPING 0
#section marcel_functions
void TBX_EXTERN ma_set_sched_holder(marcel_entity_t *e, marcel_bubble_t *bubble);
/** \brief Gets entity \e e out from its holder (which must be already locked), returns its state */
static __tbx_inline__ int ma_get_entity(marcel_entity_t *e);
#section marcel_inline
static __tbx_inline__ int ma_get_entity(marcel_entity_t *e) {
	int ret;
	ma_holder_t *h;

	ret = MA_ENTITY_SLEEPING;
	if ((h = e->run_holder)) {
		sched_debug("getting entity %p from holder %p\n", e, h);

		if (e->holder_data) {
			ret = MA_ENTITY_BLOCKED;
			ma_dequeue_entity(e, h);
		} else
			ret = MA_ENTITY_RUNNING;
		ma_deactivate_running_entity(e, h);
	}

	if (e->type == MA_BUBBLE_ENTITY) {
		ret = MA_ENTITY_BLOCKED;
		ma_set_sched_holder(e, ma_bubble_entity(e));
	}
	return ret;
}

#section marcel_functions
/** \brief Put entity \e e into holder \e h (which must be already locked) in state \e state */
static __tbx_inline__ void ma_put_entity(marcel_entity_t *e, ma_holder_t *h, int state);
#section marcel_inline
static __tbx_inline__ void ma_put_entity(marcel_entity_t *e, ma_holder_t *h, int state) {
	e->sched_holder = h;
	if (state == MA_ENTITY_SLEEPING)
		return;

	ma_activate_running_entity(e, h);
	if (state == MA_ENTITY_BLOCKED &&
		/* ne pas enqueuer une bulle dans une autre ! */
		!(e->type == MA_BUBBLE_ENTITY && h->type == MA_BUBBLE_HOLDER))
		ma_enqueue_entity(e, h);
}
#section common
#endif

#section marcel_structures

/* @} */
