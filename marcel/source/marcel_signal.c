/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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
#include "marcel_pmarcel.h"
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>


#ifdef MARCEL_SIGNALS_ENABLED


/*
 * doc/dev/posix/06-11-07-Jeuland/doc_marcel_posix.pdf provides (french) details
 * about Marcel signal management.
 */

/*
 * Signals come from several sources and to different targets.
 *
 * - Kernel or other processes to our process.  marcel_pidkill() handles the
 *   signal by just setting ksiginfo and raising the MA_SIGNAL_SOFTIRQ.  The
 *   softirq handler can then handle it less unexpectedly later: choose a thread
 *   that doesn't block it (marcel_sigtransfer), and deviate it into running
 *   marcel_deliver_sig.
 * - kernel to an LWP (SIGSEGV for instance).  This actually is directed to the
 *   marcel thread currently running on that LWP.  We can just synchronously
 *   call the application's handler.
 * - Marcel threads to marcel threads.  We can handle them internally, except
 *   when the handler is SIG_DFL, in which case we raise it to the kernel so
 *   that the default action is actually performed (e.g. crash the process)
 * - Marcel threads to other processes.  We just let the real libc function do
 *   it.
 * - Marcel threads to our process.  We also just let the glibc function do it.
 *   That goes through the kernel and back to us, which is fine and actually
 *   simpler.
 * - Marcel timers to our process.  We could call kill() ourselves, but we
 *   simulate a reception from the kernel.
 *
 * Letting the application block a signal with the default handler is
 * difficult: the threads that block it shouldn't get disturbed if they receive
 * it.  The threads that do not block it _should_ get disturbed (e.g. make the
 * processus crash).  We want to avoid changing the kernel signal mask on each
 * context switch, so we only do so for signals that have the SIG_DFL handler.
 * When sigaction is called, that means we have to notify other LWPs that the
 * signal mask should be changed and wait for them to actually do it.
 *
 * Non-user threads (not_counted_in_running) are a bit particular. They don't
 * participate to signal stuff and thus we don't use their curmask. That said,
 * when they are running on a LWP they should still let the LWP unblock the
 * signals with DFL handler which are unblocked by at least one user thread. So
 * we keep up to date the intersection of all user sigmasks in ma_thr_sigmask,
 * which is taken as the sigmask for non-user threads.
 */

/*********************variables globales*********************/
static struct marcel_sigaction csigaction[MARCEL_NSIG];
/* Protects csigaction updates.  */
static ma_rwlock_t csig_lock = MA_RW_LOCK_UNLOCKED;
/* sigpending processus */
static int ksigpending[MARCEL_NSIG];
static siginfo_t ksiginfo[MARCEL_NSIG];
static marcel_sigset_t gsigpending;
static siginfo_t gsiginfo[MARCEL_NSIG];
static ma_spinlock_t gsiglock = MA_SPIN_LOCK_UNLOCKED;
static ma_twblocked_t *list_wthreads;

/* Set of signals which have the DFL sigaction */
static marcel_sigset_t ma_dfl_sigs;
static sigset_t ma_dfl_ksigs;

/* Sigmask of non-user threads */
static marcel_sigset_t ma_thr_sigmask;
TBX_VISIBILITY_PUSH_INTERNAL
sigset_t ma_thr_ksigmask;
TBX_VISIBILITY_PUSH_DEFAULT

/*********************__ma_restore_rt************************/
#ifdef MA__LIBPTHREAD
#ifdef X86_64_ARCH
#define RESTORE(name, syscall) RESTORE2(name, syscall)
#define RESTORE2(name, syscall) asm(			\
	".text\n"					\
	".align 16\n"					\
	".global __ma_"#name"\n"			\
	".internal __ma_"#name"\n"			\
	".type __ma_"#name",@function\n"		\
	"__ma_"#name":\n"				\
	"	movq $" #syscall ", %rax\n"		\
	"	syscall\n"				\
		)

RESTORE(restore_rt,SYS_rt_sigreturn);
#endif
#endif
/*********************pause**********************************/
DEF_MARCEL_PMARCEL(int,pause,(void),(),
{
	MARCEL_LOG_IN();
	marcel_sigset_t set;
	marcel_sigemptyset(&set);
	marcel_sigmask(SIG_BLOCK, NULL, &set);
	MARCEL_LOG_RETURN(marcel_sigsuspend(&set));
})

DEF_C(int,pause,(void),())
DEF___C(int,pause,(void),())

/*********************alarm**********************************/


/* marcel timer handler that generates the SIGALARM signal for alarm( */
static void alarm_timeout(unsigned long data TBX_UNUSED)
{
	siginfo_t info;
	info.si_signo = SIGALRM;
	info.si_code = 0;
	/* alarme expirée, signaler le programme */
	if (!marcel_distribwait_sigext(&info)) {
		ksigpending[SIGALRM] = 1;
#ifdef SA_SIGINFO
		ksiginfo[SIGALRM] = info;
#endif
		ma_raise_softirq(MA_SIGNAL_SOFTIRQ);
	}
}

static struct marcel_timer_list alarm_timer = MA_TIMER_INITIALIZER(alarm_timeout, 0, 0);

DEF_MARCEL_PMARCEL(unsigned int, alarm, (unsigned int nb_sec), (nb_sec),
{
	MARCEL_LOG_IN();
	unsigned int ret = 0;
	ma_del_timer_sync(&alarm_timer);
	
	if (alarm_timer.expires > ma_jiffies) {
		MARCEL_LOG_OUT();
		ret = ((alarm_timer.expires - ma_jiffies) * marcel_gettimeslice()) / 1000000;
	}
	
	if (nb_sec) {
		ma_init_timer(&alarm_timer);
		alarm_timer.expires = ma_jiffies + ma_jiffies_from_us((nb_sec * 1000000)) + 1;
		ma_add_timer(&alarm_timer);
	}
	MARCEL_LOG_RETURN(ret);
})

#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, pmarcel_alarm, alarm, GLIBC_2_0);
#endif
DEF___C(unsigned int, alarm, (unsigned int nb_sec), (nb_sec))


/***********************get/setitimer**********************/
static void itimer_timeout(unsigned long data);

static struct marcel_timer_list itimer_timer = MA_TIMER_INITIALIZER(itimer_timeout, 0, 0);
static unsigned long interval;
static unsigned long valeur;

/* marcel timer handler that generates the SIGALARM signal for setitimer() */
static void itimer_timeout(unsigned long data TBX_UNUSED)
{
	siginfo_t info;
	info.si_signo = SIGALRM;
	info.si_code = 0;

	/* timer expired, send a signal to the application */
	if (!marcel_distribwait_sigext(&info)) {
		ksigpending[SIGALRM] = 1;
#ifdef SA_SIGINFO
		ksiginfo[SIGALRM] = info;
#endif
		ma_raise_softirq(MA_SIGNAL_SOFTIRQ);
	}
	if (interval) {
		ma_init_timer(&itimer_timer);
		itimer_timer.expires = ma_jiffies + ma_jiffies_from_us((interval)) + 1;
		ma_add_timer(&itimer_timer);
	}
}

/* Common functions to check given values */
static int check_itimer(int which)
{
	if ((which != ITIMER_REAL) && (which != ITIMER_VIRTUAL) && (which != ITIMER_PROF)) {
		MARCEL_LOG("check_itimer : invalid which value\n");
		errno = EINVAL;
		return -1;
	}
	if (which != ITIMER_REAL) {
		MARCEL_LOG("set/getitimer : value which not supported !\n");
		errno = ENOTSUP;
		return -1;
	}
	return 0;
}

static int check_setitimer(int which, const struct itimerval *value)
{
	int ret = check_itimer(which);
	if (ret)
		return ret;
	if (value) {
		if ((value->it_value.tv_usec > 999999) || (value->it_interval.tv_usec > 999999)) {
			MARCEL_LOG("check_setitimer: value is too high !\n");
			errno = EINVAL;
			return -1;
		}
	}
	return 0;
}

/***********************getitimer**********************/
DEF_MARCEL_PMARCEL(int, getitimer, (int which TBX_UNUSED, struct itimerval *value), (which, value),
{
	MARCEL_LOG_IN();
	int err = check_itimer(which);
	if (err)
		MARCEL_LOG_RETURN(err);

	value->it_interval.tv_sec = interval / 1000000;
	value->it_interval.tv_usec = interval % 1000000;
	value->it_value.tv_sec = (itimer_timer.expires - ma_jiffies) / 1000000;
	value->it_value.tv_usec = (itimer_timer.expires - ma_jiffies) % 1000000;
	MARCEL_LOG_RETURN(0);
})

DEF_C(int, getitimer, (int which, struct itimerval *value), (which, value))
DEF___C(int, getitimer, (int which, struct itimerval *value), (which, value))

/************************setitimer***********************/
DEF_MARCEL_PMARCEL(int, setitimer, (int which TBX_UNUSED, const struct itimerval *value, struct itimerval *ovalue), (which, value, ovalue),
{
	int ret;

	MARCEL_LOG_IN();
	ret = check_setitimer(which, value);
	if (ret)
		MARCEL_LOG_RETURN(ret);

	ma_del_timer_sync(&itimer_timer);
	
	if (ovalue) {
		ovalue->it_interval.tv_sec = interval / 1000000;
		ovalue->it_interval.tv_usec = interval % 1000000;
		ovalue->it_value.tv_sec = valeur / 1000000;
		ovalue->it_value.tv_usec = valeur % 1000000;
	}

	if (itimer_timer.expires > ma_jiffies)
		ret = ((itimer_timer.expires - ma_jiffies) * marcel_gettimeslice()) / 1000000;
	
	interval = value->it_interval.tv_sec * 1000000 + value->it_interval.tv_usec;
	valeur = value->it_value.tv_sec * 1000000 + value->it_value.tv_usec;

	if (valeur) {
		ma_init_timer(&itimer_timer);
		itimer_timer.expires = ma_jiffies + valeur / marcel_gettimeslice();
		ma_add_timer(&itimer_timer);
	}
	MARCEL_LOG_RETURN(ret);
})
DEF_C(int, setitimer, (int which, const struct itimerval *value, struct itimerval *ovalue), (which, value, ovalue))
DEF___C(int, setitimer, (int which, const struct itimerval *value, struct itimerval *ovalue), (which, value, ovalue))

/*********************marcel_sigtransfer*********************/
/* try to transfer process signals to threads */
static void marcel_sigtransfer(struct ma_softirq_action *action TBX_UNUSED)
{
	marcel_t t;
	int sig;
	int deliver;
	siginfo_t infobis;

	/* Fetch signals from the various handlers */
	ma_spin_lock_softirq(&gsiglock);
	infobis.si_code = 0;
	for (sig = 1; sig < MARCEL_NSIG; sig++) {
		infobis.si_signo = sig;
		if (ksigpending[sig] == 1) {
			if (! ma_distribwait_sigext(&infobis)) {
				marcel_sigaddset(&gsigpending, sig);
#ifdef SA_SIGINFO
				gsiginfo[sig] = ksiginfo[sig];
#endif
			}

			ksigpending[sig] = 0;
		}
	}

	if (marcel_sigisemptyset(&gsigpending)) {
		ma_spin_unlock_softirq(&gsiglock);
		MARCEL_LOG_OUT();
		return;
	}


	int number = ma_vpnum(MA_LWP_SELF);
	struct marcel_topo_level *vp;

	/* Iterate over all threads */
	for_all_vp_from_begin(vp, number) {
		_ma_raw_spin_lock(&ma_topo_vpdata_l(vp, threadlist_lock));
		tbx_fast_list_for_each_entry(t, &ma_topo_vpdata_l(vp, all_threads), all_threads) {
			/**
			 * XXX: UNINTERRUPTIBLE threads will be able to catch signals, but we don't have them anyway
			 * If != BORNING, we are sure it won't become BORNING after the test anyway, and will thus be able to deliver the signal
			 **/
			if (t->state != MA_TASK_BORNING) {
				_ma_raw_spin_lock(&t->siglock);
				deliver = 0;
				/* Iterate over all signals */
				for (sig = 1; sig < MARCEL_NSIG; sig++) {
					if (marcel_sigomnislash(&t->curmask, &gsigpending, &t->sigpending, sig)) {	//&(!1,2,!3)
						/* This thread can take this signal */
						marcel_sigaddset(&t->sigpending, sig);
						marcel_sigdelset(&gsigpending, sig);
#ifdef SA_SIGINFO
						t->siginfo[sig] = gsiginfo[sig];
#endif
						deliver = 1;
					}
				}
				_ma_raw_spin_unlock(&t->siglock);
				if (deliver)
					/* Deliver all signals at once */
					marcel_deviate(t, (marcel_handler_func_t) marcel_deliver_sig, NULL);
				if (marcel_sigisemptyset(&gsigpending))
					break;
			}
		}
		_ma_raw_spin_unlock(&ma_topo_vpdata_l(vp, threadlist_lock)); 	//verrou de liste des threads
		if (marcel_sigisemptyset(&gsigpending))
			break;
	} for_all_vp_from_end(vp, number);

	ma_spin_unlock_softirq(&gsiglock);

	MARCEL_LOG_OUT();
	return;
}

/*****************************raise****************************/
static int check_raise(int sig)
{
	if ((sig < 0) || (sig >= MARCEL_NSIG)) {
		MARCEL_LOG("check_raise : signal %d invalide !\n", sig);
		errno = EINVAL;
		return -1;
	}
	return 0;
}

DEF_MARCEL_PMARCEL(int, raise, (int sig), (sig),
{
	MARCEL_LOG_IN();
	int err = check_raise(sig);
	if (err)
		MARCEL_LOG_RETURN(err);

	marcel_kill(ma_self(), sig);	
	MARCEL_LOG_RETURN(0);
})

DEF_C(int,raise,(int sig),(sig))
DEF___C(int,raise,(int sig),(sig))


/*********************marcel_pidkill*************************/
/* handler of signals coming from the kernel and other processes */
#ifdef SA_SIGINFO
static void marcel_pidkill(int sig, siginfo_t *info, void *uc)
#else
static void marcel_pidkill(int sig)
#endif
{
	if ((sig < 0) || (sig >= MARCEL_NSIG))
		return;

	/* we need to execute signal handler immediately */
	if (sig == SIGABRT || sig == SIGBUS || sig == SIGILL || sig == SIGFPE ||
	    sig == SIGPIPE || sig == SIGSEGV) {
#ifdef SA_SIGINFO
		if (csigaction[sig].marcel_sa_flags & SA_SIGINFO) {
			if (csigaction[sig].marcel_sa_sigaction != (void *) SIG_DFL &&
			    csigaction[sig].marcel_sa_sigaction != (void *) SIG_IGN) {
				MA_SET_INTERRUPTED(1);
				csigaction[sig].marcel_sa_sigaction(sig, info, uc);
				return;
			}
		} else
#endif
			if (csigaction[sig].marcel_sa_handler != SIG_DFL &&
			    csigaction[sig].marcel_sa_handler != SIG_IGN) {
				MA_SET_INTERRUPTED(1);
				csigaction[sig].marcel_sa_handler(sig);
				return;
			}
	}

	ma_irq_enter();

	ksigpending[sig] = 1;
#ifdef SA_SIGINFO
	ksiginfo[sig] = *info;
#endif
	ma_raise_softirq_from_hardirq(MA_SIGNAL_SOFTIRQ);

	ma_irq_exit();
	/* TODO: check preempt */
}

/* We have either changed some sigaction to/from SIG_DFL or switched to another
 * thread or called sigmask.  We may need to update our blocked mask.
 *
 * Note: we do not protect ourselves from a concurrent call to sigaction that
 * would modify ma_dfl_sigs.  That is fine as sigaction will notify us later
 * anyway.  */

#if !HAVE_DECL_SIGANDSET
/* These versions support aliasing between dst and src1 on purpose */
static void ma_ksigandset(sigset_t *dst, const sigset_t *src1, const sigset_t *src2) {
	int sig;
	for (sig = 1; sig < MARCEL_NSIG; sig++) {
		if (sigismember(src1, sig) && sigismember(src2, sig))
			sigaddset(dst, sig);
		else
			sigdelset(dst, sig);
	}
}

static void ma_ksigorset(sigset_t *dst, sigset_t *src1, sigset_t *src2) {
	int sig;
	for (sig = 1; sig < MARCEL_NSIG; sig++) {
		if (sigismember(src1, sig) || sigismember(src2, sig))
			sigaddset(dst, sig);
		else
			sigdelset(dst, sig);
	}
}
#endif /* HAVE_DECL_SIGANDSET */

void ma_update_lwp_blocked_signals(void) {
	marcel_sigset_t should_block;

	if (! MA_LWP_SELF->online)
		return;

	/* Choose the sigmask for the thread */
	if (MA_TASK_NOT_COUNTED_IN_RUNNING(ma_self()))
		/* non-user thread, use general sigmask */
		memcpy(&should_block, &ma_thr_sigmask, sizeof(should_block));
	else
		/* user thread, use its sigmask */
		memcpy(&should_block, &SELF_GETMEM(curmask), sizeof(should_block));

	/* We can always allow all kinds of signals, except those which have
	 * DFL sigaction, for which the kernel behavior could even be dangerous
	 * to us :) */
	marcel_sigandset(&should_block, &should_block, &ma_dfl_sigs);

	if (!marcel_sigequalset(&should_block, &__ma_get_lwp_var(curmask))) {
		/* We have to update the LWP's sigmask, redo the computation for kernel sigmasks */

		sigset_t kshould_block;

		/* _softirq also disables the MA_SIGMASK_SOFTIRQ, to make sure
		 * we do not miss a ma_dfl_ksigs change between our read and
		 * the actual call to kthread_sigmask*/
		ma_spin_lock_softirq(&__ma_get_lwp_var(timer_sigmask_lock));

		if (MA_TASK_NOT_COUNTED_IN_RUNNING(ma_self()))
			memcpy(&kshould_block, &ma_thr_ksigmask, sizeof(kshould_block));
		else
			memcpy(&kshould_block, &SELF_GETMEM(kcurmask), sizeof(kshould_block));

#if HAVE_DECL_SIGANDSET
		sigandset(&kshould_block, &kshould_block, &ma_dfl_ksigs);
		/* We sometimes still need to temporarily block the timer signal */
		sigorset(&kshould_block, &kshould_block, &__ma_get_lwp_var(timer_sigmask));
#else
		ma_ksigandset(&kshould_block, &kshould_block, &ma_dfl_ksigs);
		ma_ksigorset(&kshould_block, &kshould_block, &__ma_get_lwp_var(timer_sigmask));
#endif
		
		marcel_kthread_sigmask(SIG_SETMASK, &kshould_block, NULL);
		__ma_get_lwp_var(curmask) = should_block;
		ma_spin_unlock_softirq(&__ma_get_lwp_var(timer_sigmask_lock));
	}
}

/* update ma_thr_sigmask (use by non applicative threads): call by marcel_sigmask,
 * ma_sigkill (when applicative threads change their sigmask) or marcel_lwp_start */
static tbx_bool_t update_lwps_global_sigmask(marcel_t dying_task)
{
	marcel_t t;
	tbx_bool_t is_sigmask_change;
	struct marcel_topo_level *vp;
	sigset_t updated_ksigmask;
	marcel_sigset_t updated_sigmask;
	int number;

	marcel_sigfillset(&updated_sigmask);
	sigfillset(&updated_ksigmask);
	number = ma_vpnum(MA_LWP_SELF);
	is_sigmask_change = tbx_false;

	/* Iterate over all threads */
	for_all_vp_from_begin(vp, number) {
		ma_spin_lock_softirq(&ma_topo_vpdata_l(vp, threadlist_lock));
		tbx_fast_list_for_each_entry(t, &ma_topo_vpdata_l(vp, all_threads), all_threads) {
			if (! MA_TASK_NOT_COUNTED_IN_RUNNING(t) && dying_task != t) {
				ma_spin_lock_softirq(&t->siglock);
				is_sigmask_change = tbx_true;
				marcel_sigandset(&updated_sigmask, &updated_sigmask, &t->curmask);
#if HAVE_DECL_SIGANDSET
				sigandset(&updated_ksigmask, &updated_ksigmask, &t->kcurmask);
#endif
				ma_spin_unlock_softirq(&t->siglock);
				if (marcel_sigisemptyset(&updated_sigmask))
					break;
			}
		}
		ma_spin_unlock_softirq(&ma_topo_vpdata_l(vp, threadlist_lock));
		if (marcel_sigisemptyset(&updated_sigmask))
			break;
	} for_all_vp_from_end(vp, number);

#if !HAVE_DECL_SIGANDSET
	int sig;
	for (sig = 1; sig < MARCEL_NSIG; sig ++) {
		if (! marcel_sigismember(&updated_sigmask, sig))
			sigdelset(&updated_ksigmask, sig);
	}
#endif

	if (is_sigmask_change && !marcel_sigequalset(&ma_thr_sigmask, &updated_sigmask)) {
		ma_thr_sigmask = updated_sigmask;
		ma_thr_ksigmask = updated_ksigmask;
		return tbx_true;
	}
	return tbx_false;
}

/* Number of LWPs pending an update of blocked signals.  */
static ma_atomic_t nr_pending_blocked_signals_updates = MA_ATOMIC_INIT(0);

static void update_lwp_blocked_signals_softirq(struct ma_softirq_action *action TBX_UNUSED) {
	ma_update_lwp_blocked_signals();
	ma_smp_mb();
	ma_atomic_inc(&nr_pending_blocked_signals_updates);
}

/* We have changed some sigaction to/from SIG_DFL, we may need to make other
 * LWPs update their blocked mask before continuing.
 *
 * when wait is 0, update_lwps_blocked_signals is allowed to return
 * even before LWPs have updated their signal mask. However in the
 * current implementation it was simpler to not do that optimization
 * and just always wait. */
static void update_lwps_blocked_signals(int wait TBX_UNUSED) {
	ma_lwp_t lwp;
	unsigned int signaled_lwp;

	/* First make sure other processors see the new set of signals with
	 * SIG_DFL */
	ma_smp_mb();

	/* Check that we did not miss anybody last time.  */
	while (0 != (unsigned int)ma_atomic_xchg(0, 1, &nr_pending_blocked_signals_updates)) {
		ma_smp_mb();
		ma_cpu_relax();
	}

	signaled_lwp = 0;
	ma_preempt_disable();
	ma_lwp_list_lock_read();
	ma_for_all_lwp(lwp) {
		signaled_lwp ++;
		if (lwp == MA_LWP_SELF)
			ma_update_lwp_blocked_signals();
		else
			ma_raise_softirq_lwp(lwp, MA_SIGMASK_SOFTIRQ);
	}
	ma_lwp_list_unlock_read();
	ma_preempt_enable();

	/* Wait everyone */
	do {
		ma_smp_mb();
		ma_cpu_relax();
	} while (signaled_lwp != 
		 (unsigned int)ma_atomic_xchg(signaled_lwp, 0, 
					      &nr_pending_blocked_signals_updates));
}

/*********************pthread_kill***************************/
static int check_kill(marcel_t thread, int sig)
{
	if (!MARCEL_THREAD_ISALIVE(thread)) {
		MARCEL_LOG("check_kill: thread %p dead\n", thread);
		return ESRCH;
	}
	if ((sig < 0) || (sig >= MARCEL_NSIG)) {
		MARCEL_LOG("check_kill: invalid signal number\n", sig);
		return EINVAL;
	}
	return 0;
}

DEF_MARCEL(int, kill, (marcel_t thread, int sig), (thread,sig),
{
	int err;
	siginfo_t info;

	MARCEL_LOG_IN();

	err = check_kill(thread, sig);
	if (err)
		MARCEL_LOG_RETURN(err);
	
	if (sig) {
		info.si_signo = sig;
		info.si_code = 0;
		if (!marcel_distribwait_thread(&info, thread)) {
			ma_spin_lock_softirq(&thread->siglock);
			marcel_sigaddset(&thread->sigpending, sig);
#ifdef SA_SIGINFO
			thread->siginfo[sig] = info;
#endif
			ma_spin_unlock_softirq(&thread->siglock);
			marcel_deviate(thread, (marcel_handler_func_t) marcel_deliver_sig, NULL);
		}
	}

	MARCEL_LOG_RETURN(0);
})

DEF_PMARCEL(int, kill, (pmarcel_t thread, int sig), (thread,sig),
{
	return marcel_kill((marcel_t)thread, sig);
})
DEF_PTHREAD(int, kill, (pthread_t thread, int sig), (thread,sig))
DEF___PTHREAD(int, kill, (pthread_t thread, int sig), (thread,sig))

/*************************pthread_sigqueue**********************/
#if !HAVE_DECL_PTHREAD_SIGQUEUE
int pthread_sigqueue(pthread_t __threadid, int __signo, const union sigval __value);
#endif
DEF_MARCEL(int, sigqueue, (marcel_t *thread, int sig, const union sigval value), (thread, sig, value),
{
	MARCEL_LOG_IN();

#ifndef SA_SIGINFO
	MARCEL_LOG_RETURN(ENOSYS);
#else
	int err;
	siginfo_t info;

	err = check_kill(*thread, sig);
	if (err)
		MARCEL_LOG_RETURN(err);

	if (! sig)
		MARCEL_LOG_RETURN(0);

	info.si_signo = sig;
	info.si_code = SI_QUEUE;
	info.si_value = value;
	if (!marcel_distribwait_thread(&info, *thread)) {
		ma_spin_lock_softirq(&(*thread)->siglock);
		marcel_sigaddset(&(*thread)->sigpending, sig);
		(*thread)->siginfo[sig] = info;
		ma_spin_unlock_softirq(&(*thread)->siglock);
		marcel_deviate(*thread, (marcel_handler_func_t) marcel_deliver_sig, NULL);

		MARCEL_LOG_RETURN(0);
	}

	MARCEL_LOG_RETURN(EAGAIN);
#endif
})

DEF_PMARCEL(int, sigqueue, (pmarcel_t *thread, int sig, const union sigval value), (thread, sig, value),
{
	return marcel_sigqueue((marcel_t *)thread, sig, value);
})
DEF_PTHREAD(int, sigqueue, (pthread_t *thread, int sig, const union sigval value), (thread, sig, value))

/*************************pthread_sigmask***********************/
static int check_sigmask(int how)
{
	if ((how != SIG_BLOCK) && (how != SIG_UNBLOCK) && (how != SIG_SETMASK)) {
		MARCEL_LOG("check_sigmask : valeur how %d invalide\n", how);
		return EINVAL;
	}
	return 0;
}

static int ma_update_thread_sigmask(int how, __const marcel_sigset_t * set, marcel_sigset_t * oset)
{
	int sig;
	marcel_t cthread = ma_self();
	marcel_sigset_t cset = cthread->curmask;

	if (oset != NULL)
		*oset = cset;

	/* if signal mask is not updated: go out */
	if (!set)
		return 0;

	switch(how) {
	        case SIG_BLOCK:
			marcel_sigorset(&cset, &cset, set);
			break;
	        case SIG_SETMASK:
			cset = *set;
			break;
	        case SIG_UNBLOCK: {
			for (sig = 1; sig < MARCEL_NSIG; sig++)
				if (marcel_sigismember(set, sig))
					marcel_sigdelset(&cset, sig);
			break;
		}
	        default:
			MARCEL_LOG("marcel_sigmask : pas de how\n");
			return 0;
	}

	/* SIGKILL et SIGSTOP must not be masked */
	if (marcel_sigismember(&cset, SIGKILL))
		marcel_sigdelset(&cset, SIGKILL);
	if (marcel_sigismember(&cset, SIGSTOP))
		marcel_sigdelset(&cset, SIGSTOP);

	/* do not update if current mask and new mask are the same */
	if (marcel_sigequalset(&cset, &cthread->curmask))
		return 0;

	cthread->curmask = cset;
	sigemptyset(&cthread->kcurmask);
	for (sig = 1; sig < MARCEL_NSIG; sig++)
		if (marcel_sigismember(&cset, sig))
			sigaddset(&cthread->kcurmask, sig);

	return 1;
}

DEF_MARCEL_PMARCEL(int, sigmask, (int how, __const marcel_sigset_t * set, marcel_sigset_t * oset), (how, set, oset),
{
	int err;
	int sig;
	marcel_t cthread = ma_self();
	marcel_sigset_t cset;

	MARCEL_LOG_IN();
	err = check_sigmask(how);
	if (err)
		MARCEL_LOG_RETURN(err);

	ma_spin_lock_softirq(&cthread->siglock);
	while (!marcel_signandisempty(&cthread->sigpending, &cthread->curmask)) {
		for (sig = 1; sig < MARCEL_NSIG; sig++) {
			if ((marcel_signandismember(&cthread->sigpending, &cthread->curmask, sig))) {
				marcel_sigdelset(&cthread->sigpending, sig);
				ma_spin_unlock_softirq(&cthread->siglock);
				ma_set_current_state(MA_TASK_RUNNING);
				marcel_call_function(sig);
				ma_spin_lock_softirq(&cthread->siglock);
			}
		}
	}

	/********* signal mask update ******/
	if (0 == ma_update_thread_sigmask(how, set, oset)) {
		/* thread signal mask is not new */
		ma_spin_unlock_softirq(&cthread->siglock);
		MARCEL_LOG_RETURN(0);
	}
	cset = cthread->curmask;

	/*************** signal processing ************/
	if (!marcel_signandisempty(&gsigpending, &cset)) {
		ma_spin_unlock_softirq(&cthread->siglock);
		ma_spin_lock_softirq(&gsiglock);
		_ma_raw_spin_lock(&cthread->siglock);
		if (!marcel_signandisempty(&gsigpending, &cset))
			for (sig = 1; sig < MARCEL_NSIG; sig++) {
				if (marcel_signandismember(&gsigpending, &cset, sig)) {
					marcel_sigaddset(&cthread->sigpending, sig);
					marcel_sigdelset(&gsigpending, sig);
				}
			}
		_ma_raw_spin_unlock(&gsiglock);
	}

	while (!marcel_signandisempty(&cthread->sigpending, &cset)) {
		for (sig = 1; sig < MARCEL_NSIG; sig++) {
			if (marcel_signandismember(&cthread->sigpending, &cset, sig)) {
				marcel_sigdelset(&cthread->sigpending, sig);
				ma_spin_unlock_softirq(&cthread->siglock);
				ma_set_current_state(MA_TASK_RUNNING);
				marcel_call_function(sig);
				ma_spin_lock_softirq(&cthread->siglock);
			}
		}
	}

	ma_update_lwp_blocked_signals();
	ma_spin_unlock_softirq(&cthread->siglock);
	if (update_lwps_global_sigmask(NULL))
		update_lwps_blocked_signals(1);

	MARCEL_LOG_RETURN(0);
})

void ma_sigexit(void) {
	/* Since it will die, we don't want this thread to be chosen for
	 * delivering signals any more */
	ma_spin_lock_softirq(&SELF_GETMEM(siglock));
	marcel_sigfillset(&SELF_GETMEM(curmask));
	ma_spin_unlock_softirq(&SELF_GETMEM(siglock));

	/* Warn other threads if *_sigmask was called */
	if (update_lwps_global_sigmask(ma_self()))
		update_lwps_blocked_signals(1);
}


#ifdef MA__LIBPTHREAD
int lpt_sigmask(int how, __const sigset_t * set, sigset_t * oset)
{
	MARCEL_LOG_IN();

	int signo, ret;
	marcel_sigset_t marcel_set;
	marcel_sigset_t marcel_oset;

	marcel_sigemptyset(&marcel_set);
	if (set) {
		for (signo = 1; signo < MARCEL_NSIG; signo++)
			if (sigismember(set, signo))
				marcel_sigaddset(&marcel_set, signo);
		ret = pmarcel_sigmask(how, &marcel_set, &marcel_oset);
	} else
		ret = pmarcel_sigmask(how, NULL, &marcel_oset);

	if (oset) {
		sigemptyset(oset);
		for (signo = 1; signo < MARCEL_NSIG; signo++)
			if (marcel_sigismember(&marcel_oset, signo))
				sigaddset(oset, signo);
	}

	MARCEL_LOG_RETURN(ret);
}
#endif
DEF_LIBPTHREAD(int, sigmask, (int how, __const sigset_t * set, sigset_t * oset), (how, set, oset)) 
DEF___LIBPTHREAD(int, sigmask, (int how, __const sigset_t * set, sigset_t * oset), (how, set, oset))

/***************************sigprocmask*************************/
#ifdef MA__LIBPTHREAD
int lpt_sigprocmask(int how, __const sigset_t * set, sigset_t * oset)
{
	MARCEL_LOG_IN();

	if (check_sigmask(how)) {
		errno = EINVAL;
		MARCEL_LOG_RETURN(-1);
	}

#ifdef MA__DEBUG
	if (marcel_createdthreads() != 0)
		MA_WARN_USER("sigprocmask: multithreading here ! lpt_sigmask used instead\n");
#endif
	int ret = lpt_sigmask(how, set, oset);
	if (ret) {
		MARCEL_LOG("lpt_sigprocmask: lpt_sigmask fails\n");
		MARCEL_LOG_RETURN(ret);
	}
	MARCEL_LOG_RETURN(0);
}
DEF_LIBC(int, sigprocmask, (int how, __const sigset_t * set, sigset_t * oset), (how, set, oset))
DEF___LIBC(int, sigprocmask, (int how, __const sigset_t * set, sigset_t * oset), (how, set, oset))

#endif				/*  MA__LIBPTHREAD */

/****************************sigpending**************************/
static int check_sigpending(marcel_sigset_t * set)
{
	marcel_t cthread = ma_self();
	
	if (set == NULL) {
		MARCEL_LOG("check_sigpending: set is NULL\n");
		errno = EINVAL;
		return -1;
	}
	
	ma_spin_lock_softirq(&cthread->siglock);
	if (&cthread->sigpending == NULL) {
		ma_spin_unlock_softirq(&cthread->siglock);
		MARCEL_LOG("check_sigpending: pending signal field is NULL\n");
		errno = EINVAL;
		return -1;
	}
	ma_spin_unlock_softirq(&cthread->siglock);

	return 0;
}

DEF_MARCEL_PMARCEL(int, sigpending, (marcel_sigset_t *set), (set),
{
	int sig;
	marcel_t cthread = ma_self();

	MARCEL_LOG_IN();
	int err = check_sigpending(set);
	if (err)
		MARCEL_LOG_RETURN(err);

	marcel_sigemptyset(set);
	ma_spin_lock_softirq(&gsiglock);
	_ma_raw_spin_lock(&cthread->siglock);

	for (sig = 1; sig < MARCEL_NSIG; sig++) {
		if (marcel_sigorismember(&cthread->sigpending, &gsigpending, sig))
			marcel_sigaddset(set, sig);
	}

	_ma_raw_spin_unlock(&cthread->siglock);
	ma_spin_unlock_softirq(&gsiglock);

	MARCEL_LOG_RETURN(0);
})

#ifdef MA__LIBPTHREAD
int lpt_sigpending(sigset_t *set)
{
	MARCEL_LOG_IN();
	int signo;
	marcel_sigset_t marcel_set;

	int ret = marcel_sigpending(&marcel_set);

	sigemptyset(set);
	for (signo = 1; signo < MARCEL_NSIG; signo++)
		if (marcel_sigismember(&marcel_set, signo))
			sigaddset(set, signo);
	MARCEL_LOG_RETURN(ret);
}
#endif
DEF_LIBC(int,sigpending,(sigset_t *set),(set))
DEF___LIBC(int,sigpending,(sigset_t *set),(set))

/******************************sigtimedwait*****************************/
/* we need an empty and useless handler to register marcel_pidkill 
 * via marcel_sigaction; we should never execute handler code */
static void sigwait_handler(int sig TBX_UNUSED) 
{
}

static void sigwait_prepare_or_exit(tbx_bool_t prepare, const marcel_sigset_t *__restrict set, marcel_sigset_t *oldset)
{
	int sig;
	struct marcel_sigaction sa;
	static marcel_sigset_t unblock_sigs;
	static struct marcel_sigaction old_sa[MARCEL_NSIG];
	static ma_atomic_t queued_threads = MA_ATOMIC_INIT(0);

	if (tbx_true == prepare) {
		ma_smp_mb();
		if (1 == ma_atomic_inc_return(&queued_threads))
			marcel_sigemptyset(&unblock_sigs);

		if (! marcel_sigequalset(set, &unblock_sigs)) {
			sa.marcel_sa_handler = sigwait_handler;
			sa.marcel_sa_flags = 0;
			marcel_sigemptyset(&sa.marcel_sa_mask);
			for (sig = 1; sig < MARCEL_NSIG; sig ++) {
				if (marcel_signandismember(set, &unblock_sigs, sig))
					marcel_sigaction(sig, &sa, &old_sa[sig]);
			}
			marcel_sigorset(&unblock_sigs, &unblock_sigs, set);
		}
		_ma_raw_spin_unlock(&SELF_GETMEM(siglock));
		marcel_sigmask(SIG_UNBLOCK, set, oldset);
		_ma_raw_spin_lock(&SELF_GETMEM(siglock));
	} else {
		marcel_sigmask(SIG_SETMASK, oldset, NULL);
		if (0 == ma_atomic_dec_return(&queued_threads)) {
			for (sig = 1; sig < MARCEL_NSIG; sig ++) {
				if (marcel_sigismember(&unblock_sigs, sig))
					marcel_sigaction(sig, &old_sa[sig], NULL);
			}
		}
	}
}

DEF_MARCEL_PMARCEL(int, sigtimedwait, (const marcel_sigset_t *__restrict set, siginfo_t *__restrict info,const struct timespec *__restrict timeout), 
		   (set,info,timeout),
{
	MARCEL_LOG_IN();
	
	marcel_sigset_t oset;
	marcel_t cthread = ma_self();
	int waitsig;
	int sig;

	if (set == NULL) {
		MARCEL_LOG("(p)marcel_sigtimedwait: set is NULL\n");
		errno = EINVAL;
		MARCEL_LOG_RETURN(-1);
	}

	int old = __pmarcel_enable_asynccancel();
	
	ma_spin_lock_softirq(&gsiglock);
	_ma_raw_spin_lock(&cthread->siglock);
	for (sig = 1; sig < MARCEL_NSIG; sig++) {
		if (marcel_sigismember(set, sig)) {
			if (marcel_sigorismember(&gsigpending, &cthread->sigpending, sig)) {
				if (marcel_sigismember(&cthread->sigpending, sig)) {
					marcel_sigdelset(&cthread->sigpending, sig);
					if (info != NULL)
						*info = cthread->siginfo[sig];
				} else if (marcel_sigismember(&gsigpending, sig)) {
					marcel_sigdelset(&gsigpending, sig);
					if (info != NULL)
						*info = gsiginfo[sig];
				}
				_ma_raw_spin_unlock(&cthread->siglock);
				ma_spin_unlock_softirq(&gsiglock);
				
				MARCEL_LOG_RETURN(sig);
			}
		}
	}

	/* register thread, unmask signals */
	sigwait_prepare_or_exit(tbx_true, set, &oset);

	/* add (list head) a thread which waits for a signal delivery */
	ma_twblocked_t wthread;

	cthread->waitsig = &waitsig;
	marcel_sigemptyset(&cthread->waitset);
	marcel_sigorset(&cthread->waitset, &cthread->waitset, set);
	if (info != NULL)
		cthread->waitinfo = info;

	wthread.t = cthread;	
	wthread.next_twblocked = list_wthreads;
	list_wthreads = &wthread;

	int timed;
	unsigned long int waittime;

	if (timeout) {
		struct timeval tv;

		/* need to round-up */
		tv.tv_sec = timeout->tv_sec;
		tv.tv_usec = (timeout->tv_nsec + 999) / 1000;

		timed = 1;
		waittime = ma_jiffies_from_us(MA_TIMEVAL_TO_USEC(&tv));
	} else {
		timed = 0;
		waittime =  MARCEL_MAX_SCHEDULE_TIMEOUT;
	}

	waitsig = -1;
	
waiting:
	ma_set_current_state(MA_TASK_INTERRUPTIBLE);
	_ma_raw_spin_unlock(&cthread->siglock);
	ma_spin_unlock_softirq_no_resched(&gsiglock);

	waittime = ma_schedule_timeout(waittime);

	if ((waitsig == -1) && ((timed == 0) || (waittime != 0))) {
		ma_spin_lock_softirq(&gsiglock);
		_ma_raw_spin_lock(&cthread->siglock);
		goto waiting;
	}

	/* unregister thread */
	sigwait_prepare_or_exit(tbx_false, set, &oset);

	__pmarcel_disable_asynccancel(old);

	if ((timed == 1) && (waittime == 0)) {
		/* search a thread in the pending list */
		ma_twblocked_t *search_twblocked = list_wthreads;
		ma_twblocked_t *prec_twblocked = NULL;
		
		while (!marcel_equal(search_twblocked->t, cthread)) {
			prec_twblocked = search_twblocked;
			search_twblocked = search_twblocked->next_twblocked;
		}

		/* remove the thread */
		if (!prec_twblocked)
			list_wthreads = search_twblocked->next_twblocked;
		else
			prec_twblocked->next_twblocked = search_twblocked->next_twblocked;
		
		errno = EAGAIN;
		MARCEL_LOG_RETURN(-1);
	}
	MARCEL_LOG_RETURN(waitsig);
})

#ifdef MA__LIBPTHREAD
int lpt_sigtimedwait(__const sigset_t *__restrict set,siginfo_t *__restrict info, __const struct timespec *__restrict timeout)
{
	MARCEL_LOG_IN();

	int sig;
	marcel_sigset_t marcel_set;
	marcel_sigemptyset(&marcel_set);
	for (sig = 1; sig < MARCEL_NSIG; sig++)
		if (sigismember(set, sig))
			marcel_sigaddset(&marcel_set, sig);

	MARCEL_LOG_RETURN(marcel_sigtimedwait(&marcel_set, info, timeout));
}

versioned_symbol(libpthread, lpt_sigtimedwait, sigtimedwait, GLIBC_2_1);
#endif

/************************sigwaitinfo************************/
DEF_MARCEL_PMARCEL(int, sigwaitinfo, (const marcel_sigset_t *__restrict set, siginfo_t *__restrict info), (set,info),
{
	MARCEL_LOG_IN();
	MARCEL_LOG_RETURN(marcel_sigtimedwait(set,info,NULL));
})

#ifdef MA__LIBPTHREAD
int lpt_sigwaitinfo(__const sigset_t *__restrict set,siginfo_t *__restrict info)
{
        MARCEL_LOG_IN();
        MARCEL_LOG_RETURN(lpt_sigtimedwait(set,info,NULL));
}

versioned_symbol(libpthread, lpt_sigwaitinfo, sigwaitinfo, GLIBC_2_1);
#endif

/***********************sigwait*****************************/

DEF_MARCEL_PMARCEL(int, sigwait, (const marcel_sigset_t *__restrict set, int *__restrict sig), (set,sig),
{
	MARCEL_LOG_IN();
	int ret = marcel_sigtimedwait(set,NULL,NULL);
	if (ret == -1) {
		MARCEL_LOG("marcel_sigtimedwait fails\n");
		MARCEL_LOG_RETURN(ret);
	}
	*sig = ret;
	MARCEL_LOG_RETURN(0);
})

#ifdef MA__LIBPTHREAD
int lpt_sigwait(__const sigset_t *__restrict set,int *__restrict sig)
{
        MARCEL_LOG_IN();
        siginfo_t info;
        int ret = lpt_sigtimedwait(set,&info,NULL);
        if (ret == -1) {
                MARCEL_LOG("lpt_sigwait: lpt_sigtimedwait fails\n");
		MARCEL_LOG_RETURN(ret);
        }
        *sig = ret;
        MARCEL_LOG_RETURN(0);
}
#endif
DEF_LIBC(int, sigwait, (__const sigset_t *__restrict set, int *__restrict sig),(set,sig))
DEF___LIBC(int, sigwait, (__const sigset_t *__restrict set, int *__restrict sig),(set,sig))

/************************distrib****************************/

/* Try to provide the signal directly to a thread running sigwait or similar */
int marcel_distribwait_sigext(siginfo_t *info)
{
	int sig_delivered;

	ma_spin_lock_softirq(&gsiglock);
	sig_delivered = ma_distribwait_sigext(info);
	ma_spin_unlock_softirq(&gsiglock);

	return sig_delivered;
}

int ma_distribwait_sigext(siginfo_t *info)
{
	ma_twblocked_t *wthread = list_wthreads;
	ma_twblocked_t *prec_wthread = NULL;

	for (wthread = list_wthreads, prec_wthread = NULL; wthread != NULL;
	     prec_wthread = wthread, wthread = wthread->next_twblocked) {
		if (marcel_sigismember(&wthread->t->waitset, info->si_signo)) {
			/* update thread sigwait status */
			marcel_sigemptyset(&wthread->t->waitset);
			*wthread->t->waitsig = info->si_signo;
			if (wthread->t->waitinfo)
				*wthread->t->waitinfo = *info;

			/* remove the thread from the list */
			if (prec_wthread != NULL)
				prec_wthread->next_twblocked = wthread->next_twblocked;
			else
				list_wthreads = wthread->next_twblocked;

			ma_wake_up_state(wthread->t, MA_TASK_INTERRUPTIBLE);
			return 1;
		}
	}

	return 0;
}


/* Try to provide the signal directly to this thread if it is running sigwait or similar */
int marcel_distribwait_thread(siginfo_t * info, marcel_t thread)
{
	int ret = 0;

	ma_spin_lock_softirq(&thread->siglock);
	if (marcel_sigismember(&thread->waitset, info->si_signo)) {
		ma_twblocked_t *wthread = list_wthreads;
		ma_twblocked_t *prec_wthread = NULL;
		
		/* update thread sigwait status */
		marcel_sigemptyset(&thread->waitset);
		*thread->waitsig = info->si_signo;
		if (thread->waitinfo)
			*thread->waitinfo = *info;

		/* search and remove the thread from the list */
		while (thread != wthread->t) {
			prec_wthread = wthread;
			wthread = wthread->next_twblocked;
		}
		if (prec_wthread == NULL)
			list_wthreads = wthread->next_twblocked;
		else
			prec_wthread->next_twblocked = wthread->next_twblocked;

		ma_wake_up_state(thread, MA_TASK_INTERRUPTIBLE);
		ret = 1;
	}
	ma_spin_unlock_softirq(&thread->siglock);

	return ret;
}

/****************************sigsuspend**************************/
DEF_MARCEL_PMARCEL(int,sigsuspend,(const marcel_sigset_t *sigmask),(sigmask),
{
	MARCEL_LOG_IN();
	marcel_sigset_t oldmask;
	int old = __pmarcel_enable_asynccancel();	//pmarcel in marcel
	
	/* sleep before update the signal mask */
	ma_set_current_state(MA_TASK_INTERRUPTIBLE);
	marcel_sigmask(SIG_SETMASK, sigmask, &oldmask);
	ma_schedule();
	marcel_sigmask(SIG_SETMASK, &oldmask, NULL);
	errno = EINTR;
	__pmarcel_disable_asynccancel(old);
	
	MARCEL_LOG_RETURN(-1);
})

#ifdef MA__LIBPTHREAD
int lpt_sigsuspend(const sigset_t * sigmask)
{
	MARCEL_LOG_IN();
	int signo;
	marcel_sigset_t marcel_set;
	marcel_sigemptyset(&marcel_set);
	for (signo = 1; signo < MARCEL_NSIG; signo++)
		if (sigismember(sigmask, signo))
			marcel_sigaddset(&marcel_set, signo);
	MARCEL_LOG_RETURN(marcel_sigsuspend(&marcel_set));
}
#endif
DEF_LIBC(int, sigsuspend, (const sigset_t * sigmask), (sigmask))
DEF___LIBC(int, sigsuspend, (const sigset_t * sigmask), (sigmask))

/***************************signal********************************/
DEF_MARCEL_PMARCEL(void *,signal,(int sig, void * handler),(sig,handler),
{
	MARCEL_LOG_IN();
	
	struct marcel_sigaction act;
	struct marcel_sigaction oact;

	/* Check signal extents to protect __sigismember.  */
	if (handler == SIG_ERR || sig < 0 || sig >= MARCEL_NSIG) {
		MARCEL_LOG("(p)marcel_signal : handler (%p) ou sig (%d) invalide\n", handler, sig);
		errno = EINVAL;
		MARCEL_LOG_RETURN(SIG_ERR);
	}

	act.marcel_sa_handler = (void (*)(int))handler;
	marcel_sigemptyset(&act.marcel_sa_mask);
	marcel_sigaddset(&act.marcel_sa_mask, sig);
	
	act.marcel_sa_flags = SA_RESTART | MA__SIGNAL_NODEFER;
	/* warning siginfo: NODEFER|NOMASK allows optimisations: no need to add sig to sa_mask in deliver_sig */
	if (marcel_sigaction(sig, &act, &oact) < 0) {
		MARCEL_LOG_RETURN(SIG_ERR);
	}
	
	MARCEL_LOG_RETURN(oact.marcel_sa_handler);
})

#ifdef MA__LIBPTHREAD
void *lpt___sysv_signal(int sig, void * handler)
{
	MARCEL_LOG_IN();

	struct marcel_sigaction act;
	struct marcel_sigaction oact;

	/* Check signal extents to protect __sigismember.  */
	if (handler == SIG_ERR || sig < 0 || sig >= MARCEL_NSIG) {
		MARCEL_LOG("lpt___sysv_signal : handler (%p) or sig (%d) is invalid\n", handler, sig);
		errno = EINVAL;
		MARCEL_LOG_RETURN(SIG_ERR);
	}

	act.marcel_sa_handler = (void (*)(int))handler;
	marcel_sigemptyset(&act.marcel_sa_mask);
	marcel_sigaddset(&act.marcel_sa_mask, sig);

	act.marcel_sa_flags = MA__SIGNAL_RESETHAND | MA__SIGNAL_NODEFER;
	/* warning siginfo: NODEFER|NOMASK allows optimisations: no need to add sig to sa_mask in deliver_sig */
	if (marcel_sigaction(sig, &act, &oact) < 0) {
		MARCEL_LOG_RETURN(SIG_ERR);
	}

	MARCEL_LOG_RETURN(oact.marcel_sa_handler);
}
versioned_symbol(libpthread, pmarcel_signal, signal, GLIBC_2_0);
#endif

DEF___C(void *,signal,(int sig, void * handler),(sig,handler))
DEF_LIBC(void *,__sysv_signal,(int sig, void * handler),(sig,handler))
DEF___LIBC(void *,__sysv_signal,(int sig, void * handler),(sig,handler))

/***************************sigaction*****************************/
DEF_MARCEL_PMARCEL(int,sigaction,(int sig, const struct marcel_sigaction *act,
				  struct marcel_sigaction *oact),(sig,act,oact),
{
	MARCEL_LOG_IN();
	ma_sigaction_t kact;
	void *handler;
	void *ohandler;
	
	if ((sig < 1) || (sig >= MARCEL_NSIG)) {
		MARCEL_LOG("(p)marcel_sigaction: sig (%d) invalide, but returns 0 anyway\n", sig);
		//errno = EINVAL; // some apps use RT signals
		MARCEL_LOG_RETURN(0);
	}

#ifdef MA__DEBUG
	if (sig == MARCEL_RESCHED_SIGNAL)
		MA_WARN_USER("warning: signal %d not supported\n", sig);
#endif

	if (!act) {
		/* Just reading the current sigaction */
		if (oact != NULL) {
			ma_read_lock_softirq(&csig_lock);
			*oact = csigaction[sig];
			ma_read_unlock_softirq(&csig_lock);
		}
		MARCEL_LOG_RETURN(0);
	}

	/* Really writing */
#ifdef SA_SIGINFO
	if (act->marcel_sa_flags & SA_SIGINFO)
		handler = act->marcel_sa_sigaction;
	else
#endif
		handler = act->marcel_sa_handler;

	if (((SIGKILL == sig) || (SIGSTOP == sig)) && SIG_DFL != handler) {
		MARCEL_LOG("!! signal %d ne peut etre ignore\n", sig);
		errno = EINVAL;
		MARCEL_LOG_RETURN(-1);
	}

	/* Serialize sigaction operations */
	ma_write_lock_softirq(&csig_lock);
	
	/* Record old action */
	if (oact != NULL)
		*oact = csigaction[sig];
	
#ifdef SA_SIGINFO
	if (csigaction[sig].marcel_sa_flags & SA_SIGINFO)
		ohandler = csigaction[sig].marcel_sa_sigaction;
	else
#endif
		ohandler = csigaction[sig].marcel_sa_handler;
	
	/* Record new action */
	csigaction[sig] = *act;
	
	/* don't modify kernel handling for signals that we use */
	if (
#ifdef MA__TIMER
		(sig == MARCEL_TIMER_SIGNAL) ||
#if MARCEL_TIMER_USERSIGNAL != MARCEL_TIMER_SIGNAL
		(sig == MARCEL_TIMER_USERSIGNAL) ||
#endif
#endif
		(sig == MARCEL_RESCHED_SIGNAL)) {
		ma_write_unlock_softirq(&csig_lock);
		MARCEL_LOG_RETURN(0);
	}

	/* Before changing the kernel handler to the default behavior (which
	 * could be dangerous for our process), block the signal on LWPs that
	 * should */
	if (handler == SIG_DFL) {
		marcel_sigaddset(&ma_dfl_sigs, sig);
		sigaddset(&ma_dfl_ksigs, sig);
		update_lwps_blocked_signals(1);
	}
	
	/* set the kernel handler */
	sigemptyset(&kact.sa_mask);
	if (handler == SIG_IGN || handler == SIG_DFL)
#ifdef SA_SIGINFO
		kact.sa_sigaction = (void (*)(int, siginfo_t *, void *))handler;
#else
	kact.sa_handler = (void (*)(int))handler;
#endif
	else
#ifdef SA_SIGINFO
		kact.sa_sigaction = marcel_pidkill;
#else
	kact.sa_handler = marcel_pidkill;
#endif
	kact.sa_flags =
#ifdef SA_SIGINFO
		SA_SIGINFO |
#endif
		act->marcel_sa_flags;
	
	if (ma_sigaction(sig, &kact, NULL) == -1) {
		MARCEL_LOG("!! sigaction syscall fails for signal %d\n", sig);
		ma_write_unlock_softirq(&csig_lock);
		MARCEL_LOG_RETURN(-1);
	}
	
	/* After changing the kernel handler, we can unblock the signal on LWPs
	 * that should */
	if (ohandler == SIG_DFL && handler != SIG_DFL) {
		marcel_sigdelset(&ma_dfl_sigs, sig);
		sigdelset(&ma_dfl_ksigs, sig);
		ma_write_unlock_softirq(&csig_lock);

		update_lwps_blocked_signals(0);
		MARCEL_LOG_RETURN(0);
	}
	
	ma_write_unlock_softirq(&csig_lock);
	
	MARCEL_LOG_RETURN(0);
})

#ifdef MA__LIBPTHREAD
int lpt_sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	MARCEL_LOG_IN();

	int signo;
	int ret;
	struct marcel_sigaction marcel_act;
	struct marcel_sigaction marcel_oact;

	/* convert act into marcel_act data structure */
	if (act) {
		marcel_act.marcel_sa_flags = act->sa_flags;
#ifdef SA_SIGINFO
		if (act->sa_flags & SA_SIGINFO)
			marcel_act.marcel_sa_sigaction = act->sa_sigaction;
		else
#endif
			marcel_act.marcel_sa_handler = act->sa_handler;

		/* convert sigset_t into marcel_sigset_t */
		marcel_sigemptyset(&marcel_act.marcel_sa_mask);
		for (signo = 1; signo < MARCEL_NSIG; signo++)
			if (sigismember(&act->sa_mask, signo))
				marcel_sigaddset(&marcel_act.marcel_sa_mask, signo);
	}

	/*  marcel_sigaction */
	ret = marcel_sigaction(sig, act ? &marcel_act : NULL,
			       oact ? &marcel_oact : NULL);

	/* from marcel_act to act */
	if (oact) {
		oact->sa_flags = marcel_oact.marcel_sa_flags;
#ifdef SA_SIGINFO
		if (oact->sa_flags & SA_SIGINFO)
			oact->sa_sigaction = marcel_oact.marcel_sa_sigaction;
		else
#endif
			oact->sa_handler = marcel_oact.marcel_sa_handler;

		sigemptyset(&oact->sa_mask);
		for (signo = 1; signo < MARCEL_NSIG; signo++)
			if (marcel_sigismember(&marcel_oact.marcel_sa_mask,
					       signo))
				sigaddset(&oact->sa_mask, signo);
	}

	MARCEL_LOG_RETURN(ret);
}
#endif

DEF_LIBC(int,sigaction,(int sig, const struct sigaction *act,
			struct sigaction *oact),(sig,act,oact))
DEF___LIBC(int,sigaction,(int sig, const struct sigaction *act,
			  struct sigaction *oact),(sig,act,oact))

/***********************marcel_deliver_sig**********************/

/* Called in the context of a thread (either directly from sigmask or via
 * deviation when it should deliver some signals */
int marcel_deliver_sig(void)
{
	int sig;
	int restart_deliver_sig;
	marcel_t cthread = ma_self();

	ma_spin_lock_softirq(&cthread->siglock);

	cthread->delivering_sig = 1;
	restart_deliver_sig = 0;

	for (sig = 1; sig < MARCEL_NSIG; sig++) {
		if (marcel_sigismember(&cthread->sigpending, sig)) {
			if (!marcel_sigismember(&cthread->curmask, sig)) {
				/* signal is not blocked */
				marcel_sigdelset(&cthread->sigpending, sig);
				ma_spin_unlock_softirq(&cthread->siglock);
				ma_set_current_state(MA_TASK_RUNNING);
				marcel_call_function(sig);
				ma_spin_lock_softirq(&cthread->siglock);
			} else {
				/* reschedule concurrent signal delivery */
				restart_deliver_sig = 1;
			}
		}
	}
	/* other signals to deliver ? */
	if (restart_deliver_sig)
		marcel_deviate(cthread, (marcel_handler_func_t) marcel_deliver_sig, NULL);

	cthread->delivering_sig = 0;
	ma_spin_unlock_softirq(&cthread->siglock);

	MARCEL_LOG_RETURN(0);
}

/*************************marcel_call_function******************/

/* Actually deliver the signal: ignore if SIG_IGN, raise the signal to the
 * kernel if SIG_DFL, else just call the function */
int marcel_call_function(int sig)
{
	struct marcel_sigaction cact;
	marcel_sigset_t newmask, oldmask;
	void (*handler) (int);

	MARCEL_LOG_IN();

	if ((sig < 0) || (sig >= MARCEL_NSIG)) {
		MARCEL_LOG("marcel_call_function: sig (%d) is invalid\n", sig);
		errno = EINVAL;
		MARCEL_LOG_RETURN(-1);
	}

	ma_read_lock_softirq(&csig_lock);
	cact = csigaction[sig];
	ma_read_unlock_softirq(&csig_lock);

#ifdef SA_SIGINFO
	if ((cact.marcel_sa_flags & SA_SIGINFO))
		handler = (typeof(handler)) cact.marcel_sa_sigaction;
	else
#endif
		handler = cact.marcel_sa_handler;

	/* No SA_NODEFER|NOMASK: current signal must be masked */
	newmask = cact.marcel_sa_mask;
#ifdef MA__SIGNAL_NODEFER
	if (!(cact.marcel_sa_flags & MA__SIGNAL_NODEFER))
		marcel_sigaddset(&newmask, sig);
#endif

	if (handler == SIG_IGN)
		MARCEL_LOG_RETURN(0);

	if (handler == SIG_DFL) {
		if (kill(getpid(), sig) == -1)
			MARCEL_LOG_RETURN(-1);
		MARCEL_LOG_RETURN(0);
	}

	ma_spin_lock_softirq(&SELF_GETMEM(siglock));
	ma_update_thread_sigmask(SIG_BLOCK, &newmask, &oldmask);
	ma_spin_unlock_softirq(&SELF_GETMEM(siglock));

	if (!(cact.marcel_sa_flags & SA_RESTART))
		MA_SET_INTERRUPTED(1);

	/* call signal handler */
#ifdef SA_SIGINFO
	if ((cact.marcel_sa_flags & SA_SIGINFO)) {
		void (*action) (int sig, siginfo_t * info, void *uc) = (void (*)(int, siginfo_t*, void*))handler;
		action(sig, &(ma_self()->siginfo[sig]), NULL);
	} else
#endif
		handler(sig);

	/* restore the thread sigmask */
	ma_spin_lock_softirq(&SELF_GETMEM(siglock));
	ma_update_thread_sigmask(SIG_SETMASK, &oldmask, NULL);
	ma_spin_unlock_softirq(&SELF_GETMEM(siglock));

	MARCEL_LOG_RETURN(0);
}

/****************other fonctions******************/
#if HAVE_DECL_SIGHOLD
DEF_MARCEL_PMARCEL(int,sighold,(int sig),(sig),
{
	int ret;
	marcel_sigset_t set;

	MARCEL_LOG_IN();

	if ((sig < 1) || (sig >= MARCEL_NSIG)) {
		errno = EINVAL;
		MARCEL_LOG_RETURN(-1);
	}

	marcel_sigemptyset(&set);
	marcel_sigaddset(&set, sig);
	ret = marcel_sigmask(SIG_BLOCK, &set, NULL);
	if (ret) {
		errno = ret;
		MARCEL_LOG_RETURN(-1);
	}

	MARCEL_LOG_RETURN(0);
})
#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, pmarcel_sighold, sighold, GLIBC_2_1);
#endif
#endif

#if HAVE_DECL_SIGRELSE
DEF_MARCEL_PMARCEL(int,sigrelse,(int sig),(sig),
{
	int ret;
	marcel_sigset_t set;

	MARCEL_LOG_IN();

	if ((sig < 1) || (sig >= MARCEL_NSIG)) {
		errno = EINVAL;
		MARCEL_LOG_RETURN(-1);
	}

	marcel_sigemptyset(&set);
	marcel_sigaddset(&set,sig);
	ret = marcel_sigmask(SIG_UNBLOCK,&set,NULL);
	if (ret) {
		errno = ret;
		MARCEL_LOG_RETURN(-1);
	}

	MARCEL_LOG_RETURN(0);
})
#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, pmarcel_sigrelse, sigrelse, GLIBC_2_1);
#endif
#endif

#if HAVE_DECL_SIGIGNORE
DEF_MARCEL_PMARCEL(int,sigignore,(int sig),(sig),
{
	struct marcel_sigaction act;

	MARCEL_LOG_IN();

	if ((sig < 1) || (sig >= MARCEL_NSIG)) {
		errno = EINVAL;
		MARCEL_LOG_RETURN(-1);
	}

	act.marcel_sa_flags = 0;
	act.marcel_sa_handler = SIG_IGN;
	marcel_sigemptyset(&act.marcel_sa_mask);
	MARCEL_LOG_RETURN(marcel_sigaction(sig,&act,NULL));
})
#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, pmarcel_sigignore, sigignore, GLIBC_2_1);
#endif
#endif


int marcel___sigpause(int sig_or_mask, int is_sig)
{
	int old;
	marcel_sigset_t set, oset;

	MARCEL_LOG_IN();

	if (is_sig) {
		/* System V version: remove sig from process mask */
		if ((sig_or_mask < 1) || (sig_or_mask >= MARCEL_NSIG)) {
			errno = EINVAL;
			MARCEL_LOG_RETURN(-1);
		}
		marcel_sigemptyset(&set);
		marcel_sigaddset(&set, sig_or_mask);

		old = __pmarcel_enable_asynccancel();
		/* sleep before update the signal mask */
		ma_set_current_state(MA_TASK_INTERRUPTIBLE);
		marcel_sigmask(SIG_UNBLOCK, &set, &oset);
		ma_schedule();
		marcel_sigmask(SIG_SETMASK, &oset, NULL);
		errno = EINTR;
		__pmarcel_disable_asynccancel(old);
	
		MARCEL_LOG_RETURN(-1);
	} else {
		/* BSD version: set is the complete process sigmask */
		set = sig_or_mask;
		MARCEL_LOG_RETURN(marcel_sigsuspend(&set));
	}
}

DEF_MARCEL_PMARCEL(int,sigpause,(int mask),(mask),
{
	return marcel___sigpause(mask, 0);
})

DEF_MARCEL_PMARCEL(int,xpg_sigpause,(int sig),(sig),
{
	return marcel___sigpause(sig, 1);
})
#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, marcel___sigpause, __sigpause, GLIBC_2_0);
versioned_symbol(libpthread, pmarcel_sigpause, sigpause, GLIBC_2_0);
versioned_symbol(libpthread, pmarcel_xpg_sigpause, __xpg_sigpause, GLIBC_2_2);
#endif

#if HAVE_DECL_SIGSET
void (*pmarcel_sigset(int sig,void (*dispo)(int)))
{
	int ret;
        marcel_sigset_t set, oset;
        struct marcel_sigaction act,oact;

        MARCEL_LOG_IN();

	if ((sig < 1) || (sig >= MARCEL_NSIG)) {
		errno = EINVAL;
		MARCEL_LOG_RETURN((void *)-1);
	}

        ret = marcel_sigaction(sig, NULL, &oact);
	if (ret != 0) {
		MARCEL_LOG("sigaction: can retrieve previous handler\n");
		MARCEL_LOG_RETURN(SIG_ERR);
	}

	marcel_sigemptyset(&set);
	marcel_sigaddset(&set, sig);

	if (SIG_HOLD == dispo) {
		marcel_sigmask(SIG_BLOCK, &set, &oset);
		act.marcel_sa_handler = oact.marcel_sa_handler;
	} else {
		marcel_sigmask(SIG_UNBLOCK, &set, &oset);
		act.marcel_sa_handler = dispo;
	}

	act.marcel_sa_flags = 0; //TODO : check flags we need
	marcel_sigemptyset(&act.marcel_sa_mask);
	ret = marcel_sigaction(sig, &act, NULL);
	if (ret != 0) {
		MARCEL_LOG("sigaction: can't set new handler\n");
                MARCEL_LOG_RETURN(SIG_ERR);         
	}

        MARCEL_LOG_RETURN((marcel_sigismember(&oset,sig) ? SIG_HOLD : oact.marcel_sa_handler));
}
#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, pmarcel_sigset, sigset, GLIBC_2_1);
#endif
#endif


/*******************ma_signals_init***********************/
void ma_signals_init(void)
{
	MARCEL_LOG_IN();

#ifdef MA__LWPS
	MA_BUG_ON(! ma_is_first_lwp(MA_LWP_SELF)); // called by main lwp only
	marcel_sigemptyset(&MA_LWP_SELF->curmask);
#endif
	marcel_sigemptyset(&ma_thr_sigmask);
	sigemptyset(&ma_thr_ksigmask);

	ma_open_softirq(MA_SIGNAL_SOFTIRQ, marcel_sigtransfer, NULL);
	ma_open_softirq(MA_SIGMASK_SOFTIRQ, update_lwp_blocked_signals_softirq, NULL);
	marcel_sigfillset(&ma_dfl_sigs);
	sigfillset(&ma_dfl_ksigs);
#ifdef MA__TIMER
	marcel_sigdelset(&ma_dfl_sigs, MARCEL_TIMER_SIGNAL);
	sigdelset(&ma_dfl_ksigs, MARCEL_TIMER_SIGNAL);
#if MARCEL_TIMER_USERSIGNAL != MARCEL_TIMER_SIGNAL
	marcel_sigdelset(&ma_dfl_sigs, MARCEL_TIMER_USERSIGNAL);
	sigdelset(&ma_dfl_ksigs, MARCEL_TIMER_USERSIGNAL);
#endif
#endif
	marcel_sigdelset(&ma_dfl_sigs, MARCEL_RESCHED_SIGNAL);
	sigdelset(&ma_dfl_ksigs, MARCEL_RESCHED_SIGNAL);
	MARCEL_LOG_OUT();
}

#ifdef MA__DEBUG
/***************testset*********/
void marcel_testset(__const marcel_sigset_t * set, char *what)
{
	int i;
	marcel_fprintf(stderr, "signaux du set %s : ", what);
	for (i = 1; i < MARCEL_NSIG; i++)
		if (marcel_sigismember(set, i))
			marcel_fprintf(stderr, "1");
		else
			marcel_fprintf(stderr, "0");
	marcel_fprintf(stderr, "\n");
}
#endif


#else

/** Provides pthread_sigmask: fix link with librt **/
#ifdef MA__LIBPTHREAD
int lpt_sigmask(int how TBX_UNUSED, __const sigset_t * set TBX_UNUSED, sigset_t * oset TBX_UNUSED)
{
	return ENOSYS;
}
#endif
DEF_LIBPTHREAD(int, sigmask, (int how, __const sigset_t * set, sigset_t * oset), (how, set, oset)) 
DEF___LIBPTHREAD(int, sigmask, (int how, __const sigset_t * set, sigset_t * oset), (how, set, oset))


#endif /** MARCEL_SIGNALS_ENABLED **/
