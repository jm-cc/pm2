
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
   - NET RX softirq. It is multithreaded and does not require
     any global serialization.
   - NET TX softirq. It kicks software netdevice queues, hence
     it is logically serialized per device, but this serialization
     is invisible to common code.
   - Tasklets: serialized wrt itself.
 */

static struct ma_softirq_action softirq_vec[32] /*__cacheline_aligned_in_smp*/;

/*
 * we cannot loop indefinitely here to avoid userspace starvation,
 * but we also don't want to introduce a worst case 1/HZ latency
 * to the pending events, so lets the scheduler to balance
 * the softirq load for us.
 */
static inline void ma_wakeup_softirqd(void)
{
	/* Interrupts are disabled: no need to stop preemption */
	/* Avec marcel, seul la preemption est supprimée */
	marcel_task_t *tsk = ma_topo_vpdata_l(__ma_get_lwp_var(vp_level),ksoftirqd);

	if (tsk && tsk->sched.state != MA_TASK_RUNNING)
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
	unsigned long pending;
	
#if 0
	SELF_SETMEM(softirq_pending_in_hardirq, 0);
	ma_set_thread_flag(TIF_BLOCK_HARDIRQ);
	ma_smp_mb__after_clear_bit();
#endif
	do {
		pending=ma_local_softirq_pending();
	} while (pending != 
		ma_cmpxchg(&ma_local_softirq_pending(), pending, 0));
	/* Reset the pending bitmask before enabling irqs */
#if 0
	ma_local_softirq_pending() = 0;
	ma_smp_mb__before_clear_bit();
	
	ma_clear_thread_flag(TIF_BLOCK_HARDIRQ);
	ma_smp_mb__after_clear_bit();
	pending|= SELF_GETMEM(softirq_pending_in_hardirq);
#endif
	return pending;
}

asmlinkage TBX_EXTERN void ma_do_softirq(void)
{
	int max_restart = MAX_SOFTIRQ_RESTART;
	unsigned long pending;
	//unsigned long flags;

	if (ma_in_interrupt())
		return;

	if (LWP_NUMBER(LWP_SELF) == -1)
		return;

	//local_irq_save(flags);
	
	ma_local_bh_disable();
	pending = local_softirq_pending_hardirq();

	if (pending) {
		struct ma_softirq_action *h;

		//ma_local_bh_disable();
restart:
		/* Reset the pending bitmask before enabling irqs */
		//ma_local_softirq_pending() = 0;

		//local_irq_enable();
		//ma_local_hardirq_enable();
		
		h = softirq_vec;

		do {
			if (pending & 1)
				h->action(h);
			h++;
			pending >>= 1;
		} while (pending);

		//local_irq_disable();
		
		pending = local_softirq_pending_hardirq();
		if (pending && --max_restart)
			goto restart;
#if 0
		if (pending) {
			pm2debug("Arghhh loosing softirq %lx! Please, correct me\n", pending);
			/* Il faudrait au moins remettre les pending
			 * en appelant ma_raise_softirq...(nr)
			 */
			ma_local_softirq_pending() |= pending;
			pending=0;
		}
#endif
		if (pending) {
			/* On remet les pending en place... */
			ma_local_softirq_pending() |= pending;
			ma_smp_wmb();
			ma_wakeup_softirqd();
		}
		//__ma_local_bh_enable();
	}

	//local_irq_restore(flags);
	/* S'il reste des softirq, il ne FAUT PAS les traiter maintenant */
	__ma_local_bh_enable();
}

void TBX_EXTERN ma_local_bh_enable(void)
{
	
	__ma_local_bh_enable();
	//MA_WARN_ON(ma_irqs_disabled());
	if (tbx_unlikely(!ma_in_interrupt() &&
		     ma_local_softirq_pending()))
		ma_invoke_softirq();
	ma_preempt_check_resched(0);
}

inline void __ma_raise_softirq_bhoff(unsigned int nr)
{
	ma_set_bit(nr, &ma_local_softirq_pending());
}
/*
 * This function must run with irqs disabled!
 * En fait, avec la préemption désactivée
 */
inline fastcall TBX_EXTERN void ma_raise_softirq_bhoff(unsigned int nr)
{
	//__ma_raise_softirq_irqoff(nr);
#ifdef MA__DEBUG
	MA_BUG_ON(!ma_in_atomic());
#endif
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
		ma_wakeup_softirqd();
}

fastcall TBX_EXTERN void ma_raise_softirq(unsigned int nr)
{
	//unsigned long flags;

	//local_irq_save(flags);
	//ma_local_bh_disable();
	ma_preempt_disable();
	ma_raise_softirq_bhoff(nr);
	ma_preempt_enable();
	//local_irq_restore(flags);
	//ma_local_bh_enable();
}

fastcall TBX_EXTERN void ma_raise_softirq_from_hardirq(unsigned int nr)
{
	MA_BUG_ON(!ma_in_irq());
	__ma_raise_softirq_bhoff(nr);
#if 0
	if (tbx_unlikely(ma_test_thread_flag(TIF_BLOCK_HARDIRQ))) {
		ma_set_bit(nr, &SELF_GETMEM(softirq_pending_in_hardirq));
	} else {
		ma_raise_softirq_bhoff(nr);
	}
#endif
}

TBX_EXTERN void ma_open_softirq(int nr, void (*action)(struct ma_softirq_action*), void *data)
{
	softirq_vec[nr].data = data;
	softirq_vec[nr].action = action;
}

fastcall TBX_EXTERN void __ma_tasklet_schedule(struct ma_tasklet_struct *t)
{
	//unsigned long flags;

	//local_irq_save(flags);
	ma_local_bh_disable();
	ma_remote_tasklet_lock(&ma_vp_lwp[marcel_current_vp()]->tasklet_lock);
	t->next = ma_topo_vpdata_l(__ma_get_lwp_var(vp_level),tasklet_vec).list;
	ma_topo_vpdata_l(__ma_get_lwp_var(vp_level),tasklet_vec).list = t;
	ma_raise_softirq_bhoff(MA_TASKLET_SOFTIRQ);
	ma_remote_tasklet_unlock(&ma_vp_lwp[marcel_current_vp()]->tasklet_lock);
	//local_irq_restore(flags);
	ma_local_bh_enable();
}

fastcall TBX_EXTERN void __ma_tasklet_hi_schedule(struct ma_tasklet_struct *t)
{
	//unsigned long flags;

	//local_irq_save(flags);
	ma_local_bh_disable();
	t->next = ma_topo_vpdata_l(__ma_get_lwp_var(vp_level),tasklet_hi_vec).list;
	ma_topo_vpdata_l(__ma_get_lwp_var(vp_level),tasklet_hi_vec).list = t;
	ma_raise_softirq_bhoff(MA_HI_SOFTIRQ);
	//local_irq_restore(flags);
	ma_local_bh_enable();
}

static void tasklet_action(struct ma_softirq_action *a)
{
	struct ma_tasklet_struct *list;

	//local_irq_disable();
	ma_local_bh_disable();
	ma_remote_tasklet_lock(&ma_vp_lwp[marcel_current_vp()]->tasklet_lock);
	list = ma_topo_vpdata_l(__ma_get_lwp_var(vp_level),tasklet_vec).list;
	ma_topo_vpdata_l(__ma_get_lwp_var(vp_level),tasklet_vec).list = NULL;
	ma_remote_tasklet_unlock(&ma_vp_lwp[marcel_current_vp()]->tasklet_lock);
	ma_remote_tasklet_unlock(&ma_vp_lwp[marcel_current_vp()]->tasklet_lock);
	//local_irq_enable();
	ma_local_bh_enable();

	while (list) {
		struct ma_tasklet_struct *t = list;

		list = list->next;
		if (ma_tasklet_trylock(t)) {
			if (!ma_atomic_read(&t->count)) {
				if (!ma_test_and_clear_bit(MA_TASKLET_STATE_SCHED, &t->state))
					MA_BUG();
				t->func(t->data);
				if (ma_tasklet_unlock(t))
					/* Somebody tried to schedule it, try to reschedule it here */
					ma_tasklet_schedule(t);
				continue;				
			}
			/* here, SCHED is always set so we already know it would return 1 */
			ma_tasklet_unlock(t);
		}

		//local_irq_disable();
		ma_local_bh_disable();
		ma_remote_tasklet_lock(&ma_vp_lwp[marcel_current_vp()]->tasklet_lock);
		t->next = ma_topo_vpdata_l(__ma_get_lwp_var(vp_level),tasklet_vec).list;
		ma_topo_vpdata_l(__ma_get_lwp_var(vp_level),tasklet_vec).list = t;
		ma_remote_tasklet_unlock(&ma_vp_lwp[marcel_current_vp()]->tasklet_lock);
		ma_remote_tasklet_unlock(&ma_vp_lwp[marcel_current_vp()]->tasklet_lock);
		__ma_raise_softirq_bhoff(MA_TASKLET_SOFTIRQ);
		//local_irq_enable();
		ma_local_bh_enable();
	}
}

static void tasklet_hi_action(struct ma_softirq_action *a)
{
	struct ma_tasklet_struct *list;

	//local_irq_disable();
	ma_local_bh_disable();
	list = ma_topo_vpdata_l(__ma_get_lwp_var(vp_level),tasklet_hi_vec).list;
	ma_topo_vpdata_l(__ma_get_lwp_var(vp_level),tasklet_hi_vec).list = NULL;
	//local_irq_enable();
	ma_local_bh_enable();

	while (list) {
		struct ma_tasklet_struct *t = list;

		list = list->next;

		if (ma_tasklet_trylock(t)) {
			if (!ma_atomic_read(&t->count)) {
				if (!ma_test_and_clear_bit(MA_TASKLET_STATE_SCHED, &t->state))
					MA_BUG();
				t->func(t->data);
				if (ma_tasklet_unlock(t))
					/* Somebody tried to schedule it, try to reschedule it here */
					ma_tasklet_hi_schedule(t);
				continue;
			}
			/* here, SCHED is always set so we already know it would return 1 */
			ma_tasklet_unlock(t);
		}

		//local_irq_disable();
		ma_local_bh_disable();
		t->next = ma_topo_vpdata_l(__ma_get_lwp_var(vp_level),tasklet_hi_vec).list;
		ma_topo_vpdata_l(__ma_get_lwp_var(vp_level),tasklet_hi_vec).list = t;
		__ma_raise_softirq_bhoff(MA_HI_SOFTIRQ);
		//local_irq_enable();
		ma_local_bh_enable();
	}
}


TBX_EXTERN void ma_tasklet_init(struct ma_tasklet_struct *t,
		     void (*func)(unsigned long), unsigned long data)
{
	t->next = NULL;
	t->state = 0;
	ma_atomic_set(&t->count, 0);
	t->func = func;
	t->data = data;
}

TBX_EXTERN void ma_tasklet_kill(struct ma_tasklet_struct *t)
{
	if (ma_in_interrupt())
		mdebug("Attempt to kill tasklet from interrupt\n");

	while (ma_test_and_set_bit(MA_TASKLET_STATE_SCHED, &t->state)) {
		do
			marcel_yield();
		while (ma_test_bit(MA_TASKLET_STATE_SCHED, &t->state));
	}
	ma_tasklet_unlock_wait(t);
	ma_clear_bit(MA_TASKLET_STATE_SCHED, &t->state);
}

static void __marcel_init softirq_init(void)
{
	ma_open_softirq(MA_TASKLET_SOFTIRQ, tasklet_action, NULL);
	ma_open_softirq(MA_HI_SOFTIRQ, tasklet_hi_action, NULL);
}

__ma_initfunc(softirq_init, MA_INIT_SOFTIRQ, "Initialisation des SoftIrq");

#define ma_kthread_should_stop() 0

static int ksoftirqd(void * __bind_cpu)
{
	//set_user_nice(current, 19);
	//current->flags |= PF_IOTHREAD;

	ma_set_current_state(MA_TASK_INTERRUPTIBLE);
 
	while (!ma_kthread_should_stop()) {
		if (tbx_unlikely(ma_in_atomic())) {
			pm2debug("bad: scheduling while atomic (%06x)! Did you forget to unlock a spinlock?\n",ma_preempt_count());
			ma_show_preempt_backtrace();
			MA_BUG();
		}
		if (!ma_local_softirq_pending())
			ma_schedule();

		__ma_set_current_state(MA_TASK_RUNNING);

		while (ma_local_softirq_pending()) {
			/* Preempt disable stops cpu going offline.
			   If already offline, we'll be on wrong CPU:
			   don't process */
			ma_preempt_disable();
			ma_do_softirq();
			ma_preempt_enable();
			ma_cond_resched();
		}

		__ma_set_current_state(MA_TASK_INTERRUPTIBLE);
	}
	__ma_set_current_state(MA_TASK_RUNNING);
	return 0;
}

#if 0
#ifdef CONFIG_HOTPLUG_CPU
/*
 * tasklet_kill_immediate is called to remove a tasklet which can already be
 * scheduled for execution on @cpu.
 *
 * Unlike tasklet_kill, this function removes the tasklet
 * _immediately_, even if the tasklet is in TASKLET_STATE_SCHED state.
 *
 * When this function is called, @cpu must be in the CPU_DEAD state.
 */
void tasklet_kill_immediate(struct tasklet_struct *t, unsigned int cpu)
{
	struct tasklet_struct **i;

	BUG_ON(cpu_online(cpu));
	BUG_ON(test_bit(TASKLET_STATE_RUN, &t->state));

	if (!test_bit(TASKLET_STATE_SCHED, &t->state))
		return;

	/* CPU is dead, so no lock needed. */
	for (i = &per_cpu(tasklet_vec, cpu).list; *i; i = &(*i)->next) {
		if (*i == t) {
			*i = t->next;
			return;
		}
	}
	BUG();
}

static void takeover_tasklets(unsigned int cpu)
{
	struct tasklet_struct **i;

	/* CPU is dead, so no lock needed. */
	local_irq_disable();

	/* Find end, append list for that CPU. */
	for (i = &__get_cpu_var(tasklet_vec).list; *i; i = &(*i)->next);
	*i = per_cpu(tasklet_vec, cpu).list;
	per_cpu(tasklet_vec, cpu).list = NULL;
	raise_softirq_irqoff(TASKLET_SOFTIRQ);

	for (i = &__get_cpu_var(tasklet_hi_vec).list; *i; i = &(*i)->next);
	*i = per_cpu(tasklet_hi_vec, cpu).list;
	per_cpu(tasklet_hi_vec, cpu).list = NULL;
	raise_softirq_irqoff(HI_SOFTIRQ);

	local_irq_enable();
}
#endif /* CONFIG_HOTPLUG_CPU */

#endif /* 0 */

inline static marcel_task_t* ksofirqd_start(ma_lwp_t lwp)
{
	marcel_attr_t attr;
	char name[MARCEL_MAXNAMESIZE];

	LOG_IN();
	/* Démarrage du thread responsable des terminaisons */
	marcel_attr_init(&attr);
	snprintf(name,MARCEL_MAXNAMESIZE,"ksoftirqd/%u",LWP_NUMBER(lwp));
	marcel_attr_setname(&attr,name);
	marcel_attr_setdetachstate(&attr, tbx_true);
	marcel_attr_setvpmask(&attr, MARCEL_VPMASK_ALL_BUT_VP(LWP_NUMBER(lwp)));
	marcel_attr_setflags(&attr, MA_SF_NORUN);
	marcel_attr_setprio(&attr, MA_SYS_RT_PRIO);
#ifdef PM2
	{
		char *stack = __TBX_MALLOC(2*THREAD_SLOT_SIZE, __FILE__, __LINE__);

		marcel_attr_setstackaddr(&attr, (void*)((unsigned long)(stack + THREAD_SLOT_SIZE) & ~(THREAD_SLOT_SIZE-1)));
	}
#endif
	marcel_create_special(&lwp->ksoftirqd_task, &attr, (void*(*)(void*))ksoftirqd, lwp);
	LOG_RETURN(lwp->ksoftirqd_task);
}

static void ksoftirqd_init(ma_lwp_t lwp)
{
	//marcel_task_t *p;

	if (LWP_NUMBER(lwp) == -1)
		return;

	MA_BUG_ON(ma_topo_vpdata_l(ma_per_lwp(vp_level, lwp),tasklet_vec).list);
	MA_BUG_ON(ma_topo_vpdata_l(ma_per_lwp(vp_level, lwp),tasklet_hi_vec).list);
}

static void ksoftirqd_start(ma_lwp_t lwp)
{
	marcel_task_t *p;

	if (LWP_NUMBER(lwp) == -1)
		return;
	p=ksofirqd_start(lwp);
	marcel_wake_up_created_thread(p);
	ma_topo_vpdata_l(ma_per_lwp(vp_level, lwp),ksoftirqd) = p;
}

#if 0
#ifdef CONFIG_HOTPLUG_CPU
	case CPU_UP_CANCELED:
		/* Unbind so it can run.  Fall thru. */
		kthread_bind(per_cpu(ksoftirqd, hotcpu), smp_processor_id());
	case CPU_DEAD:
		p = per_cpu(ksoftirqd, hotcpu);
		per_cpu(ksoftirqd, hotcpu) = NULL;
		kthread_stop(p);
		takeover_tasklets(hotcpu);
		break;
#endif /* CONFIG_HOTPLUG_CPU */
#endif

MA_DEFINE_LWP_NOTIFIER_START(ksoftirqd, "ksoftirqd",
			     ksoftirqd_init, "Check tasklet lists",
			     ksoftirqd_start, "Create and launch 'ksoftirqd'");
MA_LWP_NOTIFIER_CALL_UP_PREPARE(ksoftirqd, MA_INIT_SOFTIRQ_KSOFTIRQD);
MA_LWP_NOTIFIER_CALL_ONLINE(ksoftirqd, MA_INIT_SOFTIRQ_KSOFTIRQD);


