
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
	base->t_base.running_timer = timer;
#endif
}

#ifdef PM2_DEBUG
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
	ma_init_timer(timer);
}
#endif

static inline void check_timer(struct ma_timer_list *timer)
{
#ifdef PM2_DEBUG
	if (timer->magic != MA_TIMER_MAGIC)
		check_timer_failed(timer);
#endif
}


static void internal_add_timer(ma_tvec_base_t *base, struct ma_timer_list *timer)
{
	unsigned long expires = timer->expires;
	unsigned long idx = expires - base->timer_jiffies;
	struct tbx_fast_list_head *vec;

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
	tbx_fast_list_add_tail(&timer->entry, vec);
}

/*
 * Used by MA_TIMER_INITIALIZER, we can't use per_lwp(tvec_bases)
 * at compile time, and we need timer->base to lock the timer.
 *
 * Yes, this seems like a contention point, but that's only for timers using
 * MA_TIMER_INITIALIZER, and even for them it will be used only once, after
 * * * * that a per-LWP base will be used
 */
typedef struct ma_timer_base_s ma_timer_base_t;
ma_timer_base_t __ma_init_timer_base;

static inline void detach_timer(struct ma_timer_list *timer,
				int clear_pending)
{
	struct tbx_fast_list_head *entry = &timer->entry;

	__tbx_fast_list_del(entry->prev, entry->next);
	if (clear_pending)
		entry->next = NULL;
#ifdef PM2_DEBUG
	entry->prev = (void*) 0x123;
#endif
}

/*
 * We are using hashed locking: holding per_lwp(tvec_bases).t_base.lock
 * means that all timers which are tied to this base via timer->base are
 * locked, and the base itself is locked too.
 *
 * So __run_timers can safely modify all timers which could
 * be found on ->tvX lists.
 *
 * When the timer's base is locked, and the timer removed from list, it is
 * possible to set timer->base = NULL and drop the lock: the timer remains
 * locked.
 */
static ma_timer_base_t *lock_timer_base(struct ma_timer_list *timer)
{
       ma_timer_base_t *base;

       for (;;) {
               base = timer->base;
               if (tbx_likely(base != NULL)) {
                       ma_spin_lock_softirq(&base->lock);
                       if (tbx_likely(base == timer->base))
                               return base;
                       /* The timer has migrated to another LWP */
                       ma_spin_unlock_softirq(&base->lock);
               }
               ma_cpu_relax();
       }
}

TBX_EXTERN int __ma_mod_timer(struct ma_timer_list *timer, unsigned long expires)
{
	ma_timer_base_t *base;
	ma_tvec_base_t *new_base;
	int ret = 0;

	MA_BUG_ON(!timer->function);

	mdebug("modifying (or adding) timer [%p] from %li to %li\n",
	       timer, timer->expires, expires);

	check_timer(timer);

	base = lock_timer_base(timer);

	if (ma_timer_pending(timer)) {
		detach_timer(timer, 0);
		ret = 1;
	}

	new_base = &__ma_get_lwp_var(tvec_bases);

	if (base != &new_base->t_base) {
		/*
		 * We are trying to schedule the timer on the local CPU.
		 * However we can't change timer's base while it is running,
		 * otherwise del_timer_sync() can't detect that the timer's
		 * handler yet has not finished. This also guarantees that
		 * the timer is serialized wrt itself.
		 */
		if (tbx_unlikely(base->running_timer == timer)) {
		        /* The timer remains on a former base */
		        new_base = tbx_container_of(base, ma_tvec_base_t, t_base);
		} else {
		        /* See the comment in lock_timer_base() */
		        timer->base = NULL;
		        ma_spin_unlock(&base->lock);
			base = &new_base->t_base;
		        ma_spin_lock(&base->lock);
		        timer->base = base;
		}
	}

	timer->expires = expires;
	internal_add_timer(new_base, timer);

	ma_spin_unlock_softirq(&base->lock);

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
	ma_timer_base_t *base;
	int ret = 0;

	check_timer(timer);

	mdebug("Deleting timer [%p]\n", timer);

	if (ma_timer_pending(timer)) {
		base = lock_timer_base(timer);
		if (ma_timer_pending(timer)) {
			detach_timer(timer, 1);
			ret = 1;
		}
		ma_spin_unlock_softirq(&base->lock);
	}

	return ret;
}

#ifdef MA__LWPS
/***
 * ma_del_timer_sync - deactivate a timer and wait for the handler to finish.
 * @timer: the timer to be deactivated
 *
 * This function only differs from del_timer() on SMP: besides deactivating
 * the timer it also makes sure the handler has finished executing on other
 * LWPs.
 *
 * Synchronization rules: callers must prevent restarting of the timer,
 * otherwise this function is meaningless. It must not be called from
 * interrupt contexts. The caller must not hold locks which would prevent
 * completion of the timer's handler. Upon exit the timer is not queued and
 * the handler is not running on any CPU.
 *
 * The function returns whether it has deactivated a pending timer or not.
 */
TBX_EXTERN int ma_del_timer_sync(struct ma_timer_list *timer)
{
	ma_timer_base_t *base;
	int ret = -1;

	check_timer(timer);
	mdebug("Deleting timer sync [%p]\n", timer);

	do {
		base = lock_timer_base(timer);

		if (base->running_timer == timer)
			goto unlock;

		ret = 0;
		if (ma_timer_pending(timer)) {
			detach_timer(timer, 1);
			ret = 1;
		}
unlock:
		ma_spin_unlock_softirq(&base->lock);
	} while (ret < 0);

	return ret;
}
#endif

static int cascade(ma_tvec_base_t *base, ma_tvec_t *tv, int index)
{
	/* cascade all the timers from tv up one level */
	struct tbx_fast_list_head *head, *curr;

	head = tv->vec + index;
	curr = head->next;
	/*
	 * We are removing _all_ timers from the list, so we don't  have to
	 * detach them individually, just clear the list afterwards.
	 */
	while (curr != head) {
		struct ma_timer_list *tmp;

		tmp = tbx_fast_list_entry(curr, struct ma_timer_list, entry);
		MA_BUG_ON(tmp->base != &base->t_base);
		curr = curr->next;
		internal_add_timer(base, tmp);
	}
	TBX_INIT_FAST_LIST_HEAD(head);

	return index;
}

/***
 * __run_timers - run all expired timers (if any) on this LWP.
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

	ma_spin_lock_softirq(&base->t_base.lock);
	while (ma_time_after_eq(ma_jiffies, base->timer_jiffies)) {
		struct tbx_fast_list_head work_list = TBX_FAST_LIST_HEAD_INIT(work_list);
		struct tbx_fast_list_head *head = &work_list;
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
		tbx_fast_list_splice_init(base->tv1.vec + index, &work_list);
		while (!tbx_fast_list_empty(head)) {
			void (*fn)(unsigned long);
			unsigned long data;

			timer = tbx_fast_list_entry(head->next,struct ma_timer_list,entry);
 			fn = timer->function;
 			data = timer->data;

			set_running_timer(base, timer);
			detach_timer(timer, 1);
			ma_spin_unlock_softirq(&base->t_base.lock);
			fn(data);
			ma_spin_lock_softirq(&base->t_base.lock);
		}
	}
	set_running_timer(base, NULL);
	ma_spin_unlock_softirq(&base->t_base.lock);
}

static inline void do_process_times(struct marcel_task *p,
	unsigned long user TBX_UNUSED, unsigned long system TBX_UNUSED)
{
	ma_atomic_inc(&p->top_utime);
}

static void update_one_process(struct marcel_task *p, unsigned long user,
			unsigned long system, ma_lwp_t lwp TBX_UNUSED)
{
	do_process_times(p, user, system);
}	

/*
 * Called by the local, per-LWP timer interrupt on SMP.
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
	ma_lwp_t lwp = MA_LWP_SELF;
	int system = user_tick ^ 1;

	update_one_process(p, user_tick, system, lwp);
	ma_run_local_timers();
	ma_scheduler_tick(user_tick, system);
}

/*
 * This function runs timers and the timer-tq in bottom half context.
 */
static void run_timer_softirq(struct ma_softirq_action *h TBX_UNUSED)
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
 * the LWP away without a bound on the timeout. In this case the return
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
			SELF_GETMEM(state) = MA_TASK_RUNNING;
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
	ma_spin_lock_init(&base->t_base.lock);
	for (j = 0; j < TVN_SIZE; j++) {
		TBX_INIT_FAST_LIST_HEAD(base->tv5.vec + j);
		TBX_INIT_FAST_LIST_HEAD(base->tv4.vec + j);
		TBX_INIT_FAST_LIST_HEAD(base->tv3.vec + j);
		TBX_INIT_FAST_LIST_HEAD(base->tv2.vec + j);
	}
	for (j = 0; j < TVR_SIZE; j++)
		TBX_INIT_FAST_LIST_HEAD(base->tv1.vec + j);

	base->timer_jiffies = ma_jiffies;

	if (ma_is_first_lwp(lwp)) {
		ma_open_softirq(MA_TIMER_SOFTIRQ, run_timer_softirq, NULL);
	}
}

MA_DEFINE_LWP_NOTIFIER_START(timers, "Timers Linux 2.6",
			     init_timers_lwp, "Init timer",
			     (void), "[none]");
MA_LWP_NOTIFIER_CALL_UP_PREPARE(timers, MA_INIT_LINUX_TIMER);

