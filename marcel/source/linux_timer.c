
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
		//dump_stack();
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
 * ma_add_timer_on - start a timer on a particular CPU
 * @timer: the timer to be added
 * @cpu: the CPU to start it on
 *
 * This is not very scalable on SMP. Double adds are not possible.
 */
void ma_add_timer_on(struct ma_timer_list *timer, ma_lwp_t lwp)
{
	ma_tvec_base_t *base = &ma_per_lwp(tvec_bases, lwp);
  	//unsigned long flags;
  
  	MA_BUG_ON(ma_timer_pending(timer) || !timer->function);

	check_timer(timer);

	ma_spin_lock_softirq(&base->lock);
	internal_add_timer(base, timer);
	timer->base = base;
	ma_spin_unlock_softirq(&base->lock);
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
	//unsigned long flags;
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

/******************************************************************/

#if 0
/*
 * Timekeeping variables
 */
unsigned long tick_usec = TICK_USEC; 		/* USER_HZ period (usec) */
unsigned long tick_nsec = TICK_NSEC;		/* ACTHZ period (nsec) */

/* 
 * The current time 
 * wall_to_monotonic is what we need to add to xtime (or xtime corrected 
 * for sub jiffie times) to get to monotonic time.  Monotonic is pegged at zero
 * at zero at system boot time, so wall_to_monotonic will be negative,
 * however, we will ALWAYS keep the tv_nsec part positive so we can use
 * the usual normalization.
 */
struct timespec xtime __attribute__ ((aligned (16)));
struct timespec wall_to_monotonic __attribute__ ((aligned (16)));

EXPORT_SYMBOL(xtime);

/* Don't completely fail for HZ > 500.  */
int tickadj = 500/HZ ? : 1;		/* microsecs */


/*
 * phase-lock loop variables
 */
/* TIME_ERROR prevents overwriting the CMOS clock */
int time_state = TIME_OK;		/* clock synchronization status	*/
int time_status = STA_UNSYNC;		/* clock status bits		*/
long time_offset;			/* time adjustment (us)		*/
long time_constant = 2;			/* pll time constant		*/
long time_tolerance = MAXFREQ;		/* frequency tolerance (ppm)	*/
long time_precision = 1;		/* clock precision (us)		*/
long time_maxerror = NTP_PHASE_LIMIT;	/* maximum error (us)		*/
long time_esterror = NTP_PHASE_LIMIT;	/* estimated error (us)		*/
long time_phase;			/* phase offset (scaled us)	*/
long time_freq = (((NSEC_PER_SEC + HZ/2) % HZ - HZ/2) << SHIFT_USEC) / NSEC_PER_USEC;
					/* frequency offset (scaled ppm)*/
long time_adj;				/* tick adjust (scaled 1 / HZ)	*/
long time_reftime;			/* time at last adjustment (s)	*/
long time_adjust;
long time_next_adjust;

/*
 * this routine handles the overflow of the microsecond field
 *
 * The tricky bits of code to handle the accurate clock support
 * were provided by Dave Mills (Mills@UDEL.EDU) of NTP fame.
 * They were originally developed for SUN and DEC kernels.
 * All the kudos should go to Dave for this stuff.
 *
 */
static void second_overflow(void)
{
    long ltemp;

    /* Bump the maxerror field */
    time_maxerror += time_tolerance >> SHIFT_USEC;
    if ( time_maxerror > NTP_PHASE_LIMIT ) {
	time_maxerror = NTP_PHASE_LIMIT;
	time_status |= STA_UNSYNC;
    }

    /*
     * Leap second processing. If in leap-insert state at
     * the end of the day, the system clock is set back one
     * second; if in leap-delete state, the system clock is
     * set ahead one second. The microtime() routine or
     * external clock driver will insure that reported time
     * is always monotonic. The ugly divides should be
     * replaced.
     */
    switch (time_state) {

    case TIME_OK:
	if (time_status & STA_INS)
	    time_state = TIME_INS;
	else if (time_status & STA_DEL)
	    time_state = TIME_DEL;
	break;

    case TIME_INS:
	if (xtime.tv_sec % 86400 == 0) {
	    xtime.tv_sec--;
	    wall_to_monotonic.tv_sec++;
	    time_interpolator_update(-NSEC_PER_SEC);
	    time_state = TIME_OOP;
	    clock_was_set();
	    printk(KERN_NOTICE "Clock: inserting leap second 23:59:60 UTC\n");
	}
	break;

    case TIME_DEL:
	if ((xtime.tv_sec + 1) % 86400 == 0) {
	    xtime.tv_sec++;
	    wall_to_monotonic.tv_sec--;
	    time_interpolator_update(NSEC_PER_SEC);
	    time_state = TIME_WAIT;
	    clock_was_set();
	    printk(KERN_NOTICE "Clock: deleting leap second 23:59:59 UTC\n");
	}
	break;

    case TIME_OOP:
	time_state = TIME_WAIT;
	break;

    case TIME_WAIT:
	if (!(time_status & (STA_INS | STA_DEL)))
	    time_state = TIME_OK;
    }

    /*
     * Compute the phase adjustment for the next second. In
     * PLL mode, the offset is reduced by a fixed factor
     * times the time constant. In FLL mode the offset is
     * used directly. In either mode, the maximum phase
     * adjustment for each second is clamped so as to spread
     * the adjustment over not more than the number of
     * seconds between updates.
     */
    if (time_offset < 0) {
	ltemp = -time_offset;
	if (!(time_status & STA_FLL))
	    ltemp >>= SHIFT_KG + time_constant;
	if (ltemp > (MAXPHASE / MINSEC) << SHIFT_UPDATE)
	    ltemp = (MAXPHASE / MINSEC) << SHIFT_UPDATE;
	time_offset += ltemp;
	time_adj = -ltemp << (SHIFT_SCALE - SHIFT_HZ - SHIFT_UPDATE);
    } else {
	ltemp = time_offset;
	if (!(time_status & STA_FLL))
	    ltemp >>= SHIFT_KG + time_constant;
	if (ltemp > (MAXPHASE / MINSEC) << SHIFT_UPDATE)
	    ltemp = (MAXPHASE / MINSEC) << SHIFT_UPDATE;
	time_offset -= ltemp;
	time_adj = ltemp << (SHIFT_SCALE - SHIFT_HZ - SHIFT_UPDATE);
    }

    /*
     * Compute the frequency estimate and additional phase
     * adjustment due to frequency error for the next
     * second. When the PPS signal is engaged, gnaw on the
     * watchdog counter and update the frequency computed by
     * the pll and the PPS signal.
     */
    pps_valid++;
    if (pps_valid == PPS_VALID) {	/* PPS signal lost */
	pps_jitter = MAXTIME;
	pps_stabil = MAXFREQ;
	time_status &= ~(STA_PPSSIGNAL | STA_PPSJITTER |
			 STA_PPSWANDER | STA_PPSERROR);
    }
    ltemp = time_freq + pps_freq;
    if (ltemp < 0)
	time_adj -= -ltemp >>
	    (SHIFT_USEC + SHIFT_HZ - SHIFT_SCALE);
    else
	time_adj += ltemp >>
	    (SHIFT_USEC + SHIFT_HZ - SHIFT_SCALE);

#if HZ == 100
    /* Compensate for (HZ==100) != (1 << SHIFT_HZ).
     * Add 25% and 3.125% to get 128.125; => only 0.125% error (p. 14)
     */
    if (time_adj < 0)
	time_adj -= (-time_adj >> 2) + (-time_adj >> 5);
    else
	time_adj += (time_adj >> 2) + (time_adj >> 5);
#endif
#if HZ == 1000
    /* Compensate for (HZ==1000) != (1 << SHIFT_HZ).
     * Add 1.5625% and 0.78125% to get 1023.4375; => only 0.05% error (p. 14)
     */
    if (time_adj < 0)
	time_adj -= (-time_adj >> 6) + (-time_adj >> 7);
    else
	time_adj += (time_adj >> 6) + (time_adj >> 7);
#endif
}

/* in the NTP reference this is called "hardclock()" */
static void update_wall_time_one_tick(void)
{
	long time_adjust_step, delta_nsec;

	if ( (time_adjust_step = time_adjust) != 0 ) {
	    /* We are doing an adjtime thing. 
	     *
	     * Prepare time_adjust_step to be within bounds.
	     * Note that a positive time_adjust means we want the clock
	     * to run faster.
	     *
	     * Limit the amount of the step to be in the range
	     * -tickadj .. +tickadj
	     */
	     if (time_adjust > tickadj)
		time_adjust_step = tickadj;
	     else if (time_adjust < -tickadj)
		time_adjust_step = -tickadj;

	    /* Reduce by this step the amount of time left  */
	    time_adjust -= time_adjust_step;
	}
	delta_nsec = tick_nsec + time_adjust_step * 1000;
	/*
	 * Advance the phase, once it gets to one microsecond, then
	 * advance the tick more.
	 */
	time_phase += time_adj;
	if (time_phase <= -FINENSEC) {
		long ltemp = -time_phase >> (SHIFT_SCALE - 10);
		time_phase += ltemp << (SHIFT_SCALE - 10);
		delta_nsec -= ltemp;
	}
	else if (time_phase >= FINENSEC) {
		long ltemp = time_phase >> (SHIFT_SCALE - 10);
		time_phase -= ltemp << (SHIFT_SCALE - 10);
		delta_nsec += ltemp;
	}
	xtime.tv_nsec += delta_nsec;
	time_interpolator_update(delta_nsec);

	/* Changes by adjtime() do not take effect till next tick. */
	if (time_next_adjust != 0) {
		time_adjust = time_next_adjust;
		time_next_adjust = 0;
	}
}

/*
 * Using a loop looks inefficient, but "ticks" is
 * usually just one (we shouldn't be losing ticks,
 * we're doing this this way mainly for interrupt
 * latency reasons, not because we think we'll
 * have lots of lost timer ticks
 */
static void update_wall_time(unsigned long ticks)
{
	do {
		ticks--;
		update_wall_time_one_tick();
	} while (ticks);

	if (xtime.tv_nsec >= 1000000000) {
	    xtime.tv_nsec -= 1000000000;
	    xtime.tv_sec++;
	    second_overflow();
	}
}
#endif /* 0 */

static inline void do_process_times(struct marcel_task *p,
	unsigned long user, unsigned long system)
{
	/*
	p->utime += user;
	p->stime += system;
	*/
	//if (user)
		ma_atomic_inc(&p->top_utime);
	//else
		//ma_atomic_inc(&p->top_stime);
}

#if 0
static inline void do_it_virt(struct task_struct * p, unsigned long ticks)
{
	unsigned long it_virt = p->it_virt_value;

	if (it_virt) {
		it_virt -= ticks;
		if (!it_virt) {
			it_virt = p->it_virt_incr;
			send_sig(SIGVTALRM, p, 1);
		}
		p->it_virt_value = it_virt;
	}
}

static inline void do_it_prof(struct task_struct *p)
{
	unsigned long it_prof = p->it_prof_value;

	if (it_prof) {
		if (--it_prof == 0) {
			it_prof = p->it_prof_incr;
			send_sig(SIGPROF, p, 1);
		}
		p->it_prof_value = it_prof;
	}
}
#endif /* 0 */

static void update_one_process(struct marcel_task *p, unsigned long user,
			unsigned long system, ma_lwp_t lwp)
{
	do_process_times(p, user, system);
	//do_it_virt(p, user);
	//do_it_prof(p);
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

#if 0
/*
 * Nr of active tasks - counted in fixed-point numbers
 */
static unsigned long count_active_tasks(void)
{
	return (nr_running() + nr_uninterruptible()) * FIXED_1;
}

/*
 * Hmm.. Changed this, as the GNU make sources (load.c) seems to
 * imply that avenrun[] is the standard name for this kind of thing.
 * Nothing else seems to be standardized: the fractional size etc
 * all seem to differ on different machines.
 *
 * Requires xtime_lock to access.
 */
unsigned long avenrun[3];

/*
 * calc_load - given tick count, update the avenrun load estimates.
 * This is called while holding a write_lock on xtime_lock.
 */
static inline void calc_load(unsigned long ticks)
{
	unsigned long active_tasks; /* fixed-point */
	static int count = LOAD_FREQ;

	count -= ticks;
	if (count < 0) {
		count += LOAD_FREQ;
		active_tasks = count_active_tasks();
		CALC_LOAD(avenrun[0], EXP_1, active_tasks);
		CALC_LOAD(avenrun[1], EXP_5, active_tasks);
		CALC_LOAD(avenrun[2], EXP_15, active_tasks);
	}
}

/* jiffies at the most recent update of wall time */
unsigned long wall_jiffies = INITIAL_JIFFIES;

/*
 * This read-write spinlock protects us from races in SMP while
 * playing with xtime and avenrun.
 */
#ifndef ARCH_HAVE_XTIME_LOCK
seqlock_t xtime_lock __cacheline_aligned_in_smp = SEQLOCK_UNLOCKED;

EXPORT_SYMBOL(xtime_lock);
#endif
#endif /* 0 */

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

/*
 * Called by the local, per-CPU timer interrupt on SMP.
 */
void ma_run_local_timers(void)
{
	ma_raise_softirq(MA_TIMER_SOFTIRQ);
}

#if 0
/*
 * Called by the timer interrupt. xtime_lock must already be taken
 * by the timer IRQ!
 */
static inline void update_times(void)
{
	unsigned long ticks;

	ticks = jiffies - wall_jiffies;
	if (ticks) {
		wall_jiffies += ticks;
		update_wall_time(ticks);
	}
	calc_load(ticks);
}
#endif /* 0 */
  
#if 0
/*
 * The 64-bit jiffies value is not atomic - you MUST NOT read it
 * without sampling the sequence number in xtime_lock.
 * jiffies is defined in the linker script...
 */

void ma_do_timer(/*struct pt_regs *regs*/)
{
	if (IS_FIRST_LWP(LWP_SELF)) {
		jiffies_64++;
	}
//#ifndef MA__LWPS
	/* SMP process accounting uses the local APIC timer */

	ma_update_process_times(1 /*user_mode(regs)*/);
//#endif
	//update_times();
}

#endif /* 0 */

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

#if 0
static long nanosleep_restart(struct restart_block *restart)
{
	unsigned long expire = restart->arg0, now = jiffies;
	struct timespec *rmtp = (struct timespec *) restart->arg1;
	long ret;

	/* Did it expire while we handled signals? */
	if (!time_after(expire, now))
		return 0;

	current->state = TASK_INTERRUPTIBLE;
	expire = schedule_timeout(expire - now);

	ret = 0;
	if (expire) {
		struct timespec t;
		jiffies_to_timespec(expire, &t);

		ret = -ERESTART_RESTARTBLOCK;
		if (rmtp && copy_to_user(rmtp, &t, sizeof(t)))
			ret = -EFAULT;
		/* The 'restart' block is already filled in */
	}
	return ret;
}

asmlinkage long sys_nanosleep(struct timespec *rqtp, struct timespec *rmtp)
{
	struct timespec t;
	unsigned long expire;
	long ret;

	if (copy_from_user(&t, rqtp, sizeof(t)))
		return -EFAULT;

	if ((t.tv_nsec >= 1000000000L) || (t.tv_nsec < 0) || (t.tv_sec < 0))
		return -EINVAL;

	expire = timespec_to_jiffies(&t) + (t.tv_sec || t.tv_nsec);
	current->state = TASK_INTERRUPTIBLE;
	expire = schedule_timeout(expire);

	ret = 0;
	if (expire) {
		struct restart_block *restart;
		jiffies_to_timespec(expire, &t);
		if (rmtp && copy_to_user(rmtp, &t, sizeof(t)))
			return -EFAULT;

		restart = &current_thread_info()->restart_block;
		restart->fn = nanosleep_restart;
		restart->arg0 = jiffies + expire;
		restart->arg1 = (unsigned long) rmtp;
		ret = -ERESTART_RESTARTBLOCK;
	}
	return ret;
}
#endif /* 0 */

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

#ifdef CONFIG_HOTPLUG_CPU
static int migrate_timer_list(ma_tvec_base_t *new_base, struct list_head *head)
{
	struct timer_list *timer;

	while (!list_empty(head)) {
		timer = list_entry(head->next, struct timer_list, entry);
		/* We're locking backwards from __mod_timer order here,
		   beware deadlock. */
		if (!spin_trylock(&timer->lock))
			return 0;
		list_del(&timer->entry);
		internal_add_timer(new_base, timer);
		timer->base = new_base;
		spin_unlock(&timer->lock);
	}
	return 1;
}

static void __devinit migrate_timers(int cpu)
{
	ma_tvec_base_t *old_base;
	ma_tvec_base_t *new_base;
	int i;

	BUG_ON(cpu_online(cpu));
	old_base = &per_cpu(tvec_bases, cpu);
	new_base = &get_cpu_var(tvec_bases);

	local_irq_disable();
again:
	/* Prevent deadlocks via ordering by old_base < new_base. */
	if (old_base < new_base) {
		spin_lock(&new_base->lock);
		spin_lock(&old_base->lock);
	} else {
		spin_lock(&old_base->lock);
		spin_lock(&new_base->lock);
	}

	if (old_base->running_timer)
		BUG();
	for (i = 0; i < TVR_SIZE; i++)
		if (!migrate_timer_list(new_base, old_base->tv1.vec + i))
			goto unlock_again;
	for (i = 0; i < TVN_SIZE; i++)
		if (!migrate_timer_list(new_base, old_base->tv2.vec + i)
		    || !migrate_timer_list(new_base, old_base->tv3.vec + i)
		    || !migrate_timer_list(new_base, old_base->tv4.vec + i)
		    || !migrate_timer_list(new_base, old_base->tv5.vec + i))
			goto unlock_again;
	spin_unlock(&old_base->lock);
	spin_unlock(&new_base->lock);
	local_irq_enable();
	put_cpu_var(tvec_bases);
	return;

unlock_again:
	/* Avoid deadlock with __mod_timer, by backing off. */
	spin_unlock(&old_base->lock);
	spin_unlock(&new_base->lock);
	cpu_relax();
	goto again;
}
#endif /* CONFIG_HOTPLUG_CPU */

MA_DEFINE_LWP_NOTIFIER_START(timers, "Timers Linux 2.6",
			     init_timers_lwp, "Init timer",
			     (void), "[none]");
MA_LWP_NOTIFIER_CALL_UP_PREPARE(timers, MA_INIT_LINUX_TIMER);
#if 0
static int __marcel_init timer_lwp_notify(struct ma_notifier_block *self, 
				   unsigned long action, void *hlwp)
{
	ma_lwp_t lwp = (ma_lwp_t)hlwp;
	switch(action) {
	case MA_LWP_UP_PREPARE:
		init_timers_lwp(lwp);
		break;
#ifdef CONFIG_HOTPLUG_CPU
	case CPU_DEAD:
		migrate_timers(cpu);
		break;
#endif
	default:
		break;
	}
	return MA_NOTIFY_OK;
}
#endif

#if 0
#ifdef CONFIG_TIME_INTERPOLATION
volatile unsigned long last_nsec_offset;
#ifndef __HAVE_ARCH_CMPXCHG
spinlock_t last_nsec_offset_lock = SPIN_LOCK_UNLOCKED;
#endif

struct time_interpolator *time_interpolator;
static struct time_interpolator *time_interpolator_list;
static spinlock_t time_interpolator_lock = SPIN_LOCK_UNLOCKED;

static inline int
is_better_time_interpolator(struct time_interpolator *new)
{
	if (!time_interpolator)
		return 1;
	return new->frequency > 2*time_interpolator->frequency ||
	    (unsigned long)new->drift < (unsigned long)time_interpolator->drift;
}

void
register_time_interpolator(struct time_interpolator *ti)
{
	spin_lock(&time_interpolator_lock);
	write_seqlock_irq(&xtime_lock);
	if (is_better_time_interpolator(ti))
		time_interpolator = ti;
	write_sequnlock_irq(&xtime_lock);

	ti->next = time_interpolator_list;
	time_interpolator_list = ti;
	spin_unlock(&time_interpolator_lock);
}

void
unregister_time_interpolator(struct time_interpolator *ti)
{
	struct time_interpolator *curr, **prev;

	spin_lock(&time_interpolator_lock);
	prev = &time_interpolator_list;
	for (curr = *prev; curr; curr = curr->next) {
		if (curr == ti) {
			*prev = curr->next;
			break;
		}
		prev = &curr->next;
	}

	write_seqlock_irq(&xtime_lock);
	if (ti == time_interpolator) {
		/* we lost the best time-interpolator: */
		time_interpolator = NULL;
		/* find the next-best interpolator */
		for (curr = time_interpolator_list; curr; curr = curr->next)
			if (is_better_time_interpolator(curr))
				time_interpolator = curr;
	}
	write_sequnlock_irq(&xtime_lock);
	spin_unlock(&time_interpolator_lock);
}
#endif /* CONFIG_TIME_INTERPOLATION */
#endif /* 0 */
