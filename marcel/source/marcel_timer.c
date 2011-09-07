
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

#include "marcel.h"
#include "tbx_compiler.h"
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>


volatile unsigned long ma_jiffies = 0;
ma_atomic_t __ma_preemption_disabled = MA_ATOMIC_INIT(0);
static volatile unsigned long time_slice = MARCEL_MIN_TIME_SLICE; // microseconds


unsigned long marcel_clock(void)
{
	return ma_jiffies * time_slice / 1000;
}

#ifdef MA__USE_TIMER_CREATE
static void ma_slice2timer(struct itimerspec *clk_prop)
{
	clk_prop->it_interval.tv_sec = time_slice / 1000000;
	clk_prop->it_interval.tv_nsec = (time_slice % 1000000)*1000;
	clk_prop->it_value = clk_prop->it_interval;
}
#else
static void ma_slice2timer(struct itimerval *clk_prop)
{
	clk_prop->it_interval.tv_sec = time_slice / 1000000;
	clk_prop->it_interval.tv_usec = time_slice % 1000000;
	clk_prop->it_value = clk_prop->it_interval;
}
#endif


#if defined(MARCEL_SIGNALS_ENABLED) && defined(MA__LIBPTHREAD)
#ifndef SYS_signal
static ma_sighandler_t TBX_UNUSED ma_signal(int sig, ma_sighandler_t handler)
{
	ma_sigaction_t act;
	ma_sigaction_t oact;

	act.sa_handler = handler;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, sig);

	act.sa_flags = SA_RESTART;
	if (ma_sigaction(sig, &act, &oact) < 0)
		return SIG_ERR;

	return oact.sa_handler;
}
#endif
#endif

// Frequency of debug messages in timer_interrupt
#ifndef TICK_RATE
#define TICK_RATE 1
#endif
// Softirq called by the timer
static void timer_action(struct ma_softirq_action *a TBX_UNUSED)
{
#ifdef MA__DEBUG
	static unsigned long tick = 0;
	if (++tick == TICK_RATE) {
		MARCEL_TIMER_LOG("\t\t\t<<tick>>\n");
		tick = 0;
	}
#endif

#ifdef MA__DEBUG
	if (MA_LWP_SELF == NULL) {
		PM2_LOG("WARNING!!! MA_LWP_SELF == NULL in thread %p!\n", MARCEL_SELF);
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
	}
#endif
	MTRACE_TIMER("TimerSig", MARCEL_SELF);

	ma_update_process_times(1);
}

static void timer_start(void)
{
	ma_open_softirq(MA_TIMER_HARDIRQ, timer_action, NULL);
}
__ma_initfunc(timer_start, MA_INIT_TIMER, "Install TIMER SoftIRQ");


#ifdef PROFILE
static void int_catcher_exit(ma_lwp_t lwp)
{
	MARCEL_LOG_IN();
	ma_signal(SIGINT, SIG_DFL);
	MARCEL_LOG_OUT();
}

static void int_catcher(int signo)
{
	static int got;
	if (got) {
		MA_WARN_USER("SIGINT caught a second time, exitting\n");
		exit(EXIT_FAILURE);
	}
	got = 1;
	MA_WARN_USER("SIGINT caught, saving profile\n");
	PROF_EVENT(fut_stop);
	profile_stop();
	profile_exit();
	int_catcher_exit(NULL);
	raise(SIGINT);
}

static void int_catcher_init(ma_lwp_t lwp)
{
	MARCEL_LOG_IN();
	ma_signal(SIGINT, int_catcher);
	MARCEL_LOG_OUT();
}
MA_DEFINE_LWP_NOTIFIER_ONOFF(int_catcher, "Int catcher",
			     int_catcher_init, "Start int catcher",
			     int_catcher_exit, "Stop int catcher");

MA_LWP_NOTIFIER_CALL_ONLINE_PRIO(int_catcher, MA_INIT_INT_CATCHER,
				 MA_INIT_INT_CATCHER_PRIO);

#endif				/* PROFILE */

/****************************************************************
 * The TIMER signal
 *
 * In the POSIX norm, setitimer(ITIMER_REAL) sends regularly SIGALRM to the
 * _processus_, thus to only one of its threads, that thread thus has to
 * forward SIGALRM to the other threads.
 *
 * It happens that linux <= 2.4 or maybe a bit more, solaris <= I don't know,
 * ... have a "per-thread" ITIMER_REAL !!
 *
 */
#if defined(MA__LWPS) && defined(MA__TIMER) && !defined(OLD_TIMER_REAL) && !defined(MA__USE_TIMER_CREATE)
#  define MA__DISTRIBUTE_PREEMPT_SIG
#endif

static sigset_t sigalrmset, sigeptset;

// Function called each time SIGALRM is delivered to the current LWP.
#ifdef SA_SIGINFO
static void timer_interrupt(int sig TIMER_VAR_UNUSED, siginfo_t * info LWPS_VAR_UNUSED,
			    void *uc TBX_UNUSED)
#else
static void timer_interrupt(int sig)
#endif
{
#ifdef MA__TIMER
#ifdef MA__DEBUG
	static unsigned long tick = 0;
#endif
#endif

#ifdef SA_SIGINFO
	/* Don't do anything before this */
	MA_ARCH_INTERRUPT_ENTER_LWP_FIX(MARCEL_SELF, uc);
#endif

	/* Avoid recursing interrupts. Not completely safe, but better than nothing */
	if (ma_in_irq()) {
		return;
	}
	ma_irq_enter();

	PROF_EVENT(timer_interrupt);

	/* check that stack isn't overflowing */
#if !defined(ENABLE_STACK_JUMPING) && !defined(MA__SELF_VAR)
	if (marcel_stackbase(MARCEL_SELF))
		if (get_sp() < ((unsigned long) marcel_stackbase(MARCEL_SELF) + (THREAD_SLOT_SIZE / 0x10))) {
			MARCEL_TIMER_LOG("SP reached %lx while stack extends from %p to %p. Either you have a stack overflow in your program, or you just need to make THREAD_SLOT_SIZE bigger than 0x%lx in marcel/include/sys/isomalloc_archdep.c\n",
			     get_sp(), marcel_stackbase(MARCEL_SELF),
			     marcel_stackbase(MARCEL_SELF) + THREAD_SLOT_SIZE,
			     THREAD_SLOT_SIZE);
			MA_BUG();
		}
#endif

#ifdef MA__DISTRIBUTE_PREEMPT_SIG
#if !defined(MA_BOGUS_SIGINFO_CODE)
	if (!info || info->si_code == SI_KERNEL)
#elif MARCEL_TIMER_SIGNAL == MARCEL_TIMER_USERSIGNAL
#error "Can't distinguish between kernel and user signal"
#else
	if (sig == MARCEL_TIMER_SIGNAL)
#endif
	{
		/* kernel timer signal, distribute */
		ma_lwp_t lwp;
		ma_lwp_list_lock_read();
		ma_for_each_lwp_from_begin(lwp, MA_LWP_SELF) {
			if (lwp->vpnum != -1 && lwp->vpnum < (int) marcel_nbvps()
			    && !marcel_vp_is_disabled(lwp->vpnum))
				marcel_kthread_kill(lwp->pid, MARCEL_TIMER_USERSIGNAL);
		} ma_for_each_lwp_from_end();
		ma_lwp_list_unlock_read();
	}
#endif

#ifdef MA__TIMER
#ifdef MA__DEBUG
	if (sig == MARCEL_TIMER_SIGNAL)
		if (++tick == TICK_RATE) {
			MARCEL_TIMER_LOG("\t\t\t<<Sig handler>>\n");
			tick = 0;
		}
#endif
#endif

#ifdef MA__TIMER
	if (
#  ifdef MA__LWPS
		MA_LWP_SELF->vpnum != -1 && (MA_LWP_SELF->vpnum < (int) marcel_nbvps())
		&&
#  endif
		(sig == MARCEL_TIMER_SIGNAL || sig == MARCEL_TIMER_USERSIGNAL)) {
#  ifndef MA_HAVE_COMPAREEXCHANGE
		// Avoid raising softirq if compareexchange is not implemented and
		// a compare & exchange is currently running...
		if (!ma_spin_is_locked_nofail(&ma_compareexchange_spinlock))
#  endif
		{
			ma_raise_softirq_from_hardirq(MA_TIMER_HARDIRQ);
		}
#  if defined(MA__LWPS)
		/*
		 * We need to advance the time only once. This depends on the situation
		 * (only one kernel signal redistributed by hand to other LWPs, or
		 * signals already distributed by the kernel)
		 */
#    if !defined(MA_BOGUS_SIGINFO_CODE)
		if (!info ||
		    /* Distinguish by siginfo */
#      if defined(MA__USE_TIMER_CREATE)
		    /* timer_create is per-lwp */
		    (info->si_code == SI_TIMER && ma_is_first_lwp(MA_LWP_SELF))
#      else
		    /* traditional process-wide setitimer */
		    /* Sent by the kernel */
		    (info->si_code == SI_KERNEL)
#      endif
		    )
#    elif (MARCEL_TIMER_SIGNAL != MARCEL_TIMER_USERSIGNAL)
		/* Distinguish by signal number */
		if (sig == MARCEL_TIMER_SIGNAL)
#    else
		/* No way to distinguish them apart. */
		if (ma_is_first_lwp(MA_LWP_SELF))
#    endif
#  endif
			/* kernel timer signal */
		{
			ma_jiffies += MA_JIFFIES_PER_TIMER_TICK;
		}
	}
#endif

#if MA__SIGNAL_NODEFER
	ma_irq_exit();
	ma_preempt_check_resched(0);
#else
	ma_preempt_check_resched(MA_HARDIRQ_OFFSET);
	ma_irq_exit();
#endif

#ifdef SA_SIGINFO
	MA_ARCH_INTERRUPT_EXIT_LWP_FIX(MARCEL_SELF, uc);
#endif
}

void marcel_sig_pause(void)
{
#ifdef MARCEL_SIGNALS_ENABLED
	ma_sigsuspend(&ma_thr_ksigmask);
#else
	ma_sigsuspend(&sigeptset);
#endif
}

void marcel_sig_nanosleep(void)
{
	/* TODO: if possible, rather use a temporised marcel_kthread_something, to avoid the signal machinery to get interrupted. */
	struct timespec time = {.tv_sec = 0,.tv_nsec = 1 };
	ma_nanosleep(&time, NULL);
}

/* Interval timers don't need to be explicitely created, they always exist in
 * processes.
 *
 * Timers created through timer_create are however destroyed during fork, so
 * they need to be recreated from the cleanup_child_after_fork handler.
 */
void marcel_sig_create_timer(ma_lwp_t lwp TBX_UNUSED)
{
#ifdef MA__TIMER
#ifdef MA__USE_TIMER_CREATE
	/* Create per-thread timer */
	struct sigevent ev;
	int ret TBX_UNUSED;

#ifdef PM2VALGRIND
	memset(&ev, 0, sizeof(ev));
#endif
	ev.sigev_notify = SIGEV_THREAD_ID;
	ev.sigev_signo = MARCEL_TIMER_SIGNAL;
#ifndef sigev_notify_thread_id
#define sigev_notify_thread_id _sigev_un._tid
#endif
	ev.sigev_notify_thread_id = marcel_gettid();
	ret = timer_create(CLOCK_MONOTONIC, &ev, &lwp->timer);
	MA_BUG_ON(ret);
#endif
#endif
}

#undef marcel_sig_reset_perlwp_timer
void marcel_sig_reset_perlwp_timer(void)
{
	MARCEL_LOG_IN();

#ifdef MA__TIMER
#ifdef MA__USE_TIMER_CREATE
	{
		struct itimerspec value;

		ma_slice2timer(&value);
		timer_settime(__ma_get_lwp_var(timer), 0, &value, NULL);
	}
#endif
#endif

	MARCEL_LOG_OUT();
}

#undef marcel_sig_reset_timer
void marcel_sig_reset_timer(void)
{
	MARCEL_LOG_IN();

#ifdef MA__TIMER
#ifdef MA__USE_TIMER_CREATE
	marcel_sig_reset_perlwp_timer();
#else
	{
		struct itimerval value;

		ma_slice2timer(&value);
		if (ma_setitimer(MARCEL_ITIMER_TYPE, &value, (struct itimerval *) NULL)) {
			perror("can't start itimer");
			exit(1);
		}
	}
#endif
#endif

	MARCEL_LOG_OUT();
}

void marcel_settimeslice(unsigned long microsecs)
{
	MARCEL_LOG_IN();

	if (microsecs && (microsecs < MARCEL_MIN_TIME_SLICE)) {
		time_slice = MARCEL_MIN_TIME_SLICE;
	} else {
		time_slice = microsecs;
	}
	marcel_extlib_protect();
	marcel_sig_reset_timer();
	marcel_extlib_unprotect();

	MARCEL_LOG_OUT();
}

/* returns microseconds */
unsigned long marcel_gettimeslice(void)
{
	MARCEL_LOG_IN();
	MARCEL_LOG_RETURN(time_slice);
}


void __ma_sig_enable_interrupts(void)
{
#ifdef MARCEL_SIGNALS_ENABLED
	ma_spin_lock_softirq(&__ma_get_lwp_var(timer_sigmask_lock));
	sigemptyset(&__ma_get_lwp_var(timer_sigmask));
#endif
	marcel_kthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#ifdef MARCEL_SIGNALS_ENABLED
	ma_spin_unlock_softirq(&__ma_get_lwp_var(timer_sigmask_lock));
#endif
}

/* Preemption must be already disabled here, else we don't know which LWP we are
 * disabling interrupts on!
*/
void __ma_sig_disable_interrupts(void)
{
#ifdef MARCEL_SIGNALS_ENABLED
	ma_spin_lock_softirq(&__ma_get_lwp_var(timer_sigmask_lock));
	__ma_get_lwp_var(timer_sigmask) = sigalrmset;
#endif
	marcel_kthread_sigmask(SIG_BLOCK, &sigalrmset, NULL);
#ifdef MARCEL_SIGNALS_ENABLED
	ma_spin_unlock_softirq(&__ma_get_lwp_var(timer_sigmask_lock));
#endif
}

void marcel_sig_enable_interrupts(void)
{
	MA_BUG_ON(!ma_in_atomic());
	__ma_sig_enable_interrupts();
}

void marcel_sig_disable_interrupts(void)
{
	__ma_sig_disable_interrupts();
	MA_BUG_ON(!ma_in_atomic());
}


static void sig_start_timer(ma_lwp_t lwp)
{
	ma_sigaction_t sa;

	MARCEL_LOG_IN();

	sigemptyset(&sa.sa_mask);
#ifdef SA_RESTART
	sa.sa_flags = SA_RESTART;
#else
	sa.sa_flags = 0;
#endif

	
#ifdef MA__SIGNAL_NODEFER
	sa.sa_flags |= MA__SIGNAL_NODEFER;
#else
#warning no way to allow nested signals
#endif

#ifdef SA_SIGINFO
	sa.sa_flags |= SA_SIGINFO;
	sa.sa_sigaction = timer_interrupt;
#else
	sa.sa_handler = timer_interrupt;
#endif

#ifdef MA__TIMER
	/* We have to do it for each LWP for <= 2.4 linux kernels */
	ma_sigaction(MARCEL_TIMER_SIGNAL, &sa, NULL);
#if MARCEL_TIMER_USERSIGNAL != MARCEL_TIMER_SIGNAL
	ma_sigaction(MARCEL_TIMER_USERSIGNAL, &sa, NULL);
#endif

	marcel_sig_create_timer(lwp);
#endif

	ma_sigaction(MARCEL_RESCHED_SIGNAL, &sa, NULL);

#ifdef MARCEL_SIGNALS_ENABLED
	sigemptyset(&lwp->timer_sigmask);
	ma_spin_lock_init(&lwp->timer_sigmask_lock);
#endif
	__ma_sig_enable_interrupts();

#ifdef MA__DISTRIBUTE_PREEMPT_SIG
	if (ma_is_first_lwp(lwp))
#endif
		marcel_sig_reset_timer();

	MARCEL_LOG_OUT();
}

#undef marcel_sig_stop_perlwp_itimer
void marcel_sig_stop_perlwp_itimer(void)
{
#ifdef MA__TIMER
#ifdef MA__USE_TIMER_CREATE
	{
		struct itimerspec value;
		memset(&value, 0, sizeof(value));
		timer_settime(__ma_get_lwp_var(timer), 0, &value, NULL);
	}
#endif
#endif
}

#undef marcel_sig_stop_itimer
void marcel_sig_stop_itimer(void)
{
#ifdef MA__TIMER
#ifdef MA__USE_TIMER_CREATE
	marcel_sig_stop_perlwp_itimer();
#else
	{
		struct itimerval value;
		memset(&value, 0, sizeof(value));
		ma_setitimer(MARCEL_ITIMER_TYPE, &value, (struct itimerval *) NULL);
	}
#endif
#endif
}

static void sig_stop_timer(ma_lwp_t lwp TBX_UNUSED)
{
	ma_sigaction_t sa;
	MARCEL_LOG_IN();

#ifndef MA__DISTRIBUTE_PREEMPT_SIG
	marcel_sig_stop_itimer();
#endif

#ifdef MA__USE_TIMER_CREATE
	/* Delete per-thread timer */
	timer_delete(lwp->timer);
#endif

	/* Execpt with linux <= 2.4, signal handlers are _not_ per thread, we
	 * thus have to keep the common handler up to the termination, and just
	 * mask the signal.
	 *
	 * We need it in the MA__DISTRIBUTE_PREEMPT_SIG case anyway. */
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;

#ifdef MA__TIMER
	ma_sigaction(MARCEL_TIMER_SIGNAL, &sa, NULL);
#if MARCEL_TIMER_USERSIGNAL != MARCEL_TIMER_SIGNAL
	ma_sigaction(MARCEL_TIMER_USERSIGNAL, &sa, NULL);
#endif
#endif
	ma_sigaction(MARCEL_RESCHED_SIGNAL, &sa, NULL);

	MARCEL_LOG_OUT();
}

void marcel_sig_exit(void)
{
	sig_stop_timer(MA_LWP_SELF);
	return;
}

/* TODO:
 * On a real POSIX system, signal handlers are per-processus, it is thus not
 * useful to redefine them for each LWP.  Except on Linux 2.4 (herm...) */

/* TODO:
 * Detect old linux case dynamically instead */
MA_DEFINE_LWP_NOTIFIER_ONOFF(sig_timer, "Signal timer",
			     sig_start_timer, "Start signal timer",
			     sig_stop_timer, "Stop signal timer");

MA_LWP_NOTIFIER_CALL_ONLINE(sig_timer, MA_INIT_TIMER_SIG);

static void sig_init(void)
{
	sigemptyset(&sigeptset);
	sigemptyset(&sigalrmset);
#ifdef MA__TIMER
	sigaddset(&sigalrmset, MARCEL_TIMER_SIGNAL);
#if MARCEL_TIMER_USERSIGNAL != MARCEL_TIMER_SIGNAL
	sigaddset(&sigalrmset, MARCEL_TIMER_USERSIGNAL);
#endif
#endif
	sigaddset(&sigalrmset, MARCEL_RESCHED_SIGNAL);
#ifdef MA__LWPS
	/* Block signals before starting LWPs, so that LWPs started by the
	 * pthread library do not get signals at first. */
#ifdef MARCEL_SIGNALS_ENABLED
	__ma_get_lwp_var(timer_sigmask) = sigalrmset;
#endif
	marcel_kthread_sigmask(SIG_BLOCK, &sigalrmset, NULL);
#else
#ifdef MARCEL_SIGNALS_ENABLED
	sigemptyset(&(__ma_get_lwp_var(timer_sigmask)));
#endif
#endif
}

__ma_initfunc(sig_init, MA_INIT_TIMER_SIG_DATA, "Signal static data");
