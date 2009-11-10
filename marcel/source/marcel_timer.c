
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

#define MA_FILE_DEBUG timer
#include "marcel.h"
#include "tbx_compiler.h"
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#if defined(MA__LIBPTHREAD) && defined(MARCEL_SIGNALS_ENABLED)
#include <sys/syscall.h>
#define sigaction ma_kernel_sigaction
#define sigsuspend(mask) syscall(SYS_rt_sigsuspend,mask,_NSIG/8)
#define sigprocmask(how,set,oset) syscall(SYS_rt_sigprocmask,how,set,oset,_NSIG/8)
#define nanosleep(t1,t2) syscall(SYS_nanosleep,t1,t2)
#define setitimer(which,val,oval) syscall(SYS_setitimer,which,val,oval)
#ifdef SYS_signal
#define signal(sig,handler) syscall(SYS_signal,sig,handler)
#else
#define signal(sig,handler) kernel_signal(sig,handler)
static ma_sighandler_t TBX_UNUSED signal(int sig, ma_sighandler_t handler)
{
	struct sigaction act;
	struct sigaction oact;

	act.sa_handler = handler;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, sig);

	act.sa_flags = SA_RESTART;
	if (sigaction(sig, &act, &oact) < 0)
		return SIG_ERR;

	return oact.sa_handler;
}

#endif
#endif

ma_atomic_t __ma_preemption_disabled = MA_ATOMIC_INIT(0);

/* Unit : microseconds */
static volatile unsigned long time_slice = MARCEL_DEFAULT_TIME_SLICE;

#ifdef MARCEL_SIGNALS_ENABLED
sigset_t ma_timer_sigmask;
ma_spinlock_t ma_timer_sigmask_lock = MA_SPIN_LOCK_UNLOCKED;
#endif

// Frequency of debug messages in timer_interrupt
#ifndef TICK_RATE
#define TICK_RATE 1
#endif

unsigned long volatile ma_jiffies=0;

// l'horloge de marcel
static volatile unsigned long __milliseconds = 0;

unsigned long marcel_clock(void)
{
	return __milliseconds;
}

// Softirq called by the timer
static void timer_action(struct ma_softirq_action *a TBX_UNUSED)
{
#ifdef MA__DEBUG
	static unsigned long tick = 0;
#endif

	//LOG_IN();

#ifdef MA__DEBUG
	if (++tick == TICK_RATE) {
		mdebugl(7,"\t\t\t<<tick>>\n");
		tick = 0;
	}
#endif

#ifdef MA__DEBUG
	if(MA_LWP_SELF == NULL) {
		pm2debug("WARNING!!! MA_LWP_SELF == NULL in thread %p!\n",
			 MARCEL_SELF);
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
	}
#endif
	
	MTRACE_TIMER("TimerSig", MARCEL_SELF);
		
	ma_update_process_times(1);
		
	//LOG_OUT();
}

static void __marcel_init timer_start(void)
{
	ma_open_softirq(MA_TIMER_HARDIRQ, timer_action, NULL);
}

__ma_initfunc(timer_start, MA_INIT_TIMER, "Install TIMER SoftIRQ");


#ifndef __MINGW32__
#ifdef PROFILE
static void int_catcher_exit(ma_lwp_t lwp)
{
	LOG_IN();
	signal(SIGINT, SIG_DFL);
	LOG_OUT();
}
static void int_catcher(int signo)
{
	static int got;
	if (got) {
		fprintf(stderr,"SIGINT caught a second time, exitting\n");
		exit(EXIT_FAILURE);
	}
	got = 1;
	fprintf(stderr,"SIGINT caught, saving profile\n");
	PROF_EVENT(fut_stop);
	profile_stop();
	profile_exit();
	int_catcher_exit(NULL);
	raise(SIGINT);
}
static void int_catcher_init(ma_lwp_t lwp)
{
	LOG_IN();
	signal(SIGINT, int_catcher);
	LOG_OUT();
}
MA_DEFINE_LWP_NOTIFIER_ONOFF(int_catcher, "Int catcher",
			     int_catcher_init, "Start int catcher",
			     int_catcher_exit, "Stop int catcher");

MA_LWP_NOTIFIER_CALL_ONLINE_PRIO(int_catcher, MA_INIT_INT_CATCHER, MA_INIT_INT_CATCHER_PRIO);

#endif /* PROFILE */
#endif /* __MINGW32__ */

/****************************************************************
 * The TIMER signal
 *
 * In the POSIX norm, setitimer(ITIMER_REAL) sends regularly SIGALRM to the
 * _processus_, thus to only one of its threads, that thread thus has to
 * forward SIGALRM to the other threads.
 *
 * setitimer(ITIMER_VIRTUAL) is however "per-thread".
 *
 * It happens that linux <= 2.4 or maybe a bit more, solaris <= I don't know,
 * ... have a "per-thread" ITIMER_REAL !!
 *
 * We'd thus prefer ITIMER_VIRTUAL since in that case the kernel handles
 * distributing signals, but one problem is that it expires only when we
 * consume cpu...
 */
#ifndef __MINGW32__

#if defined(MA__LWPS) && defined(MA__TIMER) && !defined(USE_VIRTUAL_TIMER) && !defined(DARWIN_SYS) && !defined(OLD_TIMER_REAL) && !defined(MA__USE_TIMER_CREATE)
#define DISTRIBUTE_SIGALRM
#endif

static sigset_t sigalrmset, sigeptset;

// Function called each time SIGALRM is delivered to the current LWP.
#ifdef SA_SIGINFO
static void timer_interrupt(int sig, siginfo_t *info, void *uc TBX_UNUSED)
#else
static void timer_interrupt(int sig)
#endif
{
#ifdef MA__TIMER
#ifdef MA__DEBUG
	static unsigned long tick = 0;
#endif
#endif
	/* Don't do anything before this */
	MA_ARCH_INTERRUPT_ENTER_LWP_FIX(MARCEL_SELF, uc);

	/* Avoid recursing interrupts. Not completely safe, but better than nothing */
	if (ma_in_irq()) {
	        return;
	}

	ma_irq_enter();

	PROF_EVENT(timer_interrupt);

	/* check that stack isn't overflowing */
#if !defined(ENABLE_STACK_JUMPING) && !defined(MA__SELF_VAR)
	if (marcel_stackbase(MARCEL_SELF))
		if (get_sp() < (unsigned long) marcel_stackbase(MARCEL_SELF) + (THREAD_SLOT_SIZE / 0x10)) {
			mdebugl(0,"SP reached %lx while stack extends from %p to %p. Either you have a stack overflow in your program, or you just need to make THREAD_SLOT_SIZE bigger than 0x%lx in marcel/include/sys/isomalloc_archdep.c\n",get_sp(),marcel_stackbase(MARCEL_SELF),marcel_stackbase(MARCEL_SELF)+THREAD_SLOT_SIZE, THREAD_SLOT_SIZE);
			MA_BUG();
		}
#endif

#ifdef DISTRIBUTE_SIGALRM
#if !defined(MA_BOGUS_SIGINFO_CODE)
	if (!info || info->si_code > 0)
#elif MARCEL_TIMER_SIGNAL == MARCEL_TIMER_USERSIGNAL
#error "Can't distinguish between kernel and user signal"
#else
	if (sig == MARCEL_TIMER_SIGNAL)
#endif
	{
		/* kernel timer signal, distribute */
		ma_lwp_t lwp;
		ma_for_each_lwp_from_begin(lwp,MA_LWP_SELF) {
			if (lwp->vpnum != -1 && lwp->vpnum < marcel_nbvps())
			        marcel_kthread_kill(lwp->pid, MARCEL_TIMER_USERSIGNAL);
		} ma_for_each_lwp_from_end();
	}
#endif

#ifdef MA__TIMER
#ifdef MA__DEBUG
	if (sig == MARCEL_TIMER_SIGNAL)
		if (++tick == TICK_RATE) {
			mdebugl(7,"\t\t\t<<Sig handler>>\n");
			tick = 0;
		}
#endif
#endif

#ifdef MA__TIMER
	if (
#  ifdef MA__LWPS
	    MA_LWP_SELF->vpnum !=-1 && MA_LWP_SELF->vpnum < marcel_nbvps() &&
#  endif
	    (sig == MARCEL_TIMER_SIGNAL || sig == MARCEL_TIMER_USERSIGNAL)) {
#  ifndef MA_HAVE_COMPAREEXCHANGE
	// Avoid raising softirq if compareexchange is not implemented and
	// a compare & exchange is currently running...
		if (!ma_spin_is_locked_nofail(&ma_compareexchange_spinlock))
#  endif
			{ ma_raise_softirq_from_hardirq(MA_TIMER_HARDIRQ); }
#  if defined(MA__LWPS)
#    if !defined(MA_BOGUS_SIGINFO_CODE)
		if (!info || info->si_code > 0
#      ifdef MA__USE_TIMER_CREATE
				|| (info->si_code == SI_TIMER && ma_is_first_lwp(MA_LWP_SELF))
#      endif
				)
#    elif MARCEL_TIMER_SIGNAL == MARCEL_TIMER_USERSIGNAL
		if (ma_is_first_lwp(MA_LWP_SELF))
#    else
		if (sig == MARCEL_TIMER_SIGNAL)
#    endif
#  endif
		/* kernel timer signal */
		{
			ma_jiffies+=MA_JIFFIES_PER_TIMER_TICK;
			__milliseconds += time_slice/1000;
		}
	}
#endif

#ifdef MA_SIGNAL_NOMASK
	ma_irq_exit();
	ma_preempt_check_resched(0);
#else
	ma_preempt_check_resched(MA_HARDIRQ_OFFSET );
	ma_irq_exit();
#endif

	MA_ARCH_INTERRUPT_EXIT_LWP_FIX(MARCEL_SELF, uc);
}
#endif

void marcel_sig_pause(void)
{
#if !defined(USE_VIRTUAL_TIMER)
	sigsuspend(&sigeptset);
#else
	SCHED_YIELD();
#endif
}

void marcel_sig_nanosleep(void)
{
	/* TODO: if possible, rather use a temporised marcel_kthread_something, to avoid the signal machinery to get interrupted. */
#if !defined(USE_VIRTUAL_TIMER)
	struct timespec time = { .tv_sec = 0, .tv_nsec = 1 };
#ifdef OSF_SYS
	nanosleep_d9(&time, NULL);
#else // ! OSF_SYS
	nanosleep(&time, NULL);
#endif // OSF_SYS
#else
	SCHED_YIELD();
#endif
}

/* Interval timers don't need to be explicitely created, they always exist in
 * processes.
 *
 * Timers created through timer_create are however destroyed during fork, so
 * they need to be recreated from the cleanup_child_after_fork handler.
 */
void marcel_sig_create_timer(ma_lwp_t lwp)
{
#ifdef MA__USE_TIMER_CREATE
	/* Create per-thread timer */
	{
		struct sigevent ev = {};
		int ret;
		ev.sigev_notify = SIGEV_THREAD_ID;
		ev.sigev_signo = MARCEL_TIMER_SIGNAL;
#ifndef sigev_notify_thread_id
#define sigev_notify_thread_id _sigev_un._tid
#endif
		ev.sigev_notify_thread_id = marcel_gettid();
		ret = timer_create(CLOCK_MONOTONIC, &ev, &lwp->timer);
		MA_BUG_ON(ret);
	}
#endif
}

void marcel_sig_reset_timer(void)
{
	LOG_IN();

#ifdef MA__TIMER
#ifdef MA__USE_TIMER_CREATE
    {
	struct itimerspec value = {};

	value.it_interval.tv_sec = 0;
	value.it_interval.tv_nsec = time_slice * 1000;
	value.it_value = value.it_interval;
	timer_settime(__ma_get_lwp_var(timer), 0, &value, NULL);
    }
#else
    {
	struct itimerval value = {};
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = time_slice;
	value.it_value = value.it_interval;
	if (setitimer(MARCEL_ITIMER_TYPE, &value, 
		  (struct itimerval *)NULL)) {
		perror("can't start itimer");
		exit(1);
	}
    }
#endif
#endif
	
	LOG_OUT();
}

void marcel_settimeslice(unsigned long microsecs)
{
	LOG_IN();

	if (microsecs && (microsecs < MARCEL_MIN_TIME_SLICE)) {
		time_slice = MARCEL_MIN_TIME_SLICE;
	} else {
		time_slice = microsecs;
	}
	marcel_extlib_protect();
	marcel_sig_reset_timer();
	marcel_extlib_unprotect();
	
	LOG_OUT();
}

#ifdef MA__TIMER
/* returns microseconds */
unsigned long marcel_gettimeslice(void)
{
	LOG_IN();

	LOG_RETURN(time_slice);
}
#endif


#ifndef __MINGW32__
void marcel_sig_enable_interrupts(void)
{
#ifdef MARCEL_SIGNALS_ENABLED
	ma_spin_lock_softirq(&ma_timer_sigmask_lock);
	sigemptyset(&ma_timer_sigmask);
#endif
	marcel_kthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#ifdef MARCEL_SIGNALS_ENABLED
	ma_spin_unlock_softirq(&ma_timer_sigmask_lock);
#endif
}

void marcel_sig_disable_interrupts(void)
{
#ifdef MARCEL_SIGNALS_ENABLED
	ma_spin_lock_softirq(&ma_timer_sigmask_lock);
	ma_timer_sigmask = sigalrmset;
#endif
	marcel_kthread_sigmask(SIG_BLOCK, &sigalrmset, NULL);
#ifdef MARCEL_SIGNALS_ENABLED
	ma_spin_unlock_softirq(&ma_timer_sigmask_lock);
#endif
}


static void sig_start_timer(ma_lwp_t lwp)
{
	struct sigaction sa;

	LOG_IN();

	sigemptyset(&sa.sa_mask);
#ifdef SA_RESTART
	sa.sa_flags = SA_RESTART;
#else
	sa.sa_flags = 0;
#endif

#ifdef MA_SIGNAL_NOMASK
#if defined(SA_NOMASK)
	sa.sa_flags |= SA_NOMASK;
#elif defined(SA_NODEFER)
	sa.sa_flags |= SA_NODEFER;
#else
#warning no way to allow nested signals
#endif
#endif

#ifdef SA_SIGINFO
	sa.sa_flags |= SA_SIGINFO;
	sa.sa_sigaction = timer_interrupt;
#else
	sa.sa_handler = timer_interrupt;
#endif
	
#ifdef MA__TIMER
	/* We have to do it for each LWP for <= 2.4 linux kernels */
	sigaction(MARCEL_TIMER_SIGNAL, &sa, (struct sigaction *)NULL);
#if MARCEL_TIMER_USERSIGNAL != MARCEL_TIMER_SIGNAL
	sigaction(MARCEL_TIMER_USERSIGNAL, &sa, (struct sigaction *)NULL);
#endif

	marcel_sig_create_timer(lwp);
#endif

	sigaction(MARCEL_RESCHED_SIGNAL, &sa, (struct sigaction *)NULL);

	marcel_sig_enable_interrupts();

#ifdef DISTRIBUTE_SIGALRM
	if (ma_is_first_lwp(lwp))
#endif
		marcel_sig_reset_timer();
	
	LOG_OUT();
}

#undef marcel_sig_stop_itimer
void marcel_sig_stop_itimer(void)
{
#ifdef MA__TIMER
#ifdef MA__USE_TIMER_CREATE
    {
	struct itimerspec value = {};
	memset(&value,0,sizeof(value));
	timer_settime(__ma_get_lwp_var(timer), 0, &value, NULL);
    }
#else
    {
	struct itimerval value;
	memset(&value,0,sizeof(value));
	setitimer(MARCEL_ITIMER_TYPE, &value, (struct itimerval *)NULL);
    }
#endif
#endif
}

static void sig_stop_timer(ma_lwp_t lwp TBX_UNUSED)
{
	struct sigaction sa;
	LOG_IN();

#ifndef DISTRIBUTE_SIGALRM
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
	 * We need it in the DISTRIBUTE_SIGALRM case anyway. */
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;

#ifdef MA__TIMER
	sigaction(MARCEL_TIMER_SIGNAL, &sa, (struct sigaction *)NULL);
#if MARCEL_TIMER_USERSIGNAL != MARCEL_TIMER_SIGNAL
	sigaction(MARCEL_TIMER_USERSIGNAL, &sa, (struct sigaction *)NULL);
#endif
#endif
	sigaction(MARCEL_RESCHED_SIGNAL, &sa, (struct sigaction *)NULL);

	LOG_OUT();
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

static void __marcel_init sig_init(void)
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
	ma_timer_sigmask = sigalrmset;
#endif
	marcel_kthread_sigmask(SIG_BLOCK, &sigalrmset, NULL);
#else
#ifdef MARCEL_SIGNALS_ENABLED
	sigemptyset(&ma_timer_sigmask);
#endif
#endif
}
__ma_initfunc(sig_init, MA_INIT_TIMER_SIG_DATA, "Signal static data");
#endif
