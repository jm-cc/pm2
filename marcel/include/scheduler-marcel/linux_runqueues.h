
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

#section common
#depend "scheduler-marcel/marcel_bubble_sched.h[types]"
#include "tbx_compiler.h"

/*
 * These are the runqueue data structures:
 */

#section marcel_macros
#depend "asm/linux_types.h[macros]"
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

#define MA_MAX_USER_RT_PRIO	32
#define MA_MAX_SYS_RT_PRIO	9
#define MA_MAX_NICE		0

#define MA_SYS_RT_PRIO		MA_MAX_SYS_RT_PRIO
#define MA_MAX_RT_PRIO		(MA_SYS_RT_PRIO+1)
#define MA_RT_PRIO		(MA_MAX_RT_PRIO+MA_MAX_USER_RT_PRIO)
#define MA_DEF_PRIO		(MA_RT_PRIO+MA_MAX_NICE+1)
#define MA_BATCH_PRIO		(MA_DEF_PRIO+1)
#define MA_IDLE_PRIO		(MA_BATCH_PRIO+1)
#define MA_MAX_PRIO		(MA_IDLE_PRIO+1)

#define ma_rt_task(p)		((p)->sched.internal.prio < MA_MAX_RT_PRIO)

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

#section types
enum ma_rq_type {
	MA_DONTSCHED_RQ,
	MA_MACHINE_RQ,
#ifdef MA__LWPS
#ifdef MA__NUMA
	MA_NODE_RQ,
	MA_DIE_RQ,
	MA_CORE_RQ,
#endif
	MA_LWP_RQ,
#endif
};

/*
 * This is the main, per-CPU runqueue data structure.
 *
 * Locking rule: those places that want to lock multiple runqueues
 * (such as the load balancing or the thread migration code), lock
 * acquire operations must be ordered by ascending &runqueue.
 */
#section marcel_structures
#depend "[types]"
#depend "marcel_topology.h[types]"
#depend "marcel_topology.h[marcel_types]"
#depend "scheduler/marcel_holder.h[marcel_structures]"
struct ma_runqueue {
	struct ma_holder hold;
	unsigned long long nr_switches;
	char name[16];
	unsigned long expired_timestamp, timestamp_last_tick;
//	struct mm_struct *prev_mm;
	ma_prio_array_t *active, *expired, arrays[2];
//	int best_expired_prio, prev_cpu_load[NR_CPUS];
//#ifdef CONFIG_NUMA
//	atomic_t *node_nr_running;
//	int prev_node_load[MAX_NUMNODES];
//#endif
//	marcel_task_t *migration_thread;
//	struct list_head migration_queue;

//	ma_atomic_t nr_iowait;

#ifdef MA__LWPS
	struct ma_runqueue *father;
	int level;
	ma_cpu_set_t cpuset;
#endif
	enum ma_rq_type type;
};

#section types
typedef struct ma_runqueue ma_runqueue_t;
typedef ma_runqueue_t ma_topo_level_schedinfo;

#section marcel_macros
#define MA_DEFINE_RUNQUEUE(name) \
	TBX_SECTION(".ma.runqueues") ma_runqueue_t name

#section marcel_variables
#depend "linux_perlwp.h[marcel_macros]"
#depend "[marcel_macros]"

extern ma_runqueue_t ma_main_runqueue;
extern ma_runqueue_t ma_dontsched_runqueue;
#ifdef MA__NUMA
extern ma_runqueue_t ma_node_runqueue[MARCEL_NBMAXNODES];
extern ma_runqueue_t ma_die_runqueue[MARCEL_NBMAXDIES];
extern ma_runqueue_t ma_core_runqueue[MARCEL_NBMAXCORES];
extern ma_runqueue_t *ma_level_runqueues[];
#endif

#ifdef MA__LWPS
MA_DECLARE_PER_LWP(ma_runqueue_t, runqueue);
MA_DECLARE_PER_LWP(ma_runqueue_t, dontsched_runqueue);
#endif

#section marcel_macros
#ifdef MA__LWPS
#define ma_lwp_rq(lwp)		(&ma_per_lwp(runqueue, (lwp)))
#define ma_dontsched_rq(lwp)	(&ma_per_lwp(dontsched_runqueue, (lwp)))
#define ma_rq_covers(rq,lwp)	(MA_CPU_ISSET(LWP_NUMBER(lwp), &rq->cpuset))
#else
#define ma_lwp_rq(lwp)		(&ma_main_runqueue)
#define ma_dontsched_rq(lwp)	(&ma_dontsched_runqueue)
#define ma_rq_covers(rq,lwp)	(1)
#endif
#define ma_lwp_curr(lwp)	ma_per_lwp(current_thread, lwp)

/*
 * double_rq_lock - safely lock two runqueues
 *
 * Note this does not disable interrupts like task_rq_lock,
 * you need to do so manually before calling.
 *
 * Note: runqueues order is 
 * main_runqueue < node_runqueue < core_runqueue < lwp_runqueue
 * so that once main_runqueue locked, one can lock lwp runqueues for instance.
 */
#section marcel_functions
static __tbx_inline__ void double_rq_lock(ma_runqueue_t *rq1, ma_runqueue_t *rq2);
#section marcel_inline
static __tbx_inline__ void double_rq_lock(ma_runqueue_t *rq1, ma_runqueue_t *rq2)
{
	if (rq1 == rq2)
		ma_spin_lock(&rq1->hold.lock);
	else {
		if (rq1 < rq2) {
			ma_spin_lock(&rq1->hold.lock);
			_ma_raw_spin_lock(&rq2->hold.lock);
		} else {
			ma_spin_lock(&rq2->hold.lock);
			_ma_raw_spin_lock(&rq1->hold.lock);
		}
	}
}
#section marcel_functions
static __tbx_inline__ void double_rq_lock_softirq(ma_runqueue_t *rq1, ma_runqueue_t *rq2);
#section marcel_inline
static __tbx_inline__ void double_rq_lock_softirq(ma_runqueue_t *rq1, ma_runqueue_t *rq2)
{
	ma_local_bh_disable();

	double_rq_lock(rq1, rq2);
}

/*
 * lock_second_rq: locks another runqueue. Of course, addresses must be in
 * proper order
 */
#section marcel_functions
static __tbx_inline__ void lock_second_rq(ma_runqueue_t *rq1, ma_runqueue_t *rq2);
#section marcel_inline
static __tbx_inline__ void lock_second_rq(ma_runqueue_t *rq1, ma_runqueue_t *rq2)
{
	MA_BUG_ON(rq1 > rq2);
	if (rq1 != rq2)
		_ma_raw_spin_lock(&rq2->hold.lock);
}

/*
 * double_rq_unlock - safely unlock two runqueues
 *
 * Note this does not restore interrupts like task_rq_unlock,
 * you need to do so manually after calling.
 */
#section marcel_functions
static __tbx_inline__ void double_rq_unlock(ma_runqueue_t *rq1, ma_runqueue_t *rq2);
#section marcel_inline
static __tbx_inline__ void double_rq_unlock(ma_runqueue_t *rq1, ma_runqueue_t *rq2)
{
	if (rq1 != rq2)
		_ma_raw_spin_unlock(&rq2->hold.lock);
	ma_spin_unlock(&rq1->hold.lock);
}
#section marcel_functions
static __tbx_inline__ void double_rq_unlock_softirq(ma_runqueue_t *rq1, ma_runqueue_t *rq2);
#section marcel_inline
static __tbx_inline__ void double_rq_unlock_softirq(ma_runqueue_t *rq1, ma_runqueue_t *rq2)
{
	double_rq_unlock(rq1, rq2);
	ma_local_bh_enable();
}

/*
 * Adding/removing a task to/from a priority array:
 */
#section marcel_functions
static __tbx_inline__ void ma_rq_dequeue_task(marcel_task_t *p, ma_prio_array_t *array);
static __tbx_inline__ void ma_rq_dequeue_entity(marcel_entity_t *p, ma_prio_array_t *array);
#section marcel_inline
static __tbx_inline__ void ma_rq_dequeue_entity(marcel_entity_t *e, ma_prio_array_t *array)
{
	sched_debug("dequeueing %p (prio %d) from %p\n",e,e->prio,array);
	array->nr_active--;
	list_del(&e->run_list);
	if (list_empty(array->queue + e->prio)) {
		sched_debug("array %p (prio %d) empty\n",array, e->prio);
		__ma_clear_bit(e->prio, array->bitmap);
	}
	MA_BUG_ON(!e->data);
	e->data = NULL;
}
static __tbx_inline__ void ma_rq_dequeue_task(marcel_task_t *p, ma_prio_array_t *array)
{
	sched_debug("dequeueing %ld:%s (prio %d) from %p\n",p->number,p->name,p->sched.internal.prio,array);
	ma_rq_dequeue_entity(&p->sched.internal, array);
}

#section marcel_functions
static __tbx_inline__ void ma_rq_enqueue_task(marcel_task_t *p, ma_prio_array_t *array);
static __tbx_inline__ void ma_rq_enqueue_entity(marcel_entity_t *p, ma_prio_array_t *array);
#section marcel_inline
static __tbx_inline__ void ma_rq_enqueue_entity(marcel_entity_t *e, ma_prio_array_t *array)
{
	sched_debug("enqueueing %p (prio %d) in %p\n",e,e->prio,array);
	list_add_tail(&e->run_list, array->queue + e->prio);
	__ma_set_bit(e->prio, array->bitmap);
	array->nr_active++;
	MA_BUG_ON(e->data);
	e->data = array;
}
static __tbx_inline__ void ma_rq_enqueue_task(marcel_task_t *p, ma_prio_array_t *array)
{
	sched_debug("enqueueing %ld:%s (prio %d) in %p\n",p->number,p->name,p->sched.internal.prio,array);
	ma_rq_enqueue_entity(&p->sched.internal,array);
}

/*
 * Switch the active and expired arrays.
 */
#section marcel_functions
static __tbx_inline__ void rq_arrays_switch(ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void rq_arrays_switch(ma_runqueue_t *rq)
{
	ma_prio_array_t *array = rq->active;

	rq->active = rq->expired;
	rq->expired = array;
	array = rq->active;
	rq->expired_timestamp = 0;
//	rq->best_expired_prio = MA_MAX_PRIO;
}

/*
 * initialize runqueue
 */
#section marcel_functions
extern void init_rq(ma_runqueue_t *rq, char *name, enum ma_rq_type type);

