
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

//static DEFINE_PER_CPU(struct task_struct *, ksoftirqd);
MA_DEFINE_PER_LWP(unsigned long, softirq_pending)=0;

#if 0
/*
 * we cannot loop indefinitely here to avoid userspace starvation,
 * but we also don't want to introduce a worst case 1/HZ latency
 * to the pending events, so lets the scheduler to balance
 * the softirq load for us.
 */
static inline void wakeup_softirqd(void)
{
	/* Interrupts are disabled: no need to stop preemption */
	struct task_struct *tsk = __get_cpu_var(ksoftirqd);

	if (tsk && tsk->state != TASK_RUNNING)
		wake_up_process(tsk);
}
#endif

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


inline static int local_softirq_pending_hardirq(void)
{
	int pending;
	
	THREAD_SETMEM(MARCEL_SELF, softirq_pending_in_hardirq, 0);
	ma_set_thread_flag(TIF_BLOCK_HARDIRQ);
	ma_smp_mb__after_clear_bit();
	
	pending=ma_local_softirq_pending();
	/* Reset the pending bitmask before enabling irqs */
	ma_local_softirq_pending() = 0;
	ma_smp_mb__before_clear_bit();
	
	ma_clear_thread_flag(TIF_BLOCK_HARDIRQ);
	pending|= THREAD_GETMEM(MARCEL_SELF, softirq_pending_in_hardirq);

	return pending;
}

asmlinkage void ma_do_softirq(void)
{
	int max_restart = MAX_SOFTIRQ_RESTART;
	unsigned long pending;
	//unsigned long flags;

	if (ma_in_interrupt())
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
		if (pending) {
			pm2debug("Arghhh loosing softirq! Please, correct me\n");
			/* Il faudrait au moins remettre les pending
			 * en appelant ma_raise_softirq...(nr)
			 */
			pending=0;
		}
		/*
		  Pas encore de softirqd, mais on ne doit pas boucler
		  indéfiniment. On continue donc. Ça sera traité un
		  peu plus tard...

		if (pending)
			wakeup_softirqd();
		*/
		//__ma_local_bh_enable();
	}

	//local_irq_restore(flags);
	ma_local_bh_enable();
}

MARCEL_INT(ma_do_softirq);

void ma_local_bh_enable(void)
{
	
	__ma_local_bh_enable();
	//MA_WARN_ON(ma_irqs_disabled());
	if (tbx_unlikely(!ma_in_interrupt() &&
		     ma_local_softirq_pending()))
		ma_invoke_softirq();
	ma_preempt_check_resched();
}
MARCEL_INT(ma_local_bh_enable);

inline void __ma_raise_softirq_bhoff(unsigned int nr)
{
	ma_set_bit(nr, &ma_local_softirq_pending());
}
/*
 * This function must run with irqs disabled!
 */
inline fastcall void ma_raise_softirq_bhoff(unsigned int nr)
{
	//__ma_raise_softirq_irqoff(nr);
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
	//if (!ma_in_interrupt())
	//	wakeup_softirqd();
}

MARCEL_INT(ma_raise_softirq_irqoff);

void fastcall ma_raise_softirq(unsigned int nr)
{
	//unsigned long flags;

	//local_irq_save(flags);
	ma_local_bh_disable();
	ma_raise_softirq_bhoff(nr);
	//local_irq_restore(flags);
	ma_local_bh_enable();
}

MARCEL_INT(ma_raise_softirq);

void fastcall ma_raise_softirq_from_hardirq(unsigned int nr)
{
	MA_BUG_ON(!ma_in_irq());

	if (tbx_unlikely(ma_test_thread_flag(TIF_BLOCK_HARDIRQ))) {
		ma_set_bit(nr, &THREAD_GETMEM(MARCEL_SELF, softirq_pending_in_hardirq));
	} else {
		ma_raise_softirq_bhoff(nr);
	}
}

MARCEL_INT(ma_raise_softirq_from_hardirq);

void ma_open_softirq(int nr, void (*action)(struct ma_softirq_action*), void *data)
{
	softirq_vec[nr].data = data;
	softirq_vec[nr].action = action;
}

MARCEL_INT(ma_open_softirq);

/* Tasklets */
struct tasklet_head
{
	struct ma_tasklet_struct *list;
};

/* Some compilers disobey section attribute on statics when not
   initialized -- RR */
static MA_DEFINE_PER_LWP(struct tasklet_head, tasklet_vec) = { NULL };
static MA_DEFINE_PER_LWP(struct tasklet_head, tasklet_hi_vec) = { NULL };

void fastcall __ma_tasklet_schedule(struct ma_tasklet_struct *t)
{
	//unsigned long flags;

	//local_irq_save(flags);
	ma_local_bh_disable();
	t->next = __ma_get_lwp_var(tasklet_vec).list;
	__ma_get_lwp_var(tasklet_vec).list = t;
	ma_raise_softirq_bhoff(MA_TASKLET_SOFTIRQ);
	//local_irq_restore(flags);
	ma_local_bh_enable();
}

MARCEL_INT(__ma_tasklet_schedule);

void fastcall __ma_tasklet_hi_schedule(struct ma_tasklet_struct *t)
{
	//unsigned long flags;

	//local_irq_save(flags);
	ma_local_bh_disable();
	t->next = __ma_get_lwp_var(tasklet_hi_vec).list;
	__ma_get_lwp_var(tasklet_hi_vec).list = t;
	ma_raise_softirq_bhoff(MA_HI_SOFTIRQ);
	//local_irq_restore(flags);
	ma_local_bh_enable();
}

MARCEL_INT(__ma_tasklet_hi_schedule);

static void tasklet_action(struct ma_softirq_action *a)
{
	struct ma_tasklet_struct *list;

	//local_irq_disable();
	ma_local_bh_disable();
	list = __ma_get_lwp_var(tasklet_vec).list;
	__ma_get_lwp_var(tasklet_vec).list = NULL;
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
				ma_tasklet_unlock(t);
				continue;
			}
			ma_tasklet_unlock(t);
		}

		//local_irq_disable();
		ma_local_bh_disable();
		t->next = __ma_get_lwp_var(tasklet_vec).list;
		__ma_get_lwp_var(tasklet_vec).list = t;
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
	list = __ma_get_lwp_var(tasklet_hi_vec).list;
	__ma_get_lwp_var(tasklet_hi_vec).list = NULL;
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
				ma_tasklet_unlock(t);
				continue;
			}
			ma_tasklet_unlock(t);
		}

		//local_irq_disable();
		ma_local_bh_disable();
		t->next = __ma_get_lwp_var(tasklet_hi_vec).list;
		__ma_get_lwp_var(tasklet_hi_vec).list = t;
		__ma_raise_softirq_bhoff(MA_HI_SOFTIRQ);
		//local_irq_enable();
		ma_local_bh_enable();
	}
}


void ma_tasklet_init(struct ma_tasklet_struct *t,
		     void (*func)(unsigned long), unsigned long data)
{
	t->next = NULL;
	t->state = 0;
	ma_atomic_set(&t->count, 0);
	t->func = func;
	t->data = data;
}

MARCEL_INT(ma_tasklet_init);

void ma_tasklet_kill(struct ma_tasklet_struct *t)
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

MARCEL_INT(ma_tasklet_kill);

void __init softirq_init(void)
{
	ma_open_softirq(MA_TASKLET_SOFTIRQ, tasklet_action, NULL);
	ma_open_softirq(MA_HI_SOFTIRQ, tasklet_hi_action, NULL);
}

__ma_initfunc(softirq_init, MA_INIT_SOFTIRQ, "Initialisation des SoftIrq");

#if 0
static int ksoftirqd(void * __bind_cpu)
{
	set_user_nice(current, 19);
	current->flags |= PF_IOTHREAD;

	set_current_state(TASK_INTERRUPTIBLE);
 
	while (!kthread_should_stop()) {
		if (!local_softirq_pending())
			schedule();

		__set_current_state(TASK_RUNNING);

		while (local_softirq_pending()) {
			/* Preempt disable stops cpu going offline.
			   If already offline, we'll be on wrong CPU:
			   don't process */
			preempt_disable();
			if (cpu_is_offline((long)__bind_cpu))
				goto wait_to_die;
			do_softirq();
			preempt_enable();
			cond_resched();
		}

		__set_current_state(TASK_INTERRUPTIBLE);
	}
	__set_current_state(TASK_RUNNING);
	return 0;

wait_to_die:
	preempt_enable();
	/* Wait for kthread_stop */
	__set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		schedule();
		__set_current_state(TASK_INTERRUPTIBLE);
	}
	__set_current_state(TASK_RUNNING);
	return 0;
}

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

static int __devinit cpu_callback(struct notifier_block *nfb,
				  unsigned long action,
				  void *hcpu)
{
	int hotcpu = (unsigned long)hcpu;
	struct task_struct *p;
 
	switch (action) {
	case CPU_UP_PREPARE:
		BUG_ON(per_cpu(tasklet_vec, hotcpu).list);
		BUG_ON(per_cpu(tasklet_hi_vec, hotcpu).list);
		p = kthread_create(ksoftirqd, hcpu, "ksoftirqd/%d", hotcpu);
		if (IS_ERR(p)) {
			printk("ksoftirqd for %i failed\n", hotcpu);
			return NOTIFY_BAD;
		kthread_bind(p, hotcpu);
  		per_cpu(ksoftirqd, hotcpu) = p;
 		break;
	case CPU_ONLINE:
		wake_up_process(per_cpu(ksoftirqd, hotcpu));
		break;
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
		}
 	}
	return NOTIFY_OK;
}

static struct notifier_block __devinitdata cpu_nfb = {
	.notifier_call = cpu_callback
};

__init int spawn_ksoftirqd(void)
{
	void *cpu = (void *)(long)smp_processor_id();
	cpu_callback(&cpu_nfb, CPU_UP_PREPARE, cpu);
	cpu_callback(&cpu_nfb, CPU_ONLINE, cpu);
	register_cpu_notifier(&cpu_nfb);
	return 0;
}
#endif /* 0 */
