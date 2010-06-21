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
#include <errno.h>

#ifdef CONFIG_NUMA
#define cpu_to_node_mask(cpu) node_to_cpumask(cpu_to_node(cpu))
#else
#define cpu_to_node_mask(cpu) (cpu_online_map)
#endif

#define TASK_TASK_PREEMPT(p, q)				\
	((q)->as_entity.prio - (p)->as_entity.prio)

#define TASK_PREEMPTS_TASK(p, q)		\
	TASK_TASK_PREEMPT(p, q) > 0

#define TASK_PREEMPTS_CURR(p, lwp)					\
	TASK_PREEMPTS_TASK((p), ma_per_lwp(current_thread, (lwp)))

#define TASK_CURR_PREEMPT(p, lwp)					\
	TASK_TASK_PREEMPT((p), ma_per_lwp(current_thread, (lwp)))

/*
 * ma_resched_task - mark a task 'to be rescheduled now'.
 *
 * On UP this means the setting of the need_resched flag, on SMP it
 * might also involve a LWP killing to trigger the scheduler on
 * the target LWP.
 */
void ma_resched_task(marcel_task_t *p LWPS_VAR_UNUSED, int vp, ma_lwp_t lwp LWPS_VAR_UNUSED)
{
	PROF_EVENT2(sched_resched_task, p, vp);
#ifdef MA__LWPS
	ma_preempt_disable();

	if (tbx_unlikely(ma_get_need_resched_ext(vp, lwp))) {
		PROF_EVENT(sched_already_resched);
		goto out;
	}

	ma_set_need_resched_ext(vp, lwp, 1);

	if (lwp == MA_LWP_SELF) {
		PROF_EVENT(sched_thats_us);
		goto out;
	}

	/* NEED_RESCHED must be visible before we test POLLING_NRFLAG */
	ma_smp_mb();
	/* minimise the chance of sending an interrupt to poll_idle() */
	if (!ma_test_tsk_thread_flag(p,TIF_POLLING_NRFLAG)) {

		PROF_EVENT2(sched_resched_lwp, ma_vpnum(MA_LWP_SELF), ma_vpnum(lwp));
		MA_LWP_RESCHED(lwp);
	} else PROF_EVENT2(sched_resched_lwp_already_polling, p, ma_vpnum(lwp));
out:
	ma_preempt_enable();
#else
	ma_set_need_resched_ext(vp, lwp, 1);
#endif
}

void __ma_resched_vpset(const marcel_vpset_t *vpset)
{
	unsigned vp;
	marcel_vpset_foreach_begin(vp, vpset)
		ma_lwp_t lwp = ma_get_lwp_by_vpnum(vp);
	if (lwp) {
		marcel_t current = ma_per_lwp(current_thread, lwp);
		ma_set_tsk_need_togo(current);
		ma_resched_task(current,vp,lwp);
	}
	marcel_vpset_foreach_end()
		}

void ma_resched_vpset(const marcel_vpset_t *vpset)
{
	unsigned vp;
	ma_preempt_disable();
	ma_local_bh_disable();
	marcel_vpset_foreach_begin(vp, vpset)
		ma_lwp_t lwp = ma_get_lwp_by_vpnum(vp);
	if (lwp) {
		marcel_t current = ma_per_lwp(current_thread, lwp);
		ma_holder_rawlock(&ma_lwp_vprq(lwp)->as_holder);
		ma_set_tsk_need_togo(current);
		ma_resched_task(current,vp,lwp);
		ma_holder_rawunlock(&ma_lwp_vprq(lwp)->as_holder);
	}
	marcel_vpset_foreach_end()
		ma_preempt_enable_no_resched();
	ma_local_bh_enable();
}

void
__ma_resched_topo_level(struct marcel_topo_level *l)
{
	__ma_resched_vpset(&l->vpset);
}

void
ma_resched_topo_level(struct marcel_topo_level *l)
{
	ma_resched_vpset(&l->vpset);
}

/**
 * task_curr - is this task currently executing on a CPU?
 * @p: the task in question.
 *
 * UNUSED
 */
int marcel_task_curr(marcel_task_t *p)
{
	return ma_lwp_curr(ma_get_task_lwp(p)) == p;
}

/* tries to resched the given task in the given holder */
static void try_to_resched(marcel_task_t *p, ma_holder_t *h)
{
	marcel_lwp_t *lwp, *chosen = NULL;
	ma_runqueue_t *rq = ma_to_rq_holder(h);
	int max_preempt = 0, preempt, i, chosenvp = -1;
	if (!rq)
		return;

	/* try_to_resched may be called while ma_list_lwp has not been added any item yet */
	if (!ma_any_lwp())
		return;

	/* TODO: scalability concern */
	ma_for_all_lwp_from_begin(lwp, MA_LWP_SELF) {
		i = ma_vpnum(lwp);
		if (rq == ma_lwp_rq(lwp)) {
			preempt = TASK_CURR_PREEMPT(p, lwp);
			if (preempt > max_preempt) {
				max_preempt = preempt;
				/* the rq is an LWP rq, we dont have any other choice */
				chosen = lwp;
				chosenvp = i;
			}
			break; /* no need to go further */
		} else if (ma_rq_covers(rq, i)) {
			/* might be an interesting choice if we dont find anything better */
			preempt = TASK_CURR_PREEMPT(p, lwp);
			if (preempt > max_preempt) {
				max_preempt = preempt;
				chosen = lwp;
				chosenvp = i;
			}
		}
	} ma_for_all_lwp_from_end();

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
static int __ma_try_to_wake_up(marcel_task_t * p, unsigned int state, int sync, ma_holder_t *h)
{
	int success = 0;
	long old_state;
	ma_runqueue_t *rq;
	MARCEL_LOG_IN();

	old_state = p->state;
	if (old_state & state) {
		/* on s'occupe de la réveiller */
		PROF_EVENT2(sched_thread_wake, p, old_state);
#ifdef MARCEL_STATS_ENABLED
		ma_task_stats_set(long, p, ma_stats_nbready_offset, 1);
#endif
		p->state = MA_TASK_RUNNING;
		if (MA_TASK_IS_BLOCKED(p)) { /* not running or runnable */
			PROF_EVENT(sched_thread_wake_unblock);
			/*
			 * Fast-migrate the task if it's not running or runnable
			 * currently. Do not violate hard affinity.
			 */
#ifdef PM2_DEV
#warning TODO
#endif
			/* Attention ici: h peut être une bulle, auquel cas
			 * account_ready_or_running_entity peut lâcher la bulle pour verrouiller
			 * une runqueue */
			ma_set_ready_holder(&p->as_entity,h);
			ma_enqueue_entity(&p->as_entity,h);
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
		if (rq && ma_rq_covers(rq, ma_vpnum(MA_LWP_SELF)) && TASK_PREEMPTS_TASK(p, MARCEL_SELF)) {
			/* we can avoid remote reschedule by switching to it */
			if (!sync) /* only if we won't for sure yield() soon */
				ma_resched_task(MARCEL_SELF, ma_vpnum(MA_LWP_SELF), MA_LWP_SELF);
		} else try_to_resched(p, h);

		success = 1;
	}

	MARCEL_LOG_RETURN(success);
}

int ma_try_to_wake_up(marcel_task_t * p, unsigned int state, int sync)
{
	int success;
	ma_holder_t *h = ma_task_holder_lock_softirq(p);
	success = __ma_try_to_wake_up(p, state, sync, h);
	ma_holder_try_to_wake_up_and_unlock_softirq(h);
	return success;
}

int fastcall ma_wake_up_thread(marcel_task_t * p)
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
	ma_holder_try_to_wake_up_and_unlock_softirq(h);
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
	MARCEL_LOG_IN();

	MARCEL_SCHED_LOG("wake up created thread %p\n",p);
	if (ma_entity_task(p)->type == MA_THREAD_ENTITY) {
		MA_BUG_ON(p->state != MA_TASK_BORNING);
#ifdef MARCEL_STATS_ENABLED
		ma_task_stats_set(long, p, ma_stats_last_ran_offset, marcel_clock());
#endif
		PROF_EVENT2(sched_thread_wake, p, p->state);
		ma_set_task_state(p, MA_TASK_RUNNING);
	}
#ifdef MARCEL_STATS_ENABLED
	ma_task_stats_set(long, p, ma_stats_nbready_offset, 1);
#endif
#ifdef MA__BUBBLES
	h = ma_task_natural_holder(p);

	if (h && ma_holder_type(h) != MA_RUNQUEUE_HOLDER) {
		MARCEL_SCHED_LOG("wake up task %p in bubble %p\n",p, ma_bubble_holder(h));
		if (tbx_fast_list_empty(&p->as_entity.natural_entities_item))
			marcel_bubble_inserttask(ma_bubble_holder(h),p);
	}
#endif
#ifdef MA__LWPS
retry:
#endif

	h = ma_task_sched_holder(p);

#ifdef MA__BUBBLES
	if (h->type == MA_BUBBLE_HOLDER) {
		/* Don't directly enqueue in holding bubble, but in the thread cache. */
		marcel_bubble_t *b = ma_bubble_holder(h);
		ma_holder_t *hh;
		while ((hh = b->as_entity.sched_holder) && hh->type == MA_BUBBLE_HOLDER) {
			h = hh;
			b = ma_bubble_holder(hh);
		}
		ma_task_sched_holder(p) = h;
	}
#endif

	MA_BUG_ON(!h);
	ma_holder_lock_softirq(h);

#ifdef MA__LWPS
	if (ma_task_sched_holder(p) != h) {
		ma_holder_unlock_softirq(h);
		goto retry;
	}
#endif

#ifdef MA__BUBBLES
	if (h->type == MA_BUBBLE_HOLDER
	    && ma_bubble_holder(h)->as_entity.sched_holder
	    && ma_bubble_holder(h)->as_entity.sched_holder->type == MA_BUBBLE_HOLDER) {
		/* It got moved just before locking, retry */
		ma_holder_unlock_softirq(h);
		goto retry;
	}
#endif

	/* il est possible de démarrer sur une autre rq que celle de SELF,
	 * on ne peut donc pas profiter de ses valeurs */
	if (MA_TASK_IS_BLOCKED(p)) {
		ma_set_ready_holder(&p->as_entity,h);
		ma_enqueue_entity(&p->as_entity,h);
	}
	if (p->as_entity.type == MA_THREAD_SEED_ENTITY) {
		/* seed is now initialized, enable inlining */
		p->cur_thread_seed_runner = NULL;
	}
	ma_holder_try_to_wake_up_and_unlock_softirq(h);
	// on donne la main aussitôt, bien souvent le meilleur choix
	if (ma_holder_type(h) == MA_RUNQUEUE_HOLDER)
		PROF_EVENT2(bubble_sched_switchrq, p, ma_rq_holder(h));
	rq = ma_rq_holder(h);
	if (ma_entity_task(p)->type == MA_THREAD_ENTITY
	    && ma_holder_type(h) == MA_RUNQUEUE_HOLDER && !ma_in_atomic()
	    && ma_rq_covers(rq, ma_vpnum(MA_LWP_SELF))) {
		/* XXX: on pourrait être préempté entre-temps... */
		PROF_EVENT(exec_woken_up_created_thread);
		ma_schedule();
	} else {
		PROF_EVENT(resched_for_woken_up_created_thread);
		try_to_resched(p, h);
	}
	MARCEL_LOG_OUT();
}

int ma_sched_change_prio(marcel_t t, int prio) {
	ma_holder_t *h;
	int requeue;
	/* attention ici. Pour l'instant on n'a pas besoin de requeuer dans
	 * une bulle */
	/* quand on le voudra, il faudra faire attention que dequeue_entity peut
	 * vouloir déverouiller la bulle pour pouvoir verrouiller la runqueue */
	MA_BUG_ON(prio < 0 || prio >= MA_MAX_PRIO);
	PROF_EVENT2(sched_setprio,t,prio);
	h = ma_task_holder_lock_softirq(t);
	if ((requeue = (MA_TASK_IS_READY(t) &&
			ma_holder_type(h) == MA_RUNQUEUE_HOLDER)))
		ma_dequeue_entity(&t->as_entity,h);
	t->as_entity.prio=prio;
	if (requeue)
		ma_enqueue_entity(&t->as_entity,h);
	ma_holder_try_to_wake_up_and_unlock_softirq(h);
	return 0;
}

#ifdef MA__BUBBLES
static void *bubble_join_signal_cb(void * arg) {
	int do_signal = 0;
	marcel_bubble_t *bubble = (marcel_bubble_t *)arg;

	marcel_mutex_lock(&bubble->join_mutex);

	ma_holder_lock_softirq(&bubble->as_holder);
	bubble->nb_natural_entities--;
	if (bubble->join_empty_state == 0  &&  bubble->nb_natural_entities == 0) {
		bubble->join_empty_state = 1;
		do_signal = 1;
	}
	ma_holder_unlock_softirq(&bubble->as_holder);

	if (do_signal)
		marcel_cond_signal(&bubble->join_cond);

	marcel_mutex_unlock(&bubble->join_mutex);
	return NULL;
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
static void finish_task_switch(marcel_task_t *prev)
{
	/* note: pas de softirq ici, on est déjà en mode interruption */
	ma_holder_t *prevh = ma_task_holder_rawlock(prev);
	unsigned long prev_task_state;
#ifdef MA__BUBBLES
	ma_holder_t *h;
#endif

	MARCEL_LOG_IN();
	prev_task_state = prev->state;

	if (prev->state && ((prev->state == MA_TASK_DEAD)
			    || !(THREAD_GETMEM(prev,preempt_count) & MA_PREEMPT_ACTIVE))
		) {
		if (prev->state & MA_TASK_MOVING) {
			/* moving, make it running elsewhere */
			MTRACE("moving",prev);
			PROF_EVENT2(sched_thread_wake, prev, prev->state);
			ma_set_task_state(prev, MA_TASK_RUNNING);
			/* still runnable */
			ma_enqueue_entity(&prev->as_entity,prevh);
			try_to_resched(prev, prevh);
		} else {
			/* yes, deactivate */
			MTRACE("going to sleep",prev);
			MARCEL_SCHED_LOG("%p going to sleep\n",prev);
			ma_clear_ready_holder(&prev->as_entity,prevh);
#ifdef MARCEL_STATS_ENABLED
			ma_task_stats_set(long, prev, ma_stats_nbready_offset, 0);
#endif
		}
	} else {
		MTRACE("still running",prev);
		ma_enqueue_entity(&prev->as_entity,prevh);
	}
	/* shouldn't already be running since we just left it ! */
	MA_BUG_ON(MA_TASK_IS_RUNNING(prev));
	/* No other choice */
	MA_BUG_ON(!(MA_TASK_IS_READY(prev) || MA_TASK_IS_BLOCKED(prev)));

#ifdef MA__BUBBLES
	if ((h = (ma_task_natural_holder(prev)))
	    && h->type == MA_BUBBLE_HOLDER) {
		marcel_bubble_t *bubble = ma_bubble_holder(h);
		int remove_from_bubble;
		if ((remove_from_bubble = (prev->state & MA_TASK_DEAD)))
			ma_task_sched_holder(prev) = NULL;
		ma_holder_try_to_wake_up_and_unlock_softirq(prevh);
		/* Note: since preemption was not re-enabled (see ma_schedule()), prev thread can't vanish between releasing prevh above and bubble lock below. */
		if (remove_from_bubble) {
			marcel_entity_t *prev_e = ma_entity_task(prev);
			int becomes_empty = ma_bubble_removeentity(bubble, prev_e);
			
			if (becomes_empty) {
				/* ma_bubble_removeentity already did the work
				 * requiring only the bubble lock, we now must
				 * perform the work that requires both the
				 * join_mutex and the bubble lock. However, we cannot block here.
				 * Thus, we attempt a non blocking mutex_trylock and if it fails, we delegate
				 * the work to a new thread. */
				if (!marcel_mutex_trylock(&bubble->join_mutex)) {
					marcel_attr_t attr;
					marcel_attr_init(&attr);
					marcel_attr_setdetachstate(&attr, tbx_true);
					/* We force the root bubble here to
					 * avoid interfering with the bubble we
					 * are signaling */
					marcel_attr_setnaturalbubble(&attr, &marcel_root_bubble);
					marcel_t t;
					marcel_create(&t, &attr, bubble_join_signal_cb, bubble);
				} else {
					int do_signal = 0;
					ma_holder_lock_softirq(&bubble->as_holder);
					bubble->nb_natural_entities--;
					if (bubble->join_empty_state == 0  &&  bubble->nb_natural_entities == 0) {
						bubble->join_empty_state = 1;
						do_signal = 1;
					}
					ma_holder_unlock_softirq(&bubble->as_holder);
					/* The call to marcel_cond_signal
					 * should not be performed in the
					 * holder_lock critical section above
					 * as it may result in a dead-lock when
					 * locking the holder of the task to
					 * wake up */
					if (do_signal)
						marcel_cond_signal(&bubble->join_cond);
					marcel_mutex_unlock(&bubble->join_mutex);
				}
			}
		}
	} else
#endif
	{
		/* on sort du mode interruption */
		ma_holder_try_to_wake_up_and_unlock_softirq(prevh);

	}
	if (tbx_unlikely(prev_task_state == MA_TASK_DEAD)) {
		PROF_THREAD_DEATH(prev);
		/* mourn it */
		marcel_funerals(prev);
	}
	MARCEL_LOG_OUT();
}

/**
 * schedule_tail - first thing a freshly forked thread must call.
 * @prev: the thread we just switched away from.
 */
asmlinkage void ma_schedule_tail(marcel_task_t *prev)
{
        ma_preempt_disable();
	finish_task_switch(prev);

#ifdef MA__LWPS
	if (tbx_unlikely(MARCEL_SELF == __ma_get_lwp_var(idle_task)))
		ma_entering_idle();
#endif
	ma_preempt_enable();
}

/*
 * nb_ready_entities:
 *
 * externally visible scheduler statistics: current number of runnable
 * threads
 */
unsigned long ma_nb_ready_entities(void)
{
	unsigned long i, sum = 0;

#ifdef PM2_DEV
#warning TODO: descendre dans les bulles ...
#endif
	for (i = 0; i < MA_NR_VPS; i++)
		sum += ma_lwp_rq(ma_get_lwp_by_vpnum(i))->as_holder.nb_ready_entities;
	sum += ma_main_runqueue.as_holder.nb_ready_entities;

	return sum;
}

/*
 * This function gets called by the timer code, with HZ frequency.
 * We call it with interrupts disabled.
 *
 * It also gets called by the fork code, when changing the parent's
 * timeslices.
 */
void ma_scheduler_tick(int user_ticks, int sys_ticks)
{
	struct ma_lwp_usage_stat *lwpstat = &__ma_get_lwp_var(lwp_usage);
	marcel_task_t *p = MARCEL_SELF;
#ifdef MA__LWPS
	int vpnum = ma_vpnum(MA_LWP_SELF);
#endif

	PROF_EVENT(sched_tick);

#ifdef PM2_DEV
#warning rcu not yet implemented (utile pour les numa ?)
#endif
	//if (rcu_pending(cpu))
	//      rcu_check_callbacks(cpu, user_ticks);

// TODO Pour l'instant, on n'a pas de notion de tick système
	sys_ticks = user_ticks ;
	if (ma_hardirq_count()) {
		lwpstat->irq += sys_ticks;
		sys_ticks = 0;
		/* note: this timer irq context must be accounted for as well */
	} else if (ma_softirq_count() - MA_SOFTIRQ_OFFSET) {
		lwpstat->softirq += sys_ticks;
		sys_ticks = 0;
	}

#ifdef MA__LWPS
	if (p == __ma_get_lwp_var(idle_task))
#else
		if (ma_currently_idle)
#endif
		{
#ifdef MA__LWPS
			if (marcel_vp_is_disabled(vpnum))
				lwpstat->disabled += sys_ticks;
			else
#endif
				// TODO on n'a pas non plus de notion d'iowait, est-ce qu'on le veut vraiment ?
				/*if (atomic_read(&rq->nr_iowait) > 0)
				  lwpstat->iowait += sys_ticks;
				  else*/
				lwpstat->idle += sys_ticks;
			//rebalance_tick(rq, 1);
			//return;
			sys_ticks = 0;
		} else if (p->as_entity.prio >= MA_BATCH_PRIO)
			lwpstat->nice += user_ticks;
		else
			lwpstat->user += user_ticks;
	//lwpstat->system += sys_ticks;

	/* Task might have expired already, but not scheduled off yet */
	//if (p->as_entity.array != rq->active) {
	
	// C'est normal quand le thread est en cours de migration par exemple. Il faudrait prendre le verrou pour faire cette vérification
	if (0 && !MA_TASK_IS_RUNNING(p)) {
		PM2_LOG("Strange: %s running, but not running (ready_holder == %p, holder_data == %p) !, report or look at it (%s:%i)\n",
			p->as_entity.name, ma_task_ready_holder(p),
			ma_task_ready_holder_data(p), __FILE__, __LINE__);
		ma_set_need_resched(1);
		goto out;
	}

	if (preemption_enabled() && ma_thread_preemptible()) {
		MA_BUG_ON(ma_atomic_read(&p->as_entity.time_slice)>MARCEL_TASK_TIMESLICE);
		if (ma_atomic_dec_and_test(&p->as_entity.time_slice)) {
			ma_set_need_resched(1);
			MARCEL_SCHED_LOG("scheduler_tick: time slice expired\n");
			ma_atomic_set(&p->as_entity.time_slice,MARCEL_TASK_TIMESLICE);
		}
		// attention: rq->lock ne doit pas être pris pour pouvoir
		// verrouiller la bulle.
		{
#ifdef MA__BUBBLES
			marcel_bubble_t *b;
			ma_holder_t *h;
			if ((h = ma_task_natural_holder(p)) && 
			    ma_holder_type(h) != MA_RUNQUEUE_HOLDER) {
				b = ma_bubble_holder(h);
				if (ma_atomic_dec_and_test(&b->as_entity.time_slice))
					ma_bubble_tick(b);
			}
#endif
		}
	}
out:
	(void)0;
}



/* Scheduling functions start here.  */


static marcel_t do_switch(marcel_t prev, marcel_t next, ma_holder_t *nexth, unsigned long now STATS_VAR_UNUSED) {
	tbx_prefetch(next);

#ifdef MARCEL_STATS_ENABLED
	/* update statistics */
	ma_task_stats_set(long, prev, ma_stats_last_ran_offset, now);
	ma_task_stats_set(long, next, ma_stats_nbrunning_offset, 1);
	ma_task_stats_set(long, prev, ma_stats_nbrunning_offset, 0);
	ma_task_stats_set(long, next, ma_stats_last_vp_offset, ma_vpnum (MA_LWP_SELF));
#endif
	__ma_get_lwp_var(current_thread) = next;
	ma_dequeue_entity(&next->as_entity, nexth);

	MARCEL_SCHED_LOG("unlock(%p)\n",nexth);
	ma_holder_rawunlock(nexth);
	ma_set_task_lwp(next, MA_LWP_SELF);

#ifdef MA__LWPS
	if (tbx_unlikely(prev == __ma_get_lwp_var(idle_task)))
		ma_leaving_idle();
#endif

	prev = marcel_switch_to(prev, next);
	ma_barrier();

	ma_schedule_tail(prev);

	return prev;
}

/* Instantiate SEED, a thread seed, and store the resulting seed runner in
   RESULT, unless RESULT is NULL.  If SCHEDULE is true, schedule the
   resulting thread.  */
static int instantiate_thread_seed(marcel_t seed, tbx_bool_t schedule, marcel_t *result)
{
	int err;
	marcel_attr_t attr;
	marcel_t runner = NULL;

	MA_BUG_ON(ma_entity_task(seed)->type != MA_THREAD_SEED_ENTITY);
	MA_BUG_ON(seed->cur_thread_seed_runner != NULL && seed->cur_thread_seed_runner != (void*) 1);

	marcel_attr_init(&attr);
	marcel_attr_setdetachstate(&attr, tbx_true);

	/* Start the seed runner with the highest priority so that it's scheduled
	   right after it's been created (or woken up).  */
	marcel_attr_setprio(&attr, MA_SYS_RT_PRIO);

	marcel_attr_setschedrq(&attr, ma_lwp_rq(MA_LWP_SELF));
	marcel_attr_setpreemptible(&attr, tbx_false);

	if (schedule)
		err = marcel_create(&runner, &attr, marcel_sched_seed_runner, seed);
	else
		err = marcel_create_dontsched(&runner, &attr, marcel_sched_seed_runner, seed);

	/* Note: `seed->cur_thread_seed_runner' will be assigned in
	   `marcel_sched_seed_runner ()' when RUNNER is actually scheduled.  */

	if (result != NULL)
		*result = runner;

	return err;
}


/*
 * schedule() is the main scheduler function.
 */
asmlinkage int ma_schedule(void)
{
	marcel_task_t *prev, *cur, *next, *prev_as_next;
	marcel_entity_t *nextent;
	ma_runqueue_t *rq, *currq;
	ma_holder_t *prevh, *nexth, *prev_as_h;
	ma_prio_array_t *array;
	struct tbx_fast_list_head *queue;
	unsigned long now;
	int idx = -1;
	int max_prio, prev_as_prio;
	int go_to_sleep;
	int didswitch;
	int go_to_sleep_traced;
#ifndef MA__LWPS
	int didpoll = 0;
#endif
	int hard_preempt TBX_UNUSED;
	int need_resched;
	int vpnum LWPS_VAR_UNUSED;
	MARCEL_LOG_IN();
	need_resched = ma_get_need_resched();
	/*
	 * Test if we are atomic.  Since do_exit() needs to call into
	 * schedule() atomically, we ignore that path for now.
	 * Otherwise, whine if we are scheduling when we should not be.
	 */
	MA_BUG_ON(ma_preempt_count()<0);
	if (tbx_likely(!(SELF_GETMEM(state) & MA_TASK_DEAD))) {
		if (tbx_unlikely(ma_in_atomic())) {
			PM2_LOG("bad: scheduling while atomic (%06x)! Did you forget to unlock a spinlock?\n",ma_preempt_count());
			ma_show_preempt_backtrace();
			MA_BUG();
		}
	}

need_resched:
	/* Say the world we're currently rescheduling */
	ma_set_need_resched(0);
	ma_smp_mb__after_clear_bit();
	/* Now that we announced we're taking a decision, we can have a look at
	 * what is available */

	/* we need to disable bottom half to avoid running ma_schedule_tick() ! */
	ma_preempt_disable();
	ma_local_bh_disable();

	prev = MARCEL_SELF;
	MA_BUG_ON(!prev);

	now = marcel_clock();
	prevh = ma_task_ready_holder(prev);

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
		prev_as_prio = ma_bubble_holder(prev_as_h)->as_entity.prio;
	} else
#endif
		prev_as_prio = prev->as_entity.prio;

	if (prev->state &&
	    /* garde-fou pour éviter de s'endormir
	     * par simple préemption */
	    ((prev->state == MA_TASK_DEAD) ||
	     !(ma_preempt_count() & MA_PREEMPT_ACTIVE))) {
		if (tbx_unlikely((prev->state & MA_TASK_INTERRUPTIBLE) &&
				 tbx_unlikely(0 /*work_pending(prev)*/)))
			prev->state = MA_TASK_RUNNING;
		else
			go_to_sleep = 1;
	}

	vpnum = ma_vpnum(MA_LWP_SELF);
	if (ma_need_togo() || go_to_sleep || marcel_vp_is_disabled(vpnum)) {
		if (go_to_sleep && !go_to_sleep_traced) {
			MARCEL_SCHED_LOG("schedule: go to sleep\n");
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
		if (go_to_sleep && !prev->state)
			goto need_resched_atomic;
		prev_as_next = NULL;
		prev_as_h = &ma_dontsched_rq(MA_LWP_SELF)->as_holder;
#ifdef MA__LWPS
		prev_as_prio = ma_entity_task(__ma_get_lwp_var(current_thread))->prio;
#else
		prev_as_prio = MA_IDLE_PRIO;
#endif
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
	MARCEL_SCHED_LOG("default prio: %d\n",max_prio);
	for (currq = ma_lwp_rq(MA_LWP_SELF); currq; currq = currq->father) {
#else
		MARCEL_SCHED_LOG("default prio: %d\n",max_prio);
		currq = &ma_main_runqueue;
#endif
		if (!currq->as_holder.nb_ready_entities) {
			MARCEL_SCHED_LOG("apparently nobody in %s\n",currq->as_holder.name);
		} else {
			idx = ma_sched_find_first_bit(currq->active->bitmap);
			if (idx < max_prio) {
				MARCEL_SCHED_LOG("found better prio %d in %s\n",idx,currq->as_holder.name);
				cur = NULL;
				max_prio = idx;
				nexth = &currq->as_holder;
				/* let polling know that this context switch is urging */
				hard_preempt = 1;
			}
			if (cur && need_resched && idx == prev_as_prio && idx < MA_IDLE_PRIO) {
				/* still wanted to schedule prev, but it needs resched
				 * and this is same prio
				 */
				MARCEL_SCHED_LOG("found same prio %d in %s\n",idx,currq->as_holder.name);
				cur = NULL;
				nexth = &currq->as_holder;
			}
		}
#ifdef MA__LWPS
	}
#endif

	if (tbx_unlikely(nexth == &ma_dontsched_rq(MA_LWP_SELF)->as_holder)) {
		/* found no interesting queue, not even previous one */
#ifdef MA__LWPS
		if (prev->state == MA_TASK_INTERRUPTIBLE || prev->state == MA_TASK_UNINTERRUPTIBLE) {
			ma_about_to_idle();
			if (!prev->state) {
				/* We got woken up. */
				if (ma_get_need_resched()) {
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
		MARCEL_SCHED_LOG("rebalance\n");
//		load_balance(rq, 1, cpu_to_node_mask(smp_processor_id()));
#ifdef MA__BUBBLES
		if (ma_idle_scheduler_is_running ())
			if (ma_vpnum(MA_LWP_SELF) < (int)marcel_nbvps() && ma_vpnum(MA_LWP_SELF) >= 0)
			{
				if (ma_bubble_notify_idle_vp(ma_vpnum(MA_LWP_SELF)))
					goto need_resched_atomic;
			}
#endif
		cur = __ma_get_lwp_var(idle_task);
#else /* MONO */
		/* mono: nobody can use our stack, so there's no need for idle
		 * thread */
		if (!ma_currently_idle) {
			ma_entering_idle();
			ma_currently_idle = tbx_true;
		}
		ma_local_bh_enable();


#ifdef MARCEL_IDLE_PAUSE
		if (didpoll)
			/* already polled a bit, sleep a bit before
			 * polling again */
			marcel_sig_nanosleep();
#endif
		didpoll = ma_schedule_hooks(MARCEL_SCHEDULING_POINT_IDLE);
		if(!didpoll)
		{
		        marcel_sig_disable_interrupts();
			marcel_sig_pause();
			marcel_sig_enable_interrupts();
		} 

		ma_check_work();
		need_resched = 1;
		ma_local_bh_disable();
		goto need_resched_atomic;
#endif /* MONO */
	} else {
#ifndef MA__LWPS
		if (ma_currently_idle) {
			ma_leaving_idle();
			ma_currently_idle = tbx_false;
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
	MARCEL_SCHED_LOG("locked(%p)\n",nexth);

	if (cur) /* either prev or idle */ {
	        next = cur;
		goto switch_tasks;
	}

	/* nexth can't be a bubble here */
	MA_BUG_ON(nexth->type != MA_RUNQUEUE_HOLDER);
	rq = ma_rq_holder(nexth);
#ifdef MA__LWPS
	if (tbx_unlikely(!(rq->active->nr_active))) { //+rq->expired->nr_active))) {
		MARCEL_SCHED_LOG("someone stole the task we saw, restart\n");
		ma_holder_unlock(&rq->as_holder);
		goto restart;
	}
#endif

	MA_BUG_ON(!(rq->active->nr_active)); //+rq->expired->nr_active));

	/* now look for next *different* task */
	array = rq->active;

	idx = ma_sched_find_first_bit(array->bitmap);
#ifdef MA__LWPS
	if (tbx_unlikely(idx > max_prio)) {
	        /* We had seen a high-priority task, but it's not there any more */
		MARCEL_SCHED_LOG("someone stole the high-priority task we saw, restart\n");
		ma_holder_unlock(&rq->as_holder);
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
		ma_dequeue_entity(&next->as_entity, nexth);
		/* Immediately mark that the seed is about to germinate before unlocking the holder. */
		next->cur_thread_seed_runner = (void *)(intptr_t)1;
		ma_holder_unlock(nexth);

		if (prev->state == MA_TASK_DEAD && !(ma_preempt_count() & MA_PREEMPT_ACTIVE) && prev->cur_thread_seed) { // && prev->shared_attr == next->shared_attr) {
			/* yeepee, exec it */
			prev->cur_thread_seed = next;
			ma_set_current_state(MA_TASK_RUNNING);
			/* we disabled preemption once in marcel_exit_internal, re-enable it once */
			ma_preempt_enable();
			MARCEL_LOG_OUT();
			if (!prev->f_to_call)
				/* marcel_exit was called directly from the
				 * runner loop, just return (faster) */
				MARCEL_LOG_RETURN(0);
			else
				marcel_ctx_longjmp(SELF_GETMEM(ctx_restart), 0);
		} else {
			int err TBX_UNUSED;

			err = instantiate_thread_seed(next, tbx_true, NULL);
			MA_BUG_ON(err != 0);

			goto need_resched_atomic;
		}
	}

	MA_BUG_ON(nextent->type != MA_THREAD_ENTITY);

switch_tasks:
	MARCEL_SCHED_LOG("prio %d in %s, next %p(%s)\n",idx,nexth->name,next,next->as_entity.name);
	MTRACE("previous",prev);
	MTRACE("next",next);

	/* still wanting to go to sleep ? (now that runqueues are locked, we can
	 * safely deactivate ourselves */

	if (go_to_sleep && ((prev->state == MA_TASK_DEAD) ||
			    !(ma_preempt_count() & MA_PREEMPT_ACTIVE)))
		/* on va dormir, il _faut_ donner la main à quelqu'un d'autre */
		MA_BUG_ON(next==prev);

//Pour quand on voudra ce mécanisme...
	//ma_RCU_qsctr(ma_task_lwp(prev))++;

	if (tbx_likely(didswitch = (prev != next))) {
		ma_clear_tsk_need_togo(prev);

		/* really switch */
		prev = do_switch(prev, next, nexth, now);

		/* TODO: si !hard_preempt, appeler le polling */
	} else {
		MARCEL_SCHED_LOG("unlock(%p)\n",nexth);
		ma_holder_unlock_softirq(nexth);
#ifdef MA__LWPS
		if (! (tbx_unlikely(MARCEL_SELF == __ma_get_lwp_var(idle_task))))
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
	if (tbx_unlikely(ma_get_need_resched() && ma_thread_preemptible())) {
		MARCEL_SCHED_LOG("need resched\n");
		need_resched = 1;
		goto need_resched;
	}
	MARCEL_SCHED_LOG("switched\n");

	MA_BUG_ON(ma_preempt_count()<0);
	if (tbx_unlikely(ma_in_atomic())) {
		PM2_LOG("bad: scheduling while atomic (%06x)! Did you forget to unlock a spinlock?\n",ma_preempt_count());
		ma_show_preempt_backtrace();
		MA_BUG();
	}

	MARCEL_LOG_RETURN(didswitch);
}

int marcel_yield_to(marcel_t next)
{
	marcel_t prev = MARCEL_SELF;
	ma_holder_t *nexth;

	if (next==prev)
		return 0;

	MARCEL_LOG_IN();

	nexth = ma_task_holder_lock_softirq(next);
	if (ma_entity_task(next)->type == MA_THREAD_SEED_ENTITY) {
		if (!next->cur_thread_seed_runner) {
			marcel_t runner;
			int err TBX_UNUSED;

			err = instantiate_thread_seed(next, tbx_false, &runner);
			MA_BUG_ON(err != 0);

			ma_dequeue_entity(&next->as_entity, nexth);
			ma_task_holder_unlock_softirq(nexth);

			marcel_wake_up_created_thread(runner);
			next = runner;
		} else {
			next = next->cur_thread_seed_runner;

			ma_task_holder_unlock_softirq(nexth);
		}

		nexth = ma_task_holder_lock_softirq(next);
	}

	if (!MA_TASK_IS_READY(next)) {
#ifdef PM2DEBUG
	        int busy = MA_TASK_IS_RUNNING(next);
#endif
		ma_task_holder_unlock_softirq(nexth);
		MARCEL_SCHED_LOG("marcel_yield: %s task %p\n", busy ?"busy":"not enqueued", next);
		MARCEL_LOG_OUT();
		return -1;
	}

	// we suppose we don't want to go to sleep, and we're not yielding to a
	// dontsched thread like idle
	
	prev = do_switch(prev, next, nexth, marcel_clock());

	MARCEL_LOG_OUT();
	return 0;
}

/* This function tries to yield to one of the threads contained in
   _team_. Candidates are determined by the _mask_, and have to be on
   the same runqueue to respect affinity relations.  The _padding_
   argument represents the "gap" between two values in _mask_.*/
int marcel_yield_to_team(marcel_t *team, char *mask, unsigned padding, unsigned nb_teammates) {
	unsigned i;

	MA_BUG_ON(!padding);

	/* To be on the safe side (MA_BUG_ON could be disabled, depending on
	   flavor configuration)*/
	if (!padding)
		padding = 1;

	for (i = 0; i < nb_teammates; i++) {
		if (mask[i * padding])
			continue;
    
		/* We're looking for a ready (R*) thread... */
		if (!team[i] || team[i] == ma_self() || team[i]->state != MA_TASK_RUNNING)
			continue;
      
		/* ... that is scheduled on the same runqueue we're being executed... */
		/* TODO: handle cases where the team[i] runqueue is inside the
		   subtree under the ma_self() runqueue. */
		if (ma_to_rq_holder(ma_task_sched_holder(ma_self())) != ma_to_rq_holder(ma_task_sched_holder(team[i])))
			continue;
      
		/* ... and is not currently running (!RR). */
		if (MA_TASK_IS_RUNNING(team[i]))
			continue;

		/* Hurray ! We finally found someone !*/
		if (marcel_yield_to(team[i]) == 0) {
			MARCEL_SCHED_LOG("marcel_yield_to_team: We successfully yielded to thread %d::%p\n", i, team[i]);
			return 0;
		}
	}
  
	return -1;
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
        if (ma_get_need_resched())
                goto need_resched;
}

// Effectue un changement de contexte + éventuellement exécute des
// fonctions de scrutation...

DEF_MARCEL_PMARCEL(int, yield, (void), (),
		   {
			   MARCEL_LOG_IN();
			   ma_schedule_hooks(MARCEL_SCHEDULING_POINT_YIELD);
			   ma_set_need_resched(1);
			   ma_schedule();

			   MARCEL_LOG_OUT();
			   return 0;
		   })
/* La définition n'est pas toujours dans pthread.h */
extern int pthread_yield (void) __THROW;
DEF_PTHREAD_STRONG(int, yield, (void), ())

#ifdef MA__LWPS
ma_holder_t *
ma_bind_to_holder(int do_move, ma_holder_t *new_holder) {
	ma_holder_t *old_sched_h;
	ma_holder_t *old_natural_h BUBBLE_VAR_UNUSED;

	MARCEL_LOG_IN();
	old_sched_h = ma_task_holder_lock_softirq(MARCEL_SELF);
	if (old_sched_h == new_holder) {
		ma_task_holder_unlock_softirq(old_sched_h);
		MARCEL_LOG_OUT();
		return old_sched_h;
	}

	ma_clear_ready_holder(&MARCEL_SELF->as_entity,old_sched_h);
	ma_task_sched_holder(MARCEL_SELF) = NULL;
	ma_holder_rawunlock(old_sched_h);
#ifdef MA__BUBBLES
	old_natural_h = ma_task_natural_holder(MARCEL_SELF);
	if (old_natural_h && old_natural_h->type == MA_BUBBLE_HOLDER)
		marcel_bubble_removetask(ma_bubble_holder(old_natural_h),MARCEL_SELF);
#endif
	ma_holder_rawlock(new_holder);
	ma_task_sched_holder(MARCEL_SELF) = new_holder;
	ma_set_ready_holder(&MARCEL_SELF->as_entity,new_holder);
	ma_holder_rawunlock(new_holder);
	/* On teste si le LWP courant est interdit ou pas */
	if (do_move) {
		ma_set_current_state(MA_TASK_MOVING);
		ma_local_bh_enable();
		ma_preempt_enable_no_resched();
		ma_schedule();
	} else {
		ma_local_bh_enable();
		ma_preempt_enable();
	}
	MARCEL_LOG_RETURN(old_sched_h);
}
#define ma_apply_vpset_rq(vpset, rq)					\
	ma_bind_to_holder(ma_spare_lwp() || !marcel_vpset_isset((vpset),ma_vpnum(MA_LWP_SELF)), &(rq)->as_holder)
#define ma_apply_vpset(vpset)						\
	ma_apply_vpset_rq((vpset), marcel_sched_vpset_init_rq(vpset))
#endif

// Modifie le 'vpset' du thread courant. Le cas échéant, il faut donc
// retirer le thread de la file et le replacer dans la file
// adéquate...
// IMPORTANT : cette fonction doit marcher si on l'appelle en section atomique
// pour se déplacer sur le LWP courant (cf terminaison des threads)
void marcel_apply_vpset(const marcel_vpset_t *vpset LWPS_VAR_UNUSED)
{
#ifdef MA__LWPS
	MARCEL_LOG_IN();
	ma_apply_vpset(vpset);
	MARCEL_LOG_OUT();
#endif
}

DEF_PMARCEL(int, setaffinity_np, (pmarcel_t pid TBX_UNUSED, size_t cpusetsize TBX_UNUSED, const pmarcel_cpu_set_t *cpuset), (pid, cpusetsize, cpuset),
	    {
		    MA_BUG_ON((marcel_t) pid != ma_self());
		    MA_BUG_ON(cpusetsize != PMARCEL_CPU_SETSIZE / 8);
		    marcel_apply_vpset(cpuset);
		    return 0;
	    })

#ifdef MA__LIBPTHREAD
int lpt_setaffinity_np(pthread_t pid, size_t cpusetsize, const cpu_set_t *cpuset)
{
	marcel_vpset_t vpset;
	int ret;
	
	if (0 != pid)
		return EINVAL;

	if ((ret = marcel_cpuset2vpset(cpusetsize, cpuset, &vpset)))
		return ret;
	marcel_apply_vpset(&vpset);
	return 0;
}
versioned_symbol(libpthread, lpt_setaffinity_np, pthread_setaffinity_np, GLIBC_2_3_4);
#endif

DEF_PMARCEL(int, getaffinity_np, (pmarcel_t pid LWPS_VAR_UNUSED, size_t cpusetsize TBX_UNUSED, pmarcel_cpu_set_t *cpuset), (pid, cpusetsize, cpuset),
	    {
#ifdef MA__LWPS
		    marcel_t task = (marcel_t) pid;
		    ma_holder_t *h = ma_task_sched_holder(task);
		    ma_runqueue_t *rq = ma_to_rq_holder(h);
		    MA_BUG_ON(cpusetsize != PMARCEL_CPU_SETSIZE / 8);
		    if (!rq)
			    return EIO;
		    *cpuset = rq->vpset;
#else
		    *cpuset = MARCEL_VPSET_VP(1);
#endif
		    return 0;
	    })
#ifdef MA__LIBPTHREAD
int lpt_getaffinity_np(pthread_t pid, size_t cpusetsize, cpu_set_t *cpuset)
{
	marcel_vpset_t vpset;
	int ret;
	if ((ret = pmarcel_getaffinity_np(pid, sizeof(vpset), &vpset)))
		return ret;
	if ((ret = marcel_vpset2cpuset(&vpset, cpusetsize, cpuset)))
		return ret;
	return 0;
}
versioned_symbol(libpthread, lpt_getaffinity_np, pthread_getaffinity_np, GLIBC_2_3_4);
#endif

void marcel_bind_to_topo_level(marcel_topo_level_t *level LWPS_VAR_UNUSED)
{
#ifdef MA__LWPS
	MARCEL_LOG_IN();
	ma_apply_vpset_rq(&level->vpset, &level->rq);
	MARCEL_LOG_OUT();
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
	return p->as_entity.prio - MA_MAX_RT_PRIO;
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

void __ma_cond_resched(void)
{
	ma_set_current_state(MA_TASK_RUNNING);
	ma_schedule();
}

static void linux_sched_lwp_init(ma_lwp_t lwp LWPS_VAR_UNUSED)
{
#ifdef MA__LWPS
        int num = ma_vpnum(lwp);
	char name[16];
	ma_runqueue_t *rq;
#endif
	MARCEL_LOG_IN();
	/* en mono, rien par lwp, tout est initialisé dans sched_init */
#ifdef MA__LWPS
	rq = ma_lwp_rq(lwp);
	if (num == -1)
		/* "extra" LWPs are apart */
		rq->father=NULL;
	else {
		rq->father=&marcel_topo_vp_level[num].rq;
		if (num < (int)marcel_nbvps()) {
			marcel_vpset_set(&ma_main_runqueue.vpset,num);
			marcel_vpset_set(&ma_dontsched_runqueue.vpset,num);
		}
	}
	if (rq->father)
		MARCEL_LOG("runqueue %s has father %s\n",name,rq->father->as_holder.name);
	rq->level = marcel_topo_nblevels-1;
	ma_per_lwp(current_thread,lwp) = ma_per_lwp(run_task,lwp);
	marcel_vpset_zero(&(rq->vpset));
	if (num != -1)
	{
		if (num >= (int)marcel_nbvps()) {
			snprintf(name,sizeof(name), "vp%d", num);
			rq = &marcel_topo_vp_level[num].rq;
			ma_init_rq(rq, name);
			rq->level = marcel_topo_nblevels-1;
			rq->father = NULL;
			marcel_vpset_vp(&rq->vpset, num);
			MARCEL_LOG("runqueue %s is a supplementary runqueue\n", name);
			PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ,2),rq->level,rq);
		}
		else
			ma_remote_tasklet_init(&marcel_topo_vp_level[num].vpdata.tasklet_lock);
	}
#endif
	MARCEL_LOG_OUT();
}

static void linux_sched_lwp_start(ma_lwp_t lwp)
{
	ma_holder_t *h;
	marcel_task_t *p = ma_per_lwp(run_task,lwp);
	/* Cette tâche est en train d'être exécutée */
	h=ma_task_holder_lock_softirq(p);
	ma_set_ready_holder(&p->as_entity,h);
	ma_task_holder_unlock_softirq(h);
#ifdef MARCEL_STATS_ENABLED
	ma_task_stats_set(long, p, ma_stats_nbrunning_offset, 1);
	ma_task_stats_set(long, p, ma_stats_nbready_offset, 1);
#endif
}

MA_DEFINE_LWP_NOTIFIER_START_PRIO(linux_sched, 200, "Linux scheduler",
				  linux_sched_lwp_init, "Initialisation des runqueue",
				  linux_sched_lwp_start, "Activation de la tâche courante");

MA_LWP_NOTIFIER_CALL_UP_PREPARE(linux_sched, MA_INIT_LINUX_SCHED);

#ifdef MA__LWPS
static void init_subrunqueues(struct marcel_topo_level *level, ma_runqueue_t *rq, int levelnum) {
	unsigned i;
	char name[16];
	ma_runqueue_t *newrq;

	if (!level->arity || level->type == MARCEL_LEVEL_LAST) {
		return;
	}

	for (i=0;i<level->arity;i++) {
#ifdef MA__NUMA
		if (level->children[i]->type == MARCEL_LEVEL_FAKE)
			snprintf(name, sizeof(name), "Fake%d-%u",
				 levelnum, level->children[i]->number);
		else if (level->children[i]->type == MARCEL_LEVEL_MISC)
			snprintf(name, sizeof(name), "Misc%d-%u",
				 levelnum, level->children[i]->number);
		else
#endif
			snprintf(name,sizeof(name), "%s%u",
				 marcel_topo_level_string(level->children[i]->type),
				 level->children[i]->number);
		newrq = &level->children[i]->rq;
		ma_init_rq(newrq, name);
		newrq->topolevel = level->children[i];
		newrq->level = levelnum;
		newrq->father = rq;
		newrq->vpset = level->children[i]->vpset;
		MARCEL_LOG("runqueue %s has father %s\n", name, rq->as_holder.name);
		PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ,2),levelnum,newrq);
		init_subrunqueues(level->children[i],newrq,levelnum+1);
	}
}
#endif

void ma_linux_sched_init0(void)
{
	ma_init_rq(&ma_main_runqueue,"machine");
	ma_main_runqueue.topolevel = marcel_machine_level;
#ifdef MA__LWPS
	ma_main_runqueue.level = 0;
#endif
	ma_init_rq(&ma_dontsched_runqueue,"dontsched");
#ifdef MA__LWPS
	marcel_vpset_zero(&ma_main_runqueue.vpset);
	marcel_vpset_zero(&ma_dontsched_runqueue.vpset);
#endif

	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWLEVEL,1),1);
	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ,2),-1,&ma_dontsched_runqueue);
	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ,2),0,&ma_main_runqueue);
}


TBX_VISIBILITY_PUSH_INTERNAL
unsigned long ma_stats_nbthreads_offset, ma_stats_nbthreadseeds_offset,
	ma_stats_nbrunning_offset, ma_stats_nbready_offset,
	ma_stats_last_ran_offset, ma_stats_last_vp_offset, 
	ma_stats_last_topo_level_offset;
unsigned long ma_stats_load_offset;
#ifdef MARCEL_MAMI_ENABLED
unsigned long ma_stats_memnode_offset;
#endif /* MARCEL_MAMI_ENABLED */
TBX_VISIBILITY_POP

static void linux_sched_init(void)
{
	ma_holder_t *h;
	MARCEL_LOG_IN();

#ifdef MARCEL_STATS_ENABLED
	ma_stats_nbthreads_offset = ma_stats_alloc(ma_stats_long_sum_reset, ma_stats_long_sum_synthesis, sizeof(long));
	ma_stats_nbthreadseeds_offset = ma_stats_alloc(ma_stats_long_sum_reset, ma_stats_long_sum_synthesis, sizeof(long));
	ma_stats_nbrunning_offset = ma_stats_alloc(ma_stats_long_sum_reset, ma_stats_long_sum_synthesis, sizeof(long));
	ma_stats_nbready_offset = ma_stats_alloc(ma_stats_long_sum_reset, ma_stats_long_sum_synthesis, sizeof(long));
	ma_stats_last_ran_offset = ma_stats_alloc(ma_stats_long_max_reset, ma_stats_long_max_synthesis, sizeof(long));
#ifdef MARCEL_MAMI_ENABLED
	ma_stats_memnode_offset = ma_stats_alloc(ma_stats_memnode_sum_reset, ma_stats_memnode_sum_synthesis, marcel_nbnodes * sizeof(long));
#endif /* MARCEL_MAMI_ENABLED */
	ma_stats_last_vp_offset = ma_stats_alloc (ma_stats_last_vp_sum_reset, ma_stats_last_vp_sum_synthesis, sizeof (long));
	ma_stats_load_offset = ma_stats_alloc(ma_stats_long_sum_reset, ma_stats_long_sum_synthesis, sizeof(long));
	ma_stats_last_topo_level_offset = ma_stats_alloc (ma_stats_last_topo_level_sum_reset, NULL, sizeof (long));
	ma_task_stats_set(long, __main_thread, ma_stats_load_offset, 1);
	ma_task_stats_set(long, __main_thread, ma_stats_nbthreads_offset, 1);
	ma_task_stats_set(long, __main_thread, ma_stats_nbthreadseeds_offset, 0);
	ma_task_stats_set(long, __main_thread, ma_stats_nbrunning_offset, 1);
	ma_task_stats_set (long, __main_thread, ma_stats_last_vp_offset, ma_vpnum (MA_LWP_SELF));
#endif /* MARCEL_STATS_ENABLED */

#ifdef MA__LWPS
	if (marcel_topo_nblevels>1) {
#ifdef MA__NUMA
		unsigned i,j;
		for (i=1;i<marcel_topo_nblevels-1;i++) {
			for (j=0; !marcel_vpset_iszero(&marcel_topo_levels[i][j].vpset); j++);
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
	ma_dequeue_entity(&MARCEL_SELF->as_entity, h);
	ma_task_holder_unlock(h);
	MARCEL_LOG_OUT();
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
				ma_cpu_relax();
		}
		ma_preempt_disable();
	} while (!_ma_raw_spin_trylock(lock));
}
#endif

#if defined(MA__LWPS)
void __ma_preempt_write_lock(ma_rwlock_t *lock)
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
				ma_cpu_relax();
		}
		ma_preempt_disable();
	} while (!_ma_raw_write_trylock(lock));
}
#endif 

