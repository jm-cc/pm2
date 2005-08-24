
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

#section marcel_types
enum marcel_holder {
	MA_RUNQUEUE_HOLDER,
#ifdef MA__BUBBLES
	MA_BUBBLE_HOLDER,
#endif
};

enum marcel_entity {
	MA_TASK_ENTITY,
#ifdef MA__BUBBLES
	MA_BUBBLE_ENTITY,
#endif
};

#section structures
#depend "asm/linux_spinlock.h[types]"
#depend "[marcel_types]"

/* Un conteneur a un type (bulle/runqueue), et peut se verrouiller pour ajouter
 * des entités. */
/* verrouillage: toujours dans l'ordre contenant puis contenu:
 * runqueue, puis bulle sur la runqueues, puis bulles contenues dans la
 * bulle, mais sur la même runqueue, puis sous-runqueue, puis bulles
 * contenues dan bulle, mais sur la sous-runqueue, etc... */
struct ma_holder {
	enum marcel_holder type;
	ma_spinlock_t lock;
	unsigned long nr_running, nr_uninterruptible;
};

#section types
typedef struct ma_holder ma_holder_t;

#section marcel_macros
#define MA_HOLDER_INITIALIZER(t) { \
	.type = t, \
	.lock = MA_SPIN_LOCK_UNLOCKED, \
	.nr_running = 0, \
	.nr_uninterruptible = 0, \
}

#section marcel_functions
static __tbx_inline__ void ma_holder_init(ma_holder_t *h, enum marcel_holder type);
#section marcel_inline
static __tbx_inline__ void ma_holder_init(ma_holder_t *h, enum marcel_holder type) {
	h->type = type;
	ma_spin_lock_init(&h->lock);
	h->nr_running = h->nr_uninterruptible = 0;
}

#section marcel_macros
#define ma_rq_holder(h) tbx_container_of((h),ma_runqueue_t,hold)
#ifdef MA__BUBBLES
#define ma_bubble_holder(h) tbx_container_of((h),marcel_bubble_t,hold)
#define ma_holder_type(h) ((h)->type)
#else
#define ma_holder_type(h) MA_RUNQUEUE_HOLDER
#endif

#section structures
/* Une entité a un type (bulle/thread), a un conteneur initial (bulle ou
 * directement runqueue), un conteneur d'ordonnancement, et un conteneur où
 * elle est active (prête à être ordonnancé).  Si holder_data est NULL, c'est
 * qu'elle est en cours d'exécution. Sinon, c'est qu'elle est préemptée, prête
 * pour l'exécution dans la liste run_list.
 *
 * On peut ainsi avoir un thread contenue dans une bulle (init_holder), endormi
 * (donc run_holder == NULL), mais qui sera réveillé sur une certaine runqueue
 * (sched_holder).
 * */
#depend "[types]"
#depend "pm2_list.h"
#depend "asm/linux_atomic.h[marcel_types]"
struct ma_sched_entity {
	enum marcel_entity type;
	ma_holder_t *init_holder;
	ma_holder_t *sched_holder;
	ma_holder_t *run_holder;
	void *holder_data;
	struct list_head run_list;
	int sched_policy;
	int prio;
	unsigned long timestamp, last_ran;
	ma_atomic_t time_slice;
#ifdef MA__BUBBLES
	struct list_head entity_list;
#endif
#ifdef MA__LWPS
	int sched_level;
#endif
};

#section types
typedef struct ma_sched_entity marcel_entity_t;

#section macros
#ifdef MA__LWPS
#define SCHED_LEVEL_INIT .sched_level = MARCEL_LEVEL_DEFAULT,
#else
#define SCHED_LEVEL_INIT
#endif

#define MA_SCHED_ENTITY_INITIALIZER(e,t,p) { \
	.type = t, \
	.init_holder = NULL,.sched_holder = NULL, .run_holder = NULL, \
	.holder_data = NULL, \
	.run_list = LIST_HEAD_INIT((e).run_list), \
	/*.sched_policy = */ \
	.prio = p, \
	.timestamp = 0, .last_ran = 0, \
	.time_slice = MA_ATOMIC_INIT(0), \
	SCHED_LEVEL_INIT \
}

#section marcel_macros
#define ma_task_init_holder(p)	((p)->sched.internal.init_holder)
#define ma_task_sched_holder(p)	((p)->sched.internal.sched_holder)
#define ma_task_run_holder(p)	((p)->sched.internal.run_holder)
#define ma_this_holder()	(ma_task_run_holder(MARCEL_SELF))
#define ma_task_holder_data(p)  ((p)->sched.internal.holder_data)

#define MA_TASK_IS_RUNNING(tsk) (ma_task_run_holder(tsk)&&!ma_task_holder_data(tsk))
#define MA_TASK_IS_SLEEPING(tsk) (ma_task_run_holder(tsk)&&ma_task_holder_data(tsk))
#define MA_TASK_IS_BLOCKED(tsk) (!ma_task_run_holder(tsk))

#define ma_holder_preempt_lock(h) __ma_preempt_spin_lock(&(h)->lock)
#define ma_holder_trylock(h) _ma_raw_spin_trylock(&(h)->lock)
#define ma_holder_rawlock(h) _ma_raw_spin_lock(&(h)->lock)
#define ma_holder_lock(h) ma_spin_lock(&(h)->lock)
#define ma_holder_lock_softirq(h) ma_spin_lock_softirq(&(h)->lock)
#define ma_holder_rawunlock(h) _ma_raw_spin_unlock(&(h)->lock)
#define ma_holder_unlock(h) ma_spin_unlock(&(h)->lock)
#define ma_holder_unlock_softirq(h) ma_spin_unlock_softirq(&(h)->lock)

/* locking */

#section marcel_functions
static __tbx_inline__ ma_holder_t *ma_entity_some_holder(marcel_entity_t *e);
ma_holder_t *ma_task_some_holder(marcel_task_t *e);
#define ma_task_some_holder(t) ma_entity_some_holder(&(t)->sched.internal)
#section marcel_inline
static __tbx_inline__ ma_holder_t *ma_entity_some_holder(marcel_entity_t *e)
{
        ma_holder_t *holder;
        if ((holder = e->run_holder))
		/* currently ready */
                return holder;
	/* not ready, current runqueue */
        sched_debug("using current queue for blocked %p\n",e);
	if ((holder = e->sched_holder))
		return holder;
        sched_debug("using default queue for %p\n",e);
        return e->init_holder;

}

#section marcel_functions
static __tbx_inline__ ma_holder_t *ma_entity_holder_rawlock(marcel_entity_t *e);
static __tbx_inline__ ma_holder_t *ma_entity_holder_lock(marcel_entity_t *e);
static __tbx_inline__ ma_holder_t *ma_entity_holder_lock_softirq(marcel_entity_t *e);
ma_holder_t *ma_task_holder_rawlock(marcel_task_t *e);
ma_holder_t *ma_task_holder_lock(marcel_task_t *e);
ma_holder_t *ma_task_holder_lock_softirq(marcel_task_t *e);
#define ma_task_holder_rawlock(t) ma_entity_holder_rawlock(&(t)->sched.internal)
#define ma_task_holder_lock(t) ma_entity_holder_lock(&(t)->sched.internal)
#define ma_task_holder_lock_softirq(t) ma_entity_holder_lock_softirq(&(t)->sched.internal)
#section marcel_inline
static __tbx_inline__ ma_holder_t *ma_entity_holder_rawlock(marcel_entity_t *e) {
	ma_holder_t *h;
again:
	h = ma_entity_some_holder(e);
	sched_debug("ma_entity_holder_locking(%p)\n",h);
	ma_holder_rawlock(h);
	if (tbx_unlikely(h != ma_entity_some_holder(e))) {
		sched_debug("ma_entity_holder_unlocking(%p)\n",h);
		ma_holder_rawunlock(h);
		goto again;
	}
	sched_debug("ma_entity_holder_locked(%p)\n",h);
	return h;
}
static __tbx_inline__ ma_holder_t *ma_entity_holder_lock(marcel_entity_t *e) {
	ma_holder_t *h;
again:
	ma_preempt_disable();
	h = ma_entity_some_holder(e);
	sched_debug("ma_entity_holder_locking(%p)\n",h);
	if (tbx_unlikely(!ma_holder_trylock(h)))
#ifdef MA__LWPS
		ma_holder_preempt_lock(h)
#endif
		;
	if (tbx_unlikely(h != ma_entity_some_holder(e))) {
		sched_debug("ma_entity_holder_unlocking(%p)\n",h);
		ma_holder_unlock(h);
		goto again;
	}
	sched_debug("ma_entity_holder_locked(%p)\n",h);
	return h;
}

static __tbx_inline__ ma_holder_t *ma_entity_holder_lock_softirq(marcel_entity_t *e) {
	ma_local_bh_disable();
	return ma_entity_holder_lock(e);
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
static __tbx_inline__ void ma_entity_holder_unlock(ma_holder_t *h);
static __tbx_inline__ void ma_entity_holder_unlock_softirq(ma_holder_t *h);
void ma_task_holder_unlock(ma_holder_t *h);
void ma_task_holder_unlock_softirq(ma_holder_t *h);
#define ma_task_holder_unlock(h) ma_entity_holder_unlock(h)
#define ma_task_holder_unlock_softirq(h) ma_entity_holder_unlock_softirq(h)
#section marcel_inline
static __tbx_inline__ void ma_entity_holder_unlock(ma_holder_t *h) {
	sched_debug("ma_entity_holder_unlock(%p)\n",h);
	ma_holder_unlock(h);
}
static __tbx_inline__ void ma_entity_holder_unlock_softirq(ma_holder_t *h) {
	sched_debug("ma_entity_holder_unlock_softirq(%p)\n",h);
	ma_holder_unlock_softirq(h);
}

/* topology */

/* ma_holder_rq - go up through holders and get to a rq */
#section marcel_functions
static __tbx_inline__ ma_runqueue_t *ma_to_rq_holder(ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ ma_runqueue_t *ma_to_rq_holder(ma_holder_t *h) {
	ma_holder_t *hh;
	for (hh=h; ma_holder_type(hh) != MA_RUNQUEUE_HOLDER;
			hh=ma_bubble_holder(hh)->sched.sched_holder);
	return ma_rq_holder(hh);
}

/* activations et désactivations nécessitent que le holder soit verrouillé */

/* activation */

#section marcel_functions
static __tbx_inline__ void ma_activate_running_entity(marcel_entity_t *e, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_activate_running_entity(marcel_entity_t *e, ma_holder_t *h) {
	MA_BUG_ON(e->run_holder);
	e->run_holder = h;
	h->nr_running++;
}

#section marcel_functions
static __tbx_inline__ void ma_enqueue_entity_rq(marcel_entity_t *e, ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void ma_enqueue_entity_rq(marcel_entity_t *e, ma_runqueue_t *rq) {
	ma_rq_enqueue_entity(e, rq->active);
}

#section marcel_functions
static __tbx_inline__ void ma_enqueue_entity(marcel_entity_t *e, ma_holder_t *h);
void ma_enqueue_task(marcel_entity_t *e, ma_holder_t *h);
#define ma_enqueue_task(t, h) ma_enqueue_entity(&(t)->sched.internal, (h))
#section marcel_inline
static __tbx_inline__ void ma_enqueue_entity(marcel_entity_t *e, ma_holder_t *h) {
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		ma_enqueue_entity_rq(e, ma_rq_holder(h));
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
		bubble_sched_debug("activating %p in bubble %p\n",e,ma_bubble_holder(h));
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
		sched_debug("activating running %ld:%s in %s\n",p->number,p->name,ma_rq_holder(h)->name);
	else
		bubble_sched_debug("activating running %ld:%s in bubble %p\n",p->number,p->name,ma_bubble_holder(h));
	ma_activate_running_entity(&p->sched.internal,h);
}

/*
 * ma_activate_task - move a task to the holder
 */
#section marcel_functions
static __tbx_inline__ void ma_activate_task(marcel_task_t *p, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_activate_task(marcel_task_t *p, ma_holder_t *h) {
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		sched_debug("activating %ld:%s in %s\n",p->number,p->name,ma_rq_holder(h)->name);
	else
		bubble_sched_debug("activating %ld:%s in bubble %p\n",p->number,p->name,ma_bubble_holder(h));
	ma_activate_running_task(p,h);
	ma_enqueue_task(p,h);
}

/* deactivation */

#section marcel_functions
static __tbx_inline__ void ma_deactivate_running_entity(marcel_entity_t *e, ma_holder_t *h);
#section marcel_inline
static __tbx_inline__ void ma_deactivate_running_entity(marcel_entity_t *e, ma_holder_t *h) {
	h->nr_running--;
	MA_BUG_ON(!e->run_holder);
	e->run_holder = NULL;
}

#section marcel_functions
static __tbx_inline__ void ma_dequeue_entity_rq(marcel_entity_t *e, ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void ma_dequeue_entity_rq(marcel_entity_t *e, ma_runqueue_t *rq) {
	ma_rq_dequeue_entity(e, e->holder_data);
}

#section marcel_functions
static __tbx_inline__ void ma_dequeue_entity(marcel_entity_t *e, ma_holder_t *h);
void ma_dequeue_task(marcel_task_t *t, ma_holder_t *h);
#define ma_dequeue_task(t,h) ma_dequeue_entity(&(t)->sched.internal, (h))
#section marcel_inline
static __tbx_inline__ void ma_dequeue_entity(marcel_entity_t *e, ma_holder_t *h) {
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		ma_dequeue_entity_rq(e, ma_rq_holder(h));
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
		sched_debug("activating %p in %s\n",e,ma_rq_holder(h)->name);
	else
		bubble_sched_debug("activating %p in bubble %p\n",e,ma_bubble_holder(h));
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
		sched_debug("activating running %ld:%s in %s\n",p->number,p->name,ma_rq_holder(h)->name);
	else
		bubble_sched_debug("activating running %ld:%s in bubble %p\n",p->number,p->name,ma_bubble_holder(h));
	ma_deactivate_running_entity(&p->sched.internal,h);
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
		sched_debug("activating %ld:%s in %s\n",p->number,p->name,ma_rq_holder(h)->name);
	else
		bubble_sched_debug("activating %ld:%s in bubble %p\n",p->number,p->name,ma_bubble_holder(h));
	ma_dequeue_task(p,h);
	ma_deactivate_running_task(p,h);
}

