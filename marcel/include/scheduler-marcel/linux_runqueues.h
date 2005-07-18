
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
#depend "pm2_list.h"
#depend "marcel_descr.h[types]"
#depend "linux_spinlock.h[types]"
#depend "asm/linux_atomic.h[types]"
typedef struct prio_array ma_prio_array_t;

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
#depend "marcel_topology.h[types]"
#depend "marcel_topology.h[marcel_types]"
struct ma_runqueue {
	unsigned long long nr_switches;
	char name[16];
	ma_spinlock_t lock;
	unsigned long nr_running, expired_timestamp, nr_uninterruptible,
		timestamp_last_tick;
	struct mm_struct *prev_mm;
	ma_prio_array_t *active, *expired, arrays[2];
//	int best_expired_prio, prev_cpu_load[NR_CPUS];
#ifdef CONFIG_NUMA
	atomic_t *node_nr_running;
	int prev_node_load[MAX_NUMNODES];
#endif
	marcel_task_t *migration_thread;
	struct list_head migration_queue;

	ma_atomic_t nr_iowait;

#ifdef MA__LWPS
	struct ma_runqueue *father;
	int level;
#endif
	enum ma_rq_type type;

#ifdef MA__LWPS
	ma_cpu_set_t cpuset;
#endif
};

#section marcel_types
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
// ceci n'a plus de sens:
//#define task_rq(p)		lwp_rq(ma_task_lwp(p))
#define ma_task_cur_rq(p)	((p)->sched.internal.cur_rq)
#define ma_this_rq()		(ma_task_cur_rq(MARCEL_SELF))
#define ma_task_init_rq(p)	((p)->sched.internal.init_rq)
#define ma_prev_rq()		(__ma_get_lwp_var(prev_rq))
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

#section marcel_functions
#ifdef CONFIG_NUMA
static __tbx_inline__ void nr_running_inc(ma_runqueue_t *rq);
static __tbx_inline__ void nr_running_dec(ma_runqueue_t *rq);
#endif

#section marcel_inline
#ifdef CONFIG_NUMA
static __tbx_inline__ void nr_running_inc(ma_runqueue_t *rq)
{
	atomic_inc(rq->node_nr_running);
	rq->nr_running++;
}

static __tbx_inline__ void nr_running_dec(ma_runqueue_t *rq)
{
	atomic_dec(rq->node_nr_running);
	rq->nr_running--;
}

#else /* !CONFIG_NUMA */

# define nr_running_inc(rq)    do { (rq)->nr_running++; } while (0)
# define nr_running_dec(rq)    do { (rq)->nr_running--; } while (0)

#endif /* CONFIG_NUMA */

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
		ma_spin_lock(&rq1->lock);
	else {
		if (rq1 < rq2) {
			ma_spin_lock(&rq1->lock);
			_ma_raw_spin_lock(&rq2->lock);
		} else {
			ma_spin_lock(&rq2->lock);
			_ma_raw_spin_lock(&rq1->lock);
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
		_ma_raw_spin_lock(&rq2->lock);
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
		_ma_raw_spin_unlock(&rq2->lock);
	ma_spin_unlock(&rq1->lock);
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
 * task_rq_lock - lock the runqueue a given task resides on and disable
 * interrupts.  Note the ordering: we can safely lookup the task_rq without
 * explicitly disabling preemption.
 */
#section marcel_functions
static __tbx_inline__ ma_runqueue_t *entity_rq(marcel_bubble_entity_t *e);
#define task_rq(t) entity_rq(&(t)->sched.internal)
#section marcel_inline
static __tbx_inline__ ma_runqueue_t *entity_rq(marcel_bubble_entity_t *e)
{
	ma_runqueue_t *rq;
	if ((rq = e->cur_rq))
		return rq;
	sched_debug("using default queue for %p\n",e);
	return e->init_rq;

}

#section marcel_functions
static inline ma_runqueue_t *entity_rq_lock(marcel_bubble_entity_t *e);
#define task_rq_lock(t) entity_rq_lock(&(t)->sched.internal)
#section marcel_inline
static inline ma_runqueue_t *entity_rq_lock(marcel_bubble_entity_t *e)
{
	ma_runqueue_t *rq;

repeat_lock_task:
	//local_irq_save(*flags);
	ma_local_bh_disable();
	rq = entity_rq(e);
	sched_debug("task_rq_locking(%s)\n",rq->name);
	ma_spin_lock(&rq->lock);
	if (tbx_unlikely(rq != entity_rq(e))) {
		sched_debug("task_rq_unlocking(%s)\n",rq->name);
		ma_spin_unlock_softirq(&rq->lock);
		goto repeat_lock_task;
	}
	sched_debug("task_rq_locked(%s)\n",rq->name);
	return rq;
}

#section marcel_functions
static inline ma_runqueue_t *task_rq_rq_lock(marcel_task_t *p, ma_runqueue_t *rq);
#section marcel_inline
static inline ma_runqueue_t *task_rq_rq_lock(marcel_task_t *p, ma_runqueue_t *rq1)
{
	ma_runqueue_t *rq2;

repeat_lock_task:
	//local_irq_save(*flags);
	ma_local_bh_disable();
	rq2 = task_rq(p);
	sched_debug("task_rq_rq_locking(%p,%p)\n",rq1,rq2);
	if (rq2==&ma_main_runqueue)
	  sched_debug("main queue\n");
	double_rq_lock(rq1,rq2);
	if (tbx_unlikely(rq2 != task_rq(p))) {
		sched_debug("rask_rq_unlocking(%p,%p)\n",rq1,rq2);
		double_rq_unlock_softirq(rq1,rq2);
		goto repeat_lock_task;
	}
	sched_debug("task_rq_locked(%p,%p)\n",rq1,rq2);
	return rq2;
}

#section marcel_functions
static __tbx_inline__ void task_rq_unlock(ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void task_rq_unlock(ma_runqueue_t *rq)
{
	sched_debug("task_rq_unlock(%s)\n",rq->name);
	ma_spin_unlock_softirq(&rq->lock);
}

/*
 * rq_lock - lock a given runqueue and disable interrupts.
 */
#section marcel_functions
static __tbx_inline__ ma_runqueue_t *this_rq_lock(void);
#section marcel_inline
static __tbx_inline__ ma_runqueue_t *this_rq_lock(void)
{
	ma_runqueue_t *rq;

	ma_local_bh_disable();
	rq = ma_this_rq();
	ma_spin_lock(&rq->lock);

	return rq;
}

#section marcel_functions
static __tbx_inline__ void rq_unlock(ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void rq_unlock(ma_runqueue_t *rq)
{
	ma_spin_unlock_softirq(&rq->lock);
}

/*
 * Adding/removing a task to/from a priority array:
 */
#section marcel_functions
static __tbx_inline__ void dequeue_task(marcel_task_t *p, ma_prio_array_t *array);
static __tbx_inline__ void dequeue_entity(marcel_bubble_entity_t *p, ma_prio_array_t *array);
#section marcel_inline
static __tbx_inline__ void dequeue_entity(marcel_bubble_entity_t *e, ma_prio_array_t *array)
{
	sched_debug("dequeueing %p (prio %d) from %p\n",e,e->prio,array);
	array->nr_active--;
	list_del(&e->run_list);
	if (list_empty(array->queue + e->prio)) {
		sched_debug("array %p (prio %d) empty\n",array, e->prio);
		__ma_clear_bit(e->prio, array->bitmap);
	}
	MA_BUG_ON(!e->array);
	e->array = NULL;
}
static __tbx_inline__ void dequeue_task(marcel_task_t *p, ma_prio_array_t *array)
{
	sched_debug("dequeueing %ld:%s (prio %d) from %p\n",p->number,p->name,p->sched.internal.prio,array);
	dequeue_entity(&p->sched.internal, array);
}

#section marcel_functions
static __tbx_inline__ void enqueue_task(marcel_task_t *p, ma_prio_array_t *array);
static __tbx_inline__ void enqueue_entity(marcel_bubble_entity_t *p, ma_prio_array_t *array);
#section marcel_inline
static __tbx_inline__ void enqueue_entity(marcel_bubble_entity_t *e, ma_prio_array_t *array)
{
	sched_debug("enqueueing %p (prio %d) in %p\n",e,e->prio,array);
	list_add_tail(&e->run_list, array->queue + e->prio);
	__ma_set_bit(e->prio, array->bitmap);
	array->nr_active++;
	MA_BUG_ON(e->array);
	e->array = array;
}
static __tbx_inline__ void enqueue_task(marcel_task_t *p, ma_prio_array_t *array)
{
	sched_debug("enqueueing %ld:%s (prio %d) in %p\n",p->number,p->name,p->sched.internal.prio,array);
	enqueue_entity(&p->sched.internal,array);
}

/*
 * activate_running_task - move a task to the runqueue, but don't enqueue it
 * because it is already running (p == MARCEL_SELF)
 */
#section marcel_functions
static __tbx_inline__ void activate_running_task(marcel_task_t *p, ma_runqueue_t *rq);
#section marcel_inline             
static __tbx_inline__ void activate_running_task(marcel_task_t *p, ma_runqueue_t *rq)
{
	MA_BUG_ON(p->sched.internal.cur_rq);
	p->sched.internal.cur_rq = rq;
	nr_running_inc(rq);
}

/*
 * __activate_task - move a task to the runqueue.
 */
#section marcel_functions
static __tbx_inline__ void __activate_task(marcel_task_t *p, ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void __activate_task(marcel_task_t *p, ma_runqueue_t *rq)
{
	activate_running_task(p,rq);
	enqueue_task(p, rq->active);
}

/*
 * activate_task - move a task to the runqueue and do priority recalculation
 *
 * Update all the scheduling statistics stuff. (sleep average
 * calculation, priority modifiers, etc.)
 */
#section marcel_functions
static __tbx_inline__ void activate_task(marcel_task_t *p, ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void activate_task(marcel_task_t *p, ma_runqueue_t *rq)
{
//	unsigned long long now = sched_clock();

//	recalc_task_prio(p, now);

	/*
	 * This checks to make sure it's not an uninterruptible task
	 * that is now waking up.
	 */
//	if (!p->activated){
		/*
		 * Tasks which were woken up by interrupts (ie. hw events)
		 * are most likely of interactive nature. So we give them
		 * the credit of extending their sleep time to the period
		 * of time they spend on the runqueue, waiting for execution
		 * on a CPU, first time around:
		 */
//		if (in_interrupt())
//			p->activated = 2;
//		else
		/*
		 * Normal first-time wakeups get a credit too for on-runqueue
		 * time, but it will be weighted down:
		 */
//			p->activated = 1;
//		}
//	p->timestamp = now;

	__activate_task(p, rq);
}

/*
 * activate entity
 */
#section marcel_functions
static __tbx_inline__ void activate_entity(marcel_bubble_entity_t *e, ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void activate_entity(marcel_bubble_entity_t *e, ma_runqueue_t *rq)
{
	MA_BUG_ON(e->cur_rq);
	e->cur_rq = rq;
	nr_running_inc(rq);
	enqueue_entity(e, rq->active);
}

/*
 * deactivate_running_task - remove a task from the runqueue, without
 * dequeueing it, since it is already running.
 */
#section marcel_functions
static __tbx_inline__ void deactivate_running_task(marcel_task_t *p, ma_runqueue_t *rq);
#section marcel_inline               
static __tbx_inline__ void deactivate_running_task(marcel_task_t *p, ma_runqueue_t *rq)
{
	nr_running_dec(rq);
	if (p->sched.state == MA_TASK_UNINTERRUPTIBLE)
		rq->nr_uninterruptible++;
	MA_BUG_ON(!p->sched.internal.cur_rq);
	p->sched.internal.cur_rq = NULL;
}

/*
 * deactivate_task - remove a task from the runqueue.
 */
#section marcel_functions
static __tbx_inline__ void deactivate_task(marcel_task_t *p, ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void deactivate_task(marcel_task_t *p, ma_runqueue_t *rq)
{
	dequeue_task(p, p->sched.internal.array);
	deactivate_running_task(p,rq);
}

/*
 * deactivate entity
 */
#section marcel_functions
static __tbx_inline__ void deactivate_entity(marcel_bubble_entity_t *e, ma_runqueue_t *rq);
#section marcel_inline
static __tbx_inline__ void deactivate_entity(marcel_bubble_entity_t *e, ma_runqueue_t *rq)
{
	dequeue_entity(e, rq->active);
	nr_running_dec(rq);
	MA_BUG_ON(!e->cur_rq);
	e->cur_rq = NULL;
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

