
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
#include <linux/unistd.h>

#ifdef __NR_gettid
_syscall0(pid_t,gettid)
#endif

ma_atomic_t __preemption_disabled=MA_ATOMIC_INIT(0);

/* Unité : microsecondes */
#define MIN_TIME_SLICE		10000
#define DEFAULT_TIME_SLICE	MIN_TIME_SLICE
static volatile unsigned long time_slice = DEFAULT_TIME_SLICE;

// Fréquence des messages de debug dans timer_interrupt
#ifndef TICK_RATE
#define TICK_RATE 1
#endif

unsigned long volatile ma_jiffies=0;

// l'horloge de marcel
static volatile unsigned long __milliseconds = 0;
static void update_time()
{
	if(LWP_NUMBER(LWP_SELF) == 0) {
		__milliseconds += time_slice/1000;
	}
}

unsigned long marcel_clock(void)
{
   return __milliseconds;
}

// Softirq appellée par le timer
static void timer_action(struct ma_softirq_action *a)
{
#ifdef MA__DEBUG
	static unsigned long tick = 0;
#endif

	LOG_IN();

#ifdef MA__DEBUG
	if (++tick == TICK_RATE) {
		mdebug("\t\t\t<<tick>>\n");
		tick = 0;
	}
#endif

	update_time();
	
#ifdef MA__DEBUG
	if(LWP_SELF == NULL) {
		pm2debug("WARNING!!! LWP_SELF == NULL in thread %p!\n",
			 MARCEL_SELF);
		RAISE(PROGRAM_ERROR);
	}
#endif
	
	MTRACE_TIMER("TimerSig", MARCEL_SELF);
		
	//marcel_check_sleeping();
	//marcel_check_polling(MARCEL_POLL_AT_TIMER_SIG);
	ma_jiffies+=MA_JIFFIES_PER_TIMER_TICK;
	ma_update_process_times(1);
		
	LOG_OUT();
}

void __init timer_start(void)
{
	ma_open_softirq(MA_TIMER_HARDIRQ, timer_action, NULL);
}

__ma_initfunc(timer_start, MA_INIT_TIMER, "Install TIMER SoftIRQ");


/****************************************************************
 * Trappeur de SIGSEGV, de manière à obtenir des indications sur le
 * thread fautif (ça aide pour le debug !)
 */
static void TBX_NORETURN fault_catcher(int sig)
{
	fprintf(stderr, "OOPS!!! Signal %d catched on thread %p (%ld)\n",
		sig, MARCEL_SELF, THREAD_GETMEM(MARCEL_SELF,number));
	if(LWP_SELF != NULL)
		fprintf(stderr, "OOPS!!! current lwp is %d\n",
			LWP_NUMBER(LWP_SELF));
	
#if defined(LINUX_SYS) && defined(MA__LWPS)
	fprintf(stderr,
		"OOPS!!! Entering endless loop "
		"so you can try to attach process to gdb (pid = %d)\n",
#ifdef __NR_gettid
		gettid()!=-1 || errno!= ENOSYS ? gettid() : getpid());
#else
		getpid());
#endif
	for(;;) ;
#endif

	abort();
}

static void fault_catcher_init(void)
{
	static struct sigaction sa;

	LOG_IN();
	// On va essayer de rattraper SIGSEGV, etc.
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = fault_catcher;
	sa.sa_flags = 0;
	sigaction(SIGSEGV, &sa, (struct sigaction *)NULL);
	LOG_OUT();
}

static void fault_catcher_exit()
{
	struct sigaction sa;

	LOG_IN();
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_DFL;
	sa.sa_flags = 0;
	sigaction(SIGSEGV, &sa, (struct sigaction *)NULL);
	LOG_OUT();
}

static int fault_catcher_notify(struct ma_notifier_block *self, 
		     		unsigned long action, void *hlwp)
{
	switch(action) {
	case MA_LWP_ONLINE:
		fault_catcher_init();
		break;
	case MA_LWP_OFFLINE:
		fault_catcher_exit();
		break;
	default:
		break;
	}
	return 0;
}

static struct ma_notifier_block fault_catcher_nb = {
	.notifier_call	= fault_catcher_notify,
	.next		= NULL,
};

void __init fault_cacher_start(void)
{
	fault_catcher_notify(&fault_catcher_nb, (unsigned long)MA_LWP_ONLINE,
			     (void *)(ma_lwp_t)LWP_SELF);
	ma_register_lwp_notifier(&fault_catcher_nb);
}

__ma_initfunc_prio(fault_cacher_start, MA_INIT_FAULT_CATCHER,
		   MA_INIT_FAULT_CATCHER_PRIO, "Start SEGV catcher");

/****************************************************************
 * Le signal TIMER
 */
#ifdef MA__TIMER

// Signal utilisé pour la préemption automatique
#ifdef USE_VIRTUAL_TIMER
#  define MARCEL_TIMER_SIGNAL   SIGVTALRM
#  define MARCEL_ITIMER_TYPE    ITIMER_VIRTUAL
#else
#  define MARCEL_TIMER_SIGNAL   SIGALRM
#  define MARCEL_ITIMER_TYPE    ITIMER_REAL
#endif

static boolean __marcel_sig_initialized = FALSE;

static sigset_t sigalrmset, sigeptset;


// Fonction appelée à chaque fois que SIGALRM est délivré au LWP
// courant
static void timer_interrupt(int sig)
{
	ma_irq_enter();
	ma_raise_softirq_from_hardirq(MA_TIMER_HARDIRQ);
#ifdef MA__SMP
	marcel_kthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#else
#if defined(SOLARIS_SYS) || defined(UNICOS_SYS)
	sigrelse(MARCEL_TIMER_SIGNAL);
#else
	sigprocmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif
#endif
	/* ma_irq_exit risque de déclancher un changement de contexte,
	 * il faut réarmer les signaux AVANT
	 */
	ma_irq_exit();
}


void marcel_sig_pause(void)
{
#ifndef USE_VIRTUAL_TIMER
	sigsuspend(&sigeptset);
#else
	SCHED_YIELD();
#endif
}

static void sig_reset_timer(void)
{
	struct itimerval value;

	LOG_IN();

	marcel_sig_enable_interrupts();

	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = time_slice;
	value.it_value = value.it_interval;
	setitimer(MARCEL_ITIMER_TYPE, &value, 
		  (struct itimerval *)NULL);
	
	LOG_OUT();
}

void marcel_settimeslice(unsigned long microsecs)
{
	LOG_IN();

	ma_enter_lib();
	if (microsecs && (microsecs < MIN_TIME_SLICE)) {
		time_slice = MIN_TIME_SLICE;
	} else {
		time_slice = microsecs;
	}
	sig_reset_timer();
	ma_exit_lib();
	
	LOG_OUT();
}

/* Retour en microsecondes */
unsigned long marcel_gettimeslice(void)
{
	LOG_IN();

	LOG_OUT();
	return time_slice;
}


void marcel_sig_enable_interrupts(void)
{
#ifdef MA__SMP
	marcel_kthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#else
	sigprocmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif
}

void marcel_sig_disable_interrupts(void)
{
#ifdef MA__SMP
	marcel_kthread_sigmask(SIG_BLOCK, &sigalrmset, NULL);
#else
	sigprocmask(SIG_BLOCK, &sigalrmset, NULL);
#endif
}


static void sig_start_timer(void)
{
	static struct sigaction sa;

	LOG_IN();

	fault_catcher_init();
	
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = timer_interrupt;
#if !defined(WIN_SYS)
	sa.sa_flags = SA_RESTART;
#if defined(OSF_SYS) && defined(ALPHA_ARCH)
	sa.sa_flags |= SA_SIGINFO;
#endif
#else
	sa.sa_flags = 0;
#endif
	
	sigaction(MARCEL_TIMER_SIGNAL, &sa, (struct sigaction *)NULL);

	sig_reset_timer();
	
	LOG_OUT();
}

static void sig_stop_timer(void)
{
	struct sigaction sa;
	struct itimerval value;

	LOG_IN();

	fault_catcher_exit();

	memset(&value,0,sizeof(value));
	setitimer(MARCEL_ITIMER_TYPE, &value,
		  (struct itimerval *)NULL);

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_DFL;
	sa.sa_flags = 0;

	sigaction(MARCEL_TIMER_SIGNAL, &sa, (struct sigaction *)NULL);

	LOG_OUT();
}

void marcel_sig_exit(void)
{
	return sig_stop_timer();
}

static int sig_lwp_notify(struct ma_notifier_block *self, 
		     unsigned long action, void *hlwp)
{
	switch(action) {
	case MA_LWP_ONLINE:
		sig_start_timer();
		break;
	case MA_LWP_OFFLINE:
		sig_stop_timer();
		break;
	default:
		break;
	}
	return 0;
}

static struct ma_notifier_block sig_nb = {
	.notifier_call	= sig_lwp_notify,
	.next		= NULL,
};

void __init sig_start(void)
{
	sig_lwp_notify(&sig_nb, (unsigned long)MA_LWP_ONLINE,
			 (void *)(ma_lwp_t)LWP_SELF);
	ma_register_lwp_notifier(&sig_nb);
}

__ma_initfunc(sig_start, MA_INIT_TIMER_SIG, "Start signal timer");


void __init sig_init(void)
{
	sigemptyset(&sigeptset);
	sigemptyset(&sigalrmset);
	sigaddset(&sigalrmset, MARCEL_TIMER_SIGNAL);
	
	__marcel_sig_initialized = TRUE;
}
__ma_initfunc(sig_init, MA_INIT_TIMER_SIG_DATA, "Signal static data");

#endif /* MA__TIMER */
