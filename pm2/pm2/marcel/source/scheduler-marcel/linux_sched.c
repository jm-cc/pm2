
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

/*
 * Similar to:
 *  kernel/sched.c
 *
 *  Kernel scheduler and related syscalls
 *
 *  Copyright (C) 1991-2002  Linus Torvalds
 *
 *  1996-12-23  Modified by Dave Grothe to fix bugs in semaphores and
 *		make semaphores SMP safe
 *  1998-11-19	Implemented schedule_timeout() and related stuff
 *		by Andrea Arcangeli
 *  2002-01-04	New ultra-scalable O(1) scheduler by Ingo Molnar:
 *		hybrid priority-list and round-robin design with
 *		an array-switch method of distributing timeslices
 *		and per-CPU runqueues.  Cleanups and useful suggestions
 *		by Davide Libenzi, preemptible kernel bits by Robert Love.
 *  2003-09-03	Interactivity tuning by Con Kolivas.
 */

#include "marcel.h"

#ifdef CONFIG_NUMA
#define cpu_to_node_mask(cpu) node_to_cpumask(cpu_to_node(cpu))
#else
#define cpu_to_node_mask(cpu) (cpu_online_map)
#endif

/*
 * Convert user-nice values [ -20 ... 0 ... 19 ]
 * to static priority [ MAX_RT_PRIO..MAX_PRIO-1 ],
 * and back.
 */
#define NICE_TO_PRIO(nice)	(MA_MAX_RT_PRIO + (nice) + 20)
#define PRIO_TO_NICE(prio)	((prio) - MA_MAX_RT_PRIO - 20)
#define TASK_NICE(p)		PRIO_TO_NICE((p)->static_prio)

/*
 * 'User priority' is the nice value converted to something we
 * can work with better when scaling various scheduler parameters,
 * it's a [ 0 ... 39 ] range.
 */
#define USER_PRIO(p)		((p)-MA_MAX_RT_PRIO)
#define TASK_USER_PRIO(p)	USER_PRIO((p)->static_prio)
#define MAX_USER_PRIO		(USER_PRIO(MA_MAX_PRIO))
#define AVG_TIMESLICE	(MIN_TIMESLICE + ((MAX_TIMESLICE - MIN_TIMESLICE) *\
			(MA_MAX_PRIO-1-NICE_TO_PRIO(0))/(MAX_USER_PRIO - 1)))

/*
 * Some helpers for converting nanosecond timing to jiffy resolution
 */
#define NS_TO_JIFFIES(TIME)	((TIME) / (1000000000 / HZ))
#define JIFFIES_TO_NS(TIME)	((TIME) * (1000000000 / HZ))

/*
 * These are the 'tuning knobs' of the scheduler:
 *
 * Minimum timeslice is 10 msecs, default timeslice is 100 msecs,
 * maximum timeslice is 200 msecs. Timeslices get refilled after
 * they expire.
 */
#define MIN_TIMESLICE		( 10 * HZ / 1000)
#define MAX_TIMESLICE		(200 * HZ / 1000)
#define ON_RUNQUEUE_WEIGHT	30
#define CHILD_PENALTY		95
#define PARENT_PENALTY		100
#define EXIT_WEIGHT		3
#define PRIO_BONUS_RATIO	25
#define MAX_BONUS		(MAX_USER_PRIO * PRIO_BONUS_RATIO / 100)
#define INTERACTIVE_DELTA	2
#define MAX_SLEEP_AVG		(AVG_TIMESLICE * MAX_BONUS)
#define STARVATION_LIMIT	(MAX_SLEEP_AVG)
#define NS_MAX_SLEEP_AVG	(JIFFIES_TO_NS(MAX_SLEEP_AVG))
#define NODE_THRESHOLD		125
#define CREDIT_LIMIT		100

/*
 * If a task is 'interactive' then we reinsert it in the active
 * array after it has expired its current timeslice. (it will not
 * continue to run immediately, it will still roundrobin with
 * other interactive tasks.)
 *
 * This part scales the interactivity limit depending on niceness.
 *
 * We scale it linearly, offset by the INTERACTIVE_DELTA delta.
 * Here are a few examples of different nice levels:
 *
 *  TASK_INTERACTIVE(-20): [1,1,1,1,1,1,1,1,1,0,0]
 *  TASK_INTERACTIVE(-10): [1,1,1,1,1,1,1,0,0,0,0]
 *  TASK_INTERACTIVE(  0): [1,1,1,1,0,0,0,0,0,0,0]
 *  TASK_INTERACTIVE( 10): [1,1,0,0,0,0,0,0,0,0,0]
 *  TASK_INTERACTIVE( 19): [0,0,0,0,0,0,0,0,0,0,0]
 *
 * (the X axis represents the possible -5 ... 0 ... +5 dynamic
 *  priority range a task can explore, a value of '1' means the
 *  task is rated interactive.)
 *
 * Ie. nice +19 tasks can never get 'interactive' enough to be
 * reinserted into the active array. And only heavily CPU-hog nice -20
 * tasks will be expired. Default nice 0 tasks are somewhere between,
 * it takes some effort for them to get interactive, but it's not
 * too hard.
 */

#define CURRENT_BONUS(p) \
	(NS_TO_JIFFIES((p)->sleep_avg) * MAX_BONUS / \
		MAX_SLEEP_AVG)

#ifdef CONFIG_SMP
#define TIMESLICE_GRANULARITY(p)	(MIN_TIMESLICE * \
		(1 << (((MAX_BONUS - CURRENT_BONUS(p)) ? : 1) - 1)) * \
			num_online_cpus())
#else
#define TIMESLICE_GRANULARITY(p)	(MIN_TIMESLICE * \
		(1 << (((MAX_BONUS - CURRENT_BONUS(p)) ? : 1) - 1)))
#endif

#define SCALE(v1,v1_max,v2_max) \
	(v1) * (v2_max) / (v1_max)

#define DELTA(p) \
	(SCALE(TASK_NICE(p), 40, MAX_USER_PRIO*PRIO_BONUS_RATIO/100) + \
		INTERACTIVE_DELTA)

#define TASK_INTERACTIVE(p) \
	((p)->prio <= (p)->static_prio - DELTA(p))

#define INTERACTIVE_SLEEP(p) \
	(JIFFIES_TO_NS(MAX_SLEEP_AVG * \
		(MAX_BONUS / 2 + DELTA((p)) + 1) / MAX_BONUS - 1))

#define HIGH_CREDIT(p) \
	((p)->interactive_credit > CREDIT_LIMIT)

#define LOW_CREDIT(p) \
	((p)->interactive_credit < -CREDIT_LIMIT)

#define TASK_PREEMPTS_TASK(p, q) \
	((p)->sched.internal.prio < (q)->sched.internal.prio)
#define TASK_PREEMPTS_CURR(p, lwp) \
	TASK_PREEMPTS_TASK((p), ma_per_lwp(current_thread, (lwp)))


/*
 * BASE_TIMESLICE scales user-nice values [ -20 ... 19 ]
 * to time slice values.
 *
 * The higher a thread's priority, the bigger timeslices
 * it gets during one round of execution. But even the lowest
 * priority thread gets MIN_TIMESLICE worth of execution time.
 *
 * task_timeslice() is the interface that is used by the scheduler.
 */

#define BASE_TIMESLICE(p) (MIN_TIMESLICE + \
	((MAX_TIMESLICE - MIN_TIMESLICE) * \
	 (MA_MAX_PRIO-1 - (p)->static_prio)/(MAX_USER_PRIO - 1)))

//static inline unsigned int task_timeslice(task_t *p)
//{
//	return BASE_TIMESLICE(p);
//}

MA_DEFINE_PER_LWP(ma_runqueue_t *, prev_rq)=NULL;
MA_DEFINE_PER_LWP(marcel_task_t *, current_thread)=NULL;
MA_DEFINE_PER_LWP(struct ma_lwp_usage_stat, lwp_usage);

/*
 * Default context-switch locking:
 */
#ifndef ma_prepare_arch_switch
# define ma_prepare_arch_switch(rq, next)	do { } while(0)
# define ma_finish_arch_switch(rq, next)	ma_spin_unlock_softirq(&(rq)->lock)
# define ma_task_running(rq, p)		((p)->sched.internal.cur_rq && !(p)->sched.internal.array) //((rq)->curr == (p))
#endif

/*
 * effective_prio - return the priority that is based on the static
 * priority but is modified by bonuses/penalties.
 *
 * We scale the actual sleep average [0 .... MAX_SLEEP_AVG]
 * into the -5 ... 0 ... +5 bonus/penalty range.
 *
 * We use 25% of the full 0...39 priority range so that:
 *
 * 1) nice +19 interactive tasks do not preempt nice 0 CPU hogs.
 * 2) nice -20 CPU hogs do not get preempted by nice 0 tasks.
 *
 * Both properties are important to certain workloads.
 */
#if 0
static int effective_prio(marcel_task_t *p)
{
	int bonus, prio;

	if (rt_task(p))
		return p->sched.internal.prio;

	bonus = CURRENT_BONUS(p) - MAX_BONUS / 2;

	prio = p->static_prio - bonus;
	if (prio < MA_MAX_RT_PRIO)
		prio = MA_MAX_RT_PRIO;
	if (prio > MA_MAX_PRIO-1)
		prio = MA_MAX_PRIO-1;
	return prio;
}
#endif

#if 0
static void recalc_task_prio(marcel_task_t *p, unsigned long long now)
{
	unsigned long long __sleep_time = now - p->timestamp;
	unsigned long sleep_time;

	if (__sleep_time > NS_MAX_SLEEP_AVG)
		sleep_time = NS_MAX_SLEEP_AVG;
	else
		sleep_time = (unsigned long)__sleep_time;

	if (likely(sleep_time > 0)) {
		/*
		 * User tasks that sleep a long time are categorised as
		 * idle and will get just interactive status to stay active &
		 * prevent them suddenly becoming cpu hogs and starving
		 * other processes.
		 */
		if (p->mm && p->activated != -1 &&
			sleep_time > INTERACTIVE_SLEEP(p)){
				p->sleep_avg = JIFFIES_TO_NS(MAX_SLEEP_AVG -
						AVG_TIMESLICE);
				if (!HIGH_CREDIT(p))
					p->interactive_credit++;
		} else {
			/*
			 * The lower the sleep avg a task has the more
			 * rapidly it will rise with sleep time.
			 */
			sleep_time *= (MAX_BONUS - CURRENT_BONUS(p)) ? : 1;

			/*
			 * Tasks with low interactive_credit are limited to
			 * one timeslice worth of sleep avg bonus.
			 */
			if (LOW_CREDIT(p) &&
				sleep_time > JIFFIES_TO_NS(task_timeslice(p)))
					sleep_time =
						JIFFIES_TO_NS(task_timeslice(p));

			/*
			 * Non high_credit tasks waking from uninterruptible
			 * sleep are limited in their sleep_avg rise as they
			 * are likely to be cpu hogs waiting on I/O
			 */
			if (p->activated == -1 && !HIGH_CREDIT(p) && p->mm){
				if (p->sleep_avg >= INTERACTIVE_SLEEP(p))
					sleep_time = 0;
				else if (p->sleep_avg + sleep_time >=
					INTERACTIVE_SLEEP(p)){
						p->sleep_avg =
							INTERACTIVE_SLEEP(p);
						sleep_time = 0;
					}
			}

			/*
			 * This code gives a bonus to interactive tasks.
			 *
			 * The boost works by updating the 'average sleep time'
			 * value here, based on ->timestamp. The more time a task
			 * spends sleeping, the higher the average gets - and the
			 * higher the priority boost gets as well.
			 */
			p->sleep_avg += sleep_time;

			if (p->sleep_avg > NS_MAX_SLEEP_AVG){
				p->sleep_avg = NS_MAX_SLEEP_AVG;
				if (!HIGH_CREDIT(p))
					p->interactive_credit++;
			}
		}
	}

	p->prio = effective_prio(p);
}
#endif /* 0 */

/*
 * resched_task - mark a task 'to be rescheduled now'.
 *
 * On UP this means the setting of the need_resched flag, on SMP it
 * might also involve a LWP killing to trigger the scheduler on
 * the target LWP.
 */
static inline void resched_task(marcel_task_t *p, ma_lwp_t lwp)
{
#ifdef MA__LWPS
	int need_resched, nrpolling;

	ma_preempt_disable();
	/* minimise the chance of sending an interrupt to poll_idle() */
	nrpolling = ma_test_tsk_thread_flag(p,TIF_POLLING_NRFLAG);
	need_resched = ma_test_and_set_tsk_thread_flag(p,TIF_NEED_RESCHED);
	nrpolling |= ma_test_tsk_thread_flag(p,TIF_POLLING_NRFLAG);

	if (!need_resched && !nrpolling && lwp != LWP_SELF)
		marcel_kthread_kill(lwp->pid, MARCEL_TIMER_SIGNAL);
	ma_preempt_enable();
#else
	ma_set_tsk_need_resched(p);
#endif
}

/**
 * task_curr - is this task currently executing on a CPU?
 * @p: the task in question.
 */
inline int task_curr(marcel_task_t *p)
{
	return ma_lwp_curr(ma_task_lwp(p)) == p;
}

#if 0
#ifdef CONFIG_SMP
typedef struct {
	struct list_head list;
	task_t *task;
	struct completion done;
} migration_req_t;

/*
 * The task's runqueue lock must be held, and the new mask must be valid.
 * Returns true if you have to wait for migration thread.
 */
static int __set_cpus_allowed(task_t *p, cpumask_t new_mask,
				migration_req_t *req)
{
	ma_runqueue_t *rq = ma_task_rq(p);

	p->cpus_allowed = new_mask;
	/*
	 * Can the task run on the task's current CPU? If not then
	 * migrate the thread off to a proper CPU.
	 */
	if (cpu_isset(task_cpu(p), new_mask))
		return 0;

	/*
	 * If the task is not on a runqueue (and not running), then
	 * it is sufficient to simply update the task's cpu field.
	 */
	if (!p->array && !ma_task_running(rq, p)) {
		set_task_cpu(p, any_online_cpu(p->cpus_allowed));
		return 0;
	}

	init_completion(&req->done);
	req->task = p;
	list_add(&req->list, &rq->migration_queue);
	return 1;
}

/*
 * wait_task_inactive - wait for a thread to unschedule.
 *
 * The caller must ensure that the task *will* unschedule sometime soon,
 * else this function might spin for a *long* time. This function can't
 * be called with interrupts off, or it may introduce deadlock with
 * smp_call_function() if an IPI is sent by the same process we are
 * waiting to become inactive.
 */
void wait_task_inactive(marcel_task_t * p)
{
	unsigned long flags;
	ma_runqueue_t *rq;
	int preempted;

repeat:
	rq = task_rq_lock(p, &flags);
	/* Must be off runqueue entirely, not preempted. */
	if (unlikely(p->array)) {
		/* If it's preempted, we yield.  It could be a while. */
		preempted = !ma_task_running(rq, p);
		task_rq_unlock(rq, &flags);
		cpu_relax();
		if (preempted)
			yield();
		goto repeat;
	}
	task_rq_unlock(rq, &flags);
}

/***
 * kick_process - kick a running thread to enter/exit the kernel
 * @p: the to-be-kicked thread
 *
 * Cause a process which is running on another CPU to enter
 * kernel-mode, without any delay. (to get signals handled.)
 */
void kick_process(marcel_task_t *p)
{
	int cpu;

	preempt_disable();
	cpu = task_cpu(p);
	if ((cpu != smp_processor_id()) && task_curr(p))
		smp_send_reschedule(cpu);
	preempt_enable();
}

EXPORT_SYMBOL_GPL(kick_process);


#endif
#endif /* 0 */

/***
 * try_to_wake_up - wake up a thread
 * @p: the to-be-woken-up thread
 * @state: the mask of task states that can be woken
 * @sync: do a synchronous wakeup?
 *
 * Put it on the run-queue if it's not already there. The "current"
 * thread is always on the run-queue (except when the actual
 * re-schedule is in progress), and as such you're allowed to do
 * the simpler "current->state = TASK_RUNNING" to mark yourself
 * runnable without the overhead of this.
 *
 * returns failure only if the task is already active.
 */
static int try_to_wake_up(marcel_task_t * p, unsigned int state, int sync)
{
	int success = 0;
	long old_state;
	ma_runqueue_t *rq;

repeat_lock_task:
	rq = task_rq_lock(p);
	old_state = p->sched.state;
	if (old_state & state) {
		if (!ma_task_cur_rq(p)) { /* not running or runnable */
			/*
			 * Fast-migrate the task if it's not running or runnable
			 * currently. Do not violate hard affinity.
			 */
			if (tbx_unlikely(sync && !ma_task_running(rq, p) &&
				(ma_task_lwp(p) != LWP_SELF)
				&& lwp_isset(LWP_NUMBER(LWP_SELF),
					  p->sched.lwps_allowed)
				&& !lwp_is_offline(LWP_SELF)
				)) {

				//deactivate_task(p,rq);
				//activate_task(p,ma_lwp_rq(LWP_SELF));

				//réalisé par ma_schedule()
				//ma_set_task_lwp(p, LWP_SELF);

				task_rq_unlock(rq);
				goto repeat_lock_task;
			}
			if (old_state == MA_TASK_UNINTERRUPTIBLE){
				rq->nr_uninterruptible--;
				/*
				 * Tasks on involuntary sleep don't earn
				 * sleep_avg beyond just interactive state.
				 */
//				p->activated = -1;
			}

			activate_task(p, rq);
			/*
			 * Sync wakeups (i.e. those types of wakeups where the waker
			 * has indicated that it will leave the CPU in short order)
			 * don't trigger a preemption, if the woken up task will run on
			 * this cpu. (in this case the 'I will reschedule' promise of
			 * the waker guarantees that the freshly woken up task is going
			 * to be considered on this CPU.)
			 */
			if (ma_rq_covers(rq, LWP_SELF) && TASK_PREEMPTS_TASK(p, MARCEL_SELF)) {
				/* we can avoid remote reschedule by switching to it */
				if (!sync) /* only if we won't for sure yield() soon */
					resched_task(MARCEL_SELF, LWP_SELF);
			} else {
				marcel_lwp_t *lwp;
				for_each_lwp_from_begin(lwp, LWP_SELF)
					/* TODO: regarder celui qui préeempte le plus ? (histoire d'aller taper dans Idle plutôt) */
					if (ma_rq_covers(rq, lwp) && TASK_PREEMPTS_CURR(p, lwp)) {
						resched_task(ma_per_lwp(current_thread, lwp), lwp);
						break;
					}
				for_each_lwp_from_end();
			}
			success = 1;
		}
		p->sched.state = MA_TASK_RUNNING;
	}
	task_rq_unlock(rq);

	return success;
}

int fastcall ma_wake_up_thread(marcel_task_t * p)
{
	return try_to_wake_up(p, MA_TASK_STOPPED | 
			      MA_TASK_INTERRUPTIBLE |
			      MA_TASK_UNINTERRUPTIBLE, 0);
}

MARCEL_INT(ma_wake_up_thread);

int fastcall ma_wake_up_state(marcel_task_t *p, unsigned int state)
{
	return try_to_wake_up(p, state, 0);
}

/*
 * Perform scheduler related setup for a newly forked process p.
 * p is forked by current.
 */
#if 0 /* done in marcel_sched_init_marcel_thread() */
void fastcall ma_sched_fork(marcel_task_t *p)
{
	/*
	 * We mark the process as running here, but have not actually
	 * inserted it onto the runqueue yet. This guarantees that
	 * nobody will actually run it, and a signal or other external
	 * event cannot wake it up and insert it on the runqueue either.
	 */
	p->state = TASK_RUNNING;
	INIT_LIST_HEAD(&p->run_list);
	p->array = NULL;
	spin_lock_init(&p->switch_lock);
#ifdef CONFIG_PREEMPT
	/*
	 * During context-switch we hold precisely one spinlock, which
	 * schedule_tail drops. (in the common case it's ma_this_rq()->lock,
	 * but it also can be p->switch_lock.) So we compensate with a count
	 * of 1. Also, we want to start with kernel preemption disabled.
	 */
	p->thread_info->preempt_count = 1;
#endif
}
#endif /* O */

/*
 * wake_up_forked_process - wake up a freshly forked process.
 *
 * This function will do some initial scheduler statistics housekeeping
 * that must be done for every newly created process.
 */
void fastcall ma_wake_up_created_thread(marcel_task_t * p)
{
	ma_runqueue_t *rq = ma_task_init_rq(p);
	LOG_IN();

	MA_BUG_ON(!rq);

	ma_spin_lock_softirq(&rq->lock);

	MA_BUG_ON(p->sched.state != MA_TASK_RUNNING);

	p->sched.internal.timestamp = marcel_clock();
	/*
	 * We decrease the sleep average of forking parents
	 * and children as well, to keep max-interactive tasks
	 * from forking tasks that are max-interactive.
	 */
//	current->sleep_avg = JIFFIES_TO_NS(CURRENT_BONUS(current) *
//		PARENT_PENALTY / 100 * MAX_SLEEP_AVG / MAX_BONUS);

//	p->sleep_avg = JIFFIES_TO_NS(CURRENT_BONUS(p) *
//		CHILD_PENALTY / 100 * MAX_SLEEP_AVG / MAX_BONUS);

//	p->interactive_credit = 0;

//	p->prio = effective_prio(p);
//	ma_set_task_lwp(p, LWP_SELF);

	/* il est possible de démarrer sur une autre rq que celle de SELF,
	 * on ne peut donc pas profiter de ses valeurs */
//	if (tbx_unlikely(!MARCEL_SELF->sched.internal.array))
	if (!p->sched.internal.array)
		__activate_task(p, rq);
//	else {
//		p->sched.internal.prio = MARCEL_SELF->sched.internal.prio;
//		list_add_tail(&p->sched.internal.run_list,
//			      &MARCEL_SELF->sched.internal.run_list);
//		p->sched.internal.array = MARCEL_SELF->sched.internal.array;
//		p->sched.internal.array->nr_active++;
//#ifdef MA__LWPS
//		p->sched.internal.cur_rq = rq;
//#endif
//		nr_running_inc(rq);
//	}
	ma_spin_unlock_softirq(&rq->lock);
	// on donne la main aussitôt, bien souvent le meilleur choix
	if (!ma_in_atomic())
		ma_schedule();
	LOG_OUT();
}

int ma_sched_change_prio(marcel_t t, int prio) {
	ma_runqueue_t *rq;
	ma_prio_array_t *array;
	LOG_IN();
	MA_BUG_ON(prio < 0 || prio >= MA_MAX_PRIO);
	rq = task_rq_lock(t);
	array = t->sched.internal.array;
	if (array)
		dequeue_task(t,array);
	t->sched.internal.prio=prio;
	if (array)
		enqueue_task(t,array);
	task_rq_unlock(rq);
	LOG_OUT();
	return 0;
}

#if 0
/*
 * Potentially available exiting-child timeslices are
 * retrieved here - this way the parent does not get
 * penalized for creating too many threads.
 *
 * (this cannot be used to 'generate' timeslices
 * artificially, because any timeslice recovered here
 * was given away by the parent in the first place.)
 */
void fastcall sched_exit(task_t * p)
{
	unsigned long flags;
	ma_runqueue_t *rq;

	local_irq_save(flags);
	if (p->first_time_slice) {
		p->parent->time_slice += p->time_slice;
		if (unlikely(p->parent->time_slice > MAX_TIMESLICE))
			p->parent->time_slice = MAX_TIMESLICE;
	}
	local_irq_restore(flags);
	/*
	 * If the child was a (relative-) CPU hog then decrease
	 * the sleep_avg of the parent as well.
	 */
	rq = task_rq_lock(p->parent, &flags);
	if (p->sleep_avg < p->parent->sleep_avg)
		p->parent->sleep_avg = p->parent->sleep_avg /
		(EXIT_WEIGHT + 1) * EXIT_WEIGHT + p->sleep_avg /
		(EXIT_WEIGHT + 1);
	task_rq_unlock(rq, &flags);
}
#endif

/**
 * finish_task_switch - clean up after a task-switch
 * @prev: the thread we just switched away from.
 *
 * We enter this with the runqueue still locked, and finish_arch_switch()
 * will unlock it along with doing any other architecture-specific cleanup
 * actions.
 *
 * Note that we may have delayed dropping an mm in context_switch(). If
 * so, we finish that here outside of the runqueue lock.  (Doing it
 * with the lock held can cause deadlocks; see schedule() for
 * details.)
 */
static inline void finish_task_switch(marcel_task_t *prev)
{
	ma_runqueue_t *prevrq = ma_prev_rq();
	ma_runqueue_t *rq = ma_this_rq();
//	struct mm_struct *mm = rq->prev_mm;
	unsigned long prev_task_flags;

//	rq->prev_mm = NULL;

	/*
	 * A task struct has one reference for the use as "current".
	 * If a task dies, then it sets TASK_ZOMBIE in tsk->state and calls
	 * schedule one last time. The schedule call will never return,
	 * and the scheduled task must drop that reference.
	 * The test for TASK_ZOMBIE must occur while the runqueue locks are
	 * still held, otherwise prev could be scheduled on another cpu, die
	 * there before we look at prev->state, and then the reference would
	 * be dropped twice.
	 * 		Manfred Spraul <manfred@colorfullife.com>
	 */
	prev_task_flags = prev->flags;

	/* si le lwp change de runqueue, il faut libérer les deux.
	 * finish_arch_switch s'occupe de la précédente (ou s'en est déjà occupé
	 * dans prepare_arch_switch) */
	if (rq && prevrq!=rq)
		_ma_raw_spin_unlock(&rq->lock);
	ma_finish_arch_switch(prevrq, prev);
//	if (mm)
//		mmdrop(mm);
//	if (tbx_unlikely(prev_task_flags & MA_PF_DEAD))
//		ma_put_task_struct(prev);
}

/**
 * schedule_tail - first thing a freshly forked thread must call.
 * @prev: the thread we just switched away from.
 */
asmlinkage void ma_schedule_tail(marcel_task_t *prev)
{
	finish_task_switch(prev);

//	if (current->set_child_tid)
//		put_user(current->pid, current->set_child_tid);
}

/*
 * context_switch - switch to the new MM and the new
 * thread's register state.
 */
static inline marcel_task_t * context_switch(ma_runqueue_t *rq, marcel_task_t *prev, marcel_task_t *next)
{
//	struct mm_struct *mm = next->mm;
//	struct mm_struct *oldmm = prev->active_mm;

//	if (unlikely(!mm)) {
//		next->active_mm = oldmm;
//		atomic_inc(&oldmm->mm_count);
//		enter_lazy_tlb(oldmm, next);
//	} else
//		switch_mm(oldmm, mm, next);

//	if (unlikely(!prev->mm)) {
//		prev->active_mm = NULL;
//		WARN_ON(rq->prev_mm);
//		rq->prev_mm = oldmm;
//	}

	/* Here we just switch the register state and the stack. */
	__ma_get_lwp_var(prev_rq)=rq;
	prev=marcel_switch_to(prev, next);//switch_to(prev, next, prev);

	return prev;
}

/*
 * nr_running, nr_uninterruptible and nr_context_switches:
 *
 * externally visible scheduler statistics: current number of runnable
 * threads, current number of uninterruptible-sleeping threads, total
 * number of context switches performed since bootup.
 */
unsigned long ma_nr_running(void)
{
	unsigned long i, sum = 0;

	for (i = 0; i < MA_NR_LWPS; i++)
		sum += ma_lwp_rq(GET_LWP_BY_NUMBER(i))->nr_running;
	sum += ma_main_runqueue.nr_running;

	return sum;
}

#if 0
unsigned long nr_uninterruptible(void)
{
	unsigned long i, sum = 0;

	for_each_cpu(i)
		sum += cpu_rq(i)->nr_uninterruptible;
	return sum;
}

unsigned long long nr_context_switches(void)
{
	unsigned long i;
	unsigned long long sum = 0;

	for_each_cpu(i)
		sum += cpu_rq(i)->nr_switches;
	return sum;
}

unsigned long nr_iowait(void)
{
	unsigned long i, sum = 0;

	for_each_cpu(i)
		sum += atomic_read(&cpu_rq(i)->nr_iowait);
	return sum;
}
#endif

#if 0
#ifdef CONFIG_NUMA
/*
 * If dest_cpu is allowed for this process, migrate the task to it.
 * This is accomplished by forcing the cpu_allowed mask to only
 * allow dest_cpu, which will force the cpu onto dest_cpu.  Then
 * the cpu_allowed mask is restored.
 */
static void sched_migrate_task(task_t *p, int dest_cpu)
{
	ma_runqueue_t *rq;
	migration_req_t req;
	unsigned long flags;
	cpumask_t old_mask, new_mask = cpumask_of_cpu(dest_cpu);
 
	lock_cpu_hotplug();
	rq = task_rq_lock(p, &flags);
	old_mask = p->cpus_allowed;
	if (!cpu_isset(dest_cpu, old_mask) || !cpu_online(dest_cpu))
		goto out;

	/* force the process onto the specified CPU */
	if (__set_cpus_allowed(p, new_mask, &req)) {
		/* Need to wait for migration thread. */
		task_rq_unlock(rq, &flags);
		wake_up_process(rq->migration_thread);
		wait_for_completion(&req.done);

		/* If we raced with sys_sched_setaffinity, don't
		 * restore mask. */
		rq = task_rq_lock(p, &flags);
		if (likely(cpus_equal(p->cpus_allowed, new_mask))) {
			/* Restore old mask: won't need migration
			 * thread, since current cpu is allowed. */
			BUG_ON(__set_cpus_allowed(p, old_mask, NULL));
		}
	}
out:
	task_rq_unlock(rq, &flags);
	unlock_cpu_hotplug();
}

/*
 * Find the least loaded CPU.  Slightly favor the current CPU by
 * setting its runqueue length as the minimum to start.
 */
static int sched_best_cpu(struct task_struct *p)
{
	int i, minload, load, best_cpu, node = 0;
	cpumask_t cpumask;

	best_cpu = task_cpu(p);
	if (cpu_rq(best_cpu)->nr_running <= 2)
		return best_cpu;

	minload = 10000000;
	for_each_node_with_cpus(i) {
		/*
		 * Node load is always divided by nr_cpus_node to normalise 
		 * load values in case cpu count differs from node to node.
		 * We first multiply node_nr_running by 10 to get a little
		 * better resolution.   
		 */
		load = 10 * atomic_read(&node_nr_running[i]) / nr_cpus_node(i);
		if (load < minload) {
			minload = load;
			node = i;
		}
	}

	minload = 10000000;
	cpumask = node_to_cpumask(node);
	for (i = 0; i < NR_CPUS; ++i) {
		if (!cpu_isset(i, cpumask))
			continue;
		if (cpu_rq(i)->nr_running < minload) {
			best_cpu = i;
			minload = cpu_rq(i)->nr_running;
		}
	}
	return best_cpu;
}

void sched_balance_exec(void)
{
	int new_cpu;

	if (numnodes > 1) {
		new_cpu = sched_best_cpu(current);
		if (new_cpu != smp_processor_id())
			sched_migrate_task(current, new_cpu);
	}
}

/*
 * Find the busiest node. All previous node loads contribute with a
 * geometrically deccaying weight to the load measure:
 *      load_{t} = load_{t-1}/2 + nr_node_running_{t}
 * This way sudden load peaks are flattened out a bit.
 * Node load is divided by nr_cpus_node() in order to compare nodes
 * of different cpu count but also [first] multiplied by 10 to 
 * provide better resolution.
 */
static int find_busiest_node(int this_node)
{
	int i, node = -1, load, this_load, maxload;

	if (!nr_cpus_node(this_node))
		return node;
	this_load = maxload = (ma_this_rq()->prev_node_load[this_node] >> 1)
		+ (10 * atomic_read(&node_nr_running[this_node])
		/ nr_cpus_node(this_node));
	ma_this_rq()->prev_node_load[this_node] = this_load;
	for_each_node_with_cpus(i) {
		if (i == this_node)
			continue;
		load = (ma_this_rq()->prev_node_load[i] >> 1)
			+ (10 * atomic_read(&node_nr_running[i])
			/ nr_cpus_node(i));
		ma_this_rq()->prev_node_load[i] = load;
		if (load > maxload && (100*load > NODE_THRESHOLD*this_load)) {
			maxload = load;
			node = i;
		}
	}
	return node;
}

#endif /* CONFIG_NUMA */
#endif

#ifdef MA__LWPS
#ifdef MA__DEBUG
#warning "Load balancing non géré pour l'instant. A faire ([^MS])..."
#endif
#if 0

/*
 * double_lock_balance - lock the busiest runqueue
 *
 * ma_this_rq is locked already. Recalculate nr_running if we have to
 * drop the runqueue lock.
 */
static inline unsigned int double_lock_balance(ma_runqueue_t *this_rq,
	ma_runqueue_t *busiest, int this_cpu, int idle, unsigned int nr_running)
{
	if (unlikely(!spin_trylock(&busiest->lock))) {
		if (busiest < this_rq) {
			spin_unlock(&this_rq->lock);
			spin_lock(&busiest->lock);
			spin_lock(&this_rq->lock);
			/* Need to recalculate nr_running */
			if (idle || (this_rq->nr_running > this_rq->prev_cpu_load[this_cpu]))
				nr_running = this_rq->nr_running;
			else
				nr_running = this_rq->prev_cpu_load[this_cpu];
		} else
			spin_lock(&busiest->lock);
	}
	return nr_running;
}

/*
 * find_busiest_queue - find the busiest runqueue among the cpus in cpumask.
 */
static inline ma_runqueue_t *find_busiest_queue(ma_runqueue_t *this_rq, int this_cpu, int idle, int *imbalance, cpumask_t cpumask)
{
	int nr_running, load, max_load, i;
	ma_runqueue_t *busiest, *rq_src;

	/*
	 * We search all runqueues to find the most busy one.
	 * We do this lockless to reduce cache-bouncing overhead,
	 * we re-check the 'best' source CPU later on again, with
	 * the lock held.
	 *
	 * We fend off statistical fluctuations in runqueue lengths by
	 * saving the runqueue length (as seen by the balancing CPU) during
	 * the previous load-balancing operation and using the smaller one
	 * of the current and saved lengths. If a runqueue is long enough
	 * for a longer amount of time then we recognize it and pull tasks
	 * from it.
	 *
	 * The 'current runqueue length' is a statistical maximum variable,
	 * for that one we take the longer one - to avoid fluctuations in
	 * the other direction. So for a load-balance to happen it needs
	 * stable long runqueue on the target CPU and stable short runqueue
	 * on the local runqueue.
	 *
	 * We make an exception if this CPU is about to become idle - in
	 * that case we are less picky about moving a task across CPUs and
	 * take what can be taken.
	 */
	if (idle || (this_rq->nr_running > this_rq->prev_cpu_load[this_cpu]))
		nr_running = this_rq->nr_running;
	else
		nr_running = this_rq->prev_cpu_load[this_cpu];

	busiest = NULL;
	max_load = 1;
	for (i = 0; i < NR_CPUS; i++) {
		if (!cpu_isset(i, cpumask))
			continue;

		rq_src = cpu_rq(i);
		if (idle || (rq_src->nr_running < this_rq->prev_cpu_load[i]))
			load = rq_src->nr_running;
		else
			load = this_rq->prev_cpu_load[i];
		this_rq->prev_cpu_load[i] = rq_src->nr_running;

		if ((load > max_load) && (rq_src != this_rq)) {
			busiest = rq_src;
			max_load = load;
		}
	}

	if (likely(!busiest))
		goto out;

	*imbalance = max_load - nr_running;

	/* It needs an at least ~25% imbalance to trigger balancing. */
	if (!idle && ((*imbalance)*4 < max_load)) {
		busiest = NULL;
		goto out;
	}

	nr_running = double_lock_balance(this_rq, busiest, this_cpu, idle, nr_running);
	/*
	 * Make sure nothing changed since we checked the
	 * runqueue length.
	 */
	if (busiest->nr_running <= nr_running) {
		spin_unlock(&busiest->lock);
		busiest = NULL;
	}
out:
	return busiest;
}

/*
 * pull_task - move a task from a remote runqueue to the local runqueue.
 * Both runqueues must be locked.
 */
static inline void pull_task(ma_runqueue_t *src_rq, ma_prio_array_t *src_array, task_t *p, ma_runqueue_t *this_rq, int this_cpu)
{
	dequeue_task(p, src_array);
	nr_running_dec(src_rq);
	set_task_cpu(p, this_cpu);
	nr_running_inc(this_rq);
	enqueue_task(p, this_rq->active);
	p->timestamp = sched_clock() -
				(src_rq->timestamp_last_tick - p->timestamp);
	/*
	 * Note that idle threads have a prio of MA_MAX_PRIO, for this test
	 * to be always true for them.
	 */
	if (TASK_PREEMPTS_CURR(p, this_rq))
		set_need_resched();
}

/*
 * can_migrate_task - may task p from runqueue rq be migrated to this_cpu?
 */

static inline int
can_migrate_task(task_t *tsk, ma_runqueue_t *rq, int this_cpu, int idle)
{
	unsigned long delta = rq->timestamp_last_tick - tsk->timestamp;

	/*
	 * We do not migrate tasks that are:
	 * 1) running (obviously), or
	 * 2) cannot be migrated to this CPU due to cpus_allowed, or
	 * 3) are cache-hot on their current CPU.
	 */
	if (ma_task_running(rq, tsk))
		return 0;
	if (!cpu_isset(this_cpu, tsk->cpus_allowed))
		return 0;
	if (!idle && (delta <= JIFFIES_TO_NS(cache_decay_ticks)))
		return 0;
	return 1;
}

/*
 * Current runqueue is empty, or rebalance tick: if there is an
 * inbalance (current runqueue is too short) then pull from
 * busiest runqueue(s).
 *
 * We call this with the current runqueue locked,
 * irqs disabled.
 */
static void load_balance(ma_runqueue_t *this_rq, int idle, cpumask_t cpumask)
{
	int imbalance, idx, this_cpu = smp_processor_id();
	ma_runqueue_t *busiest;
	ma_prio_array_t *array;
	struct list_head *head, *curr;
	task_t *tmp;

	if (cpu_is_offline(this_cpu))
		goto out;

	busiest = find_busiest_queue(this_rq, this_cpu, idle,
				     &imbalance, cpumask);
	if (!busiest)
		goto out;

	/*
	 * We only want to steal a number of tasks equal to 1/2 the imbalance,
	 * otherwise we'll just shift the imbalance to the new queue:
	 */
	imbalance /= 2;

	/*
	 * We first consider expired tasks. Those will likely not be
	 * executed in the near future, and they are most likely to
	 * be cache-cold, thus switching CPUs has the least effect
	 * on them.
	 */
	if (busiest->expired->nr_active)
		array = busiest->expired;
	else
		array = busiest->active;

new_array:
	/* Start searching at priority 0: */
	idx = 0;
skip_bitmap:
	if (!idx)
		idx = sched_find_first_bit(array->bitmap);
	else
		idx = find_next_bit(array->bitmap, MA_MAX_PRIO, idx);
	if (idx >= MA_MAX_PRIO) {
		if (array == busiest->expired) {
			array = busiest->active;
			goto new_array;
		}
		goto out_unlock;
	}

	head = array->queue + idx;
	curr = head->prev;
skip_queue:
	tmp = list_entry(curr, task_t, run_list);

	curr = curr->prev;

	if (!can_migrate_task(tmp, busiest, this_cpu, idle)) {
		if (curr != head)
			goto skip_queue;
		idx++;
		goto skip_bitmap;
	}
	pull_task(busiest, array, tmp, this_rq, this_cpu);

	/* Only migrate one task if we are idle */
	if (!idle && --imbalance) {
		if (curr != head)
			goto skip_queue;
		idx++;
		goto skip_bitmap;
	}
out_unlock:
	spin_unlock(&busiest->lock);
out:
	;
}

/*
 * One of the idle_cpu_tick() and busy_cpu_tick() functions will
 * get called every timer tick, on every CPU. Our balancing action
 * frequency and balancing agressivity depends on whether the CPU is
 * idle or not.
 *
 * busy-rebalance every 200 msecs. idle-rebalance every 1 msec. (or on
 * systems with HZ=100, every 10 msecs.)
 *
 * On NUMA, do a node-rebalance every 400 msecs.
 */
#define IDLE_REBALANCE_TICK (HZ/1000 ?: 1)
#define BUSY_REBALANCE_TICK (HZ/5 ?: 1)
#define IDLE_NODE_REBALANCE_TICK (IDLE_REBALANCE_TICK * 5)
#define BUSY_NODE_REBALANCE_TICK (BUSY_REBALANCE_TICK * 2)

#ifdef CONFIG_NUMA
static void balance_node(ma_runqueue_t *this_rq, int idle, int this_cpu)
{
	int node = find_busiest_node(cpu_to_node(this_cpu));

	if (node >= 0) {
		cpumask_t cpumask = node_to_cpumask(node);
		cpu_set(this_cpu, cpumask);
		spin_lock(&this_rq->lock);
		load_balance(this_rq, idle, cpumask);
		spin_unlock(&this_rq->lock);
	}
}
#endif

static void rebalance_tick(ma_runqueue_t *this_rq, int idle)
{
#ifdef CONFIG_NUMA
	int this_cpu = smp_processor_id();
#endif
	unsigned long j = jiffies;

	/*
	 * First do inter-node rebalancing, then intra-node rebalancing,
	 * if both events happen in the same tick. The inter-node
	 * rebalancing does not necessarily have to create a perfect
	 * balance within the node, since we load-balance the most loaded
	 * node with the current CPU. (ie. other CPUs in the local node
	 * are not balanced.)
	 */
	if (idle) {
#ifdef CONFIG_NUMA
		if (!(j % IDLE_NODE_REBALANCE_TICK))
			balance_node(this_rq, idle, this_cpu);
#endif
		if (!(j % IDLE_REBALANCE_TICK)) {
			spin_lock(&this_rq->lock);
			load_balance(this_rq, idle, cpu_to_node_mask(this_cpu));
			spin_unlock(&this_rq->lock);
		}
		return;
	}
#ifdef CONFIG_NUMA
	if (!(j % BUSY_NODE_REBALANCE_TICK))
		balance_node(this_rq, idle, this_cpu);
#endif
	if (!(j % BUSY_REBALANCE_TICK)) {
		spin_lock(&this_rq->lock);
		load_balance(this_rq, idle, cpu_to_node_mask(this_cpu));
		spin_unlock(&this_rq->lock);
	}
}
#endif /* 0 */
/* a priori, c'est l'application qui rebalance les choses */
static inline void rebalance_tick(ma_runqueue_t *this_rq, int idle)
{
}
#else
/*
 * on UP we do not need to balance between CPUs:
 */
static inline void rebalance_tick(ma_runqueue_t *this_rq, int idle)
{
}
#endif

//DEFINE_PER_CPU(struct kernel_stat, kstat);

//EXPORT_PER_CPU_SYMBOL(kstat);

/*
 * We place interactive tasks back into the active array, if possible.
 *
 * To guarantee that this does not starve expired tasks we ignore the
 * interactivity of a task if the first expired task had to wait more
 * than a 'reasonable' amount of time. This deadline timeout is
 * load-dependent, as the frequency of array switched decreases with
 * increasing number of running tasks. We also ignore the interactivity
 * if a better static_prio task has expired:
 */
#define EXPIRED_STARVING(rq) \
	((STARVATION_LIMIT && ((rq)->expired_timestamp && \
 		(jiffies - (rq)->expired_timestamp >= \
			STARVATION_LIMIT * ((rq)->nr_running) + 1))) /*||*/ \
			/*((rq)->curr->static_prio > (rq)->best_expired_prio)*/)
/*
 * This function gets called by the timer code, with HZ frequency.
 * We call it with interrupts disabled.
 *
 * It also gets called by the fork code, when changing the parent's
 * timeslices.
 */
void ma_scheduler_tick(int user_ticks, int sys_ticks)
{
	//int cpu = smp_processor_id();
	struct ma_lwp_usage_stat *lwpstat = &ma_per_lwp(lwp_usage,LWP_SELF);
	ma_runqueue_t *rq = ma_this_rq();
	marcel_task_t *p = MARCEL_SELF;

	LOG_IN();

	rq->timestamp_last_tick = marcel_clock();

#ifdef MA__DEBUG
#warning rcu not yet implemented (utile pour les numa ?)
#endif
	//if (rcu_pending(cpu))
	//      rcu_check_callbacks(cpu, user_ticks);

// TODO Pour l'instant, on n'a pas de notion de tick système.
#define sys_ticks user_ticks
	if (ma_hardirq_count()) {
		lwpstat->irq += sys_ticks;
		sys_ticks = 0;
	/* note: this timer irq context must be accounted for as well */
	} else if (ma_softirq_count() - MA_SOFTIRQ_OFFSET) {
		lwpstat->softirq += sys_ticks;
		sys_ticks = 0;
	}

	if (p->sched.internal.init_rq->type == MA_DONTSCHED_RQ) {
		// TODO on n'a pas non plus de notion d'iowait
		/*if (atomic_read(&rq->nr_iowait) > 0)
			lwpstat->iowait += sys_ticks;
		else*/
			lwpstat->idle += sys_ticks;
		//rebalance_tick(rq, 1);
		//return;
		sys_ticks = 0;
	}
	//if (TASK_NICE(p) > 0)
	if (p->sched.internal.prio >= MA_BATCH_PRIO)
		lwpstat->nice += user_ticks;
	else
		lwpstat->user += user_ticks;
	//lwpstat->system += sys_ticks;
#undef sys_ticks

	/* Task might have expired already, but not scheduled off yet */
	//if (p->sched.internal.array != rq->active) {
	if (p->sched.internal.array != NULL) {
		pm2debug("Strange: %s running, but on an array !, report or look at it (%s:%i)\n",
				p->name, __FILE__, __LINE__);
		ma_set_tsk_need_resched(p);
		goto out;
	}
	ma_spin_lock(&rq->lock);
	/*
	 * The task was running during this tick - update the
	 * time slice counter. Note: we do not update a thread's
	 * priority until it either goes to sleep or uses up its
	 * timeslice. This makes it possible for interactive tasks
	 * to use up their timeslices at their highest priority levels.
	 */
#if 0
	if (unlikely(rt_task(p))) {
		/*
		 * RR tasks need a special form of timeslice management.
		 * FIFO tasks have no timeslices.
		 */
		if ((p->policy == SCHED_RR) && !--p->time_slice) {
			p->time_slice = task_timeslice(p);
			p->first_time_slice = 0;
			set_tsk_need_resched(p);

			/* put it at the end of the queue: */
			dequeue_task(p, rq->active);
			enqueue_task(p, rq->active);
		}
		goto out_unlock;
	}
#endif
	if (preemption_enabled() && ma_thread_preemptible() &&
			!--p->sched.internal.time_slice) {
		ma_set_tsk_need_resched(p);
		//p->prio = effective_prio(p);
		p->sched.internal.time_slice = 1; /* TODO: utiliser la priorité pour le calculer */
				//task_timeslice(p);
		//p->first_time_slice = 0;

		//if (!rq->expired_timestamp)
			//rq->expired_timestamp = jiffies;
		//if (!TASK_INTERACTIVE(p) || EXPIRED_STARVING(rq)) {
//			if (p->static_prio < rq->best_expired_prio)
//				rq->best_expired_prio = p->static_prio;
		//}
	}
#if 0
	else {
		/*
		 * Prevent a too long timeslice allowing a task to monopolize
		 * the CPU. We do this by splitting up the timeslice into
		 * smaller pieces.
		 *
		 * Note: this does not mean the task's timeslices expire or
		 * get lost in any way, they just might be preempted by
		 * another task of equal priority. (one with higher
		 * priority would have preempted this task already.) We
		 * requeue this task to the end of the list on this priority
		 * level, which is in essence a round-robin of tasks with
		 * equal priority.
		 *
		 * This only applies to tasks in the interactive
		 * delta range with at least TIMESLICE_GRANULARITY to requeue.
		 */
		if (TASK_INTERACTIVE(p) && !((task_timeslice(p) -
			p->time_slice) % TIMESLICE_GRANULARITY(p)) &&
			(p->time_slice >= TIMESLICE_GRANULARITY(p)) &&
			(p->array == rq->active)) {
			if (preemption_enabled() && ma_thread_preemptible()) {
				//dequeue_task(p, rq->active);
				ma_set_tsk_need_resched(p);
//				p->prio = effective_prio(p);
				//enqueue_task(p, rq->active);
			}
		}
	}
out_unlock:
#endif
	ma_spin_unlock(&rq->lock);
out:
	rebalance_tick(rq, 0);
	LOG_OUT();
}

void ma_scheduling_functions_start_here(void) { }

/*
 * schedule() is the main scheduler function.
 */
asmlinkage void ma_schedule(void)
{
//	long *switch_count;
	marcel_task_t *prev, *next, *prev_as_next;
	ma_runqueue_t *rq,*prevrq,*currq, *prev_as_rq;
	ma_prio_array_t *array;
	struct list_head *queue;
	unsigned long now;
	unsigned long run_time;
	int idx;
	int max_prio, prev_as_prio;
	LOG_IN();

	/*
	 * Test if we are atomic.  Since do_exit() needs to call into
	 * schedule() atomically, we ignore that path for now.
	 * Otherwise, whine if we are scheduling when we should not be.
	 */
	if (tbx_likely(!(MARCEL_SELF->sched.state & (MA_TASK_DEAD | MA_TASK_ZOMBIE)))) {
		if (tbx_unlikely(ma_in_atomic())) {
			pm2debug("bad: scheduling while atomic (%06x)!\n",ma_preempt_count());
			MA_BUG();
		}
	}

need_resched:
	/* we need to disable bottom half to avoid running ma_schedule_tick() ! */
	ma_preempt_disable();
	ma_local_bh_disable();

	/* by default, reschedule this thread */
	prev_as_next = prev = MARCEL_SELF;
	MA_BUG_ON(!prev);

	now = marcel_clock();
	//if (likely(now - prev->timestamp < NS_MAX_SLEEP_AVG))
		run_time = now - prev->sched.internal.timestamp;
	/*
	else
		run_time = NS_MAX_SLEEP_AVG;
		*/

	prev_as_rq = prevrq = ma_this_rq();
	MA_BUG_ON(!prevrq);
	prev_as_prio = MARCEL_SELF->sched.internal.prio;

	if (prev->sched.state && !(ma_preempt_count() & MA_PREEMPT_ACTIVE)) {
		//switch_count = &prev->nvcsw;
		if (tbx_unlikely((prev->sched.state & MA_TASK_INTERRUPTIBLE) &&
				 tbx_unlikely(0 /*work_pending(prev)*/)))
			prev->sched.state = MA_TASK_RUNNING;
		else {
			sched_debug("schedule: go to sleep\n");
			prev_as_next = NULL;
			prev_as_rq = ma_dontsched_rq(LWP_SELF);
			prev_as_prio = MA_IDLE_PRIO;
		}
	}

#ifdef MA__LWPS
restart:
#endif
	next = prev_as_next;
	rq = prev_as_rq;
	max_prio = prev_as_prio;

#ifdef MA__LWPS
	sched_debug("default prio: %d, rq: %p\n",max_prio,rq);
	for (currq = ma_lwp_rq(LWP_SELF); currq; currq = currq->father) {
#else
	currq = &ma_main_runqueue;
#endif
		if (!currq->nr_running)
			sched_debug("apparently nobody in %p\n",currq);
		idx = ma_sched_find_first_bit(currq->active->bitmap);
		if (idx < max_prio) {
			sched_debug("found better prio %d in rq %p\n",idx,currq);
			next = NULL;
			max_prio = idx;
			rq = currq;
		}
		if (next && ma_need_resched() && idx == prev_as_prio) {
		/* still wanted to schedule prev, but it needs resched
		 * and this is same prio
		 */
			sched_debug("found same prio %d in rq %p\n",idx,currq);
			next = NULL;
			rq = currq;
		}
#ifdef MA__LWPS
	}
#endif

	double_rq_lock(prevrq,rq);
	sched_debug("locked\n");

	if (tbx_unlikely(rq == ma_dontsched_rq(LWP_SELF))) {
		/* found no interesting queue, not even previous one */
#ifdef MA__LWPS
		sched_debug("rebalance\n");
#ifdef MA__DEBUG
#warning TODO: demander à l application de rebalancer
#endif
//		load_balance(rq, 1, cpu_to_node_mask(smp_processor_id()));
#endif
		next = ma_per_lwp(idle_task, LWP_SELF);//rq->idle;
	}

	if (next) /* either prev or idle */
		goto switch_tasks;

#ifdef MA__LWPS
	if (tbx_unlikely(!(rq->active->nr_active+rq->expired->nr_active))) {
		sched_debug("someone stole the task we saw, restart\n");
		double_rq_unlock(prevrq,rq);
		goto restart;
	}
#endif

	MA_BUG_ON(!(rq->active->nr_active+rq->expired->nr_active));

	/* now look for next *different* task */
	array = rq->active;
	if (tbx_unlikely(!array->nr_active)) {
		sched_debug("arrays switch\n");
		/* XXX: todo: handle all rqs... */
		rq_arrays_switch(rq);
	}

	idx = ma_sched_find_first_bit(array->bitmap);
	queue = array->queue + idx;
	/* we may here see ourselves, if someone awoked us */
	next = list_entry(queue->next, marcel_task_t, sched.internal.run_list);
	
	sched_debug("prio %d in %p, next %p(%s)\n",idx,rq,next,next->name);

//	if (next->activated > 0) {
//		unsigned long long delta = now - next->timestamp;

//		if (next->activated == 1)
//			delta = delta * (ON_RUNQUEUE_WEIGHT * 128 / 100) / 128;

//		array = next->array;
//		dequeue_task(next, array);
//		recalc_task_prio(next, next->timestamp + delta);
//		enqueue_task(next, array);
//	}
//	next->activated = 0;
switch_tasks:
	/* still wanting to go to sleep ? (now that runqueues are locked, we can
	 * safely deactivate ourselves */
	if (prev->sched.state && !(ma_preempt_count() & MA_PREEMPT_ACTIVE)) {
		if (prev->sched.state & MA_TASK_MOVING)
			/* moving, make it running elsewhere */
			ma_set_current_state(MA_TASK_RUNNING);
		else
			/* yes, deactivate */
			deactivate_running_task(prev,prevrq);
	}

	prefetch(next);
	ma_clear_tsk_need_resched(prev);
//Pour quand on voudra ce mécanisme...
	//ma_RCU_qsctr(ma_task_lwp(prev))++;

//	prev->sleep_avg -= run_time;
//	if ((long)prev->sleep_avg <= 0) {
//		prev->sleep_avg = 0;
//		if (!(HIGH_CREDIT(prev) || LOW_CREDIT(prev)))
//			prev->interactive_credit--;
//	}
	prev->sched.internal.timestamp = prev->sched.internal.last_ran = now;

	if (tbx_likely(prev != next)) {
//		next->timestamp = now;
//		rq->nr_switches++;
		ma_per_lwp(current_thread, LWP_SELF) = next;
//		++*switch_count;

		dequeue_task(next, next->sched.internal.array);
		/* still runnable */
		if (prev->sched.internal.cur_rq)
			enqueue_task(prev,prevrq->active);
		ma_set_task_lwp(next, LWP_SELF);

		ma_prepare_arch_switch(rq, next);
		prev = context_switch(prevrq, prev, next);
		ma_barrier();

		finish_task_switch(prev);
	} else
		ma_spin_unlock_softirq(&rq->lock);

//	reacquire_kernel_lock(current);
	ma_preempt_enable_no_resched();
	if (ma_test_thread_flag(TIF_NEED_RESCHED))
		goto need_resched;
	LOG_OUT();
}

MARCEL_INT(ma_schedule);

int marcel_yield_to(marcel_t next)
{
	marcel_t prev = MARCEL_SELF;
	ma_runqueue_t *prevrq = ma_this_rq(), *nextrq;

	if (next==prev)
		return 0;

	nextrq = task_rq_rq_lock(next, prevrq);

	if (!next->sched.internal.cur_rq || !next->sched.internal.array) {
		double_rq_unlock_softirq(prevrq, nextrq);
		sched_debug("marcel_yield: %s task %p\n",
			!next->sched.internal.cur_rq?"not enqueued":"busy", next);
		return -1;
	}

	// we suppose we don't want to go to sleep, and we're not yielding to a
	// dontsched thread like idle or activations
	
	ma_per_lwp(current_thread, LWP_SELF) = next;
	dequeue_task(next, next->sched.internal.array);
	enqueue_task(prev, prevrq->active);
	ma_set_task_lwp(next, LWP_SELF);

	ma_prepare_arch_switch(nextrq,next);
	prev = context_switch(prevrq, prev, next);
	ma_barrier();
	finish_task_switch(prev);

	return 0;
}

asmlinkage void ma_preempt_schedule(void)
{
        marcel_task_t *ti = MARCEL_SELF;

        /*
         * If there is a non-zero preempt_count or interrupts are disabled,
         * we do not want to preempt the current task.  Just return..
         */
        if (tbx_unlikely(ti->preempt_count)) { // || irqs_disabled()))
                return;
	}

need_resched:
        ti->preempt_count = MA_PREEMPT_ACTIVE;
        ma_schedule();
        ti->preempt_count = 0;

        /* we could miss a preemption opportunity between schedule and now */
        ma_barrier();
        if (tbx_unlikely(ma_test_thread_flag(TIF_NEED_RESCHED)))
                goto need_resched;
}
MARCEL_INT(ma_preempt_schedule);

#define reschedule_idle(p) RAISE(NOT_IMPLEMENTED)

#define lwp_id() GET_LWP_NUMBER(MARCEL_SELF()) // TODO: Bof

// Effectue un changement de contexte + éventuellement exécute des
// fonctions de scrutation...
DEF_MARCEL_POSIX(int, yield, (void))
{
  LOG_IN();

  //lock_task();
  marcel_check_polling(MARCEL_EV_POLL_AT_YIELD);
  marcel_yield_intern();
  //unlock_task();

  LOG_OUT();
  return 0;
}
/* La définition n'est pas toujours dans pthread.h */
extern int pthread_yield (void) __THROW;
DEF_PTHREAD_STRONG(yield)

// Modifie le 'vpmask' du thread courant. Le cas échéant, il faut donc
// retirer le thread de la file et le replacer dans la file
// adéquate...
// IMPORTANT : cette fonction doit marcher si on l'appelle en section atomique
// pour se déplacer sur le LWP courant (cf terminaison des threads)
void marcel_change_vpmask(marcel_vpmask_t mask)
{
#ifdef MA__LWPS
	ma_runqueue_t *old_rq, *new_rq;
	LOG_IN();
	/* empêcher ma_schedule() */
	ma_preempt_disable();
	/* empêcher même scheduler_tick() */
	ma_local_bh_disable();
	old_rq=ma_this_rq();
	new_rq=marcel_sched_vpmask_init_rq(mask);
	if (old_rq==new_rq) {
		ma_preempt_enable();
		LOG_OUT();
		return;
	}
	deactivate_running_task(MARCEL_SELF,ma_this_rq());
	activate_running_task(MARCEL_SELF,marcel_sched_vpmask_init_rq(mask));
	/* On teste si le LWP courant est interdit ou pas */
	if (marcel_vpmask_vp_ismember(mask,LWP_NUMBER(LWP_SELF))) {
		ma_set_current_state(MA_TASK_MOVING);
		ma_local_bh_enable();
		ma_preempt_enable();
		ma_schedule();
	} else {
		ma_local_bh_enable();
		ma_preempt_enable();
	}
	LOG_OUT();
#endif
}

#if 0
int ma_default_wake_function(wait_queue_t *curr, unsigned mode, int sync)
{
	marcel_task_t *p = curr->task;
	return try_to_wake_up(p, mode, sync);
}

EXPORT_SYMBOL(ma_default_wake_function);

/*
 * The core wakeup function.  Non-exclusive wakeups (nr_exclusive == 0) just
 * wake everything up.  If it's an exclusive wakeup (nr_exclusive == small +ve
 * number) then we wake all the non-exclusive tasks and one exclusive task.
 *
 * There are circumstances in which we can try to wake a task which has already
 * started to run but is not in state TASK_RUNNING.  try_to_wake_up() returns
 * zero in this (rare) case, and we handle it by continuing to scan the queue.
 */
static void __wake_up_common(wait_queue_head_t *q, unsigned int mode, int nr_exclusive, int sync)
{
	struct list_head *tmp, *next;

	list_for_each_safe(tmp, next, &q->task_list) {
		wait_queue_t *curr;
		unsigned flags;
		curr = list_entry(tmp, wait_queue_t, task_list);
		flags = curr->flags;
		if (curr->func(curr, mode, sync) &&
		    (flags & WQ_FLAG_EXCLUSIVE) &&
		    !--nr_exclusive)
			break;
	}
}

/**
 * __wake_up - wake up threads blocked on a waitqueue.
 * @q: the waitqueue
 * @mode: which threads
 * @nr_exclusive: how many wake-one or wake-many threads to wake up
 */
void fastcall __wake_up(wait_queue_head_t *q, unsigned int mode, int nr_exclusive)
{
	unsigned long flags;

	spin_lock_irqsave(&q->lock, flags);
	__wake_up_common(q, mode, nr_exclusive, 0);
	spin_unlock_irqrestore(&q->lock, flags);
}

EXPORT_SYMBOL(__wake_up);

/*
 * Same as __wake_up but called with the spinlock in wait_queue_head_t held.
 */
void fastcall __wake_up_locked(wait_queue_head_t *q, unsigned int mode)
{
	__wake_up_common(q, mode, 1, 0);
}

/**
 * __wake_up - sync- wake up threads blocked on a waitqueue.
 * @q: the waitqueue
 * @mode: which threads
 * @nr_exclusive: how many wake-one or wake-many threads to wake up
 *
 * The sync wakeup differs that the waker knows that it will schedule
 * away soon, so while the target thread will be woken up, it will not
 * be migrated to another CPU - ie. the two threads are 'synchronized'
 * with each other. This can prevent needless bouncing between CPUs.
 *
 * On UP it can prevent extra preemption.
 */
void fastcall __wake_up_sync(wait_queue_head_t *q, unsigned int mode, int nr_exclusive)
{
	unsigned long flags;

	if (unlikely(!q))
		return;

	spin_lock_irqsave(&q->lock, flags);
	if (likely(nr_exclusive))
		__wake_up_common(q, mode, nr_exclusive, 1);
	else
		__wake_up_common(q, mode, nr_exclusive, 0);
	spin_unlock_irqrestore(&q->lock, flags);
}

EXPORT_SYMBOL_GPL(__wake_up_sync);	/* For internal use only */

void fastcall complete(struct completion *x)
{
	unsigned long flags;

	spin_lock_irqsave(&x->wait.lock, flags);
	x->done++;
	__wake_up_common(&x->wait, TASK_UNINTERRUPTIBLE | TASK_INTERRUPTIBLE, 1, 0);
	spin_unlock_irqrestore(&x->wait.lock, flags);
}

EXPORT_SYMBOL(complete);

void fastcall complete_all(struct completion *x)
{
	unsigned long flags;

	spin_lock_irqsave(&x->wait.lock, flags);
	x->done += UINT_MAX/2;
	__wake_up_common(&x->wait, TASK_UNINTERRUPTIBLE | TASK_INTERRUPTIBLE, 0, 0);
	spin_unlock_irqrestore(&x->wait.lock, flags);
}

void fastcall wait_for_completion(struct completion *x)
{
	might_sleep();
	spin_lock_irq(&x->wait.lock);
	if (!x->done) {
		DECLARE_WAITQUEUE(wait, current);

		wait.flags |= WQ_FLAG_EXCLUSIVE;
		__add_wait_queue_tail(&x->wait, &wait);
		do {
			__set_current_state(TASK_UNINTERRUPTIBLE);
			spin_unlock_irq(&x->wait.lock);
			schedule();
			spin_lock_irq(&x->wait.lock);
		} while (!x->done);
		__remove_wait_queue(&x->wait, &wait);
	}
	x->done--;
	spin_unlock_irq(&x->wait.lock);
}

EXPORT_SYMBOL(wait_for_completion);

#define	SLEEP_ON_VAR				\
	unsigned long flags;			\
	wait_queue_t wait;			\
	init_waitqueue_entry(&wait, current);

#define SLEEP_ON_HEAD					\
	spin_lock_irqsave(&q->lock,flags);		\
	__add_wait_queue(q, &wait);			\
	spin_unlock(&q->lock);

#define	SLEEP_ON_TAIL						\
	spin_lock_irq(&q->lock);				\
	__remove_wait_queue(q, &wait);				\
	spin_unlock_irqrestore(&q->lock, flags);

void fastcall interruptible_sleep_on(wait_queue_head_t *q)
{
	SLEEP_ON_VAR

	current->state = TASK_INTERRUPTIBLE;

	SLEEP_ON_HEAD
	schedule();
	SLEEP_ON_TAIL
}

EXPORT_SYMBOL(interruptible_sleep_on);

long fastcall interruptible_sleep_on_timeout(wait_queue_head_t *q, long timeout)
{
	SLEEP_ON_VAR

	current->state = TASK_INTERRUPTIBLE;

	SLEEP_ON_HEAD
	timeout = schedule_timeout(timeout);
	SLEEP_ON_TAIL

	return timeout;
}

EXPORT_SYMBOL(interruptible_sleep_on_timeout);

void fastcall sleep_on(wait_queue_head_t *q)
{
	SLEEP_ON_VAR

	current->state = TASK_UNINTERRUPTIBLE;

	SLEEP_ON_HEAD
	schedule();
	SLEEP_ON_TAIL
}

EXPORT_SYMBOL(sleep_on);

long fastcall sleep_on_timeout(wait_queue_head_t *q, long timeout)
{
	SLEEP_ON_VAR

	current->state = TASK_UNINTERRUPTIBLE;

	SLEEP_ON_HEAD
	timeout = schedule_timeout(timeout);
	SLEEP_ON_TAIL

	return timeout;
}

EXPORT_SYMBOL(sleep_on_timeout);

void scheduling_functions_end_here(void) { }

void set_user_nice(task_t *p, long nice)
{
	unsigned long flags;
	ma_prio_array_t *array;
	ma_runqueue_t *rq;
	int old_prio, new_prio, delta;

	if (TASK_NICE(p) == nice || nice < -20 || nice > 19)
		return;
	/*
	 * We have to be careful, if called from sys_setpriority(),
	 * the task might be in the middle of scheduling on another CPU.
	 */
	rq = task_rq_lock(p, &flags);
	/*
	 * The RT priorities are set via setscheduler(), but we still
	 * allow the 'normal' nice value to be set - but as expected
	 * it wont have any effect on scheduling until the task is
	 * not SCHED_NORMAL:
	 */
	if (rt_task(p)) {
		p->static_prio = NICE_TO_PRIO(nice);
		goto out_unlock;
	}
	array = p->array;
	if (array)
		dequeue_task(p, array);

	old_prio = p->prio;
	new_prio = NICE_TO_PRIO(nice);
	delta = new_prio - old_prio;
	p->static_prio = NICE_TO_PRIO(nice);
	p->prio += delta;

	if (array) {
		enqueue_task(p, array);
		/*
		 * If the task increased its priority or is running and
		 * lowered its priority, then reschedule its CPU:
		 */
		// TODO: plus de curr
		if (delta < 0 || (delta > 0 && ma_task_running(rq, p)))
			resched_task(rq->curr);
	}
out_unlock:
	task_rq_unlock(rq, &flags);
}

EXPORT_SYMBOL(set_user_nice);

#ifndef __alpha__

/*
 * sys_nice - change the priority of the current process.
 * @increment: priority increment
 *
 * sys_setpriority is a more generic, but much slower function that
 * does similar things.
 */
asmlinkage long sys_nice(int increment)
{
	int retval;
	long nice;

	/*
	 *	Setpriority might change our priority at the same moment.
	 *	We don't have to worry. Conceptually one call occurs first
	 *	and we have a single winner.
	 */
	if (increment < 0) {
		if (!capable(CAP_SYS_NICE))
			return -EPERM;
		if (increment < -40)
			increment = -40;
	}
	if (increment > 40)
		increment = 40;

	nice = PRIO_TO_NICE(current->static_prio) + increment;
	if (nice < -20)
		nice = -20;
	if (nice > 19)
		nice = 19;

	retval = security_task_setnice(current, nice);
	if (retval)
		return retval;

	set_user_nice(current, nice);
	return 0;
}

#endif
#endif /* 0 */

/**
 * task_prio - return the priority value of a given task.
 * @p: the task in question.
 *
 * This is the priority value as seen by users in /proc.
 * RT tasks are offset by -200. Normal tasks are centered
 * around 0, value goes from -16 to +15.
 */
int task_prio(marcel_task_t *p)
{
	return p->sched.internal.prio;
//	return p->prio - MA_MAX_RT_PRIO;
}

#if 0
/**
 * task_nice - return the nice value of a given task.
 * @p: the task in question.
 */
int task_nice(marcel_task_t *p)
{
	return TASK_NICE(p);
}

MARCEL_INT(task_nice);
#endif

/**
 * idle_lwp - is a given cpu idle currently?
 * @lwp: the processor in question.
 */
int idle_lwp(ma_lwp_t lwp)
{
	return ma_lwp_curr(lwp) == ma_per_lwp(idle_task, lwp);//ma_lwp_rq(lwp)->idle;
}

MARCEL_INT(idle_lwp);

#if 0
/**
 * find_process_by_pid - find a process with a matching PID value.
 * @pid: the pid in question.
 */
static inline task_t *find_process_by_pid(pid_t pid)
{
	return pid ? find_task_by_pid(pid) : current;
}

/* Actually do priority change: must hold rq lock. */
static void __setscheduler(struct task_struct *p, int policy, int prio)
{
	BUG_ON(p->array);
	p->policy = policy;
	p->rt_priority = prio;
	if (policy != SCHED_NORMAL)
		p->prio = MA_MAX_USER_RT_PRIO-1 - p->rt_priority;
	else
		p->prio = p->static_prio;
}

/*
 * setscheduler - change the scheduling policy and/or RT priority of a thread.
 */
static int setscheduler(pid_t pid, int policy, struct sched_param __user *param)
{
	struct sched_param lp;
	int retval = -EINVAL;
	int oldprio;
	ma_prio_array_t *array;
	unsigned long flags;
	ma_runqueue_t *rq;
	task_t *p;

	if (!param || pid < 0)
		goto out_nounlock;

	retval = -EFAULT;
	if (copy_from_user(&lp, param, sizeof(struct sched_param)))
		goto out_nounlock;

	/*
	 * We play safe to avoid deadlocks.
	 */
	read_lock_irq(&tasklist_lock);

	p = find_process_by_pid(pid);

	retval = -ESRCH;
	if (!p)
		goto out_unlock_tasklist;

	/*
	 * To be able to change p->policy safely, the apropriate
	 * runqueue lock must be held.
	 */
	rq = task_rq_lock(p, &flags);

	if (policy < 0)
		policy = p->policy;
	else {
		retval = -EINVAL;
		if (policy != SCHED_FIFO && policy != SCHED_RR &&
				policy != SCHED_NORMAL)
			goto out_unlock;
	}

	/*
	 * Valid priorities for SCHED_FIFO and SCHED_RR are
	 * 1..MA_MAX_USER_RT_PRIO-1, valid priority for SCHED_NORMAL is 0.
	 */
	retval = -EINVAL;
	if (lp.sched_priority < 0 || lp.sched_priority > MA_MAX_USER_RT_PRIO-1)
		goto out_unlock;
	if ((policy == SCHED_NORMAL) != (lp.sched_priority == 0))
		goto out_unlock;

	retval = -EPERM;
	if ((policy == SCHED_FIFO || policy == SCHED_RR) &&
	    !capable(CAP_SYS_NICE))
		goto out_unlock;
	if ((current->euid != p->euid) && (current->euid != p->uid) &&
	    !capable(CAP_SYS_NICE))
		goto out_unlock;

	retval = security_task_setscheduler(p, policy, &lp);
	if (retval)
		goto out_unlock;

	array = p->array;
	if (array)
		deactivate_task(p, task_rq(p));
	retval = 0;
	oldprio = p->prio;
	__setscheduler(p, policy, lp.sched_priority);
	if (array) {
		__activate_task(p, task_rq(p));
		/*
		 * Reschedule if we are currently running on this runqueue and
		 * our priority decreased, or if we are not currently running on
		 * this runqueue and our priority is higher than the current's
		 */
		// todo: plus de cur
		if if (ma_task_running(rq, p)) {
			if (p->prio > oldprio)
				resched_task(rq->curr);
		} else if (p->prio < rq->curr->prio)
			resched_task(rq->curr);
	}

out_unlock:
	task_rq_unlock(rq, &flags);
out_unlock_tasklist:
	read_unlock_irq(&tasklist_lock);

out_nounlock:
	return retval;
}

/**
 * sys_sched_setscheduler - set/change the scheduler policy and RT priority
 * @pid: the pid in question.
 * @policy: new policy
 * @param: structure containing the new RT priority.
 */
asmlinkage long sys_sched_setscheduler(pid_t pid, int policy,
				      struct sched_param __user *param)
{
	return setscheduler(pid, policy, param);
}

/**
 * sys_sched_setparam - set/change the RT priority of a thread
 * @pid: the pid in question.
 * @param: structure containing the new RT priority.
 */
asmlinkage long sys_sched_setparam(pid_t pid, struct sched_param __user *param)
{
	return setscheduler(pid, -1, param);
}

/**
 * sys_sched_getscheduler - get the policy (scheduling class) of a thread
 * @pid: the pid in question.
 */
asmlinkage long sys_sched_getscheduler(pid_t pid)
{
	int retval = -EINVAL;
	task_t *p;

	if (pid < 0)
		goto out_nounlock;

	retval = -ESRCH;
	read_lock(&tasklist_lock);
	p = find_process_by_pid(pid);
	if (p) {
		retval = security_task_getscheduler(p);
		if (!retval)
			retval = p->policy;
	}
	read_unlock(&tasklist_lock);

out_nounlock:
	return retval;
}

/**
 * sys_sched_getscheduler - get the RT priority of a thread
 * @pid: the pid in question.
 * @param: structure containing the RT priority.
 */
asmlinkage long sys_sched_getparam(pid_t pid, struct sched_param __user *param)
{
	struct sched_param lp;
	int retval = -EINVAL;
	task_t *p;

	if (!param || pid < 0)
		goto out_nounlock;

	read_lock(&tasklist_lock);
	p = find_process_by_pid(pid);
	retval = -ESRCH;
	if (!p)
		goto out_unlock;

	retval = security_task_getscheduler(p);
	if (retval)
		goto out_unlock;

	lp.sched_priority = p->rt_priority;
	read_unlock(&tasklist_lock);

	/*
	 * This one might sleep, we cannot do it with a spinlock held ...
	 */
	retval = copy_to_user(param, &lp, sizeof(*param)) ? -EFAULT : 0;

out_nounlock:
	return retval;

out_unlock:
	read_unlock(&tasklist_lock);
	return retval;
}

/**
 * sys_sched_setaffinity - set the cpu affinity of a process
 * @pid: pid of the process
 * @len: length in bytes of the bitmask pointed to by user_mask_ptr
 * @user_mask_ptr: user-space pointer to the new cpu mask
 */
asmlinkage long sys_sched_setaffinity(pid_t pid, unsigned int len,
				      unsigned long __user *user_mask_ptr)
{
	cpumask_t new_mask;
	int retval;
	task_t *p;

	if (len < sizeof(new_mask))
		return -EINVAL;

	if (copy_from_user(&new_mask, user_mask_ptr, sizeof(new_mask)))
		return -EFAULT;

	lock_cpu_hotplug();
	read_lock(&tasklist_lock);

	p = find_process_by_pid(pid);
	if (!p) {
		read_unlock(&tasklist_lock);
		unlock_cpu_hotplug();
		return -ESRCH;
	}

	/*
	 * It is not safe to call set_cpus_allowed with the
	 * tasklist_lock held.  We will bump the task_struct's
	 * usage count and then drop tasklist_lock.
	 */
	get_task_struct(p);
	read_unlock(&tasklist_lock);

	retval = -EPERM;
	if ((current->euid != p->euid) && (current->euid != p->uid) &&
			!capable(CAP_SYS_NICE))
		goto out_unlock;

	retval = set_cpus_allowed(p, new_mask);

out_unlock:
	ma_put_task_struct(p);
	unlock_cpu_hotplug();
	return retval;
}

/**
 * sys_sched_getaffinity - get the cpu affinity of a process
 * @pid: pid of the process
 * @len: length in bytes of the bitmask pointed to by user_mask_ptr
 * @user_mask_ptr: user-space pointer to hold the current cpu mask
 */
asmlinkage long sys_sched_getaffinity(pid_t pid, unsigned int len,
				      unsigned long __user *user_mask_ptr)
{
	unsigned int real_len;
	cpumask_t mask;
	int retval;
	task_t *p;

	real_len = sizeof(mask);
	if (len < real_len)
		return -EINVAL;

	read_lock(&tasklist_lock);

	retval = -ESRCH;
	p = find_process_by_pid(pid);
	if (!p)
		goto out_unlock;

	retval = 0;
	cpus_and(mask, p->cpus_allowed, cpu_possible_map);

out_unlock:
	read_unlock(&tasklist_lock);
	if (retval)
		return retval;
	if (copy_to_user(user_mask_ptr, &mask, real_len))
		return -EFAULT;
	return real_len;
}

/**
 * sys_sched_yield - yield the current processor to other threads.
 *
 * this function yields the current CPU by moving the calling thread
 * to the expired array. If there are no other threads running on this
 * CPU then this function will return.
 */
asmlinkage long sys_sched_yield(void)
{
	ma_runqueue_t *rq = this_rq_lock();
	ma_prio_array_t *array = current->array;

	/*
	 * We implement yielding by moving the task into the expired
	 * queue.
	 *
	 * (special rule: RT tasks will just roundrobin in the active
	 *  array.)
	 */
	if (likely(!rt_task(current))) {
		dequeue_task(current, array);
		enqueue_task(current, rq->expired);
	} else {
		list_del(&current->run_list);
		list_add_tail(&current->run_list, array->queue + current->prio);
	}
	/*
	 * Since we are going to call schedule() anyway, there's
	 * no need to preempt:
	 */
	_raw_spin_unlock(&rq->lock);
	preempt_enable_no_resched();

	schedule();

	return 0;
}
#endif

void __ma_cond_resched(void)
{
	ma_set_current_state(MA_TASK_RUNNING);
	ma_schedule();
}

MARCEL_INT(__ma_cond_resched);

#if 0
/**
 * yield - yield the current processor to other threads.
 *
 * this is a shortcut for kernel-space yielding - it marks the
 * thread runnable and calls sys_sched_yield().
 */
void ma_yield(void)
{
	set_current_state(TASK_RUNNING);
	sys_sched_yield();
}

EXPORT_SYMBOL(yield);

/*
 * This task is about to go to sleep on IO.  Increment rq->nr_iowait so
 * that process accounting knows that this is a task in IO wait state.
 *
 * But don't do that if it is a deliberate, throttling IO wait (this task
 * has set its backing_dev_info: the queue against which it should throttle)
 */
void io_schedule(void)
{
	ma_runqueue_t *rq = ma_this_rq();

	atomic_inc(&rq->nr_iowait);
	schedule();
	atomic_dec(&rq->nr_iowait);
}

EXPORT_SYMBOL(io_schedule);

long io_schedule_timeout(long timeout)
{
	ma_runqueue_t *rq = ma_this_rq();
	long ret;

	atomic_inc(&rq->nr_iowait);
	ret = schedule_timeout(timeout);
	atomic_dec(&rq->nr_iowait);
	return ret;
}

/**
 * sys_sched_get_priority_max - return maximum RT priority.
 * @policy: scheduling class.
 *
 * this syscall returns the maximum rt_priority that can be used
 * by a given scheduling class.
 */
asmlinkage long sys_sched_get_priority_max(int policy)
{
	int ret = -EINVAL;

	switch (policy) {
	case SCHED_FIFO:
	case SCHED_RR:
		ret = MA_MAX_USER_RT_PRIO-1;
		break;
	case SCHED_NORMAL:
		ret = 0;
		break;
	}
	return ret;
}

/**
 * sys_sched_get_priority_min - return minimum RT priority.
 * @policy: scheduling class.
 *
 * this syscall returns the minimum rt_priority that can be used
 * by a given scheduling class.
 */
asmlinkage long sys_sched_get_priority_min(int policy)
{
	int ret = -EINVAL;

	switch (policy) {
	case SCHED_FIFO:
	case SCHED_RR:
		ret = 1;
		break;
	case SCHED_NORMAL:
		ret = 0;
	}
	return ret;
}

/**
 * sys_sched_rr_get_interval - return the default timeslice of a process.
 * @pid: pid of the process.
 * @interval: userspace pointer to the timeslice value.
 *
 * this syscall writes the default timeslice value of a given process
 * into the user-space timespec buffer. A value of '0' means infinity.
 */
asmlinkage long sys_sched_rr_get_interval(pid_t pid, struct timespec __user *interval)
{
	int retval = -EINVAL;
	struct timespec t;
	task_t *p;

	if (pid < 0)
		goto out_nounlock;

	retval = -ESRCH;
	read_lock(&tasklist_lock);
	p = find_process_by_pid(pid);
	if (!p)
		goto out_unlock;

	retval = security_task_getscheduler(p);
	if (retval)
		goto out_unlock;

	jiffies_to_timespec(p->policy & SCHED_FIFO ?
				0 : task_timeslice(p), &t);
	read_unlock(&tasklist_lock);
	retval = copy_to_user(interval, &t, sizeof(t)) ? -EFAULT : 0;
out_nounlock:
	return retval;
out_unlock:
	read_unlock(&tasklist_lock);
	return retval;
}

static inline struct task_struct *eldest_child(struct task_struct *p)
{
	if (list_empty(&p->children)) return NULL;
	return list_entry(p->children.next,struct task_struct,sibling);
}

static inline struct task_struct *older_sibling(struct task_struct *p)
{
	if (p->sibling.prev==&p->parent->children) return NULL;
	return list_entry(p->sibling.prev,struct task_struct,sibling);
}

static inline struct task_struct *younger_sibling(struct task_struct *p)
{
	if (p->sibling.next==&p->parent->children) return NULL;
	return list_entry(p->sibling.next,struct task_struct,sibling);
}

static void show_task(task_t * p)
{
	unsigned long free = 0;
	task_t *relative;
	int state;
	static const char * stat_nam[] = { "R", "S", "D", "T", "Z", "W" };

	printk("%-13.13s ", p->comm);
	state = p->state ? __ffs(p->state) + 1 : 0;
	if (((unsigned) state) < sizeof(stat_nam)/sizeof(char *))
		printk(stat_nam[state]);
	else
		printk(" ");
#if (BITS_PER_LONG == 32)
	if (p == current)
		printk(" current  ");
	else
		printk(" %08lX ", thread_saved_pc(p));
#else
	if (p == current)
		printk("   current task   ");
	else
		printk(" %016lx ", thread_saved_pc(p));
#endif
	{
		unsigned long * n = (unsigned long *) (p->thread_info+1);
		while (!*n)
			n++;
		free = (unsigned long) n - (unsigned long)(p->thread_info+1);
	}
	printk("%5lu %5d %6d ", free, p->pid, p->parent->pid);
	if ((relative = eldest_child(p)))
		printk("%5d ", relative->pid);
	else
		printk("      ");
	if ((relative = younger_sibling(p)))
		printk("%7d", relative->pid);
	else
		printk("       ");
	if ((relative = older_sibling(p)))
		printk(" %5d", relative->pid);
	else
		printk("      ");
	if (!p->mm)
		printk(" (L-TLB)\n");
	else
		printk(" (NOTLB)\n");

	show_stack(p, NULL);
}

void show_state(void)
{
	task_t *g, *p;

#if (BITS_PER_LONG == 32)
	printk("\n"
	       "                         free                        sibling\n");
	printk("  task             PC    stack   pid father child younger older\n");
#else
	printk("\n"
	       "                                 free                        sibling\n");
	printk("  task                 PC        stack   pid father child younger older\n");
#endif
	read_lock(&tasklist_lock);
	do_each_thread(g, p) {
		/*
		 * reset the NMI-timeout, listing all files on a slow
		 * console might take alot of time:
		 */
		touch_nmi_watchdog();
		show_task(p);
	} while_each_thread(g, p);

	read_unlock(&tasklist_lock);
}

void __marcel_init init_idle(task_t *idle, int cpu)
{
	ma_runqueue_t *idle_rq = cpu_rq(cpu), *rq = cpu_rq(task_cpu(idle));
	unsigned long flags;

	local_irq_save(flags);
	double_rq_lock(idle_rq, rq);

	// supprimé
	//idle_rq->curr = idle_rq->idle = idle;
	deactivate_task(idle, rq);
	idle->array = NULL;
	idle->prio = MA_MAX_PRIO;
	idle->state = TASK_RUNNING;
	set_task_cpu(idle, cpu);
	double_rq_unlock(idle_rq, rq);
	set_tsk_need_resched(idle);
	local_irq_restore(flags);

	/* Set the preempt count _outside_ the spinlocks! */
#ifdef CONFIG_PREEMPT
	idle->thread_info->preempt_count = (idle->lock_depth >= 0);
#else
	idle->thread_info->preempt_count = 0;
#endif
}

#ifdef CONFIG_SMP
/*
 * This is how migration works:
 *
 * 1) we queue a migration_req_t structure in the source CPU's
 *    runqueue and wake up that CPU's migration thread.
 * 2) we down() the locked semaphore => thread blocks.
 * 3) migration thread wakes up (implicitly it forces the migrated
 *    thread off the CPU)
 * 4) it gets the migration request and checks whether the migrated
 *    task is still in the wrong runqueue.
 * 5) if it's in the wrong runqueue then the migration thread removes
 *    it and puts it into the right queue.
 * 6) migration thread up()s the semaphore.
 * 7) we wake up and the migration is done.
 */

/*
 * Change a given task's CPU affinity. Migrate the thread to a
 * proper CPU and schedule it away if the CPU it's executing on
 * is removed from the allowed bitmask.
 *
 * NOTE: the caller must have a valid reference to the task, the
 * task must not exit() & deallocate itself prematurely.  The
 * call is not atomic; no spinlocks may be held.
 */
int set_cpus_allowed(task_t *p, cpumask_t new_mask)
{
	unsigned long flags;
	int ret = 0;
	migration_req_t req;
	ma_runqueue_t *rq;

	rq = task_rq_lock(p, &flags);
	if (any_online_cpu(new_mask) == NR_CPUS) {
		ret = -EINVAL;
		goto out;
	}

	if (__set_cpus_allowed(p, new_mask, &req)) {
		/* Need help from migration thread: drop lock and wait. */
		task_rq_unlock(rq, &flags);
		wake_up_process(rq->migration_thread);
		wait_for_completion(&req.done);
		return 0;
	}
 out:
	task_rq_unlock(rq, &flags);
	return ret;
}

EXPORT_SYMBOL_GPL(set_cpus_allowed);

/* Move (not current) task off this cpu, onto dest cpu. */
static void move_task_away(struct task_struct *p, int dest_cpu)
{
	ma_runqueue_t *rq_dest;

	rq_dest = cpu_rq(dest_cpu);

	double_rq_lock(ma_this_rq(), rq_dest);
	if (task_cpu(p) != smp_processor_id())
		goto out; /* Already moved */

	set_task_cpu(p, dest_cpu);
	if (p->array) {
		deactivate_task(p, ma_this_rq());
		activate_task(p, rq_dest);
		// TODO: plus de curr
		if (p->prio < rq_dest->curr->prio)
			resched_task(rq_dest->curr);
	}
	p->timestamp = rq_dest->timestamp_last_tick;

 out:
	double_rq_unlock(ma_this_rq(), rq_dest);
	local_irq_restore(flags);
}

/*
 * migration_thread - this is a highprio system thread that performs
 * thread migration by bumping thread off CPU then 'pushing' onto
 * another runqueue.
 */
static int migration_thread(void * data)
{
	ma_runqueue_t *rq;
	int cpu = (long)data;

	rq = cpu_rq(cpu);
	BUG_ON(rq->migration_thread != current);

	while (!kthread_should_stop()) {
		struct list_head *head;
		migration_req_t *req;

		if (current->flags & PF_FREEZE)
			refrigerator(PF_IOTHREAD);

		spin_lock_irq(&rq->lock);
		head = &rq->migration_queue;
		current->state = TASK_INTERRUPTIBLE;
		if (list_empty(head)) {
			spin_unlock(&rq->lock);
			schedule();
			continue;
		}
		req = list_entry(head->next, migration_req_t, list);
		list_del_init(head->next);
		spin_unlock_irq(&rq->lock);

		move_task_away(req->task,
			       any_online_cpu(req->task->cpus_allowed));
		local_irq_enable();
		complete(&req->done);
	}
	return 0;
}

#ifdef CONFIG_HOTPLUG_CPU
/* migrate_all_tasks - function to migrate all the tasks from the
 * current cpu caller must have already scheduled this to the target
 * cpu via set_cpus_allowed.  Machine is stopped.  */
void migrate_all_tasks(void)
{
	struct task_struct *tsk, *t;
	int dest_cpu, src_cpu;
	unsigned int node;

	/* We're nailed to this CPU. */
	src_cpu = smp_processor_id();

	/* Not required, but here for neatness. */
	write_lock(&tasklist_lock);

	/* watch out for per node tasks, let's stay on this node */
	node = cpu_to_node(src_cpu);

	do_each_thread(t, tsk) {
		cpumask_t mask;
		if (tsk == current)
			continue;

		if (task_cpu(tsk) != src_cpu)
			continue;

		/* Figure out where this task should go (attempting to
		 * keep it on-node), and check if it can be migrated
		 * as-is.  NOTE that kernel threads bound to more than
		 * one online cpu will be migrated. */
		mask = node_to_cpumask(node);
		cpus_and(mask, mask, tsk->cpus_allowed);
		dest_cpu = any_online_cpu(mask);
		if (dest_cpu == NR_CPUS)
			dest_cpu = any_online_cpu(tsk->cpus_allowed);
		if (dest_cpu == NR_CPUS) {
			cpus_clear(tsk->cpus_allowed);
			cpus_complement(tsk->cpus_allowed);
			dest_cpu = any_online_cpu(tsk->cpus_allowed);

			/* Don't tell them about moving exiting tasks
			   or kernel threads (both mm NULL), since
			   they never leave kernel. */
			if (tsk->mm && printk_ratelimit())
				printk(KERN_INFO "process %d (%s) no "
				       "longer affine to cpu%d\n",
				       tsk->pid, tsk->comm, src_cpu);
		}

		move_task_away(tsk, dest_cpu);
	} while_each_thread(t, tsk);

	write_unlock(&tasklist_lock);
}
#endif /* CONFIG_HOTPLUG_CPU */

/*
 * migration_call - callback that gets triggered when a CPU is added.
 * Here we can start up the necessary migration thread for the new CPU.
 */
static int migration_call(struct notifier_block *nfb,
			  unsigned long action,
			  void *hcpu)
{
	int cpu = (long) hcpu;
	struct task_struct *p;
	ma_runqueue_t *rq;
	unsigned long flags;

	switch (action) {
	case CPU_UP_PREPARE:
		p = kthread_create(migration_thread, hcpu, "migration/%d",cpu);
		if (IS_ERR(p))
			return NOTIFY_BAD;
		kthread_bind(p, cpu);
		/* Must be high prio: stop_machine expects to yield to it. */
		rq = task_rq_lock(p, &flags);
		__setscheduler(p, SCHED_FIFO, MA_MAX_RT_PRIO-1);
		task_rq_unlock(rq, &flags);
		cpu_rq(cpu)->migration_thread = p;
		break;
	case CPU_ONLINE:
		/* Strictly unneccessary, as first user will wake it. */
		wake_up_process(cpu_rq(cpu)->migration_thread);
 		break;
#ifdef CONFIG_HOTPLUG_CPU
	case CPU_UP_CANCELED:
		/* Unbind it from offline cpu so it can run.  Fall thru. */
		kthread_bind(cpu_rq(cpu)->migration_thread,smp_processor_id());
	case CPU_DEAD:
		kthread_stop(cpu_rq(cpu)->migration_thread);
		cpu_rq(cpu)->migration_thread = NULL;
 		BUG_ON(cpu_rq(cpu)->nr_running != 0);
 		break;
#endif
	}
	return NOTIFY_OK;
}

static struct notifier_block __devinitdata migration_notifier = {
	.notifier_call = migration_call,
};

int __marcel_init migration_init(void)
{
	void *cpu = (void *)(long)smp_processor_id();
	/* Start one for boot CPU. */
	migration_call(&migration_notifier, CPU_UP_PREPARE, cpu);
	migration_call(&migration_notifier, CPU_ONLINE, cpu);
	register_cpu_notifier(&migration_notifier);
	return 0;
}
#endif

/*
 * The 'big kernel lock'
 *
 * This spinlock is taken and released recursively by lock_kernel()
 * and unlock_kernel().  It is transparently dropped and reaquired
 * over schedule().  It is used to protect legacy code that hasn't
 * been migrated to a proper locking design yet.
 *
 * Don't use in new code.
 *
 * Note: spinlock debugging needs this even on !CONFIG_SMP.
 */
spinlock_t kernel_flag __cacheline_aligned_in_smp = SPIN_LOCK_UNLOCKED;
EXPORT_SYMBOL(kernel_flag);

#endif /* 0 */

static void linux_sched_lwp_init(ma_lwp_t lwp)
{
	LOG_IN();
	/* en mono, rien par lwp, tout est initialisé dans sched_init */
#ifdef MA__LWPS
	init_rq(ma_lwp_rq(lwp), MA_LWP_RQ);
	init_rq(&ma_per_lwp(dontsched_runqueue,lwp), MA_DONTSCHED_RQ);
	ma_per_lwp(current_thread,lwp) = ma_per_lwp(run_task,lwp);
#ifdef MA__SMP
	MA_CPU_ZERO(&(ma_lwp_rq(lwp)->cpuset));
	MA_CPU_SET(LWP_NUMBER(lwp),&(ma_lwp_rq(lwp)->cpuset));
	MA_CPU_ZERO(&(ma_per_lwp(dontsched_runqueue,lwp).cpuset));
	MA_CPU_SET(LWP_NUMBER(lwp),&(ma_per_lwp(dontsched_runqueue,lwp).cpuset));
	MA_CPU_SET(LWP_NUMBER(lwp),&ma_main_runqueue.cpuset);
	MA_CPU_SET(LWP_NUMBER(lwp),&ma_dontsched_runqueue.cpuset);
#endif
#endif
	LOG_OUT();
}

static void linux_sched_lwp_start(ma_lwp_t lwp)
{
	ma_runqueue_t *rq;
	marcel_task_t *p = ma_per_lwp(run_task,lwp);
	/* Cette tâche est en train d'être exécutée */
	rq=task_rq_lock(p);
	activate_running_task(p, rq);
	task_rq_unlock(rq);
}

MA_DEFINE_LWP_NOTIFIER_START_PRIO(linux_sched, 200, "Linux scheduler",
				  linux_sched_lwp_init, "Initialisation des runqueue",
				  linux_sched_lwp_start, "Activation de la tâche courante");

MA_LWP_NOTIFIER_CALL_UP_PREPARE(linux_sched, MA_INIT_LINUX_SCHED);

void __marcel_init sched_init(void)
{
	LOG_IN();

	init_rq(&ma_main_runqueue, MA_MACHINE_RQ);
	init_rq(&ma_dontsched_runqueue, MA_DONTSCHED_RQ);
#ifdef MA__LWPS
	MA_CPU_ZERO(&ma_main_runqueue.cpuset);
	MA_CPU_ZERO(&ma_dontsched_runqueue.cpuset);
#endif

	/*
	 * We have to do a little magic to get the first
	 * thread right in SMP mode.
	 */
#ifndef MA__LWPS
	ma_per_lwp__current_thread = MARCEL_SELF;
#endif
	ma_wake_up_created_thread(MARCEL_SELF);
	/* since it is actually already running */
	ma_spin_lock(&ma_main_runqueue.lock);
	dequeue_task(MARCEL_SELF, MARCEL_SELF->sched.internal.array);
	ma_spin_unlock(&ma_main_runqueue.lock);

#ifdef MA__NUMA
	/* initialiser des runqueues par noeud / core / ht */
#endif

//	init_timers();

	/*
	 * The boot idle thread does lazy MMU switching as well:
	 */
//	atomic_inc(&init_mm.mm_count);
//	enter_lazy_tlb(&init_mm, current);
	LOG_OUT();
}

__ma_initfunc(sched_init, MA_INIT_LINUX_SCHED,
	      "Scheduler Linux 2.6");

#if 0
#ifdef CONFIG_DEBUG_SPINLOCK_SLEEP
void __might_sleep(char *file, int line)
{
#if defined(in_atomic)
	static unsigned long prev_jiffy;	/* ratelimiting */

	if ((in_atomic() || irqs_disabled()) && system_running) {
		if (time_before(jiffies, prev_jiffy + HZ) && prev_jiffy)
			return;
		prev_jiffy = jiffies;
		printk(KERN_ERR "Debug: sleeping function called from invalid"
				" context at %s:%d\n", file, line);
		printk("in_atomic():%d, irqs_disabled():%d\n",
		       in_atomic(), irqs_disabled());
		dump_stack();
	}
#endif
}
EXPORT_SYMBOL(__might_sleep);
#endif
#endif /* 0 */

#if defined(MA__LWPS)
/*
 * This could be a long-held lock.  If another CPU holds it for a long time,
 * and that CPU is not asked to reschedule then *this* CPU will spin on the
 * lock for a long time, even if *this* CPU is asked to reschedule.
 *
 * So what we do here, in the slow (contended) path is to spin on the lock by
 * hand while permitting preemption.
 *
 * Called inside preempt_disable().
 */
void __ma_preempt_spin_lock(ma_spinlock_t *lock)
{
	if (ma_preempt_count() > 1) {
		_ma_raw_spin_lock(lock);
		return;
	}
	do {
		ma_preempt_enable();
		while (ma_spin_is_locked(lock))
			cpu_relax();
		ma_preempt_disable();
	} while (!_ma_raw_spin_trylock(lock));
}

MARCEL_INT(__ma_preempt_spin_lock);

void __ma_preempt_write_lock(ma_rwlock_t *lock)
{
	if (ma_preempt_count() > 1) {
		_ma_raw_write_lock(lock);
		return;
	}

	do {
		ma_preempt_enable();
		while (ma_rwlock_is_locked(lock))
			cpu_relax();
		ma_preempt_disable();
	} while (!_ma_raw_write_trylock(lock));
}

MARCEL_INT(__ma_preempt_write_lock);
#endif 

