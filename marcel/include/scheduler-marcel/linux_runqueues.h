
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
 * \brief Linux runqueues
 *
 * \defgroup marcel_runqueues Runqueues
 *
 * Runqueues hold entities. Each topology level has its own runqueue, where
 * processors covered by this level can take work.
 *
 * Entities in the runqueues are indexed by priority, whose values can be in:
 *
 * - MA_NOSCHED_PRIO (never scheduled)
 * - MA_IDLE_PRIO (idle thread)
 * - MA_LOWBATCH_PRIO (usually used for blocked batch thread)
 * - MA_BATCH_PRIO (batch thread)
 * - MA_DEF_PRIO .. MA_DEF_PRIO + MAX_MAX_NICE (default priority)
 * - MA_RT_PRIO .. MA_MAX_RT_PRIO == MA_RT_PRIO + MA_MAX_USER_RT_PRIO (application real-time priority)
 * - MA_SYS_RT_PRIO (Marcel-internal Real-Time priority)
 *
 * ::ma_rq_queue provides access to the queue of entities of a given priority.
 *
 * @{
 */
#section common
#depend "scheduler-marcel/marcel_bubble_sched.h[types]"
#include "tbx_compiler.h"

/*
 * These are the runqueue data structures:
 */

#section types
/** \brief Type of runqueues */
typedef struct ma_runqueue ma_runqueue_t;
typedef ma_runqueue_t ma_topo_level_schedinfo;

#section macros
/*
 * Priority of a process goes from 0..MAX_PRIO-1, valid RT
 * priority is 0..MAX_RT_PRIO-1, and SCHED_NORMAL tasks are
 * in the range MAX_RT_PRIO..MAX_PRIO-1. Priority values
 * are inverted: lower p->prio value means higher priority.
 *
 * The MA_MAX_RT_USER_PRIO value allows the actual maximum
 * RT priority to be separate from the value exported to
 * user-space.  This allows kernel threads to set their
 * priority to a value higher than any user task. Note:
 * MAX_RT_PRIO must not be smaller than MAX_USER_RT_PRIO.
 */

#define MA_SYS_RT_PRIO		MA_MAX_SYS_RT_PRIO
#define MA_MAX_RT_PRIO		(MA_SYS_RT_PRIO+1)
#define MA_RT_PRIO		(MA_MAX_RT_PRIO+MA_MAX_USER_RT_PRIO)
#define MA_DEF_PRIO		(MA_RT_PRIO+MA_MAX_NICE+1)
#define MA_BATCH_PRIO		(MA_DEF_PRIO+1)
#define MA_LOWBATCH_PRIO        (MA_BATCH_PRIO+1)
#define MA_IDLE_PRIO		(MA_LOWBATCH_PRIO+1)
#define MA_NOSCHED_PRIO		(MA_IDLE_PRIO+1)
#define MA_MAX_PRIO		(MA_NOSCHED_PRIO+1)

/** \brief Whether the thread has Real-Time priority */
#define ma_rt_task(p)		\
	((p)->as_entity.prio < MA_RT_PRIO)

#section marcel_macros
#depend "[macros]"
#depend "asm/linux_types.h[macros]"
#define MA_BITMAP_SIZE ((MA_MAX_PRIO+1+MA_BITS_PER_LONG)/MA_BITS_PER_LONG)

#section marcel_structures
#depend "pm2_list.h"

struct prio_array {
	int nr_active;
	unsigned long bitmap[MA_BITMAP_SIZE];
	struct list_head queue[MA_MAX_PRIO];
};

#section marcel_types
typedef struct prio_array ma_prio_array_t;

#section marcel_structures
#depend "[types]"
#depend "marcel_topology.h[types]"
#depend "scheduler/marcel_holder.h[marcel_structures]"
/**
 * Locking rule: those places that want to lock multiple runqueues (such as the
 * load balancing or the thread migration code), lock acquire operations must
 * be ordered by descending level: the main runqueue first, then lower levels,
 * and on a given level, from low indices to high indices.
 */
struct ma_runqueue {
	/** \brief Holder information */
	struct ma_holder as_holder;
	/* Number of switches */
	//unsigned long long nr_switches;
	/** \brief Name of the runqueue */
	char name[16];
	//unsigned long expired_timestamp, timestamp_last_tick;
	/** \brief active arrays of queues */
	ma_prio_array_t active[1];
	//ma_prio_array_t *active;
	/* \brief expired arrays of queues */
	//ma_prio_array_t *expired, arrays[2];
/* 	int best_expired_prio, prev_cpu_load[NR_CPUS]; */
/* #ifdef CONFIG_NUMA */
/* 	atomic_t *node_nr_ready; */
/* 	int prev_node_load[MAX_NUMNODES]; */
/* #endif */
/* 	marcel_task_t *migration_thread; */
/* 	struct list_head migration_queue; */

/* 	ma_atomic_t nr_iowait; */

#ifdef MA__LWPS
	/** \brief "Father" of the runqueue in the topology */
	struct ma_runqueue *father;
	/** \brief "Level" of the runqueue in the topology */
	int level;
	/** \brief Vpset of the runqueue */
	marcel_vpset_t vpset;
#endif
	/** \brief Corresponding topo-level */
	struct marcel_topo_level *topolevel;

#ifdef MA__BUBBLES
        /** \brief General-purpose list link for bubble schedulers */
	struct list_head next;
#endif
};

#section marcel_variables
#depend "linux_perlwp.h[marcel_macros]"
#depend "[marcel_macros]"

/** \brief The main runqueue (for the whole machine) */
extern ma_runqueue_t ma_main_runqueue;
#define ma_main_runqueue (marcel_machine_level[0].rq)
extern TBX_EXTERN ma_runqueue_t ma_dontsched_runqueue;

#section marcel_macros
#ifdef MA__LWPS
/* \brief Runqueue of LWP \e lwp */
ma_runqueue_t *ma_lwp_rq(ma_lwp_t lwp);
#define ma_lwp_rq(lwp)		(&ma_per_lwp(runqueue, (lwp)))
/** \brief Runqueue of VP run by LWP \e lwp */
ma_runqueue_t *ma_lwp_vprq(ma_lwp_t lwp);
#define ma_lwp_vprq(lwp)	(&(lwp)->vp_level->rq)
/* \brief "Don't sched" runqueue of LWP \e lwp (for idle & such) */
ma_runqueue_t *ma_dontsched_rq(ma_lwp_t lwp);
#define ma_dontsched_rq(lwp)	(&ma_per_lwp(dontsched_runqueue, (lwp)))
/** \brief Whether runqueue covers VP \e vpnum */
int ma_rq_covers(ma_runqueue_t *rq, int vpnum);
#define ma_rq_covers(rq,vpnum)	(vpnum != -1 && marcel_vpset_isset(&rq->vpset, vpnum))
#else
#define ma_lwp_rq(lwp)		(&ma_main_runqueue)
#define ma_lwp_vprq(lwp)		(&ma_main_runqueue)
#define ma_dontsched_rq(lwp)	(&ma_dontsched_runqueue)
#define ma_rq_covers(rq,vpnum)	((void)(rq),(void)(vpnum),1)
#endif
/** \brief Current thread */
marcel_task_t ma_lwp_curr(ma_lwp_t lwp);
#define ma_lwp_curr(lwp)	ma_per_lwp(current_thread, lwp)
struct prio_array;
/* \brief Queue of entities with priority \e prio in array \e array */
struct list_head *ma_array_queue(struct prio_array *array, int prio);
#define ma_array_queue(array,prio)	((array)->queue + (prio))
/** \brief Queue of priority \e prio in runqueue \e rq */
struct list_head *ma_rq_queue(ma_runqueue_t *rq, int prio);
#define ma_rq_queue(rq,prio)	ma_array_queue((rq)->active, (prio))
/** \brief Whether queue \e queue is empty */
int ma_queue_empty(struct list_head *queue);
#define ma_queue_empty(queue)	list_empty(queue)
/** \brief First entity in queue \e queue */
marcel_entity_t *ma_queue_entry(struct list_head *queue);
#define ma_queue_entry(queue)	list_entry((queue)->next, marcel_entity_t, run_list)

/** \brief Iterate through the entities held in queue \e queue */
#define ma_queue_for_each_entry(e, queue) list_for_each_entry(e, queue, run_list)
/** \brief Same as ma_queue_for_each_entry(), but safe version against current
 * item removal: prefetches the next entity in \e ee. */
#define ma_queue_for_each_entry_safe(e, ee, queue) list_for_each_entry_safe(e, ee, queue, run_list)

/*
 * Adding/removing a task to/from a priority array:
 */
#section marcel_functions
static __tbx_inline__ void ma_array_dequeue_task(marcel_task_t *p, ma_prio_array_t *array);
static __tbx_inline__ void ma_array_dequeue_entity(marcel_entity_t *p, ma_prio_array_t *array);
#section marcel_inline
static __tbx_inline__ void ma_array_dequeue_entity(marcel_entity_t *e, ma_prio_array_t *array)
{
	sched_debug("dequeueing %p (prio %d) from %p\n",e,e->prio,array);
	if (e->prio != MA_NOSCHED_PRIO)
		array->nr_active--;
	list_del(&e->run_list);
	if (list_empty(ma_array_queue(array, e->prio))) {
		sched_debug("array %p (prio %d) empty\n",array, e->prio);
		__ma_clear_bit(e->prio, array->bitmap);
	}
	MA_BUG_ON(!e->run_holder_data);
	e->run_holder_data = NULL;
}
static __tbx_inline__ void ma_array_dequeue_task(marcel_task_t *p, ma_prio_array_t *array)
{
	sched_debug("dequeueing %d:%s (prio %d) from %p\n",p->number,p->name,p->as_entity.prio,array);
	ma_array_dequeue_entity(&p->as_entity, array);
}

#section marcel_functions
static __tbx_inline__ void ma_array_enqueue_task(marcel_task_t *p, ma_prio_array_t *array);
static __tbx_inline__ void ma_array_enqueue_entity(marcel_entity_t *p, ma_prio_array_t *array);
#section marcel_inline
static __tbx_inline__ void ma_array_enqueue_entity(marcel_entity_t *e, ma_prio_array_t *array)
{
	sched_debug("enqueueing %p (prio %d) in %p\n",e,e->prio,array);
	if ((e->prio >= MA_BATCH_PRIO) && (e->prio != MA_LOWBATCH_PRIO))
		list_add(&e->run_list, ma_array_queue(array, e->prio));
	else
		list_add_tail(&e->run_list, ma_array_queue(array, e->prio));
	__ma_set_bit(e->prio, array->bitmap);
	if (e->prio != MA_NOSCHED_PRIO)
		array->nr_active++;
	MA_BUG_ON(e->run_holder_data);
	e->run_holder_data = array;
}
static __tbx_inline__ void ma_array_enqueue_task(marcel_task_t *p, ma_prio_array_t *array)
{
	sched_debug("enqueueing %d:%s (prio %d) in %p\n",p->number,p->name,p->as_entity.prio,array);
	ma_array_enqueue_entity(&p->as_entity,array);
}

#section marcel_functions
static __tbx_inline__ void ma_array_entity_list_add(struct list_head *head, marcel_entity_t *e, ma_prio_array_t *array, ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void ma_array_entity_list_add(struct list_head *head, marcel_entity_t *e, ma_prio_array_t *array, ma_runqueue_t *rq) {
	MA_BUG_ON(e->run_holder_data);
	list_add_tail(&e->run_list, head);
	e->run_holder_data = array;
	MA_BUG_ON(e->run_holder);
	e->run_holder = &rq->as_holder;
}

#section marcel_functions
static __tbx_inline__ void ma_array_enqueue_entity_list(struct list_head *head, int num, int prio, ma_prio_array_t *array, ma_runqueue_t *rq);
#define ma_array_enqueue_task_list ma_array_enqueue_entity_list
#section marcel_inline
static __tbx_inline__ void ma_array_enqueue_entity_list(struct list_head *head, int num, int prio, ma_prio_array_t *array, ma_runqueue_t *rq)
{
	list_splice(head, ma_array_queue(array, prio));
	__ma_set_bit(prio, array->bitmap);
	array->nr_active += num;
}

#section marcel_functions
/* \brief Switch the active and expired arrays of runqueue \e rq. */
//static __tbx_inline__ void rq_arrays_switch(ma_runqueue_t *rq);
#section marcel_inline
#if 0
static __tbx_inline__ void rq_arrays_switch(ma_runqueue_t *rq)
{
	ma_prio_array_t *array = rq->active;

	rq->active = rq->expired;
	rq->expired = array;
	array = rq->active;
	//rq->expired_timestamp = 0;
/* 	rq->best_expired_prio = MA_MAX_PRIO; */
}
#endif

#section marcel_functions
extern void ma_init_rq(ma_runqueue_t *rq, char *name);

/* @} */
