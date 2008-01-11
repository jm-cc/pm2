
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

#ifdef PIOMAN
#include "pioman.h"
#endif /* PIOMAN */
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

#define TASK_TASK_PREEMPT(p, q) \
	((q)->sched.internal.entity.prio - (p)->sched.internal.entity.prio)
#define TASK_PREEMPTS_TASK(p, q) \
	TASK_TASK_PREEMPT(p, q) > 0
#define TASK_PREEMPTS_CURR(p, lwp) \
	TASK_PREEMPTS_TASK((p), ma_per_lwp(current_thread, (lwp)))
#define TASK_CURR_PREEMPT(p, lwp) \
	TASK_TASK_PREEMPT((p), ma_per_lwp(current_thread, (lwp)))


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

#ifndef MA__LWPS
/* mono: no idle thread, but on interrupts we need to when whether we're
 * idling. */
static int currently_idle;
#endif

/*
 * ma_resched_task - mark a task 'to be rescheduled now'.
 *
 * On UP this means the setting of the need_resched flag, on SMP it
 * might also involve a LWP killing to trigger the scheduler on
 * the target LWP.
 */
void ma_resched_task(marcel_task_t *p, int vp, ma_lwp_t lwp)
{
	PROF_EVENT2(sched_resched_task, p, vp);
#ifdef MA__LWPS
	ma_preempt_disable();
	
	if (tbx_unlikely(ma_topo_vpdata(&marcel_topo_vp_level[vp], need_resched))) {
		PROF_EVENT(sched_already_resched);
		goto out;
	}

	ma_topo_vpdata(&marcel_topo_vp_level[vp], need_resched) = 1;

	if (lwp == LWP_SELF) {
		PROF_EVENT(sched_thats_us);
		goto out;
	}

	/* NEED_RESCHED must be visible before we test POLLING_NRFLAG */
	ma_smp_mb();
	/* minimise the chance of sending an interrupt to poll_idle() */
	if (!ma_test_tsk_thread_flag(p,TIF_POLLING_NRFLAG)) {

               PROF_EVENT2(sched_resched_lwp, LWP_NUMBER(LWP_SELF), LWP_NUMBER(lwp));
		MA_LWP_RESCHED(lwp);
	} else PROF_EVENT2(sched_resched_lwp_already_polling, p, LWP_NUMBER(lwp));
out:
	ma_preempt_enable();
#else
	ma_topo_vpdata(&marcel_topo_vp_level[vp], need_resched) = 1;
#endif
}

/**
 * task_curr - is this task currently executing on a CPU?
 * @p: the task in question.
 */
int marcel_task_curr(marcel_task_t *p)
{
	return ma_lwp_curr(ma_task_lwp(p)) == p;
}

/* tries to resched the given task in the given holder */
static void try_to_resched(marcel_task_t *p, ma_holder_t *h)
{
	marcel_lwp_t *lwp, *chosen = NULL;
	ma_runqueue_t *rq = ma_to_rq_holder(h);
	int max_preempt = 0, preempt, i, chosenvp = -1;
	if (!rq)
		return;
	for (i=0; i<marcel_nbvps()+MARCEL_NBMAXVPSUP; i++) {
		lwp = GET_LWP_BY_NUM(i);
		if (lwp && (ma_rq_covers(rq, i) || rq == ma_lwp_rq(lwp))) {
			preempt = TASK_CURR_PREEMPT(p, lwp);
			if (preempt > max_preempt) {
				max_preempt = preempt;
				chosen = lwp;
				chosenvp = i;
			}
		}
	}
	if (chosen)
		ma_resched_task(ma_per_lwp(current_thread, chosen), chosenvp, chosen);
	else
		PROF_EVENT2(couldnt_resched_task, p, h);
}

/***
 * ma_try_to_wake_up - wake up a thread
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
int __ma_try_to_wake_up(marcel_task_t * p, unsigned int state, int sync, ma_holder_t *h)
{
	int success = 0;
	long old_state;
	ma_runqueue_t *rq;
	LOG_IN();

//repeat_lock_task:
	old_state = p->sched.state;
	if (old_state & state) {
		/* on s'occupe de la réveiller */
		PROF_EVENT2(sched_thread_wake, p, old_state);
		p->sched.state = MA_TASK_RUNNING;
		if (MA_TASK_IS_BLOCKED(p)) { /* not running or runnable */
			PROF_EVENT(sched_thread_wake_unblock);
			/*
			 * Fast-migrate the task if it's not running or runnable
			 * currently. Do not violate hard affinity.
			 */
#ifdef PM2_DEV
#warning TODO
#endif
			#if 0
			if (tbx_unlikely(sync && /*!MA_TASK_IS_RUNNING(p) inutile &&*/
				(ma_task_lwp(p) != LWP_SELF)
				&& lwp_isset(LWP_NUMBER(LWP_SELF),
					  p->sched.lwps_allowed)
				&& ma_lwp_online(LWP_SELF)
				)) {

				//ma_deactivate_task(p,h);
				//ma_activate_task(p,&ma_lwp_vprq(LWP_SELF)->hold);

				//réalisé par ma_schedule()
				//ma_set_task_lwp(p, LWP_SELF);

				ma_task_holder_unlock_softirq(h);
				goto repeat_lock_task;
			}
			#endif
			if (old_state == MA_TASK_UNINTERRUPTIBLE){
				h->nr_uninterruptible--;
				/*
				 * Tasks on involuntary sleep don't earn
				 * sleep_avg beyond just interactive state.
				 */
//				p->activated = -1;
			}

			/* Attention ici: h peut être une bulle, auquel cas
			 * activate_task peut lâcher la bulle pour verrouiller
			 * une runqueue */
			ma_activate_task(p, h);
			/*
			 * Sync wakeups (i.e. those types of wakeups where the waker
			 * has indicated that it will leave the CPU in short order)
			 * don't trigger a preemption, if the woken up task will run on
			 * this cpu. (in this case the 'I will reschedule' promise of
			 * the waker guarantees that the freshly woken up task is going
			 * to be considered on this CPU.)
			 */
		}

		rq = ma_to_rq_holder(h);
		if (rq && ma_rq_covers(rq, LWP_NUMBER(LWP_SELF)) && TASK_PREEMPTS_TASK(p, MARCEL_SELF)) {
			/* we can avoid remote reschedule by switching to it */
			if (!sync) /* only if we won't for sure yield() soon */
				ma_resched_task(MARCEL_SELF, LWP_NUMBER(LWP_SELF), LWP_SELF);
		} else try_to_resched(p, h);

		success = 1;
	}

	LOG_RETURN(success);
}

int ma_try_to_wake_up(marcel_task_t * p, unsigned int state, int sync)
{
	int success;
	ma_holder_t *h = ma_task_holder_lock_softirq(p);
	success = __ma_try_to_wake_up(p, state, sync, h);
	ma_task_holder_unlock_softirq(h);
	return success;
}

TBX_EXTERN int fastcall ma_wake_up_thread(marcel_task_t * p)
{
	return ma_try_to_wake_up(p, MA_TASK_INTERRUPTIBLE |
			      MA_TASK_UNINTERRUPTIBLE, 0);
}

int fastcall ma_wake_up_state(marcel_task_t *p, unsigned int state)
{
	return ma_try_to_wake_up(p, state, 0);
}

int fastcall ma_wake_up_thread_async(marcel_task_t * p)
{
	int success;
	ma_holder_t *h = ma_task_holder_lock_softirq_async(p);
	if (!h) {
		ma_task_holder_unlock_softirq(h);
		return 0;
	}
	success = __ma_try_to_wake_up(p, MA_TASK_INTERRUPTIBLE |
				MA_TASK_UNINTERRUPTIBLE, 1, h);
	ma_task_holder_unlock_softirq(h);
	return success;
}

/*
 * wake_up_forked_process - wake up a freshly forked process.
 *
 * This function will do some initial scheduler statistics housekeeping
 * that must be done for every newly created process.
 */
void marcel_wake_up_created_thread(marcel_task_t * p)
{
	ma_holder_t *h;
	ma_runqueue_t *rq;
	LOG_IN();

	sched_debug("wake up created thread %p\n",p);
	if (ma_entity_task(p)->type == MA_THREAD_ENTITY) {
		MA_BUG_ON(p->sched.state != MA_TASK_BORNING);

		ma_task_stats_set(long, p, ma_stats_last_ran_offset, marcel_clock());
		PROF_EVENT2(sched_thread_wake, p, p->sched.state);
		ma_set_task_state(p, MA_TASK_RUNNING);
	}

#ifdef MA__BUBBLES
	h = ma_task_init_holder(p);

	if (h && ma_holder_type(h) != MA_RUNQUEUE_HOLDER) {
		bubble_sched_debugl(7,"wake up task %p in bubble %p\n",p, ma_bubble_holder(h));
		if (list_empty(&p->sched.internal.entity.bubble_entity_list))
			marcel_bubble_inserttask(ma_bubble_holder(h),p);
	}
#endif

	h = ma_task_sched_holder(p);

#ifdef MA__BUBBLES
	if (h->type == MA_BUBBLE_HOLDER) {
		/* Don't directly enqueue in holding bubble, but in the thread cache. */
		marcel_bubble_t *b = ma_bubble_holder(h);
		ma_holder_t *hh;
		while ((hh = b->sched.sched_holder) && hh->type == MA_BUBBLE_HOLDER) {
			h = hh;
			b = ma_bubble_holder(hh);
		}
		ma_task_sched_holder(p) = h;
	}
#endif

	MA_BUG_ON(!h);
	ma_holder_lock_softirq(h);

	/*
	 * We decrease the sleep average of forking parents
	 * and children as well, to keep max-interactive tasks
	 * from forking tasks that are max-interactive.
	 */
//	p->prio = effective_prio(p);
//	ma_set_task_lwp(p, LWP_SELF);

	/* il est possible de démarrer sur une autre rq que celle de SELF,
	 * on ne peut donc pas profiter de ses valeurs */
//	if (tbx_unlikely(!SELF_GETMEM(sched).internal.entity.array))
	if (MA_TASK_IS_BLOCKED(p))
		ma_activate_task(p, h);
//	else {
//		p->sched.internal.entity.prio = SELF_GETMEM(sched).internal.entity.prio;
//		list_add_tail(&p->sched.internal.entity.run_list,
//			      &SELF_GETMEM(sched).internal.entity.run_list);
//		p->sched.internal.entity.data = SELF_GETMEM(sched).internal.entity.data;
//		p->sched.internal.entity.data->nr_active++;
//#ifdef MA__LWPS
//		p->sched.internal.entity.cur_holder = &rq.hold;
//#endif
//		rq->hold.running++;
//	}
	ma_holder_unlock_softirq(h);
	// on donne la main aussitôt, bien souvent le meilleur choix
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		PROF_EVENT2(bubble_sched_switchrq, p, ma_rq_holder(h));
	rq = ma_rq_holder(h);
	if (ma_entity_task(p)->type == MA_THREAD_ENTITY
	    && ma_holder_type(h) == MA_RUNQUEUE_HOLDER && !ma_in_atomic()
	    && ma_rq_covers(rq, LWP_NUMBER(LWP_SELF))) {
		/* XXX: on pourrait être préempté entre-temps... */
		PROF_EVENT(exec_woken_up_created_thread);
		ma_schedule();
	} else {
		PROF_EVENT(resched_for_woken_up_created_thread);
		try_to_resched(p, h);
	}
	LOG_OUT();
}

int ma_sched_change_prio(marcel_t t, int prio) {
	ma_holder_t *h;
	int requeue;
	/* attention ici. Pour l'instant on n'a pas besoin de requeuer dans
	 * une bulle */
	/* quand on le voudra, il faudra faire attention que dequeue_task peut
	 * vouloir déverouiller la bulle pour pouvoir verrouiller la runqueue */
	MA_BUG_ON(prio < 0 || prio >= MA_MAX_PRIO);
	PROF_EVENT2(sched_setprio,t,prio);
	h = ma_task_holder_lock_softirq(t);
	if ((requeue = (MA_TASK_IS_READY(t) &&
			ma_holder_type(h) == MA_RUNQUEUE_HOLDER)))
		ma_dequeue_task(t,h);
	t->sched.internal.entity.prio=prio;
	if (requeue)
		ma_enqueue_task(t,h);
	ma_task_holder_unlock_softirq(h);
	return 0;
}

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
static void finish_task_switch(marcel_task_t *prev)
{
	/* note: pas de softirq ici, on est déjà en mode interruption */
	ma_holder_t *prevh = ma_task_holder_rawlock(prev);
	unsigned long prev_task_state;
#ifdef MA__BUBBLES
	ma_holder_t *h;
#endif

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
	prev_task_state = prev->sched.state;

	if (prev->sched.state && ((prev->sched.state == MA_TASK_DEAD)
				|| !(THREAD_GETMEM(prev,preempt_count) & MA_PREEMPT_ACTIVE))
			) {
		if (prev->sched.state & MA_TASK_MOVING) {
			/* moving, make it running elsewhere */
			MTRACE("moving",prev);
			PROF_EVENT2(sched_thread_wake, prev, prev->sched.state);
			ma_set_task_state(prev, MA_TASK_RUNNING);
			/* still runnable */
			ma_enqueue_task(prev,prevh);
			try_to_resched(prev, prevh);
		} else {
			/* yes, deactivate */
			MTRACE("going to sleep",prev);
			sched_debug("%p going to sleep\n",prev);
			ma_deactivate_running_task(prev,prevh);
		}
	} else {
		MTRACE("still running",prev);
		ma_enqueue_task(prev,prevh);
	}
	/* shouldn't already be running since we just left it ! */
	MA_BUG_ON(MA_TASK_IS_RUNNING(prev));
	/* No other choice */
	MA_BUG_ON(!(MA_TASK_IS_READY(prev) || MA_TASK_IS_BLOCKED(prev)));

#ifdef MA__BUBBLES
	if ((h = (ma_task_init_holder(prev)))
		&& h->type == MA_BUBBLE_HOLDER) {
		marcel_bubble_t *bubble = ma_bubble_holder(h);
		int remove_from_bubble;
		if ((remove_from_bubble = (prev->sched.state & MA_TASK_DEAD)))
			ma_task_sched_holder(prev) = NULL;
		ma_task_holder_unlock_softirq(prevh);
		/* Note: since preemption was not re-enabled (see ma_schedule()), prev thread can't vanish between releasing prevh above and bubble lock below. */
		if (remove_from_bubble)
			marcel_bubble_removetask(bubble, prev);
	} else
#endif
	{
		/* on sort du mode interruption */
		ma_task_holder_unlock_softirq(prevh);

	}
	if (tbx_unlikely(prev_task_state == MA_TASK_DEAD)) {
		PROF_THREAD_DEATH(prev);
		/* mourn it */
		marcel_funerals(prev);
	}
}

/**
 * schedule_tail - first thing a freshly forked thread must call.
 * @prev: the thread we just switched away from.
 */
asmlinkage void ma_schedule_tail(marcel_task_t *prev)
{
	finish_task_switch(prev);

#ifdef MA__LWPS
	if (tbx_unlikely(MARCEL_SELF == __ma_get_lwp_var(idle_task)))
		ma_entering_idle();
#endif
}

/*
 * nr_ready, nr_uninterruptible and nr_context_switches:
 *
 * externally visible scheduler statistics: current number of runnable
 * threads, current number of uninterruptible-sleeping threads, total
 * number of context switches performed since bootup.
 */
unsigned long ma_nr_ready(void)
{
	unsigned long i, sum = 0;

#ifdef PM2_DEV
#warning TODO: descendre dans les bulles ...
#endif
	for (i = 0; i < MA_NR_LWPS; i++)
		sum += ma_lwp_rq(GET_LWP_BY_NUM(i))->hold.nr_ready;
	sum += ma_main_runqueue.hold.nr_ready;

	return sum;
}

#if 0
/* a priori, c'est l'application qui rebalance les choses */
static inline void rebalance_tick(ma_runqueue_t *this_rq, int idle)
{
}
#endif

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
			STARVATION_LIMIT * ((rq)->nr_ready) + 1))) /*||*/ \
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
	struct ma_lwp_usage_stat *lwpstat = &__ma_get_lwp_var(lwp_usage);
	//ma_runqueue_t *rq;
	marcel_task_t *p = MARCEL_SELF;

	//LOG_IN();

	PROF_EVENT(sched_tick);

#ifdef PM2_DEV
#warning rcu not yet implemented (utile pour les numa ?)
#endif
	//if (rcu_pending(cpu))
	//      rcu_check_callbacks(cpu, user_ticks);

// TODO Pour l'instant, on n'a pas de notion de tick système
#define sys_ticks user_ticks
	if (ma_hardirq_count()) {
		lwpstat->irq += sys_ticks;
		sys_ticks = 0;
	/* note: this timer irq context must be accounted for as well */
	} else if (ma_softirq_count() - MA_SOFTIRQ_OFFSET) {
		lwpstat->softirq += sys_ticks;
		sys_ticks = 0;
	}

#ifdef MA__LWPS
	if (p->sched.internal.entity.prio == MA_IDLE_PRIO)
#else
	if (currently_idle)
#endif
	{
		// TODO on n'a pas non plus de notion d'iowait, est-ce qu'on le veut vraiment ?
		/*if (atomic_read(&rq->nr_iowait) > 0)
			lwpstat->iowait += sys_ticks;
		else*/
			lwpstat->idle += sys_ticks;
		//rebalance_tick(rq, 1);
		//return;
		sys_ticks = 0;
	} else if (p->sched.internal.entity.prio >= MA_BATCH_PRIO)
		lwpstat->nice += user_ticks;
	else
		lwpstat->user += user_ticks;
	//lwpstat->system += sys_ticks;
#undef sys_ticks

	/* Task might have expired already, but not scheduled off yet */
	//if (p->sched.internal.entity.array != rq->active) {
	
	// C'est normal quand le thread est en cours de migration par exemple. Il faudrait prendre le verrou pour faire cette vérification
	if (0 && !MA_TASK_IS_RUNNING(p)) {
		pm2debug("Strange: %s running, but not running (run_holder == %p, holder_data == %p) !, report or look at it (%s:%i)\n",
				p->name, ma_task_run_holder(p),
				ma_task_holder_data(p), __FILE__, __LINE__);
		ma_need_resched() = 1;
		goto out;
	}

	if (preemption_enabled() && ma_thread_preemptible()) {
		MA_BUG_ON(ma_atomic_read(&p->sched.internal.entity.time_slice)>MARCEL_TASK_TIMESLICE);
		if (ma_atomic_dec_and_test(&p->sched.internal.entity.time_slice)) {
			ma_need_resched() = 1;
			sched_debug("scheduler_tick: time slice expired\n");
			//p->prio = effective_prio(p);
			ma_atomic_set(&p->sched.internal.entity.time_slice,MARCEL_TASK_TIMESLICE);
					//task_timeslice(p);
			//p->first_time_slice = 0;

			//if (!rq->expired_timestamp)
				//rq->expired_timestamp = jiffies;
			//if (!TASK_INTERACTIVE(p) || EXPIRED_STARVING(rq)) {
	//			if (p->static_prio < rq->best_expired_prio)
	//				rq->best_expired_prio = p->static_prio;
			//}
		}
		// attention: rq->lock ne doit pas être pris pour pouvoir
		// verrouiller la bulle.
		{
#ifdef MA__BUBBLES
			marcel_bubble_t *b;
			ma_holder_t *h;
			if ((h = ma_task_init_holder(p)) && 
					ma_holder_type(h) != MA_RUNQUEUE_HOLDER) {
				b = ma_bubble_holder(h);
				if (ma_atomic_dec_and_test(&b->sched.time_slice) && current_sched->tick)
					current_sched->tick(b);
			}
#endif
		}
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
				//ma_dequeue_task(p, &rq->hold);
				ma_need_resched() = 1;
//				p->prio = effective_prio(p);
				//ma_enqueue_task(p, &rq->hold);
			}
		}
	}
out_unlock:
	ma_spin_unlock(&rq->lock);
#endif
out:
	(void)0;
	//rebalance_tick(rq, 0);
	//LOG_OUT();
}

void ma_scheduling_functions_start_here(void) { }


static marcel_t do_switch(marcel_t prev, marcel_t next, ma_holder_t *nexth, unsigned long now) {
	tbx_prefetch(next);

	/* update statistics */
	ma_task_stats_set(long, prev, ma_stats_last_ran_offset, now);
	ma_task_stats_set(long, next, ma_stats_nbrunning_offset, 1);
	ma_task_stats_set(long, prev, ma_stats_nbrunning_offset, 0);

	__ma_get_lwp_var(current_thread) = next;
	ma_dequeue_task(next, nexth);

	sched_debug("unlock(%p)\n",nexth);
	ma_holder_rawunlock(nexth);
	ma_set_task_lwp(next, LWP_SELF);

#ifdef MA__LWPS
	if (tbx_unlikely(prev == __ma_get_lwp_var(idle_task)))
		ma_leaving_idle();
#endif

	prev = marcel_switch_to(prev, next);
	ma_barrier();

	ma_schedule_tail(prev);

	return prev;
}

/*
 * schedule() is the main scheduler function.
 */
asmlinkage TBX_EXTERN int ma_schedule(void)
{
//	long *switch_count;
	marcel_task_t *prev, *cur, *next, *prev_as_next;
	marcel_entity_t *nextent;
	ma_runqueue_t *rq, *currq;
	ma_holder_t *prevh, *nexth, *prev_as_h;
	ma_prio_array_t *array;
	struct list_head *queue;
	unsigned long now;
	//unsigned long run_time;
	int idx;
	int max_prio, prev_as_prio;
	int go_to_sleep;
	int didswitch;
	int go_to_sleep_traced;
#ifndef MA__LWPS
	int didpoll = 0;
#endif
	int hard_preempt;
	int need_resched = ma_need_resched();
	LOG_IN();

	/*
	 * Test if we are atomic.  Since do_exit() needs to call into
	 * schedule() atomically, we ignore that path for now.
	 * Otherwise, whine if we are scheduling when we should not be.
	 */
	MA_BUG_ON(ma_preempt_count()<0);
	if (tbx_likely(!(SELF_GETMEM(sched).state & MA_TASK_DEAD))) {
		if (tbx_unlikely(ma_in_atomic())) {
			pm2debug("bad: scheduling while atomic (%06x)! Did you forget to unlock a spinlock?\n",ma_preempt_count());
			ma_show_preempt_backtrace();
			MA_BUG();
		}
	}

need_resched:
	/* Say the world we're currently rescheduling */
	ma_need_resched() = 0;
	ma_smp_mb__after_clear_bit();
	/* Now that we announced we're taking a decision, we can have a look at
	 * what is available */

	/* we need to disable bottom half to avoid running ma_schedule_tick() ! */
	ma_preempt_disable();
	ma_local_bh_disable();

	prev = MARCEL_SELF;
	MA_BUG_ON(!prev);

	now = marcel_clock();
	//if (likely(now - prev->timestamp < NS_MAX_SLEEP_AVG))
		//run_time = now - prev->sched.internal.timestamp;
	/*
	else
		run_time = NS_MAX_SLEEP_AVG;
		*/

	prevh = ma_task_run_holder(prev);

	go_to_sleep_traced = 0;
need_resched_atomic:
	/* Note: maybe we got woken up in the meanwhile */
	go_to_sleep = 0;
	/* by default, reschedule this thread */
	prev_as_next = prev;
	prev_as_h = prevh;
#ifdef MA__BUBBLES
	if (prev_as_h && prev_as_h->type != MA_RUNQUEUE_HOLDER) {
		/* the real priority is the holding bubble's */
		prev_as_prio = ma_bubble_holder(prev_as_h)->sched.prio;
	} else
#endif
		prev_as_prio = prev->sched.internal.entity.prio;

	if (prev->sched.state &&
			/* garde-fou pour éviter de s'endormir
			 * par simple préemption */
			((prev->sched.state == MA_TASK_DEAD) ||
			!(ma_preempt_count() & MA_PREEMPT_ACTIVE))) {
		//switch_count = &prev->nvcsw;
		if (tbx_unlikely((prev->sched.state & MA_TASK_INTERRUPTIBLE) &&
				 tbx_unlikely(0 /*work_pending(prev)*/)))
			prev->sched.state = MA_TASK_RUNNING;
		else
			go_to_sleep = 1;
	}

	if (ma_need_togo() || go_to_sleep) {
		if (go_to_sleep && !go_to_sleep_traced) {
			sched_debug("schedule: go to sleep\n");
			PROF_EVENT(sched_thread_blocked);
			go_to_sleep_traced = 1;
		}
#ifdef MA__LWPS
		/* Let people know as soon as now that for now the next thread
		 * will be idle, hence letting higher priority thread wake-ups
		 * ask for reschedule.
		 *
		 * need_resched needs to have been set _before_ this
		 */
		__ma_get_lwp_var(current_thread) = __ma_get_lwp_var(idle_task);
		ma_smp_mb();
#endif
		/* Now we said that, check again that nobody woke us in the
		 * meanwhile. */
		if (go_to_sleep && !prev->sched.state)
			goto need_resched_atomic;
		prev_as_next = NULL;
		prev_as_h = &ma_dontsched_rq(LWP_SELF)->hold;
		prev_as_prio = MA_IDLE_PRIO;
	}

#ifdef MA__LWPS
restart:
#endif
	cur = prev_as_next;
	nexth = prev_as_h;
	max_prio = prev_as_prio;
	hard_preempt = 0;

	/* Iterate over runqueues that cover this LWP */
#ifdef MA__LWPS
	sched_debug("default prio: %d\n",max_prio);
	for (currq = ma_lwp_rq(LWP_SELF); currq; currq = currq->father) {
#else
	sched_debug("default prio: %d\n",max_prio);
	currq = &ma_main_runqueue;
#endif
		if (!currq->hold.nr_ready)
			sched_debug("apparently nobody in %s\n",currq->name);
		idx = ma_sched_find_first_bit(currq->active->bitmap);
		if (idx < max_prio) {
			sched_debug("found better prio %d in rq %s\n",idx,currq->name);
			cur = NULL;
			max_prio = idx;
			nexth = &currq->hold;
			/* let polling know that this context switch is urging */
			hard_preempt = 1;
		}
		if (cur && need_resched && idx == prev_as_prio && idx < MA_BATCH_PRIO) {
		/* still wanted to schedule prev, but it needs resched
		 * and this is same prio
		 */
			sched_debug("found same prio %d in rq %s\n",idx,currq->name);
			cur = NULL;
			nexth = &currq->hold;
		}
#ifdef MA__LWPS
	}
#endif

	if (tbx_unlikely(nexth == &ma_dontsched_rq(LWP_SELF)->hold)) {
		/* found no interesting queue, not even previous one */
#ifdef MA__LWPS
		if (prev->sched.state == MA_TASK_INTERRUPTIBLE || prev->sched.state == MA_TASK_UNINTERRUPTIBLE) {
			ma_about_to_idle();
			if (!prev->sched.state) {
				/* We got woken up. */
				if (ma_need_resched()) {
					/* But we need to resched.  Restart in
					 * case that's for somebody else */
					goto need_resched_atomic;
				} else {
					/* we are ready, just return */
					didswitch = 0;
					ma_local_bh_enable();
					goto out;
				}
			}
		}
		sched_debug("rebalance\n");
//		load_balance(rq, 1, cpu_to_node_mask(smp_processor_id()));
#ifdef MA__BUBBLES
		if (ma_idle_scheduler)
		  if (current_sched->vp_is_idle)
		    {
		      if (current_sched->vp_is_idle(LWP_NUMBER(LWP_SELF))) 
			goto need_resched_atomic;
		    }
#endif
		cur = __ma_get_lwp_var(idle_task);
#else /* MONO */
		/* mono: nobody can use our stack, so there's no need for idle
		 * thread */
		if (!currently_idle) {
			ma_entering_idle();
			currently_idle = 1;
		}
		ma_local_bh_enable();
#ifdef PIOMAN
		if (!piom_polling_is_required(PIOM_POLL_AT_IDLE))
#else
		if (!marcel_polling_is_required(MARCEL_EV_POLL_AT_IDLE))
#endif /* PIOMAN */
		{
		        marcel_sig_disable_interrupts();
			marcel_sig_pause();
			marcel_sig_enable_interrupts();
		} else {
#ifdef MARCEL_IDLE_PAUSE
		        if (didpoll)
			        /* already polled a bit, sleep a bit before
				 * polling again */
 			        marcel_sig_nanosleep();
#endif
#ifdef PIOMAN
			__piom_check_polling(PIOM_POLL_AT_IDLE);
#else
			__marcel_check_polling(MARCEL_EV_POLL_AT_IDLE);
#endif /* PIOMAN */
			didpoll = 1;
		}
		ma_check_work();
		need_resched = 1;
		ma_local_bh_disable();
		goto need_resched_atomic;
#endif /* MONO */
	} else {
#ifndef MA__LWPS
		if (currently_idle) {
			ma_leaving_idle();
			currently_idle = 0;
		}
#endif
	}

	/* found something interesting, lock the holder */

#ifdef MA__LWPS
	if (tbx_unlikely(!nexth)) {
		/* no run holder, we are probably being moved */
		if (cur && cur == prev) {
			/* OK, we didn't want to give hand anyway */
			didswitch = 0;
			ma_local_bh_enable();
			goto out;
		}
		/* Loop again, until the move is complete */
		goto restart;
	}
#endif
	ma_holder_lock(nexth);
	sched_debug("locked(%p)\n",nexth);

	if (cur) /* either prev or idle */ {
	        next = cur;
		goto switch_tasks;
	}

	/* nexth can't be a bubble here */
	MA_BUG_ON(nexth->type != MA_RUNQUEUE_HOLDER);
	rq = ma_rq_holder(nexth);
#ifdef MA__LWPS
	if (tbx_unlikely(!(rq->active->nr_active))) { //+rq->expired->nr_active))) {
		sched_debug("someone stole the task we saw, restart\n");
		ma_holder_unlock(&rq->hold);
		goto restart;
	}
#endif

	MA_BUG_ON(!(rq->active->nr_active)); //+rq->expired->nr_active));

	/* now look for next *different* task */
	array = rq->active;
#if 0
	if (tbx_unlikely(!array->nr_active)) {
		sched_debug("arrays switch\n");
		/* XXX: todo: handle all rqs... */
		rq_arrays_switch(rq);
	}
#endif

	idx = ma_sched_find_first_bit(array->bitmap);
#ifdef MA__LWPS
	if (tbx_unlikely(idx > max_prio)) {
	        /* We had seen a high-priority task, but it's not there any more */
		sched_debug("someone stole the high-priority task we saw, restart\n");
		ma_holder_unlock(&rq->hold);
		goto restart;
	}
#endif
	queue = ma_array_queue(array, idx);
	/* we may here see ourselves, if someone awoked us */
	nextent = ma_queue_entry(queue);
	
#ifdef MA__BUBBLES
	if (!(nextent = ma_bubble_sched(nextent, rq, &nexth, idx)))
		/* ma_bubble_sched aura libéré prevrq */
		goto need_resched_atomic;
#endif
	next = ma_task_entity(nextent);

	if (ma_entity_task(next)->type == MA_THREAD_SEED_ENTITY) {
		/* A thread seed, take it */
		ma_dequeue_task(next, nexth);
		ma_holder_unlock(nexth);

		if (prev->sched.state == MA_TASK_DEAD && !(ma_preempt_count() & MA_PREEMPT_ACTIVE) && prev->cur_thread_seed) { // && prev->shared_attr == next->shared_attr) {
			/* yeepee, exec it */
			prev->cur_thread_seed = next;
			/* we disabled preemption once in marcel_exit_internal, re-enable it once */
			ma_preempt_enable();
			LOG_OUT();
			if (!prev->f_to_call)
				/* marcel_exit was called directly from the
				 * runner loop, just return (faster) */
				return 0;
			else
				marcel_ctx_longjmp(SELF_GETMEM(ctx_restart), 0);
		} else {
			/* gasp, create a launcher thread */
			marcel_attr_t attr; // = *next->shared_attr;
			marcel_attr_init(&attr);
			marcel_attr_setdetachstate(&attr, tbx_true);
			marcel_attr_setprio(&attr, MA_SYS_RT_PRIO);
			marcel_attr_setinitrq(&attr, ma_lwp_rq(LWP_SELF));
			marcel_attr_setpreemptible(&attr, tbx_false);
			/* TODO: on devrait être capable de brancher directement dessus */
			marcel_create(NULL, &attr, marcel_sched_seed_runner, next);
			goto need_resched_atomic;
		}
	}

	MA_BUG_ON(nextent->type != MA_THREAD_ENTITY);

switch_tasks:
	if (nexth->type == MA_RUNQUEUE_HOLDER)
		sched_debug("prio %d in %s, next %p(%s)\n",idx,ma_rq_holder(nexth)->name,next,next->name);
	else
		sched_debug("prio %d in %p, next %p(%s)\n",idx,nexth,next,next->name);
	MTRACE("previous",prev);
	MTRACE("next",next);

	/* still wanting to go to sleep ? (now that runqueues are locked, we can
	 * safely deactivate ourselves */

	if (go_to_sleep && ((prev->sched.state == MA_TASK_DEAD) ||
				!(ma_preempt_count() & MA_PREEMPT_ACTIVE)))
		/* on va dormir, il _faut_ donner la main à quelqu'un d'autre */
		MA_BUG_ON(next==prev);

//Pour quand on voudra ce mécanisme...
	//ma_RCU_qsctr(ma_task_lwp(prev))++;

//	prev->sleep_avg -= run_time;
//	if ((long)prev->sleep_avg <= 0) {
//		prev->sleep_avg = 0;
//		if (!(HIGH_CREDIT(prev) || LOW_CREDIT(prev)))
//			prev->interactive_credit--;
//	}

	if (tbx_likely(didswitch = (prev != next))) {
		ma_clear_tsk_need_togo(prev);

		/* really switch */
		prev = do_switch(prev, next, nexth, now);

		/* TODO: si !hard_preempt, appeler le polling */
	} else {
		sched_debug("unlock(%p)\n",nexth);
		ma_holder_unlock_softirq(nexth);
#ifdef MA__LWPS
		if (tbx_unlikely(MARCEL_SELF == __ma_get_lwp_var(idle_task)))
			ma_still_idle();
		else
#endif
		  {
#ifdef PIOMAN
		    /* TODO: appeler le polling pour lui faire faire un peu de poll */
#endif
		  }
	}

#ifdef MA__LWPS
out:
#endif
//	reacquire_kernel_lock(current);
	ma_preempt_enable_no_resched();
	if (tbx_unlikely(ma_need_resched() && ma_thread_preemptible())) {
		sched_debug("need resched\n");
		need_resched = 1;
		goto need_resched;
	}
	sched_debug("switched\n");
	LOG_RETURN(didswitch);
}

int marcel_yield_to(marcel_t next)
{
	marcel_t prev = MARCEL_SELF;
	ma_holder_t *nexth;
	int busy;

	MA_BUG_ON(ma_entity_task(next)->type == MA_THREAD_SEED_ENTITY);
	if (next==prev)
		return 0;

	LOG_IN();

	nexth = ma_task_holder_lock_softirq(next);

	if (!MA_TASK_IS_READY(next)) {
		busy = MA_TASK_IS_RUNNING(next);
		ma_task_holder_unlock_softirq(nexth);
		sched_debug("marcel_yield: %s task %p\n",
			busy?"busy":"not enqueued", next);
		LOG_OUT();
		return -1;
	}

	// we suppose we don't want to go to sleep, and we're not yielding to a
	// dontsched thread like idle or activations
	
	prev = do_switch(prev, next, nexth, marcel_clock());

	LOG_OUT();
	return 0;
}

void ma_preempt_schedule(int irq)
{
        marcel_task_t *ti = MARCEL_SELF;

	MA_BUG_ON(ti->preempt_count != irq);

need_resched:

        ti->preempt_count = MA_PREEMPT_ACTIVE;

	if (irq)
		/* now we recorded preemption, we can safely re-enable signals */
		marcel_sig_enable_interrupts();

        ma_schedule();

	if (irq)
		/* before forgetting about preemption, re-disable signals */
		marcel_sig_disable_interrupts();

	MA_BUG_ON(ti->preempt_count != MA_PREEMPT_ACTIVE);

        ti->preempt_count = irq;

        /* we could miss a preemption opportunity between schedule and now */
        ma_barrier();
        if (ma_need_resched())
                goto need_resched;
}

// Effectue un changement de contexte + éventuellement exécute des
// fonctions de scrutation...

DEF_MARCEL_POSIX(int, yield, (void), (),
{
  LOG_IN();

  marcel_check_polling(MARCEL_EV_POLL_AT_YIELD);
  ma_need_resched() = 1;
  ma_schedule();

  LOG_OUT();
  return 0;
})
/* La définition n'est pas toujours dans pthread.h */
extern int pthread_yield (void) __THROW;
DEF_PTHREAD_STRONG(int, yield, (void), ())

// Modifie le 'vpmask' du thread courant. Le cas échéant, il faut donc
// retirer le thread de la file et le replacer dans la file
// adéquate...
// IMPORTANT : cette fonction doit marcher si on l'appelle en section atomique
// pour se déplacer sur le LWP courant (cf terminaison des threads)
void marcel_change_vpmask(marcel_vpmask_t *mask)
{
#ifdef MA__LWPS
	ma_holder_t *old_h;
	ma_runqueue_t *new_rq;
	LOG_IN();
	old_h = ma_task_holder_lock_softirq(MARCEL_SELF);
	new_rq=marcel_sched_vpmask_init_rq(mask);
	if (old_h == &new_rq->hold) {
		ma_task_holder_unlock_softirq(old_h);
		LOG_OUT();
		return;
	}
	ma_deactivate_running_task(MARCEL_SELF,old_h);
	ma_task_sched_holder(MARCEL_SELF) = NULL;
	ma_holder_rawunlock(old_h);
#ifdef MA__BUBBLES
	old_h = ma_task_init_holder(MARCEL_SELF);
	if (old_h && old_h->type == MA_BUBBLE_HOLDER)
		marcel_bubble_removetask(ma_bubble_holder(old_h),MARCEL_SELF);
#endif
	ma_holder_rawlock(&new_rq->hold);
	ma_task_sched_holder(MARCEL_SELF) = &new_rq->hold;
	ma_activate_running_task(MARCEL_SELF,&new_rq->hold);
	ma_holder_rawunlock(&new_rq->hold);
	/* On teste si le LWP courant est interdit ou pas */
	if (LWP_NUMBER(LWP_SELF) == -1 || marcel_vpmask_vp_ismember(mask,LWP_NUMBER(LWP_SELF))) {
		ma_set_current_state(MA_TASK_MOVING);
		ma_local_bh_enable();
		ma_preempt_enable_no_resched();
		ma_schedule();
	} else {
		ma_local_bh_enable();
		ma_preempt_enable();
	}
	LOG_OUT();
#endif
}

/**
 * task_prio - return the priority value of a given task.
 * @p: the task in question.
 *
 * This is the priority value as seen by users in /proc.
 * RT tasks are offset by -200. Normal tasks are centered
 * around 0, value goes from -16 to +15.
 */
int marcel_task_prio(marcel_task_t *p)
{
	return p->sched.internal.entity.prio - MA_MAX_RT_PRIO;
}

/**
 * marcel_idle_lwp - is a given cpu idle currently?
 * @lwp: the processor in question.
 */
#ifdef MA__LWPS
int marcel_idle_lwp(ma_lwp_t lwp)
{
	return ma_lwp_curr(lwp) == ma_per_lwp(idle_task, lwp);
}
#endif

TBX_EXTERN void __ma_cond_resched(void)
{
	ma_set_current_state(MA_TASK_RUNNING);
	ma_schedule();
}

static void linux_sched_lwp_init(ma_lwp_t lwp)
{
#ifdef MA__LWPS
	unsigned num = LWP_NUMBER(lwp);
	char name[16];
	ma_runqueue_t *rq;
#endif
	LOG_IN();
	/* en mono, rien par lwp, tout est initialisé dans sched_init */
#ifdef MA__LWPS
	rq = ma_lwp_rq(lwp);
	snprintf(name,sizeof(name),"lwp%d",num);
	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWLWPRQ,2),num,rq);
	ma_init_rq(rq, name, MA_LWP_RQ);
	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ,2),-1,&ma_per_lwp(dontsched_runqueue,lwp));
	if (num == -1)
		/* "extra" LWPs are apart */
		rq->father=NULL;
	else {
		rq->father=&marcel_topo_vp_level[num].sched;
#ifdef MA__SMP
		if (num < marcel_nbvps()) {
			marcel_vpmask_add_vp(&ma_main_runqueue.vpset,num);
			marcel_vpmask_add_vp(&ma_dontsched_runqueue.vpset,num);
		}
#endif
	}
	if (rq->father)
		mdebug("runqueue %s has father %s\n",name,rq->father->name);
	snprintf(name,sizeof(name),"dontsched%d",num);
	ma_init_rq(&ma_per_lwp(dontsched_runqueue,lwp),name, MA_DONTSCHED_RQ);
	rq->level = marcel_topo_nblevels-1;
	ma_per_lwp(current_thread,lwp) = ma_per_lwp(run_task,lwp);
#ifdef MA__SMP
	marcel_vpmask_only_vp(&(rq->vpset),num);
	marcel_vpmask_only_vp(&(ma_per_lwp(dontsched_runqueue,lwp).vpset),num);
#endif
	if (num != -1 && num >= marcel_nbvps()) {
		snprintf(name,sizeof(name), "vp%d", num);
		rq = &marcel_topo_vp_level[num].sched;
		ma_init_rq(rq, name, MA_VP_RQ);
		rq->level = marcel_topo_nblevels-1;
		rq->father = NULL;
		marcel_vpmask_only_vp(&rq->vpset, num);
		mdebug("runqueue %s is a supplementary runqueue\n", name);
		PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ,2),rq->level,rq);
	}
#endif
	ma_spin_lock_init(&lwp->tasklet_lock);
	LOG_OUT();
}

static void linux_sched_lwp_start(ma_lwp_t lwp)
{
	ma_holder_t *h;
	marcel_task_t *p = ma_per_lwp(run_task,lwp);
	/* Cette tâche est en train d'être exécutée */
	h=ma_task_holder_lock_softirq(p);
	ma_activate_running_task(p, h);
	ma_task_holder_unlock_softirq(h);
	ma_task_stats_set(long, p, ma_stats_nbrunning_offset, 1);
}

MA_DEFINE_LWP_NOTIFIER_START_PRIO(linux_sched, 200, "Linux scheduler",
				  linux_sched_lwp_init, "Initialisation des runqueue",
				  linux_sched_lwp_start, "Activation de la tâche courante");

MA_LWP_NOTIFIER_CALL_UP_PREPARE(linux_sched, MA_INIT_LINUX_SCHED);

#ifdef MA__SMP
static void init_subrunqueues(struct marcel_topo_level *level, ma_runqueue_t *rq, int levelnum) {
	unsigned i;
	char name[16];
	ma_runqueue_t *newrq;
	static const char *base[] = {
#ifdef MA__NUMA
		[MARCEL_LEVEL_NODE]	= "node",
		[MARCEL_LEVEL_DIE]	= "die",
		[MARCEL_LEVEL_L3]	= "L3",
		[MARCEL_LEVEL_L2]	= "L2",
		[MARCEL_LEVEL_CORE]	= "core",
		[MARCEL_LEVEL_PROC]	= "proc",
#endif /* MA__NUMA */
		[MARCEL_LEVEL_VP]	= "vp",
	};
	static const enum ma_rq_type rqtypes[] = {
#ifdef MA__NUMA
		[MARCEL_LEVEL_FAKE]	= MA_FAKE_RQ,
		[MARCEL_LEVEL_NODE]	= MA_NODE_RQ,
		[MARCEL_LEVEL_DIE]	= MA_DIE_RQ,
		[MARCEL_LEVEL_L3]	= MA_L3_RQ,
		[MARCEL_LEVEL_L2]	= MA_L2_RQ,
		[MARCEL_LEVEL_CORE]	= MA_CORE_RQ,
		[MARCEL_LEVEL_PROC]	= MA_PROC_RQ,
#endif /* MA__NUMA */
		[MARCEL_LEVEL_VP]	= MA_VP_RQ,
	};

	if (!level->arity || level->type == MARCEL_LEVEL_LAST) {
		return;
	}

	for (i=0;i<level->arity;i++) {
		if (level->children[i]->type == MARCEL_LEVEL_FAKE)
			snprintf(name, sizeof(name), "fake%d-%d",
				levelnum, level->children[i]->number);
		else
			snprintf(name,sizeof(name), "%s%d",
				base[level->children[i]->type], level->children[i]->number);
		newrq = &level->children[i]->sched;
		ma_init_rq(newrq, name, rqtypes[level->children[i]->type]);
		newrq->level = levelnum;
		newrq->father = rq;
		newrq->vpset = level->children[i]->vpset;
		mdebug("runqueue %s has father %s\n", name, rq->name);
		PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ,2),levelnum,newrq);
		init_subrunqueues(level->children[i],newrq,levelnum+1);
	}
}
#endif

void __marcel_init ma_linux_sched_init0(void)
{
	ma_init_rq(&ma_main_runqueue,"machine", MA_MACHINE_RQ);
#ifdef MA__LWPS
	ma_main_runqueue.level = 0;
#endif
	ma_init_rq(&ma_dontsched_runqueue,"dontsched", MA_DONTSCHED_RQ);
#ifdef MA__LWPS
	marcel_vpmask_empty(&ma_main_runqueue.vpset);
	marcel_vpmask_empty(&ma_dontsched_runqueue.vpset);
#endif

	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWLEVEL,1),1);
	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ,2),-1,&ma_dontsched_runqueue);
	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ,2),0,&ma_main_runqueue);
}


unsigned long ma_stats_nbthreads_offset, ma_stats_nbthreadseeds_offset,
		ma_stats_nbrunning_offset, ma_stats_last_ran_offset;
unsigned long marcel_stats_load_offset;

static void __marcel_init linux_sched_init(void)
{
	LOG_IN();
	ma_holder_t *h;

#ifdef MARCEL_STATS_ENABLED
	ma_stats_nbthreads_offset = ma_stats_alloc(ma_stats_long_sum_reset, ma_stats_long_sum_synthesis, sizeof(long));
	ma_stats_nbthreadseeds_offset = ma_stats_alloc(ma_stats_long_sum_reset, ma_stats_long_sum_synthesis, sizeof(long));
	ma_stats_nbrunning_offset = ma_stats_alloc(ma_stats_long_sum_reset, ma_stats_long_sum_synthesis, sizeof(long));
	ma_stats_last_ran_offset = ma_stats_alloc(ma_stats_long_max_reset, ma_stats_long_max_synthesis, sizeof(long));
	marcel_stats_load_offset = ma_stats_alloc(ma_stats_long_sum_reset, ma_stats_long_sum_synthesis, sizeof(long));
	ma_task_stats_set(long, __main_thread, ma_stats_nbthreads_offset, 1);
	ma_task_stats_set(long, __main_thread, ma_stats_nbthreadseeds_offset, 0);
	ma_task_stats_set(long, __main_thread, ma_stats_nbrunning_offset, 1);
#endif /* MARCEL_STATS_ENABLED */

#ifdef MA__SMP
	if (marcel_topo_nblevels>1) {
#ifdef MA__NUMA
		unsigned i,j;
		for (i=1;i<marcel_topo_nblevels-1;i++) {
			for (j=0; marcel_topo_levels[i][j].vpset; j++);
			PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWLEVEL,1),j);
		}
#endif
		PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWLEVEL,1), marcel_nbvps());
		init_subrunqueues(marcel_machine_level, &ma_main_runqueue, 1);
	}
#endif

	/*
	 * We have to do a little magic to get the first
	 * thread right in SMP mode.
	 */
#ifndef MA__LWPS
	ma_per_lwp(current_thread,&__main_lwp) = MARCEL_SELF;
#endif
	marcel_wake_up_created_thread(MARCEL_SELF);
	/* since it is actually already running */
	h = ma_task_holder_lock(MARCEL_SELF);
	ma_dequeue_task(MARCEL_SELF, h);
	ma_task_holder_unlock(h);

//	init_timers();

	/*
	 * The boot idle thread does lazy MMU switching as well:
	 */
//	atomic_inc(&init_mm.mm_count);
//	enter_lazy_tlb(&init_mm, current);
	LOG_OUT();
}

__ma_initfunc_prio(linux_sched_init, MA_INIT_LINUX_SCHED, MA_INIT_LINUX_SCHED_PRIO, "Scheduler Linux 2.6");

#if defined(MA__LWPS) || !defined(MA_HAVE_COMPAREEXCHANGE)
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
#ifdef PROFILE
		do {
			if (ma_spin_is_locked(lock)) {
				PROF_EVENT(spin_lock_busy);
				while(ma_spin_is_locked(lock))
					;
			}
		} while(!_ma_raw_spin_trylock(lock));
#else
		_ma_raw_spin_lock(lock);
#endif
		return;
	}
	do {
		ma_preempt_enable();
		if (ma_spin_is_locked(lock)) {
			PROF_EVENT(spin_lock_busy);
			while (ma_spin_is_locked(lock))
				;
				//cpu_relax();
		}
		ma_preempt_disable();
	} while (!_ma_raw_spin_trylock(lock));
}
#endif

#if defined(MA__LWPS)
TBX_EXTERN void __ma_preempt_write_lock(ma_rwlock_t *lock)
{
	if (ma_preempt_count() > 1) {
#ifdef PROFILE
		do {
			if (ma_rwlock_is_locked(lock)) {
				PROF_EVENT(rwlock_lock_busy);
				while (ma_rwlock_is_locked(lock))
					;
			}
		} while (!_ma_raw_write_trylock(lock));
#else
		_ma_raw_write_lock(lock);
#endif
		return;
	}

	do {
		ma_preempt_enable();
		if (ma_rwlock_is_locked(lock)) {
			PROF_EVENT(rwlock_lock_busy);
			while (ma_rwlock_is_locked(lock))
				;//cpu_relax();
		}
		ma_preempt_disable();
	} while (!_ma_raw_write_trylock(lock));
}
#endif 





