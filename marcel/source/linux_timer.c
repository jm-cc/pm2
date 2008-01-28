
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
 *  linux/kernel/timer.c
 *
 *  Kernel internal timers, kernel timekeeping, basic process system calls
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  1997-01-28  Modified by Finn Arne Gangstad to make timers scale better.
 *
 *  1997-09-10  Updated NTP code according to technical memorandum Jan '96
 *              "A Kernel Model for Precision Timekeeping" by Dave Mills
 *  1998-12-24  Fixed a xtime SMP race (we need the xtime_lock rw spinlock to
 *              serialize accesses to xtime/lost_ticks).
 *                              Copyright (C) 1998  Andrea Arcangeli
 *  1999-03-10  Improved NTP compatibility by Ulrich Windl
 *  2002-05-31	Move sys_sysinfo here and make its locking sane, Robert Love
 *  2000-10-05  Implemented scalable SMP per-CPU timer handling.
 *                              Copyright (C) 2000, 2001, 2002  Ingo Molnar
 *              Designed by David S. Miller, Alexey Kuznetsov and Ingo Molnar
 */

#define MA_FILE_DEBUG linux_timer
#include "marcel.h"

static inline void set_running_timer(ma_tvec_base_t *base,
				     struct ma_timer_list *timer)
{
#ifdef MA__LWPS
	base->running_timer = timer;
#endif
}

static void check_timer_failed(struct ma_timer_list *timer)
{
	static int whine_count;
	if (whine_count < 16) {
		whine_count++;
		pm2debug("Uninitialised timer!\n");
		pm2debug("This is just a warning.  Your computer is OK\n");
		pm2debug("function=0x%p, data=0x%lx\n",
			 timer->function, timer->data);
	}
	/*
	 * Now fix it up
	 */
	ma_spin_lock_init(&timer->lock);
	timer->magic = MA_TIMER_MAGIC;
}

static inline void check_timer(struct ma_timer_list *timer)
{
	if (timer->magic != MA_TIMER_MAGIC)
		check_timer_failed(timer);
}


static void internal_add_timer(ma_tvec_base_t *base, struct ma_timer_list *timer)
{
	unsigned long expires = timer->expires;
	unsigned long idx = expires - base->timer_jiffies;
	struct list_head *vec;

	mdebug("adding timer [%p] at %li\n", timer, expires); 

	if (idx < TVR_SIZE) {
		int i = expires & TVR_MASK;
		vec = base->tv1.vec + i;
	} else if (idx < 1 << (TVR_BITS + TVN_BITS)) {
		int i = (expires >> TVR_BITS) & TVN_MASK;
		vec = base->tv2.vec + i;
	} else if (idx < 1 << (TVR_BITS + 2 * TVN_BITS)) {
		int i = (expires >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
		vec = base->tv3.vec + i;
	} else if (idx < 1 << (TVR_BITS + 3 * TVN_BITS)) {
		int i = (expires >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
		vec = base->tv4.vec + i;
	} else if ((signed long) idx < 0) {
		/*
		 * Can happen if you add a timer with expires == jiffies,
		 * or you set a timer to go off in the past
		 */
		vec = base->tv1.vec + (base->timer_jiffies & TVR_MASK);
	} else {
		int i;
		/* If the timeout is larger than 0xffffffff on 64-bit
		 * architectures then we use the maximum timeout:
		 */
		if (idx > 0xffffffffUL) {
			idx = 0xffffffffUL;
			expires = idx + base->timer_jiffies;
		}
		i = (expires >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
		vec = base->tv5.vec + i;
	}
	/*
	 * Timers are FIFO:
	 */
	list_add_tail(&timer->entry, vec);
}

TBX_EXTERN int __ma_mod_timer(struct ma_timer_list *timer, unsigned long expires)
{
	ma_tvec_base_t *old_base, *new_base;
	int ret = 0;

	MA_BUG_ON(!timer->function);

	mdebug("modifying (or adding) timer [%p] from %li to %li\n",
	       timer, timer->expires, expires);

	check_timer(timer);

	ma_spin_lock_softirq(&timer->lock);
	new_base = &__ma_get_lwp_var(tvec_bases);
repeat:
	old_base = timer->base;

	/*
	 * Prevent deadlocks via ordering by old_base < new_base.
	 */
	if (old_base && (new_base != old_base)) {
		if (old_base < new_base) {
			ma_spin_lock(&new_base->lock);
			ma_spin_lock(&old_base->lock);
		} else {
			ma_spin_lock(&old_base->lock);
			ma_spin_lock(&new_base->lock);
		}
		/*
		 * The timer base might have been cancelled while we were
		 * trying to take the lock(s):
		 */
		if (timer->base != old_base) {
			ma_spin_unlock(&new_base->lock);
			ma_spin_unlock(&old_base->lock);
			goto repeat;
		}
	} else {
		ma_spin_lock(&new_base->lock);
		if (timer->base != old_base) {
			ma_spin_unlock(&new_base->lock);
			goto repeat;
		}
	}

	/*
	 * Delete the previous timeout (if there was any), and install
	 * the new one:
	 */
	if (old_base) {
		list_del(&timer->entry);
		ret = 1;
	}
	timer->expires = expires;
	internal_add_timer(new_base, timer);
	timer->base = new_base;

	if (old_base && (new_base != old_base))
		ma_spin_unlock(&old_base->lock);
	ma_spin_unlock(&new_base->lock);
	ma_spin_unlock_softirq(&timer->lock);

	return ret;
}

/***
 * mod_timer - modify a timer's timeout
 * @timer: the timer to be modified
 *
 * mod_timer is a more efficient way to update the expire field of an
 * active timer (if the timer is inactive it will be activated)
 *
 * mod_timer(timer, expires) is equivalent to:
 *
 *     del_timer(timer); timer->expires = expires; add_timer(timer);
 *
 * Note that if there are multiple unserialized concurrent users of the
 * same timer, then mod_timer() is the only safe way to modify the timeout,
 * since add_timer() cannot modify an already running timer.
 *
 * The function returns whether it has modified a pending timer or not.
 * (ie. mod_timer() of an inactive timer returns 0, mod_timer() of an
 * active timer returns 1.)
 */
TBX_EXTERN int ma_mod_timer(struct ma_timer_list *timer, unsigned long expires)
{
	MA_BUG_ON(!timer->function);

	check_timer(timer);
	mdebug("Real modifying timer at %li\n", expires);

	/*
	 * This is a common optimization triggered by the
	 * networking code - if the timer is re-modified
	 * to be the same thing then just return:
	 */
	if (timer->expires == expires && ma_timer_pending(timer))
		return 1;

	return __ma_mod_timer(timer, expires);
}

/***
 * del_timer - deactive a timer.
 * @timer: the timer to be deactivated
 *
 * del_timer() deactivates a timer - this works on both active and inactive
 * timers.
 *
 * The function returns whether it has deactivated a pending timer or not.
 * (ie. del_timer() of an inactive timer returns 0, del_timer() of an
 * active timer returns 1.)
 */
TBX_EXTERN int ma_del_timer(struct ma_timer_list *timer)
{
	ma_tvec_base_t *base;

	check_timer(timer);

	mdebug("Deleting timer [%p]\n", timer);

repeat:
 	base = timer->base;
	if (!base)
		return 0;
	ma_spin_lock_softirq(&base->lock);
	if (base != timer->base) {
		ma_spin_unlock_softirq(&base->lock);
		goto repeat;
	}
	list_del(&timer->entry);
	timer->base = NULL;
	ma_spin_unlock_softirq(&base->lock);

	return 1;
}

#ifdef MA__LWPS
/***
 * ma_del_timer_sync - deactivate a timer and wait for the handler to finish.
 * @timer: the timer to be deactivated
 *
 * This function only differs from del_timer() on SMP: besides deactivating
 * the timer it also makes sure the handler has finished executing on other
 * CPUs.
 *
 * Synchronization rules: callers must prevent restarting of the timer,
 * otherwise this function is meaningless. It must not be called from
 * interrupt contexts. Upon exit the timer is not queued and the handler
 * is not running on any CPU.
 *
 * The function returns whether it has deactivated a pending timer or not.
 */
TBX_EXTERN int ma_del_timer_sync(struct ma_timer_list *timer)
{
	ma_tvec_base_t *base;
	int ret = 0;
	ma_lwp_t lwp;

	check_timer(timer);
	mdebug("Deleting timer sync [%p]\n", timer);

del_again:
	ret += ma_del_timer(timer);

	for_each_lwp_begin(lwp)
		base = &ma_per_lwp(tvec_bases, lwp);
		if (base->running_timer == timer) {
			while (base->running_timer == timer) {
				cpu_relax();
				ma_preempt_check_resched(0);
			}
			break;
		}
	for_each_lwp_end()
	ma_smp_rmb();
	if (ma_timer_pending(timer))
		goto del_again;

	return ret;
}
#endif

static int cascade(ma_tvec_base_t *base, ma_tvec_t *tv, int index)
{
	/* cascade all the timers from tv up one level */
	struct list_head *head, *curr;

	head = tv->vec + index;
	curr = head->next;
	/*
	 * We are removing _all_ timers from the list, so we don't  have to
	 * detach them individually, just clear the list afterwards.
	 */
	while (curr != head) {
		struct ma_timer_list *tmp;

		tmp = list_entry(curr, struct ma_timer_list, entry);
		MA_BUG_ON(tmp->base != base);
		curr = curr->next;
		internal_add_timer(base, tmp);
	}
	INIT_LIST_HEAD(head);

	return index;
}

/***
 * __run_timers - run all expired timers (if any) on this CPU.
 * @base: the timer vector to be processed.
 *
 * This function cascades all vectors and executes all expired timer
 * vectors.
 */
#define INDEX(N) (base->timer_jiffies >> (TVR_BITS + N * TVN_BITS)) & TVN_MASK

static inline void __run_timers(ma_tvec_base_t *base)
{
	struct ma_timer_list *timer;

	mdebugl(7, "Running Timers (next at %li, current : %li)\n",
		base->timer_jiffies, ma_jiffies);

	ma_spin_lock_softirq(&base->lock);
	while (ma_time_after_eq(ma_jiffies, base->timer_jiffies)) {
		struct list_head work_list = LIST_HEAD_INIT(work_list);
		struct list_head *head = &work_list;
 		int index = base->timer_jiffies & TVR_MASK;
 
		/*
		 * Cascade timers:
		 */
		if (!index &&
			(!cascade(base, &base->tv2, INDEX(0))) &&
				(!cascade(base, &base->tv3, INDEX(1))) &&
					!cascade(base, &base->tv4, INDEX(2)))
			cascade(base, &base->tv5, INDEX(3));
		++base->timer_jiffies; 
		list_splice_init(base->tv1.vec + index, &work_list);
repeat:
		if (!list_empty(head)) {
			void (*fn)(unsigned long);
			unsigned long data;

			timer = list_entry(head->next,struct ma_timer_list,entry);
 			fn = timer->function;
 			data = timer->data;

			list_del(&timer->entry);
			set_running_timer(base, timer);
			ma_smp_wmb();
			timer->base = NULL;
			ma_spin_unlock_softirq(&base->lock);
			fn(data);
			ma_spin_lock_softirq(&base->lock);
			goto repeat;
		}
	}
	set_running_timer(base, NULL);
	ma_spin_unlock_softirq(&base->lock);
}

static inline void do_process_times(struct marcel_task *p,
	unsigned long user, unsigned long system)
{
	ma_atomic_inc(&p->top_utime);
}

static void update_one_process(struct marcel_task *p, unsigned long user,
			unsigned long system, ma_lwp_t lwp)
{
	do_process_times(p, user, system);
}	

/*
 * Called by the local, per-CPU timer interrupt on SMP.
 */
static void ma_run_local_timers(void)
{
	ma_raise_softirq(MA_TIMER_SOFTIRQ);
}

/*
 * Called from the timer interrupt handler to charge one tick to the current 
 * process.  user_tick is 1 if the tick is user time, 0 for system.
 */
void ma_update_process_times(int user_tick)
{
	struct marcel_task *p = MARCEL_SELF;
	ma_lwp_t lwp = LWP_SELF;
	int system = user_tick ^ 1;

	update_one_process(p, user_tick, system, lwp);
	ma_run_local_timers();
	ma_scheduler_tick(user_tick, system);
}

/*
 * This function runs timers and the timer-tq in bottom half context.
 */
static void run_timer_softirq(struct ma_softirq_action *h)
{
	ma_tvec_base_t *base = &__ma_get_lwp_var(tvec_bases);

	mdebugl(8, "Running Softirq Timers (next at %li, current : %li)\n",
		base->timer_jiffies, ma_jiffies);

	if (ma_time_after_eq(ma_jiffies, base->timer_jiffies))
		__run_timers(base);
}

void ma_process_timeout(unsigned long __data)
{
	ma_wake_up_thread((marcel_task_t *)__data);
}

/**
 * ma_schedule_timeout - sleep until timeout
 * @timeout: timeout value in jiffies
 *
 * Make the current task sleep until @timeout jiffies have
 * elapsed. The routine will return immediately unless
 * the current task state has been set (see set_current_state()).
 *
 * You can set the task state as follows -
 *
 * %TASK_UNINTERRUPTIBLE - at least @timeout jiffies are guaranteed to
 * pass before the routine returns. The routine will return 0
 *
 * %TASK_INTERRUPTIBLE - the routine may return early if a signal is
 * delivered to the current task. In this case the remaining time
 * in jiffies will be returned, or 0 if the timer expired in time
 *
 * The current task state is guaranteed to be TASK_RUNNING when this
 * routine returns.
 *
 * Specifying a @timeout value of %MAX_SCHEDULE_TIMEOUT will schedule
 * the CPU away without a bound on the timeout. In this case the return
 * value will be %MAX_SCHEDULE_TIMEOUT.
 *
 * In all cases the return value is guaranteed to be non-negative.
 */
fastcall TBX_EXTERN signed long ma_schedule_timeout(signed long timeout)
{
	unsigned long expire;

	switch (timeout)
	{
	case MA_MAX_SCHEDULE_TIMEOUT:
		/*
		 * These two special cases are useful to be comfortable
		 * in the caller. Nothing more. We could take
		 * MAX_SCHEDULE_TIMEOUT from one of the negative value
		 * but I' d like to return a valid offset (>=0) to allow
		 * the caller to do everything it want with the retval.
		 */
		ma_schedule();
		goto out;
	default:
		/*
		 * Another bit of PARANOID. Note that the retval will be
		 * 0 since no piece of kernel is supposed to do a check
		 * for a negative retval of schedule_timeout() (since it
		 * should never happens anyway). You just have the printk()
		 * that will tell you if something is gone wrong and where.
		 */
		if (timeout < 0)
		{
			pm2debug("schedule_timeout: wrong timeout "
				 "value %lx from %p\n", timeout,
				 __builtin_return_address(0));
			SELF_GETMEM(sched).state = MA_TASK_RUNNING;
			goto out;
		}
	}

	expire = timeout + ma_jiffies;

	ma_mod_timer(&SELF_GETMEM(schedule_timeout_timer), expire);
	ma_schedule();
	ma_del_timer_sync(&SELF_GETMEM(schedule_timeout_timer));

	timeout = expire - ma_jiffies;

 out:
	return timeout < 0 ? 0 : timeout;
}

static void __marcel_init init_timers_lwp(ma_lwp_t lwp)
{
	int j;
	ma_tvec_base_t *base;
       
	base = &ma_per_lwp(tvec_bases, lwp);
	ma_spin_lock_init(&base->lock);
	for (j = 0; j < TVN_SIZE; j++) {
		INIT_LIST_HEAD(base->tv5.vec + j);
		INIT_LIST_HEAD(base->tv4.vec + j);
		INIT_LIST_HEAD(base->tv3.vec + j);
		INIT_LIST_HEAD(base->tv2.vec + j);
	}
	for (j = 0; j < TVR_SIZE; j++)
		INIT_LIST_HEAD(base->tv1.vec + j);

	base->timer_jiffies = ma_jiffies;

	if (IS_FIRST_LWP(lwp)) {
		ma_open_softirq(MA_TIMER_SOFTIRQ, run_timer_softirq, NULL);
	}
}

MA_DEFINE_LWP_NOTIFIER_START(timers, "Timers Linux 2.6",
			     init_timers_lwp, "Init timer",
			     (void), "[none]");
MA_LWP_NOTIFIER_CALL_UP_PREPARE(timers, MA_INIT_LINUX_TIMER);

