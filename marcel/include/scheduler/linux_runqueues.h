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


#ifndef __LINUX_RUNQUEUES_H__
#define __LINUX_RUNQUEUES_H__


#include "tbx_compiler.h"
#include "marcel_config.h"
#include "scheduler/marcel_sched_types.h"
#include "sys/linux_types.h"
#include "sys/marcel_types.h"
#include "sys/marcel_lwp.h"


/** Public macros **/
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


/** Public data types **/
/** \brief Type of runqueues */
typedef struct ma_runqueue ma_runqueue_t;
typedef ma_runqueue_t ma_topo_level_schedinfo;


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#define MA_BITMAP_BITS (MA_MAX_PRIO+1)
#if MA_BITMAP_BITS > 140
#error ma_sched_find_first_bit implementations need to be extended.
#endif
#define MA_BITMAP_SIZE ((140+MA_BITS_PER_LONG)/MA_BITS_PER_LONG)

#ifdef MA__LWPS
/* \brief Runqueue of LWP \e lwp */
    ma_runqueue_t * ma_lwp_rq(ma_lwp_t lwp);
#define ma_lwp_rq(lwp)		(&ma_per_lwp(runqueue, (lwp)))
/** \brief Runqueue of VP run by LWP \e lwp */
ma_runqueue_t *ma_lwp_vprq(ma_lwp_t lwp);
#define ma_lwp_vprq(lwp)	(&(lwp)->vp_level->rq)
/* \brief "Don't sched" runqueue of LWP \e lwp (for idle & such) */
ma_runqueue_t *ma_dontsched_rq(ma_lwp_t lwp);
#define ma_dontsched_rq(lwp)	(&ma_per_lwp(dontsched_runqueue, (lwp)))
/** \brief Whether runqueue covers VP \e vpnum */
int ma_rq_covers(ma_runqueue_t * rq, int vpnum);
#define ma_rq_covers(rq,vpnum)	(vpnum != -1 && marcel_vpset_isset(&rq->vpset, vpnum))
#else
#define ma_lwp_rq(lwp)		(&ma_main_runqueue)
#define ma_lwp_vprq(lwp)		(&ma_main_runqueue)
#define ma_dontsched_rq(lwp)	(&ma_dontsched_runqueue)
#define ma_rq_covers(rq,vpnum)	((void)(rq),(void)(vpnum),1)
#endif				/* MA__LWPS */
/** \brief Current thread */
    marcel_task_t ma_lwp_curr(ma_lwp_t lwp);
#define ma_lwp_curr(lwp)	ma_per_lwp(current_thread, lwp)
struct prio_array;
/* \brief Queue of entities with priority \e prio in array \e array */
struct tbx_fast_list_head *ma_array_queue(struct prio_array *array, int prio);
#define ma_array_queue(array,prio)	((array)->queue + (prio))
/** \brief Queue of priority \e prio in runqueue \e rq */
struct tbx_fast_list_head *ma_rq_queue(ma_runqueue_t * rq, int prio);
#define ma_rq_queue(rq,prio)	ma_array_queue((rq)->active, (prio))
/** \brief Whether queue \e queue is empty */
int ma_queue_empty(struct tbx_fast_list_head *queue);
#define ma_queue_empty(queue)	tbx_fast_list_empty(queue)
/** \brief First entity in queue \e queue */
marcel_entity_t *ma_queue_entry(struct tbx_fast_list_head *queue);
#define ma_queue_entry(queue)	tbx_fast_list_entry((queue)->next, marcel_entity_t, cached_entities_item)

/** \brief Iterate through the entities held in queue \e queue */
#define ma_queue_for_each_entry(e, queue) tbx_fast_list_for_each_entry(e, queue, cached_entities_item)
/** \brief Same as ma_queue_for_each_entry(), but safe version against current
 * item removal: prefetches the next entity in \e ee. */
#define ma_queue_for_each_entry_safe(e, ee, queue) tbx_fast_list_for_each_entry_safe(e, ee, queue, cached_entities_item)


/** Internal data types **/
typedef struct prio_array ma_prio_array_t;


/** Internal data structures **/
struct prio_array {
	int nr_active;
	unsigned long bitmap[MA_BITMAP_SIZE];
	struct tbx_fast_list_head queue[MA_MAX_PRIO];
};

/**
 * Locking rule: those places that want to lock multiple runqueues (such as the
 * load balancing or the thread migration code), lock acquire operations must
 * be ordered by descending level: the main runqueue first, then lower levels,
 * and on a given level, from low indices to high indices.
 */
struct ma_runqueue {
	/** \brief Holder information */
	struct ma_holder as_holder;
	/** \brief active arrays of queues */
	ma_prio_array_t active[1];

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
	/** \brief For LWP runqueue: corresponding lwp structure */
	ma_lwp_t lwp;

#ifdef MA__BUBBLES
	/** \brief General-purpose list link for bubble schedulers */
	struct tbx_fast_list_head next;
#endif
};


/** Internal global variables **/
/** \brief The main runqueue (for the whole machine) */
extern ma_runqueue_t ma_main_runqueue;
#define ma_main_runqueue (marcel_machine_level[0].rq)
extern ma_runqueue_t ma_dontsched_runqueue;


/** Public inline functions functions **/
static __tbx_inline__ void ma_array_dequeue_task(marcel_task_t * p,
						 ma_prio_array_t * array);
static __tbx_inline__ void ma_array_dequeue_entity(marcel_entity_t * p,
						   ma_prio_array_t * array);
static __tbx_inline__ void ma_array_enqueue_task(marcel_task_t * p,
						 ma_prio_array_t * array);
static __tbx_inline__ void ma_array_enqueue_entity(marcel_entity_t * p,
						   ma_prio_array_t * array);
static __tbx_inline__ void ma_array_entity_list_add(struct tbx_fast_list_head *head,
						    marcel_entity_t * e,
						    ma_prio_array_t * array,
						    ma_runqueue_t * rq);
static __tbx_inline__ void ma_array_enqueue_entity_list(struct tbx_fast_list_head *head,
							int num, int prio,
							ma_prio_array_t * array,
							ma_runqueue_t * rq);
#define ma_array_enqueue_task_list ma_array_enqueue_entity_list
extern void ma_init_rq(ma_runqueue_t * rq, const char *name);
extern void ma_init_lwp_rq(ma_runqueue_t * rq, const char *name, ma_lwp_t lwp);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __LINUX_RUNQUEUES_H__ **/
