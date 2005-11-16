
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

/* pour déboggage seulement */
//#define MA_DO_NOT_LAUNCH_SIGNAL_TIMER

#ifdef MA_DO_NOT_LAUNCH_SIGNAL_TIMER
#ifdef PM2_DEV
#warning NO SIGNAL TIMER ENABLE
#warning I hope you are debugging
#endif
#endif

ma_atomic_t __preemption_disabled = MA_ATOMIC_INIT(0);

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
		mdebugl(7,"\t\t\t<<tick>>\n");
		tick = 0;
	}
#endif

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
	ma_update_process_times(1);
		
	LOG_OUT();
}

static void __marcel_init timer_start(void)
{
	ma_open_softirq(MA_TIMER_HARDIRQ, timer_action, NULL);
}

__ma_initfunc(timer_start, MA_INIT_TIMER, "Install TIMER SoftIRQ");


/****************************************************************
 * Trappeur de SIGSEGV, de manière à obtenir des indications sur le
 * thread fautif (ça aide pour le debug !)
 */
#ifdef SA_SIGINFO
#include <ucontext.h>
static void TBX_NORETURN fault_catcher(int sig, siginfo_t *act, void *data)
#else
static void TBX_NORETURN fault_catcher(int sig)
#endif
{
#ifdef SA_SIGINFO
	ucontext_t *ctx =(ucontext_t *)data;
	pm2debug("OOPS!!! Signal %d catched on thread %p (%d)\n"
			"si_code=%x, si_signo=%x, si_addr=%p, ctx=%p\n",
		sig, MARCEL_SELF, THREAD_GETMEM(MARCEL_SELF,number),
		act->si_code, act->si_signo, act->si_addr, ctx);
#endif
	if(LWP_SELF != NULL)
		pm2debug("OOPS!!! current lwp is %d\n",
			LWP_NUMBER(LWP_SELF));
	
#if defined(LINUX_SYS) && defined(MA__LWPS)
	fprintf(stderr,
		"OOPS!!! Entering endless loop "
		"so you can try to attach process to gdb (pid = %d)\n",
		marcel_gettid());
	for(;;) ;
#endif

	abort();
}

static void fault_catcher_init(ma_lwp_t lwp)
{
	static struct sigaction sa;
	/* obligé de le faire sur chaque lwp pour les noyaux linux <= 2.4 */

	LOG_IN();
	// On va essayer de rattraper SIGSEGV, etc.
	sigemptyset(&sa.sa_mask);
#ifdef SA_SIGINFO
	sa.sa_sigaction = fault_catcher;
	sa.sa_flags = SA_SIGINFO;
#else
	sa.sa_handler = fault_catcher;
#endif
	sigaction(SIGSEGV, &sa, (struct sigaction *)NULL);
	LOG_OUT();
}

static void fault_catcher_exit(ma_lwp_t lwp)
{
	struct sigaction sa;

	LOG_IN();
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_DFL;
	sa.sa_flags = 0;
	sigaction(SIGSEGV, &sa, (struct sigaction *)NULL);
	LOG_OUT();
}

MA_DEFINE_LWP_NOTIFIER_ONOFF(fault_catcher, "Fault catcher",
			     fault_catcher_init, "Start fault catcher",
			     fault_catcher_exit, "Stop fault catcher");

//MA_LWP_NOTIFIER_CALL_ONLINE_PRIO(fault_catcher, MA_INIT_FAULT_CATCHER,
//				 MA_INIT_FAULT_CATCHER_PRIO);

#ifdef PROFILE
static void int_catcher_exit(ma_lwp_t lwp)
{
	LOG_IN();
	signal(SIGINT, SIG_DFL);
	LOG_OUT();
}
static void int_catcher(int signo)
{
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

MA_LWP_NOTIFIER_CALL_ONLINE_PRIO(int_catcher, MA_INIT_INT_CATCHER,
				 MA_INIT_INT_CATCHER_PRIO);

#endif

/****************************************************************
 * Le signal TIMER
 *
 * dans la norme posix, setitimer(ITIMER_REAL) envoie régulièrement un SIGALRM
 * au _processus_ (donc à un seul des threads), il est partagé entre tous les
 * threads; par contre setitimer(ITIMER_VIRTUAL) est "par thread".
 *
 * Il se trouve que linux <= 2.4 voire un peu plus, solaris <= je ne sais
 * pas,... ont un ITIMER_REAL "par thread" !
 *
 * La manière sûre d'utiliser ITIMER_REAL est donc de l'appeler seulement pour
 * le LWP0 et bloquer SIGALRM sur les autres threads, chaque LWP s'occupant de
 * propager le signal au suivant.
 *
 * Si l'utilisateur sait avoir une ancienne sémantique, il peut activer
 * l'option old real timer.
 *
 * On préfèrerait donc ITIMER_VIRTUAL, où c'est le noyau qui s'occupe de
 * distribuer (bien, a priori). Seul problème, c'est qu'il n'expire que si on
 * consomme du cpu...
 */
#if defined(MA__SMP) && !defined(USE_VIRTUAL_TIMER) && !defined(OLD_ITIMER_REAL)
#define CHAINED_SIGALRM
#endif

#ifdef MA__TIMER

static sigset_t sigalrmset, sigeptset;

#ifdef CHAINED_SIGALRM
static MA_DEFINE_PER_LWP(int, _no_interrupt, 0);
#define no_interrupt __ma_get_lwp_var(_no_interrupt)
#else
#define no_interrupt 0
#endif

// Fonction appelée à chaque fois que SIGALRM est délivré au LWP
// courant
static void timer_interrupt(int sig, siginfo_t *info, void *uc)
{
#ifdef MA__DEBUG
	static unsigned long tick = 0;
#endif

	MA_ARCH_INTERRUPT_ENTER_LWP_FIX(MARCEL_SELF, uc);

#ifdef CHAINED_SIGALRM
	if (info->si_code > 0) {
		/* kernel timer signal, distribute */
		ma_lwp_t lwp;
		for_each_lwp_from_begin(lwp,LWP_SELF)
			marcel_kthread_kill(lwp->pid, MARCEL_TIMER_SIGNAL);
		for_each_lwp_from_end()
	}
#endif
	if (no_interrupt)
		goto out;

#ifdef MA__DEBUG
	if (++tick == TICK_RATE) {
		mdebugl(7,"\t\t\t<<Sig handler>>\n");
		tick = 0;
	}
#endif
	ma_irq_enter();
#ifndef MA_HAVE_COMPAREEXCHANGE
	// Avoid raising softirq if compareexchange is not implemented and
	// a compare & exchange is currently running...
	if (!ma_spin_is_locked(&ma_compareexchange_spinlock))
#endif
		ma_raise_softirq_from_hardirq(MA_TIMER_HARDIRQ);
	if (info->si_code > 0)
		/* kernel timer signal */
#ifndef CHAINED_SIGALRM
		if (IS_FIRST_LWP(LWP_SELF))
#endif
		{
			ma_jiffies+=MA_JIFFIES_PER_TIMER_TICK;
			__milliseconds += time_slice/1000;
		}
#ifdef MA__SMP
	//SA_NOMASK est mis dans l'appel à sigaction
	//marcel_kthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#else
#if defined(SOLARIS_SYS) || defined(UNICOS_SYS)
	sigrelse(MARCEL_TIMER_SIGNAL);
#else
	//SA_NOMASK est mis dans l'appel à sigaction
	//sigprocmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif
#endif
	/* ma_irq_exit risque de déclancher un changement de contexte,
	 * il faut réarmer les signaux AVANT
	 */
	ma_irq_exit();
out:
	MA_ARCH_INTERRUPT_EXIT_LWP_FIX(MARCEL_SELF, uc);
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

#ifndef MA_DO_NOT_LAUNCH_SIGNAL_TIMER
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = time_slice;
	value.it_value = value.it_interval;
	setitimer(MARCEL_ITIMER_TYPE, &value, 
		  (struct itimerval *)NULL);
#endif
	
	LOG_OUT();
}

void marcel_settimeslice(unsigned long microsecs)
{
	LOG_IN();

	if (microsecs && (microsecs < MIN_TIME_SLICE)) {
		time_slice = MIN_TIME_SLICE;
	} else {
		time_slice = microsecs;
	}
	marcel_extlib_protect();
	sig_reset_timer();
	marcel_extlib_unprotect();
	
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
#ifdef CHAINED_SIGALRM
	no_interrupt = 0;
#else
	marcel_kthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif
#else
	sigprocmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif
}

void marcel_sig_disable_interrupts(void)
{
#ifdef MA__SMP
#ifdef CHAINED_SIGALRM
	no_interrupt = 1;
#else
	marcel_kthread_sigmask(SIG_BLOCK, &sigalrmset, NULL);
#endif
#else
	sigprocmask(SIG_BLOCK, &sigalrmset, NULL);
#endif
}


static void sig_start_timer(ma_lwp_t lwp)
{
	struct sigaction sa;

	LOG_IN();

#ifndef MA_DO_NOT_LAUNCH_SIGNAL_TIMER
	sigemptyset(&sa.sa_mask);
#if !defined(WIN_SYS)
	sa.sa_flags = SA_RESTART;
#if defined(SA_NOMASK)
	sa.sa_flags |= SA_NOMASK;
#elif defined(SA_NODEFER)
	sa.sa_flags |= SA_NODEFER;
#else
	// TODO: oui, c'est bien "allow" qu'indique NOMASK/NODEFER.
	// Préfère-t-on dépenser deux appels systèmes par préemption que de
	// risquer une récursion des signaux ?
#warning no way to allow nested signals
#endif

#if defined(OSF_SYS) && defined(ALPHA_ARCH)
	sa.sa_flags |= SA_SIGINFO;
#endif
#else
	sa.sa_flags = 0;
#endif
	sa.sa_flags |= SA_SIGINFO;
	sa.sa_sigaction = timer_interrupt;
	
	/* obligé de le faire sur chaque lwp pour les noyaux linux <= 2.4 */
	sigaction(MARCEL_TIMER_SIGNAL, &sa, (struct sigaction *)NULL);
#endif

	marcel_sig_enable_interrupts();

#ifdef CHAINED_SIGALRM
	if (IS_FIRST_LWP(lwp))
#endif
		sig_reset_timer();
	
	LOG_OUT();
}

static void sig_stop_itimer(void)
{
	struct itimerval value;
	memset(&value,0,sizeof(value));
	setitimer(MARCEL_ITIMER_TYPE, &value, (struct itimerval *)NULL);
}

static void sig_stop_timer(ma_lwp_t lwp)
{
	LOG_IN();

#ifndef MA_DO_NOT_LAUNCH_SIGNAL_TIMER
	sig_stop_itimer();

	/* à part avec linux <= 2.4, les traitants de signaux ne sont _pas_
	 * par thread, il faut donc garder le traitant commun jusqu'au bout, en
	 * désactivant simplement l'interruption.
	 *
	 * Et puis on en a besoin dans le cas CHAINED_SIGALRM. */
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;

	sigaction(MARCEL_TIMER_SIGNAL, &sa, (struct sigaction *)NULL);
#endif

	LOG_OUT();
}

void marcel_sig_exit(void)
{
	fault_catcher_exit(LWP_SELF);
	sig_stop_timer(LWP_SELF);
	return;
}

MA_DEFINE_LWP_NOTIFIER_ONOFF(sig_timer, "Signal timer",
			     sig_start_timer, "Start signal timer",
			     sig_stop_timer, "Stop signal timer");

MA_LWP_NOTIFIER_CALL_ONLINE(sig_timer, MA_INIT_TIMER_SIG);

static void __marcel_init sig_init(void)
{
	sigemptyset(&sigeptset);
	sigemptyset(&sigalrmset);
	sigaddset(&sigalrmset, MARCEL_TIMER_SIGNAL);
}
__ma_initfunc(sig_init, MA_INIT_TIMER_SIG_DATA, "Signal static data");

#endif /* MA__TIMER */
