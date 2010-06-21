
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
 *	linux/kernel/softirq.c
 *
 *	Copyright (C) 1992 Linus Torvalds
 *
 * Rewritten. Old one was good in 2.2, but in 2.3 it was immoral. --ANK (990903)
 */

#include "marcel.h"
#include <stdio.h>

/*
   - No shared variables, all the data are CPU local.
   - If a softirq needs serialization, let it serialize itself
     by its own spinlocks.
   - Even if softirq is serialized, only local cpu is marked for
     execution. Hence, we get something sort of weak cpu binding.
     Though it is still not clear, will it result in better locality
     or will not.

   Examples:
   - Tasklets: serialized wrt itself.
 */

static struct ma_softirq_action softirq_vec[MA_NR_SOFTIRQs];

/*
 * we cannot loop indefinitely here to avoid userspace starvation,
 * but we also don't want to introduce a worst case 1/HZ latency
 * to the pending events, so lets the scheduler to balance
 * the softirq load for us.
 */
static inline void ma_wakeup_vp_softirqd(void)
{
	/* Interrupts are disabled: no need to stop preemption */
	/* Avec marcel, seul la preemption est supprimée */
	marcel_task_t *tsk = ma_topo_vpdata_l(__ma_get_lwp_var(vp_level), ksoftirqd);

	if (tsk && tsk->state != MA_TASK_RUNNING)
		ma_wake_up_thread(tsk);
}

static inline void ma_wakeup_lwp_softirqd(void)
{
	/* Interrupts are disabled: no need to stop preemption */
	/* Avec marcel, seul la preemption est supprimée */
	marcel_task_t *tsk = __ma_get_lwp_var(ksoftirqd_task);

	if (tsk && tsk->state != MA_TASK_RUNNING)
		ma_wake_up_thread(tsk);
}

/*
 * We restart softirq processing MAX_SOFTIRQ_RESTART times,
 * and we fall back to softirqd after that.
 *
 * This number has been established via experimentation.
 * The two things to balance is latency against fairness -
 * we want to handle softirqs as soon as possible, but they
 * should not be able to lock up the box.
 */
#define MAX_SOFTIRQ_RESTART 10

inline static unsigned long local_softirq_pending_hardirq(void)
{
	unsigned long v;
	do {
		v = ma_local_softirq_pending();
	} while (v != ma_cmpxchg(&ma_local_softirq_pending(), v, 0));
	return v;
}

inline static unsigned long vp_softirq_pending_hardirq(void)
{
	unsigned long v;
	do {
		v = ma_softirq_pending_vp(ma_vpnum(MA_LWP_SELF));
	} while (v != ma_cmpxchg(&ma_softirq_pending_vp(ma_vpnum(MA_LWP_SELF)), v, 0));
	return v;
}

asmlinkage void ma_do_softirq(void)
{
	int max_restart;
	unsigned long pending;
	ma_lwp_t lwp = MA_LWP_SELF;
	unsigned vp = ma_vpnum(lwp);

	if (ma_in_interrupt())
		return;

	if (ma_spare_lwp())
		return;

	ma_local_bh_disable();

#define DO_SOFTIRQ(local_pending,level) \
	max_restart = MAX_SOFTIRQ_RESTART; \
	pending = local_pending(); \
\
	if (pending) { \
		struct ma_softirq_action *h; \
\
restart##level: \
		h = softirq_vec; \
\
		do { \
			if (pending & 1) \
				h->action(h); \
			h++; \
			pending >>= 1; \
		} while (pending); \
		 \
		pending = local_pending(); \
		if (pending && --max_restart) \
			goto restart##level; \
		if (pending) { \
			/* Too much work, let's stop for now and just let the \
			 * softirq daemon handle them */ \
			ma_softirq_pending_##level(level) |= pending; \
			ma_smp_wmb(); \
			ma_wakeup_##level##_softirqd(); \
		} \
	}

	DO_SOFTIRQ(local_softirq_pending_hardirq, lwp)
	    DO_SOFTIRQ(vp_softirq_pending_hardirq, vp)

	    /* Do not process still pending softirqs */
	    __ma_local_bh_enable();
}

void ma_local_bh_enable_no_resched(void)
{
	__ma_local_bh_enable();
	if (tbx_likely(!ma_spare_lwp())) {
		if (tbx_unlikely(!ma_in_interrupt() &&
				 (ma_local_softirq_pending() ||
				  (ma_vpnum(MA_LWP_SELF) != -1
				   && ma_softirq_pending_vp(ma_vpnum(MA_LWP_SELF))))))
			ma_do_softirq();
	}
}

void ma_local_bh_enable(void)
{
	ma_local_bh_enable_no_resched();
	ma_preempt_check_resched(0);
}

void marcel_tasklet_enable(void)
{
	ma_local_bh_enable();
}

void marcel_spin_unlock_tasklet_enable(marcel_spinlock_t * lock)
{
	ma_spin_unlock_softirq(&lock->lock);
}

void marcel_tasklet_disable(void)
{
	ma_local_bh_disable();
}

void marcel_spin_lock_tasklet_disable(marcel_spinlock_t * lock)
{
	ma_spin_lock_softirq(&lock->lock);
}

static inline void __ma_raise_softirq_bhoff(unsigned int nr)
{
	MA_BUG_ON(nr >= MA_NR_SOFTIRQs);
	ma_set_bit(nr, &ma_softirq_pending_vp(ma_vpnum(MA_LWP_SELF)));
}

static inline void __ma_raise_softirq_bhoff_lwp(ma_lwp_t lwp, unsigned int nr)
{
	MA_BUG_ON(nr >= MA_NR_SOFTIRQs);
	ma_set_bit(nr, &ma_softirq_pending_lwp(lwp));
}

/*
 * This function must run with irqs disabled!
 * En fait, avec la préemption désactivée
 */
inline fastcall void ma_raise_softirq_bhoff(unsigned int nr)
{
	MA_BUG_ON(!ma_in_atomic());
	__ma_raise_softirq_bhoff(nr);

	/*
	 * If we're in an interrupt or softirq, we're done
	 * (this also catches softirq-disabled code). We will
	 * actually run the softirq once we return from
	 * the irq or softirq.
	 *
	 * Otherwise we wake up ksoftirqd to make sure we
	 * schedule the softirq soon.
	 */
	if (!ma_in_interrupt())
		ma_wakeup_vp_softirqd();
}

fastcall void ma_raise_softirq_lwp(ma_lwp_t lwp, unsigned int nr)
{
	ma_preempt_disable();
	__ma_raise_softirq_bhoff_lwp(lwp, nr);
#ifdef MA__LWPS
	if (lwp == MA_LWP_SELF)
#endif
		ma_wakeup_lwp_softirqd();
#ifdef MA__LWPS
	else
		MA_LWP_RESCHED(lwp);
#endif
	ma_preempt_enable();
}

fastcall void ma_raise_softirq(unsigned int nr)
{
	ma_preempt_disable();
	ma_raise_softirq_bhoff(nr);
	ma_preempt_enable();
}

fastcall void ma_raise_softirq_from_hardirq(unsigned int nr)
{
	MA_BUG_ON(!ma_in_irq());
	__ma_raise_softirq_bhoff(nr);
}

void ma_open_softirq(int nr, void (*action) (struct ma_softirq_action *), void *data)
{
	softirq_vec[nr].data = data;
	softirq_vec[nr].action = action;
}

fastcall void __ma_tasklet_schedule(struct marcel_tasklet_struct *t)
{
	struct marcel_topo_vpdata *const vp = ma_topo_vpdata_self();

	ma_local_bh_disable();

	ma_remote_tasklet_lock(&vp->tasklet_lock);
	t->next = vp->tasklet_vec.list;
	vp->tasklet_vec.list = t;
	ma_raise_softirq_bhoff(MA_TASKLET_SOFTIRQ);
	ma_remote_tasklet_unlock(&vp->tasklet_lock);

	ma_local_bh_enable();
}

fastcall void __ma_tasklet_hi_schedule(struct marcel_tasklet_struct *t)
{
	struct marcel_topo_vpdata *const vp = ma_topo_vpdata_self();

	ma_local_bh_disable();

	t->next = vp->tasklet_hi_vec.list;
	vp->tasklet_hi_vec.list = t;
	ma_raise_softirq_bhoff(MA_HI_SOFTIRQ);

	ma_local_bh_enable();
}

static void tasklet_action(struct ma_softirq_action *a TBX_UNUSED)
{
	struct marcel_topo_vpdata *const vp = ma_topo_vpdata_self();
	struct marcel_tasklet_struct *list;

	ma_local_bh_disable();

	ma_remote_tasklet_lock(&vp->tasklet_lock);
	list = vp->tasklet_vec.list;
	vp->tasklet_vec.list = NULL;
	ma_remote_tasklet_unlock(&vp->tasklet_lock);

	ma_local_bh_enable();

	while (list) {
		struct marcel_tasklet_struct *t = list;
		list = list->next;
		if (ma_tasklet_trylock(t)) {
			if (!ma_atomic_read(&t->count)) {
				if (!ma_test_and_clear_bit
				    (MA_TASKLET_STATE_SCHED, &t->state))
					MA_BUG();
				t->func(t->data);

				/***********************************************************************************
                                 * if (ma_tasklet_unlock(t))                                                       *
				 * 	/\* Somebody tried to schedule it, try to reschedule it here *\/           *
				 * 	ma_tasklet_schedule(t);x                                                   *
                                 ***********************************************************************************/

				ma_tasklet_unlock(t);
				continue;
			}
			/* here, SCHED is always set so we already know it would return 1 */
			ma_tasklet_unlock(t);
		}

		ma_local_bh_disable();
		ma_remote_tasklet_lock(&vp->tasklet_lock);
		t->next = vp->tasklet_vec.list;
		vp->tasklet_vec.list = t;
		ma_remote_tasklet_unlock(&vp->tasklet_lock);
		__ma_raise_softirq_bhoff(MA_TASKLET_SOFTIRQ);

		ma_local_bh_enable();
	}
}

static void tasklet_hi_action(struct ma_softirq_action *a TBX_UNUSED)
{
	struct marcel_topo_vpdata *const vp = ma_topo_vpdata_self();
	struct marcel_tasklet_struct *list;

	ma_local_bh_disable();

	list = vp->tasklet_hi_vec.list;
	vp->tasklet_hi_vec.list = NULL;

	ma_local_bh_enable();

	while (list) {
		struct marcel_tasklet_struct *t = list;
		list = list->next;
		if (ma_tasklet_trylock(t)) {
			if (!ma_atomic_read(&t->count)) {
				if (!ma_test_and_clear_bit
				    (MA_TASKLET_STATE_SCHED, &t->state))
					MA_BUG();
				t->func(t->data);

				/***********************************************************************************
                                 * if (ma_tasklet_unlock(t))                                                       *
				 * 	/\* Somebody tried to schedule it, try to reschedule it here *\/           *
				 * 	ma_tasklet_hi_schedule(t);                                                 *
                                 ***********************************************************************************/

				ma_tasklet_unlock(t);
				continue;
			}
			/* here, SCHED is always set so we already know it would return 1 */
			ma_tasklet_unlock(t);
		}

		ma_local_bh_disable();

		t->next = vp->tasklet_hi_vec.list;
		vp->tasklet_hi_vec.list = t;
		__ma_raise_softirq_bhoff(MA_HI_SOFTIRQ);

		ma_local_bh_enable();
	}
}


void ma_tasklet_init(struct marcel_tasklet_struct *t,
		     void (*func) (unsigned long), unsigned long data, int preempt)
{
	t->next = NULL;
	t->state = 0;
	ma_atomic_init(&t->count, 0);
	t->func = func;
	t->data = data;
	t->preempt = preempt;
#ifdef MARCEL_REMOTE_TASKLETS
	marcel_vpset_fill(&t->vp_set);
#endif				/* MARCEL_REMOTE_TASKLETS */
}


void marcel_tasklet_init(struct marcel_tasklet_struct *t, void (*func) (unsigned long), unsigned long data, int preempt)
{
	ma_tasklet_init(t, func, data, preempt);
}


void marcel_tasklet_set_vpset(struct marcel_tasklet_struct *t, marcel_vpset_t vp_set)
{
#ifdef MARCEL_REMOTE_TASKLETS	
	t->vp_set = vp_set;
#endif
}


void ma_tasklet_kill(struct marcel_tasklet_struct *t)
{
	if (ma_in_interrupt())
		MARCEL_LOG("Attempt to kill tasklet from interrupt\n");

	while (ma_test_and_set_bit(MA_TASKLET_STATE_SCHED, &t->state)) {
		do {
			//marcel_yield();
		} while (ma_test_bit(MA_TASKLET_STATE_SCHED, &t->state));

	}
	ma_tasklet_unlock_wait(t);
	ma_clear_bit(MA_TASKLET_STATE_SCHED, &t->state);
}

static void softirq_init(void)
{
	ma_open_softirq(MA_TASKLET_SOFTIRQ, tasklet_action, NULL);
	ma_open_softirq(MA_HI_SOFTIRQ, tasklet_hi_action, NULL);
}

__ma_initfunc(softirq_init, MA_INIT_SOFTIRQ, "Initialisation des SoftIrq");

#define ma_kthread_should_stop() 0

static int ksoftirqd(void *foo TBX_UNUSED)
{
	ma_set_current_state(MA_TASK_INTERRUPTIBLE);
	ma_preempt_disable();

	while (!ma_kthread_should_stop()) {
		if (!ma_local_softirq_pending()
		    && (ma_vpnum(MA_LWP_SELF) == -1
			|| !ma_softirq_pending_vp(ma_vpnum(MA_LWP_SELF)))) {
			ma_preempt_enable();
			if (tbx_unlikely(ma_in_atomic())) {
				PM2_LOG
				    ("bad: scheduling while atomic (%06x)! Did you forget to unlock a spinlock?\n",
				     ma_preempt_count());
				ma_show_preempt_backtrace();
				MA_BUG();
			}
			ma_schedule();
			ma_preempt_disable();
		}

		__ma_set_current_state(MA_TASK_RUNNING);

		while (ma_local_softirq_pending()
		       || (ma_vpnum(MA_LWP_SELF) != -1
			   && ma_softirq_pending_vp(ma_vpnum(MA_LWP_SELF)))) {
			/* Preempt disable stops cpu going offline.
			   If already offline, we'll be on wrong CPU:
			   don't process */
			ma_do_softirq();
		}

		__ma_set_current_state(MA_TASK_INTERRUPTIBLE);
	}

	/** function never returns
	 *  __ma_set_current_state(MA_TASK_RUNNING);
	 *  return 0;
	 **/
}

inline static marcel_task_t *ksofirqd_start(ma_lwp_t lwp, int vp, const char *fmt, ...)
{
	marcel_t t;
	marcel_attr_t attr;
	char name[MARCEL_MAXNAMESIZE];
	int err TBX_UNUSED;
	va_list ap;

	MARCEL_LOG_IN();
	/* Démarrage du thread responsable des terminaisons */
	marcel_attr_init(&attr);
	va_start(ap, fmt);
	vsnprintf(name, MARCEL_MAXNAMESIZE, fmt, ap);
	va_end(ap);
	marcel_attr_setname(&attr, name);
	marcel_attr_setdetachstate(&attr, tbx_true);
	if (lwp)
		marcel_attr_setschedrq(&attr, ma_lwp_rq(lwp));
	else
		marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(vp));
	marcel_attr_setflags(&attr, MA_SF_NORUN);
	marcel_attr_setprio(&attr, MA_SYS_RT_PRIO);
#ifdef PM2
	{
		char *stack = malloc(2 * THREAD_SLOT_SIZE);
		marcel_attr_setstackaddr(&attr,
					 (void
					  *) ((unsigned long) (stack +
							       THREAD_SLOT_SIZE) &
					      ~(THREAD_SLOT_SIZE - 1)));
	}
#endif
	err = marcel_create_special(&t, &attr, (void *(*)(void *)) ksoftirqd, NULL);
	MA_BUG_ON(err != 0);
	marcel_wake_up_created_thread(t);

	return t;
}

static void ksoftirqd_init(ma_lwp_t lwp TBX_UNUSED)
{
	if (ma_spare_lwp_ext(lwp))
		return;
	MA_BUG_ON(ma_topo_vpdata_l(ma_per_lwp(vp_level, lwp), tasklet_vec).list);
	MA_BUG_ON(ma_topo_vpdata_l(ma_per_lwp(vp_level, lwp), tasklet_hi_vec).list);
}

static void ksoftirqd_start(ma_lwp_t lwp)
{
	marcel_task_t *p;
	if (ma_spare_lwp_ext(lwp))
		return;
	p = ksofirqd_start(NULL, ma_vpnum(lwp), "ksoftirqd/vp%2d", ma_vpnum(lwp));
	ma_topo_vpdata_l(ma_per_lwp(vp_level, lwp), ksoftirqd) = p;
	p = ksofirqd_start(lwp, -1, "ksoftirqd/lwp%p", lwp);
	ma_per_lwp(ksoftirqd_task, lwp) = p;
}

MA_DEFINE_LWP_NOTIFIER_START(ksoftirqd, "ksoftirqd",
			     ksoftirqd_init, "Check tasklet lists",
			     ksoftirqd_start, "Create and launch 'ksoftirqd'");
MA_LWP_NOTIFIER_CALL_UP_PREPARE(ksoftirqd, MA_INIT_SOFTIRQ_KSOFTIRQD);
MA_LWP_NOTIFIER_CALL_ONLINE(ksoftirqd, MA_INIT_SOFTIRQ_KSOFTIRQD);
