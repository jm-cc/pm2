
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

#ifdef MA__LIBPTHREAD
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
static sighandler_t TBX_UNUSED signal(int sig, sighandler_t handler)
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

/* Unit� : microsecondes */
static volatile unsigned long time_slice = MARCEL_DEFAULT_TIME_SLICE;

// Fr�quence des messages de debug dans timer_interrupt
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

// Softirq appell�e par le timer
static void timer_action(struct ma_softirq_action *a)
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
	if(LWP_SELF == NULL) {
		pm2debug("WARNING!!! LWP_SELF == NULL in thread %p!\n",
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
/****************************************************************
 * Trappeur de SIGSEGV, de mani�re � obtenir des indications sur le
 * thread fautif (�a aide pour le debug !)
 */
#ifdef SA_SIGINFO
#ifndef WIN_SYS
#include <ucontext.h>
#endif
static void TBX_NORETURN fault_catcher(int sig, siginfo_t *act, void *data)
#else
static void TBX_NORETURN fault_catcher(int sig)
#endif
{
#ifdef SA_SIGINFO
#ifndef WIN_SYS
	TBX_UNUSED ucontext_t *ctx =(ucontext_t *)data;
#endif
	pm2debug("OOPS!!! Signal %d catched on thread %p (%d)\n"
			"si_code=%x, si_signo=%x, si_addr=%p, ctx=%p\n"
			,
		sig, MARCEL_SELF, THREAD_GETMEM(MARCEL_SELF,number),
		act->si_code, act->si_signo, act->si_addr, data);
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
	struct sigaction sa;
	/* oblig� de le faire sur chaque lwp pour les noyaux linux <= 2.4 */

	LOG_IN();
	// On va essayer de rattraper SIGSEGV, etc.
	sigemptyset(&sa.sa_mask);
#ifdef SA_SIGINFO
	sa.sa_sigaction = fault_catcher;
	sa.sa_flags = SA_SIGINFO;
#else
	sa.sa_handler = fault_catcher;
	sa.sa_flags = 0;
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

//MA_LWP_NOTIFIER_CALL_ONLINE_PRIO(fault_catcher, MA_INIT_FAULT_CATCHER, MA_INIT_FAULT_CATCHER_PRIO);

#ifdef PROFILE
static void int_catcher_exit(ma_lwp_t lwp)
{
	LOG_IN();
	signal(SIGINT, SIG_DFL);
	LOG_OUT();
}
static void int_catcher(int signo)
{
	fprintf(stderr,"SIGINT caught, saving profile\n");
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

#endif
#endif

/****************************************************************
 * Le signal TIMER
 *
 * dans la norme posix, setitimer(ITIMER_REAL) envoie r�guli�rement un SIGALRM
 * au _processus_ (donc � un seul des threads), ce thread renvoie donc SIGALRM
 * aux autres threads.
 *
 * par contre setitimer(ITIMER_VIRTUAL) est "par thread".
 *
 * Il se trouve que linux <= 2.4 voire un peu plus, solaris <= je ne sais
 * pas,... ont un ITIMER_REAL "par thread" !
 *
 * On pr�f�rerait donc ITIMER_VIRTUAL, o� c'est le noyau qui s'occupe de
 * distribuer (bien, a priori). Seul probl�me, c'est qu'il n'expire que si on
 * consomme du cpu...
 */
#ifndef __MINGW32__

#if defined(MA__LWPS) && defined(MA__TIMER) && !defined(USE_VIRTUAL_TIMER) && !defined(OLD_ITIMER_REAL)
#define DISTRIBUTE_SIGALRM
#endif

static sigset_t sigalrmset, sigeptset;

// Fonction appel�e � chaque fois que SIGALRM est d�livr� au LWP
// courant
#ifdef SA_SIGINFO
static void timer_interrupt(int sig, siginfo_t *info, void *uc)
#else
static void timer_interrupt(int sig)
#endif
{
#ifdef MA__TIMER
#ifdef MA__DEBUG
	static unsigned long tick = 0;
#endif
#endif

	MA_ARCH_INTERRUPT_ENTER_LWP_FIX(MARCEL_SELF, uc);

	/* check that stack isn't overflowing */
	MA_BUG_ON(get_sp() < (unsigned long) marcel_stackbase(MARCEL_SELF) + (THREAD_SLOT_SIZE / 0x10));

#ifdef DISTRIBUTE_SIGALRM
#if !defined(MA_BOGUS_SIGINFO_CODE)
	if (!info || info->si_code > 0))
#elif MARCEL_TIMER_SIGNAL == MARCEL_TIMER_USERSIGNAL
#error "Can't distinguish between kernel and user signal"
#else
	if (sig == MARCEL_TIMER_SIGNAL)
#endif
	{
		/* kernel timer signal, distribute */
		ma_lwp_t lwp;
		for_each_lwp_from_begin(lwp,LWP_SELF)
			if (lwp->number != -1 && lwp->number < marcel_nbvps())
				marcel_kthread_kill(lwp->pid, MARCEL_TIMER_USERSIGNAL);
		for_each_lwp_from_end()
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
	ma_irq_enter();
#ifdef MA__TIMER
	if (
#ifdef MA__LWPS
	    LWP_SELF->number !=-1 && LWP_SELF->number < marcel_nbvps() &&
#endif
	    (sig == MARCEL_TIMER_SIGNAL || sig == MARCEL_TIMER_USERSIGNAL)) {
#ifndef MA_HAVE_COMPAREEXCHANGE
	// Avoid raising softirq if compareexchange is not implemented and
	// a compare & exchange is currently running...
		if (!ma_spin_is_locked_nofail(&ma_compareexchange_spinlock)) {
#endif
			ma_raise_softirq_from_hardirq(MA_TIMER_HARDIRQ);
#ifndef MA_HAVE_COMPAREEXCHANGE
		}
#endif
#if defined(MA__LWPS)
#if !defined(MA_BOGUS_SIGINFO_CODE)
		if (!info || info->si_code > 0)
#elif MARCEL_TIMER_SIGNAL == MARCEL_TIMER_USERSIGNAL
		if (IS_FIRST_LWP(LWP_SELF))
#else
		if (sig == MARCEL_TIMER_SIGNAL)
#endif
#endif
		/* kernel timer signal */
		{
			ma_jiffies+=MA_JIFFIES_PER_TIMER_TICK;
			__milliseconds += time_slice/1000;
		}
	}
#endif
#ifdef MA_TIMER_NOMASK
	ma_irq_exit();
	ma_preempt_check_resched(0);
#else
	ma_preempt_check_resched(1);
	ma_irq_exit();
#endif

	MA_ARCH_INTERRUPT_EXIT_LWP_FIX(MARCEL_SELF, uc);
}
#endif

#undef marcel_sig_pause
void marcel_sig_pause(void)
{
#if defined(MA__TIMER) && !defined(USE_VIRTUAL_TIMER)
	sigsuspend(&sigeptset);
#else
	SCHED_YIELD();
#endif
}

void marcel_sig_nanosleep(void)
{
#if defined(MA__TIMER) && !defined(USE_VIRTUAL_TIMER)
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

void marcel_sig_reset_timer(void)
{
#ifdef MA__TIMER
	struct itimerval value;
#endif

	LOG_IN();

#ifdef MA__TIMER
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = time_slice;
	value.it_value = value.it_interval;
	if (setitimer(MARCEL_ITIMER_TYPE, &value, 
		  (struct itimerval *)NULL)) {
		perror("can't start itimer");
		exit(1);
	}
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
/* Retour en microsecondes */
unsigned long marcel_gettimeslice(void)
{
	LOG_IN();

	LOG_OUT();
	return time_slice;
}
#endif


#ifndef __MINGW32__
#undef marcel_sig_enable_interrupts
void marcel_sig_enable_interrupts(void)
{
#ifdef MA__SMP
	marcel_kthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#else
#if defined(SOLARIS_SYS) || defined(UNICOS_SYS)
	sigrelse(sig);
#else
	sigprocmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif
#endif
}

#undef marcel_sig_disable_interrupts
void marcel_sig_disable_interrupts(void)
{
#ifdef MA__SMP
	marcel_kthread_sigmask(SIG_BLOCK, &sigalrmset, NULL);
#else
#if defined(SOLARIS_SYS) || defined(UNICOS_SYS)
	sighold(sig);
#else
	sigprocmask(SIG_BLOCK, &sigalrmset, NULL);
#endif
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

#ifdef MA_TIMER_NOMASK
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
	/* oblig� de le faire sur chaque lwp pour les noyaux linux <= 2.4 */
	sigaction(MARCEL_TIMER_SIGNAL, &sa, (struct sigaction *)NULL);
#if MARCEL_TIMER_USERSIGNAL != MARCEL_TIMER_SIGNAL
	sigaction(MARCEL_TIMER_USERSIGNAL, &sa, (struct sigaction *)NULL);
#endif
#endif
	sigaction(MARCEL_RESCHED_SIGNAL, &sa, (struct sigaction *)NULL);

#ifdef MA__SMP
#ifdef DISTRIBUTE_SIGALRM
	marcel_kthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif
#endif
	marcel_sig_enable_interrupts();

#ifdef DISTRIBUTE_SIGALRM
	if (IS_FIRST_LWP(lwp))
#endif
		marcel_sig_reset_timer();
	
	LOG_OUT();
}

#undef marcel_sig_stop_itimer
void marcel_sig_stop_itimer(void)
{
#ifdef MA__TIMER
	struct itimerval value;
	memset(&value,0,sizeof(value));
	setitimer(MARCEL_ITIMER_TYPE, &value, (struct itimerval *)NULL);
#endif
}

static void sig_stop_timer(ma_lwp_t lwp)
{
	struct sigaction sa;
	LOG_IN();

#ifndef DISTRIBUTE_SIGALRM
	marcel_sig_stop_itimer();
#endif

	/* � part avec linux <= 2.4, les traitants de signaux ne sont _pas_
	 * par thread, il faut donc garder le traitant commun jusqu'au bout, en
	 * d�sactivant simplement l'interruption.
	 *
	 * Et puis on en a besoin dans le cas DISTRIBUTE_SIGALRM. */
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
	fault_catcher_exit(LWP_SELF);
	sig_stop_timer(LWP_SELF);
	return;
}

// TODO: sur un vrai syst�me POSIX, les traitants de signaux sont par processus.
// Il n'est donc pas utile de les red�finir pour chaque LWP... Sauf pour linux
// 2.4 (hem...)
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
#ifdef MA__SMP
	/* bloquer les signaux avant de lancer les lwps, pour que les lwps
	 * lanc�s par la librairie de threads n'en recoivent pas */
	sigprocmask(SIG_BLOCK, &sigalrmset, NULL);
	marcel_kthread_atfork(NULL,NULL,marcel_sig_exit);
#endif
}
__ma_initfunc(sig_init, MA_INIT_TIMER_SIG_DATA, "Signal static data");
#endif
