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

#ifdef MARCEL_SIGNALS_ENABLED

#include "marcel.h"
#include <setjmp.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

/*
 * doc/dev/posix/06-11-07-Jeuland/doc_marcel_posix.pdf provides (french) details
 * about Marcel signal management.
 */

/*
 * Signals come from several sources and to different targets.
 *
 * - Kernel or other processes to our process.  marcel_pidkill() handles the
 *   signal by just setting ksiginfo and raising the MA_SIGNAL_SOFTIRQ.  The
 *   softirq handler can then handle it less expectedly later: choose a thread
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
 */

/*********************MA__DEBUG******************************/
#define MA__DEBUG
/*********************variables globales*********************/
static struct marcel_sigaction csigaction[MARCEL_NSIG];
/* Protects csigaction updates.  */
static marcel_rwlock_t rwlock = MARCEL_RWLOCK_INITIALIZER;
/* sigpending processus */
static int ksigpending[MARCEL_NSIG];
static siginfo_t ksiginfo[MARCEL_NSIG];
static marcel_sigset_t gsigpending;
static siginfo_t gsiginfo[MARCEL_NSIG];
static ma_spinlock_t gsiglock = MA_SPIN_LOCK_UNLOCKED;
static ma_twblocked_t *list_wthreads;

static marcel_sigset_t ma_dfl_sigs;
static sigset_t ma_dfl_ksigs;

int marcel_deliver_sig(void);

/*********************__ma_restore_rt************************/
#ifdef MA__LIBPTHREAD
#ifdef X86_64_ARCH
#define RESTORE(name, syscall) RESTORE2(name, syscall)
#define RESTORE2(name, syscall) asm( \
	".text\n" \
	".align 16\n" \
	".global __ma_"#name"\n" \
	".type __ma_"#name",@function\n" \
	"__ma_"#name":\n" \
	"	movq $" #syscall ", %rax\n" \
	"	syscall\n" \
);

RESTORE(restore_rt,SYS_rt_sigreturn);
#endif
#endif
/*********************pause**********************************/
DEF_MARCEL_POSIX(int,pause,(void),(),
{
	LOG_IN();
	marcel_sigset_t set;
	marcel_sigemptyset(&set);
	marcel_sigmask(SIG_BLOCK, NULL, &set);
	LOG_RETURN(marcel_sigsuspend(&set));
})

DEF_C(int,pause,(void),())
DEF___C(int,pause,(void),())

/*********************alarm**********************************/


/* marcel timer handler that generates the SIGALARM signal for alarm( */
static void alarm_timeout(unsigned long data)
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

static struct ma_timer_list alarm_timer = MA_TIMER_INITIALIZER(alarm_timeout, 0, 0);

DEF_MARCEL_POSIX(unsigned int, alarm, (unsigned int nb_sec), (nb_sec),
{
	LOG_IN();
	unsigned int ret = 0;
	ma_del_timer_sync(&alarm_timer);

	if (alarm_timer.expires > ma_jiffies) {
		LOG_OUT();
		ret =
		    ((alarm_timer.expires -
			ma_jiffies) * marcel_gettimeslice()) / 1000000;
	}

	if (nb_sec) {
		ma_init_timer(&alarm_timer);
		alarm_timer.expires =
		    ma_jiffies + (nb_sec * 1000000) / marcel_gettimeslice();
		ma_add_timer(&alarm_timer);
	}
	LOG_RETURN(ret);
})

#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, pmarcel_alarm, alarm, GLIBC_2_0);
#endif
DEF___C(unsigned int, alarm, (unsigned int nb_sec), (nb_sec))


/***********************get/setitimer**********************/
static void itimer_timeout(unsigned long data);

static struct ma_timer_list itimer_timer = MA_TIMER_INITIALIZER(itimer_timeout, 0, 0);
unsigned long interval;
unsigned long valeur;

/* marcel timer handler that generates the SIGALARM signal for setitimer() */
static void itimer_timeout(unsigned long data)
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
	if (interval) {
		ma_init_timer(&itimer_timer);
		itimer_timer.expires =
		    ma_jiffies + interval / marcel_gettimeslice();
		ma_add_timer(&itimer_timer);
	}
}

/* Common functions to check given values */
static int check_itimer(int which)
{
	if ((which != ITIMER_REAL) && (which != ITIMER_VIRTUAL)
	    && (which != ITIMER_PROF)) {
		mdebug("check_itimer : valeur which invalide !\n");
		errno = EINVAL;
		return -1;
	}
	if (which != ITIMER_REAL) {
		marcel_fprintf(stderr,
		    "set/getitimer : value which not supported !\n");
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
		if ((value->it_value.tv_usec > 999999)
		    || (value->it_interval.tv_usec > 999999)) {
			mdebug
			    ("check_setitimer : valeur temporelle trop elevee !\n");
			errno = EINVAL;
			return -1;
		}
	}
	return 0;
}
/***********************getitimer**********************/

DEF_MARCEL_POSIX(int, getitimer, (int which, struct itimerval *value), (which, value),
{
	LOG_IN();

	/* error checking */
	int ret = check_itimer(which);
	if (ret) {
		LOG_RETURN(ret);
	}
	/* error checking end */

	value->it_interval.tv_sec = interval / 1000000;
	value->it_interval.tv_usec = interval % 1000000;
	value->it_value.tv_sec = (itimer_timer.expires - ma_jiffies) / 1000000;
	value->it_value.tv_usec = (itimer_timer.expires - ma_jiffies) % 1000000;
	LOG_RETURN(0);
})

DEF_C(int, getitimer, (int which, struct itimerval *value), (which, value))
DEF___C(int, getitimer, (int which, struct itimerval *value), (which, value))

/************************setitimer***********************/

DEF_MARCEL_POSIX(int, setitimer, (int which, const struct itimerval *value, struct itimerval *ovalue), (which, value, ovalue),
{
	LOG_IN();

	/* error checking */
	unsigned int ret = check_setitimer(which, value);
	if (ret) {
		LOG_RETURN(ret);
	}
	/* error checking end */

	ma_del_timer_sync(&itimer_timer);

	if (ovalue) {
		ovalue->it_interval.tv_sec = interval / 1000000;
		ovalue->it_interval.tv_usec = interval % 1000000;
		ovalue->it_value.tv_sec = valeur / 1000000;
		ovalue->it_value.tv_usec = valeur % 1000000;
	}

	if (itimer_timer.expires > ma_jiffies)
		ret =
		    ((itimer_timer.expires -
			ma_jiffies) * marcel_gettimeslice()) / 1000000;

	interval =
	    value->it_interval.tv_sec * 1000000 + value->it_interval.tv_usec;
	valeur = value->it_value.tv_sec * 1000000 + value->it_value.tv_usec;

	if (valeur) {
		ma_init_timer(&itimer_timer);
		itimer_timer.expires =
		    ma_jiffies + valeur / marcel_gettimeslice();
		ma_add_timer(&itimer_timer);
	}

	LOG_RETURN(ret);
})
DEF_C(int, setitimer, (int which, const struct itimerval *value, struct itimerval *ovalue), (which, value, ovalue))
DEF___C(int, setitimer, (int which, const struct itimerval *value, struct itimerval *ovalue), (which, value, ovalue))

/*********************marcel_sigtransfer*********************/
/* try to transfer process signals to threads */
static void marcel_sigtransfer(struct ma_softirq_action *action)
{
	marcel_t t;
	int sig;
	int deliver;

	/* Fetch signals from the various handlers */
	ma_spin_lock_softirq(&gsiglock);
	for (sig = 1; sig < MARCEL_NSIG; sig++) {
		if (ksigpending[sig] == 1) {
			marcel_sigaddset(&gsigpending, sig);
			ksigpending[sig] = 0;
#ifdef SA_SIGINFO
			gsiginfo[sig] = ksiginfo[sig];
#endif
		}
	}

	struct marcel_topo_level *vp;
	int number = ma_vpnum(MA_LWP_SELF);

	/* Iterate over all threads */
	for_all_vp_from_begin(vp, number) {
		_ma_raw_spin_lock(&ma_topo_vpdata_l(vp, threadlist_lock));
		list_for_each_entry(t, &ma_topo_vpdata_l(vp, all_threads),
		    all_threads) {
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
			if (deliver)
				/* Deliver all signals at once */
				marcel_deviate(t,
				    (handler_func_t) marcel_deliver_sig, NULL);
			_ma_raw_spin_unlock(&t->siglock);
			if (marcel_sigisemptyset(&gsigpending))
				break;
		}
		_ma_raw_spin_unlock(&ma_topo_vpdata_l(vp, threadlist_lock));	//verrou de liste des threads
		if (marcel_sigisemptyset(&gsigpending))
			break;
	}
	for_all_vp_from_end(vp, number)

	ma_spin_unlock_softirq(&gsiglock);

	LOG_OUT();
	return;
}

/*****************************raise****************************/
static int check_raise(int sig)
{
	if ((sig < 0) || (sig >= MARCEL_NSIG)) {
		mdebug("check_raise : signal %d invalide !\n", sig);
		errno = EINVAL;
		return -1;
	}
	return 0;
}

DEF_MARCEL(int, raise, (int sig), (sig),
{
	LOG_IN();

#ifdef MA__DEBUG
	int ret = check_raise(sig);
	if (ret) {
		LOG_RETURN(ret);
	}
#endif

	marcel_kill(marcel_self(), sig);

	LOG_RETURN(0);
})

DEF_POSIX(int, raise, (int sig), (sig),
{
	LOG_IN();

	/* error checking */
	int ret = check_raise(sig);
	if (ret) {
		LOG_RETURN(ret);
	}
	/* error checking end */

	marcel_kill(marcel_self(), sig);

	LOG_RETURN(0);
})

DEF_C(int,raise,(int sig),(sig))
DEF___C(int,raise,(int sig),(sig))


/*********************marcel_pidkill*************************/
int nb_pidkill = 0;

/* handler of signals coming from the kernel and other processes */
#ifdef SA_SIGINFO
static void marcel_pidkill(int sig, siginfo_t *info, void *uc)
#else
static void marcel_pidkill(int sig)
#endif
{
	if ((sig < 0) || (sig >= MARCEL_NSIG))
		return;

	if (sig == SIGABRT || sig == SIGBUS || sig == SIGILL || sig == SIGFPE
	    || sig == SIGPIPE || sig == SIGSEGV) {
#ifdef SA_SIGINFO
		if (csigaction[sig].marcel_sa_flags & SA_SIGINFO) {
			if (csigaction[sig].marcel_sa_sigaction !=
			    (void *) SIG_DFL
			    && csigaction[sig].marcel_sa_sigaction !=
			    (void *) SIG_IGN) {
				csigaction[sig].marcel_sa_sigaction(sig, info,
				    uc);
				return;
			}
		} else
#endif
		if (csigaction[sig].marcel_sa_handler != SIG_DFL &&
		    csigaction[sig].marcel_sa_handler != SIG_IGN) {
			csigaction[sig].marcel_sa_handler(sig);
			return;
		}
	}

	ma_irq_enter();

	siginfo_t infobis;
	infobis.si_signo = sig;
	infobis.si_code = 0;

	if (!marcel_distribwait_sigext(&infobis)) {
		ksigpending[sig] = 1;
#ifdef SA_SIGINFO
		ksiginfo[sig] = *info;
#endif
		ma_raise_softirq_from_hardirq(MA_SIGNAL_SOFTIRQ);
	}

	ma_irq_exit();
	/* TODO: check preempt */
}

/* We have either changed some sigaction to/from SIG_DFL or switched to another
 * thread or called sigmask.  We may need to update our blocked mask.
 *
 * Note: we do not protect ourselves from a concurrent call to sigaction that
 * would modify ma_dfl_sigs.  That is fine as sigaction will notify us later
 * anyway.  */

void ma_update_lwp_blocked_signals(void) {
	marcel_sigset_t should_block;
	marcel_sigandset(&should_block, &ma_dfl_sigs, &SELF_GETMEM(curmask));
	if (!marcel_sigequalset(&should_block, &__ma_get_lwp_var(curmask))) {
		/* We have to update the LWP's sigmask */
		sigset_t kshould_block;
		/* _softirq also disables the MA_SIGMASK_SOFTIRQ, to make sure
		 * we do not miss a ma_dfl_ksigs change between our read and
		 * the actual call to kthread_sigmask*/
		ma_spin_lock_softirq(&ma_timer_sigmask_lock);
#ifdef __GLIBC__
		sigandset(&kshould_block, &ma_dfl_ksigs, &SELF_GETMEM(kcurmask));
		sigorset(&kshould_block, &kshould_block, &ma_timer_sigmask);
#else
		int sig;

		sigemptyset(&kshould_block);
		for (sig = 1; sig < MARCEL_NSIG; sig++)
			if (marcel_sigismember(&ma_dfl_sigs, sig) &&
				marcel_sigismember(&SELF_GETMEM(curmask), sig) ||
				sigismember(&ma_timer_sigmask))
				sigaddset(&kshould_block, sig);
#endif
		marcel_kthread_sigmask(SIG_SETMASK, &kshould_block, NULL);
		ma_spin_unlock_softirq(&ma_timer_sigmask_lock);
		__ma_get_lwp_var(curmask) = should_block;
	}
}

/* Number of LWPs pending an update of blocked signals.  */
static ma_atomic_t nr_pending_blocked_signals_updates = MA_ATOMIC_INIT(0);
static marcel_sem_t blocked_signals_sem = MARCEL_SEM_INITIALIZER(0);;

static void update_lwp_blocked_signals_softirq(struct ma_softirq_action *action) {
	ma_update_lwp_blocked_signals();
	if (!ma_atomic_dec_return(&nr_pending_blocked_signals_updates))
		marcel_sem_V(&blocked_signals_sem);
}

/* We have changed some sigaction to/from SIG_DFL, we may need to make other
 * LWPs update their blocked mask before continuing.  */
static void update_lwps_blocked_signals(int wait) {
	ma_lwp_t lwp;
	/* First make sure other processors see the new set of signals with
	 * SIG_DFL */
	ma_smp_mb();
	int res;

	/* Check that we did not miss anybody last time.  */
	MA_BUG_ON(ma_atomic_read(&nr_pending_blocked_signals_updates));
	marcel_sem_getvalue(&blocked_signals_sem,&res);
	MA_BUG_ON(res);

	/* Make sure no two LWPs run sem_V */
	ma_atomic_inc(&nr_pending_blocked_signals_updates);
	ma_for_all_lwp(lwp) {
		if (lwp == MA_LWP_SELF)
			ma_update_lwp_blocked_signals();
		else {
			ma_raise_softirq_lwp(lwp, MA_SIGMASK_SOFTIRQ);
			ma_atomic_inc(&nr_pending_blocked_signals_updates);
		}
	}
	if (ma_atomic_dec_return(&nr_pending_blocked_signals_updates))
		marcel_sem_P(&blocked_signals_sem);

	/* Check that we did not miss anybody.  */
	MA_BUG_ON(ma_atomic_read(&nr_pending_blocked_signals_updates));
	marcel_sem_getvalue(&blocked_signals_sem,&res);
	MA_BUG_ON(res);
}

/*********************pthread_kill***************************/
static int check_kill(marcel_t thread, int sig)
{
	if (!MARCEL_THREAD_ISALIVE(thread)) {
		mdebug("check_kill : thread %p dead\n", thread);
		return ESRCH;
	}
	if ((sig < 0) || (sig >= MARCEL_NSIG)) {
		mdebug("check_kill : numero de signal %d invalide\n", sig);
		return EINVAL;
	}
	return 0;
}

DEF_MARCEL(int, kill, (marcel_t thread, int sig), (thread,sig),
{
	LOG_IN();

#ifdef MA__DEBUG
	int err = check_kill(thread, sig);
	if (err) {
		LOG_RETURN(err);
	}
#endif

	if (!sig) {
		LOG_RETURN(0);
	}

	siginfo_t info;
	info.si_signo = sig;
	info.si_code = 0;

	if (!marcel_distribwait_thread(&info, thread)) {
		ma_spin_lock_softirq(&thread->siglock);
		marcel_sigaddset(&thread->sigpending, sig);
#ifdef SA_SIGINFO
		thread->siginfo[sig] = info;
#endif
		ma_spin_unlock_softirq(&thread->siglock);
		marcel_deviate(thread, (handler_func_t) marcel_deliver_sig,
		    NULL);
	}

	LOG_RETURN(0);
})

DEF_POSIX(int, kill, (pmarcel_t pmthread, int sig), (pmthread,sig),
{
	LOG_IN();

	marcel_t thread = (marcel_t) pmthread;

	/* codes d'erreur */
	int err = check_kill(thread, sig);
	if (err) {
		LOG_RETURN(err);
	}
	/* fin codes d'erreur */

	if (!sig) {
		LOG_RETURN(0);
	}

	siginfo_t info;
	info.si_signo = sig;
	info.si_code = 0;

	if (!marcel_distribwait_thread(&info, thread)) {
		ma_spin_lock_softirq(&thread->siglock);
		marcel_sigaddset(&thread->sigpending, sig);
#ifdef SA_SIGINFO
		thread->siginfo[sig] = info;
#endif
		ma_spin_unlock_softirq(&thread->siglock);
		marcel_deviate(thread, (handler_func_t) marcel_deliver_sig,
		    NULL);
	}

	LOG_RETURN(0);
})

DEF_PTHREAD(int, kill, (pthread_t thread, int sig), (thread,sig))
DEF___PTHREAD(int, kill, (pthread_t thread, int sig), (thread,sig))

/*************************pthread_sigmask***********************/
static int check_sigmask(int how)
{
	if ((how != SIG_BLOCK) && (how != SIG_UNBLOCK)
	    && (how != SIG_SETMASK)) {
		mdebug("check_sigmask : valeur how %d invalide\n", how);
		return EINVAL;
	}
	return 0;
}

int marcel_sigmask(int how, __const marcel_sigset_t * set,
    marcel_sigset_t * oset)
{
	LOG_IN();

	int sig;
	marcel_t cthread = marcel_self();
	marcel_sigset_t cset = cthread->curmask;

#ifdef MA__DEBUG
	int ret = check_sigmask(how);
	if (ret) {
		LOG_RETURN(ret);
	}
#endif

restart:

	ma_spin_lock_softirq(&cthread->siglock);
	if (!marcel_signandisempty(&cthread->sigpending, &cthread->curmask))
		for (sig = 1; sig < MARCEL_NSIG; sig++)
			if ((marcel_signandismember(&cthread->sigpending,
				    &cthread->curmask, sig))) {
				marcel_sigdelset(&cthread->sigpending, sig);
				ma_spin_unlock_softirq(&cthread->siglock);
				ma_set_current_state(MA_TASK_RUNNING);
				marcel_call_function(sig);
				goto restart;
			}

   /*********modifs des masques******/
	if (oset != NULL)
		*oset = cset;

	if (!set) {
		/* si on ne définit pas de nouveau masque, on retourne */
		ma_spin_unlock_softirq(&cthread->siglock);
		LOG_RETURN(0);
	}

	if (how == SIG_BLOCK)
		marcel_sigorset(&cset, &cset, set);
	else if (how == SIG_SETMASK)
		cset = *set;
	else if (how == SIG_UNBLOCK) {
		for (sig = 1; sig < MARCEL_NSIG; sig++)
			if (marcel_sigismember(set, sig))
				marcel_sigdelset(&cset, sig);
	} else
		mdebug("marcel_sigmask : pas de how\n");

	/* SIGKILL et SIGSTOP ne doivent pas être masqués */
	if (marcel_sigismember(&cset, SIGKILL))
		marcel_sigdelset(&cset, SIGKILL);
	if (marcel_sigismember(&cset, SIGSTOP))
		marcel_sigdelset(&cset, SIGSTOP);

	/* si le nouveau masque est le meme que le masque courant, on retourne */
	if (marcel_sigequalset(&cset, &cthread->curmask)) {
		ma_spin_unlock_softirq(&cthread->siglock);
		LOG_RETURN(0);
	}

	cthread->curmask = cset;
#ifdef __GLIBC__
	sigemptyset(&cthread->kcurmask);
	for (sig = 1; sig < MARCEL_NSIG; sig++)
		if (marcel_sigismember(&cset, sig))
			sigaddset(&cthread->kcurmask, sig);
#endif

   /***************traitement des signaux************/
	if (!marcel_signandisempty(&gsigpending, &cset)) {
		ma_spin_unlock_softirq(&cthread->siglock);
		ma_spin_lock_softirq(&gsiglock);
		ma_spin_lock_softirq(&cthread->siglock);
		if (!marcel_signandisempty(&gsigpending, &cset))
			for (sig = 1; sig < MARCEL_NSIG; sig++) {
				if (marcel_signandismember(&gsigpending, &cset,
					sig)) {
					marcel_sigaddset(&cthread->sigpending,
					    sig);
					marcel_sigdelset(&gsigpending, sig);
				}
			}
		ma_spin_unlock_softirq(&gsiglock);
	}

	while (!marcel_signandisempty(&cthread->sigpending, &cset)) {
		for (sig = 1; sig < MARCEL_NSIG; sig++) {
			if (marcel_signandismember(&cthread->sigpending, &cset,
				sig)) {
				marcel_sigdelset(&cthread->sigpending, sig);
				ma_spin_unlock_softirq(&cthread->siglock);
				ma_set_current_state(MA_TASK_RUNNING);
				marcel_call_function(sig);
				ma_spin_lock_softirq(&cthread->siglock);
			}
		}
	}
	ma_spin_unlock_softirq(&cthread->siglock);

	ma_update_lwp_blocked_signals();

	LOG_RETURN(0);
}

void ma_sigexit(void) {
	/* Since it will die, we don't want this thread to be chosen for
	 * delivering signals any more */
	ma_spin_lock_softirq(&SELF_GETMEM(siglock));
	marcel_sigfillset(&SELF_GETMEM(curmask));
	ma_spin_unlock_softirq(&SELF_GETMEM(siglock));
}

int pmarcel_sigmask(int how, __const marcel_sigset_t * set,
    marcel_sigset_t * oset)
{
	LOG_IN();
	/* error checking */
	int ret = check_sigmask(how);
	if (ret)
		LOG_RETURN(ret);
	/*error checking end */

	LOG_RETURN(marcel_sigmask(how, set, oset));
}

#ifdef MA__LIBPTHREAD
int lpt_sigmask(int how, __const sigset_t * set, sigset_t * oset)
{
	LOG_IN();

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

	LOG_RETURN(ret);
}
#endif

DEF_LIBPTHREAD(int, sigmask, (int how, __const sigset_t * set, sigset_t * oset),
    (how, set, oset)) DEF___LIBPTHREAD(int, sigmask, (int how,
	__const sigset_t * set, sigset_t * oset), (how, set, oset))

/***************************sigprocmask*************************/
#ifdef MA__LIBPTHREAD
int lpt_sigprocmask(int how, __const sigset_t * set, sigset_t * oset)
{
	LOG_IN();

	if (marcel_createdthreads() != 0)
		marcel_fprintf(stderr,
		    "sigprocmask : multithreading here ! lpt_sigmask used instead\n");
	int ret = lpt_sigmask(how, set, oset);
	if (ret) {
		mdebug("lpt_sigprocmask : retour d'erreur de lpt_sigmask\n");
		errno = ret;
		LOG_RETURN(-1);
	}
	LOG_RETURN(0);
}

DEF_LIBC(int, sigprocmask, (int how, __const sigset_t * set, sigset_t * oset), (how, set, oset));
DEF___LIBC(int, sigprocmask, (int how, __const sigset_t * set, sigset_t * oset), (how, set, oset));

#endif				/*  MA__LIBPTHREAD */

/****************************sigpending**************************/
static int check_sigpending(marcel_sigset_t * set)
{
	marcel_t cthread = marcel_self();
	if (set == NULL) {
		mdebug("check_sigpending : valeur set NULL\n");
		errno = EINVAL;
		return -1;
	}
	ma_spin_lock_softirq(&cthread->siglock);
	if (&cthread->sigpending == NULL) {
		mdebug
		    ("check_sigpending : valeur de champ des signaux en suspend  NULL\n");
		errno = EINVAL;
		return -1;
	}
	ma_spin_unlock_softirq(&cthread->siglock);
	return 0;
}

DEF_MARCEL(int, sigpending, (marcel_sigset_t *set), (set),
{
	LOG_IN();

	marcel_t cthread = marcel_self();
	int sig;

#ifdef MA__DEBUG
	int ret = check_sigpending(set);
	if (ret)
		LOG_RETURN(ret);
#endif

	marcel_sigemptyset(set);
	ma_spin_lock_softirq(&gsiglock);
	ma_spin_lock_softirq(&cthread->siglock);

	for (sig = 1; sig < MARCEL_NSIG; sig++) {
		if (marcel_sigorismember(&cthread->sigpending, &gsigpending,
			sig))
			marcel_sigaddset(set, sig);
	}

	ma_spin_unlock_softirq(&cthread->siglock);
	ma_spin_unlock_softirq(&gsiglock);

	LOG_RETURN(0);
})

DEF_POSIX(int, sigpending, (marcel_sigset_t *set), (set),
{
	LOG_IN();

	marcel_t cthread = marcel_self();
	int sig;

	/* error checking */
	int ret = check_sigpending(set);
	if (ret)
		LOG_RETURN(ret);
	/* error checking end */

	marcel_sigemptyset(set);
	ma_spin_lock_softirq(&gsiglock);
	ma_spin_lock_softirq(&cthread->siglock);
	for (sig = 1; sig < MARCEL_NSIG; sig++) {
		if (marcel_sigorismember(&cthread->sigpending, &gsigpending,
			sig)) {
			MA_BUG_ON(!marcel_sigismember(&cthread->curmask, sig));
			marcel_sigaddset(set, sig);
		}
	}
	ma_spin_unlock_softirq(&cthread->siglock);
	ma_spin_unlock_softirq(&gsiglock);

	LOG_RETURN(0);
})

#ifdef MA__LIBPTHREAD
int lpt_sigpending(sigset_t *set)
{
	LOG_IN();
	int signo;
	marcel_sigset_t marcel_set;

	int ret = marcel_sigpending(&marcel_set);

	sigemptyset(set);
	for (signo = 1; signo < MARCEL_NSIG; signo++)
		if (marcel_sigismember(&marcel_set, signo))
			sigaddset(set, signo);
	LOG_RETURN(ret);
}
#endif

DEF_LIBC(int,sigpending,(sigset_t *set),(set))
DEF___LIBC(int,sigpending,(sigset_t *set),(set))

/******************************sigtimedwait*****************************/
DEF_MARCEL_POSIX(int, sigtimedwait, (const marcel_sigset_t *__restrict set, 
siginfo_t *__restrict info,const struct timespec *__restrict timeout), 
(set,info,timeout),
{
	LOG_IN();

	marcel_t cthread = marcel_self();
	int waitsig;
	int sig;

	if (set == NULL) {
		mdebug("(p)marcel_sigtimedwait : valeur set NULL\n");
		errno = EINVAL;
		LOG_RETURN(-1);
	}

	int old = __pmarcel_enable_asynccancel();

	ma_spin_lock_softirq(&gsiglock);
	ma_spin_lock_softirq(&cthread->siglock);
	for (sig = 1; sig < MARCEL_NSIG; sig++)
		if (marcel_sigismember(set, sig))
			if (marcel_sigorismember(&gsigpending,
				&cthread->sigpending, sig)) {
				if (marcel_sigismember(&cthread->sigpending,
					sig)) {
					marcel_sigdelset(&cthread->sigpending,
					    sig);
					if (info != NULL)
						*info = cthread->siginfo[sig];
				} else if (marcel_sigismember(&gsigpending,
					sig)) {
					marcel_sigdelset(&gsigpending, sig);
					if (info != NULL)
						*info = gsiginfo[sig];
				}
				ma_spin_unlock_softirq(&gsiglock);
				ma_spin_unlock_softirq(&cthread->siglock);

				LOG_RETURN(sig);
			}

	/* ajout d'un thread en attente d'un signal en début de liste */
	ma_twblocked_t wthread;

	wthread.t = cthread;

	marcel_sigemptyset(&wthread.waitset);
	marcel_sigorset(&wthread.waitset, &wthread.waitset, set);

	if (info != NULL)
		wthread.waitinfo = info;
	wthread.waitsig = &waitsig;
	cthread->waitsig = &waitsig;

	wthread.next_twblocked = list_wthreads;
	list_wthreads = &wthread;

	/* pour le thread lui-meme */
	marcel_sigemptyset(&cthread->waitset);
	marcel_sigorset(&cthread->waitset, &cthread->waitset, set);

	if (info != NULL) {
		cthread->waitinfo = info;
		waitsig = -1;
	}

	int timed = 0;
	unsigned long int waittime = MA_MAX_SCHEDULE_TIMEOUT;

	if (timeout) {
		timed = 1;
		struct timeval tv;

		/* il faut arrondir au supérieur */
		tv.tv_sec = timeout->tv_sec;
		tv.tv_usec = (timeout->tv_nsec + 999) / 1000;

		waittime =
		    ((tv.tv_sec * 1e6 + tv.tv_usec) + marcel_gettimeslice() -
		    1) / marcel_gettimeslice() + 1;
	}

	waitsig = -1;

waiting:
	ma_set_current_state(MA_TASK_INTERRUPTIBLE);
	ma_spin_unlock_softirq_no_resched(&gsiglock);
	ma_spin_unlock_softirq_no_resched(&cthread->siglock);

	waittime = ma_schedule_timeout(waittime);

	if ((waitsig == -1) && ((timed == 0) || (waittime != 0))) {
		ma_spin_lock_softirq(&gsiglock);
		ma_spin_lock_softirq(&cthread->siglock);
		goto waiting;
	}

	__pmarcel_disable_asynccancel(old);

	if ((timed == 1) && (waittime == 0)) {
		/* recherche du thread dans la liste d'attente */
		ma_twblocked_t *search_twblocked = list_wthreads;
		ma_twblocked_t *prec_twblocked = NULL;

		while (!marcel_equal(search_twblocked->t, cthread)) {
			prec_twblocked = search_twblocked;
			search_twblocked = search_twblocked->next_twblocked;
		}

		/* on l'enlève */
		if (!prec_twblocked)
			list_wthreads = search_twblocked->next_twblocked;
		else
			prec_twblocked->next_twblocked =
			    search_twblocked->next_twblocked;

		errno = EAGAIN;
		LOG_RETURN(-1);
	}
	LOG_RETURN(waitsig);
})

#ifdef MA__LIBPTHREAD
int lpt_sigtimedwait(__const sigset_t *__restrict set,siginfo_t *__restrict info,
__const struct timespec *__restrict timeout)
{
	LOG_IN();

	int sig;
	marcel_sigset_t marcel_set;
	marcel_sigemptyset(&marcel_set);
	for (sig = 1; sig < MARCEL_NSIG; sig++)
		if (sigismember(set, sig))
			marcel_sigaddset(&marcel_set, sig);

	LOG_RETURN(marcel_sigtimedwait(&marcel_set, info, timeout));
}

versioned_symbol(libpthread, lpt_sigtimedwait,
	              sigtimedwait, GLIBC_2_1);
#endif

/************************sigwaitinfo************************/
DEF_MARCEL_POSIX(int, sigwaitinfo, (const marcel_sigset_t *__restrict set, siginfo_t *__restrict info), (set,info),
{
        LOG_IN();
        LOG_RETURN(marcel_sigtimedwait(set,info,NULL));
})

#ifdef MA__LIBPTHREAD
int lpt_sigwaitinfo(__const sigset_t *__restrict set,siginfo_t *__restrict info)
{
        LOG_IN();
        LOG_RETURN(lpt_sigtimedwait(set,info,NULL));
}

versioned_symbol(libpthread, lpt_sigwaitinfo,
	              sigwaitinfo, GLIBC_2_1);
#endif

/***********************sigwait*****************************/

DEF_MARCEL_POSIX(int, sigwait, (const marcel_sigset_t *__restrict set, int *__restrict sig), (set,sig),
{
        LOG_IN();
        int ret = marcel_sigtimedwait(set,NULL,NULL);
        if (ret == -1) {
                mdebug("retour d'erreur de marcel_sigtimedwait\n");
			       LOG_RETURN(ret);
        }
        *sig = ret;
        LOG_RETURN(0);
})

#ifdef MA__LIBPTHREAD
int lpt_sigwait(__const sigset_t *__restrict set,int *__restrict sig)
{
        LOG_IN();
        siginfo_t info;
        int ret = lpt_sigtimedwait(set,&info,NULL);
        if (ret == -1) {
                mdebug("lpt_sigwait : retour d'erreur de lpt_sigtimedwait\n");
			       LOG_RETURN(ret);
        }
        *sig = ret;
        LOG_RETURN(0);
}
#endif

DEF_LIBC(int, sigwait, (__const sigset_t *__restrict set, int *__restrict sig),(set,sig));
DEF___LIBC(int, sigwait, (__const sigset_t *__restrict set, int *__restrict sig),(set,sig));

/************************distrib****************************/

/* Try to provide the signal directly to a thread running sigwait or similar */
int marcel_distribwait_sigext(siginfo_t *info)
{
	ma_spin_lock_softirq(&gsiglock);

	ma_twblocked_t *wthread = list_wthreads;
	ma_twblocked_t *prec_wthread = NULL;

	for (wthread = list_wthreads, prec_wthread = NULL;
	    wthread != NULL;
	    prec_wthread = wthread, wthread = wthread->next_twblocked) {
		if (marcel_sigismember(&wthread->waitset, info->si_signo)) {
			*wthread->waitinfo = *info;
			*wthread->waitsig = info->si_signo;

			if (prec_wthread != NULL)
				prec_wthread->next_twblocked =
				    wthread->next_twblocked;
			else	/*prec_wthread == NULL */
				list_wthreads = wthread->next_twblocked;

			ma_wake_up_state(wthread->t, MA_TASK_INTERRUPTIBLE);
			ma_spin_unlock_softirq(&gsiglock);
			return 1;
		}
	}
	ma_spin_unlock_softirq(&gsiglock);

	return 0;
}

/* Try to provide the signal directly to this thread if it is running sigwait or similar */
int marcel_distribwait_thread(siginfo_t * info, marcel_t thread)
{
	int ret = 0;
	ma_spin_lock_softirq(&thread->siglock);

	if (marcel_sigismember(&thread->waitset, info->si_signo)) {

		*thread->waitinfo = *info;
		*thread->waitsig = info->si_signo;
		marcel_sigemptyset(&thread->waitset);
		ma_wake_up_state(thread, MA_TASK_INTERRUPTIBLE);
		ret = 1;

		/* il faut enlever le thread en fin d'attente de la liste */
		ma_twblocked_t *wthread = list_wthreads;
		ma_twblocked_t *prec_wthread = NULL;

		while (thread != wthread->t) {
			prec_wthread = wthread;
			wthread = wthread->next_twblocked;
		}
		if (prec_wthread == NULL)
			list_wthreads = wthread->next_twblocked;
		else
			prec_wthread->next_twblocked = wthread->next_twblocked;
	}

	ma_spin_unlock_softirq(&thread->siglock);

	return ret;
}

/****************************sigsuspend**************************/
DEF_MARCEL_POSIX(int,sigsuspend,(const marcel_sigset_t *sigmask),(sigmask),
{
	LOG_IN();
	marcel_sigset_t oldmask;
	int old = __pmarcel_enable_asynccancel();	//pmarcel dans marcel

	/* dormir avant de changer le masque */
	ma_set_current_state(MA_TASK_INTERRUPTIBLE);
	marcel_sigmask(SIG_SETMASK, sigmask, &oldmask);
	ma_schedule();
	marcel_sigmask(SIG_SETMASK, &oldmask, NULL);
	errno = EINTR;
	__pmarcel_disable_asynccancel(old);

	LOG_RETURN(-1);
})

#ifdef MA__LIBPTHREAD
int lpt_sigsuspend(const sigset_t * sigmask)
{
	LOG_IN();
	int signo;
	marcel_sigset_t marcel_set;
	marcel_sigemptyset(&marcel_set);
	for (signo = 1; signo < MARCEL_NSIG; signo++)
		if (sigismember(sigmask, signo))
			marcel_sigaddset(&marcel_set, signo);
	LOG_RETURN(marcel_sigsuspend(&marcel_set));
}
#endif

DEF_LIBC(int, sigsuspend, (const sigset_t * sigmask), (sigmask));
DEF___LIBC(int, sigsuspend, (const sigset_t * sigmask), (sigmask));

/***********************sigjmps******************************/
#ifdef MA__LIBPTHREAD
#if LINUX_SYS
int ma_savesigs(sigjmp_buf env, int savemask)
{
#ifdef MA__DEBUG
	if (env == NULL) {
		mdebug("ma_savesigs : valeur env invalide NULL\n");
		errno = EINVAL;
		return -1;
	}
#endif
	env->__mask_was_saved = savemask;

	if (savemask != 0) {
		sigset_t set;
		sigemptyset(&set);
		lpt_sigmask(SIG_BLOCK, &set, &env->__saved_mask);
	}
	return 0;
}
extern void __libc_longjmp(sigjmp_buf env, int val);
DEF_MARCEL_POSIX(void, siglongjmp, (sigjmp_buf env, int val), (env,val),
{
	LOG_IN();

	marcel_t cthread = marcel_self();

	if (env == NULL) {
		mdebug("(p)marcel_siglongjump : valeur env NULL\n");
		errno = EINVAL;
	}

	if (env->__mask_was_saved)
		lpt_sigmask(SIG_SETMASK, &env->__saved_mask, NULL);

	if (cthread->delivering_sig) {
		cthread->delivering_sig = 0;
		marcel_deliver_sig();
	}
	// TODO: vérifier qu'on longjumpe bien dans le même thread !!

#if defined(X86_ARCH) || defined(X86_64_ARCH)
	/* We define our own sigsetjmp using our own setjmp function, so
	 * use our own longjmp.  */
	longjmp(env->__jmpbuf, val);
#else
	__libc_longjmp(env, val);
#endif

	LOG_OUT();
})

DEF_C(void,siglongjmp,(sigjmp_buf env, int val),(env,val))
DEF___C(void,siglongjmp,(sigjmp_buf env, int val),(env,val))
#undef longjmp
#ifdef MA__LIBPTHREAD
weak_alias (siglongjmp, longjmp);
#endif
#endif // LINUX_SYS
#endif

#ifndef OSF_SYS
/***************************signal********************************/
DEF_MARCEL_POSIX(void *,signal,(int sig, void * handler),(sig,handler),
{
	LOG_IN();

	struct marcel_sigaction act;
	struct marcel_sigaction oact;

	/* Check signal extents to protect __sigismember.  */
	if (handler == SIG_ERR || sig < 0 || sig >= MARCEL_NSIG) {
		mdebug("(p)marcel_signal : handler (%p) ou sig (%d) invalide\n",
		    handler, sig);
		errno = EINVAL;
		LOG_RETURN(SIG_ERR);
	}

	act.marcel_sa_handler = handler;
	marcel_sigemptyset(&act.marcel_sa_mask);
	marcel_sigaddset(&act.marcel_sa_mask, sig);

	act.marcel_sa_flags = SA_RESTART | SA_NODEFER;
	//attention au siginfo. NODEFER est juste là pour optimiser: deliver_sig n'a pas besoin de re-ajouter sig à sa_mask
	if (marcel_sigaction(sig, &act, &oact) < 0) {
		LOG_RETURN(SIG_ERR);
	}

	LOG_RETURN(oact.marcel_sa_handler);
})

#ifdef MA__LIBPTHREAD
void *lpt___sysv_signal(int sig, void * handler)
{
	LOG_IN();

	struct marcel_sigaction act;
	struct marcel_sigaction oact;

	/* Check signal extents to protect __sigismember.  */
	if (handler == SIG_ERR || sig < 0 || sig >= MARCEL_NSIG) {
		mdebug
		    ("lpt___sysv_signal : handler (%p) ou sig (%d) invalide\n",
		    handler, sig);
		errno = EINVAL;
		LOG_RETURN(SIG_ERR);
	}

	act.marcel_sa_handler = handler;
	marcel_sigemptyset(&act.marcel_sa_mask);
	marcel_sigaddset(&act.marcel_sa_mask, sig);

	act.marcel_sa_flags = SA_RESETHAND | SA_NODEFER;
	//attention au siginfo. NODEFER est juste là pour optimiser: deliver_sig n'a pas besoin de re-ajouter sig à sa_mask
	if (marcel_sigaction(sig, &act, &oact) < 0) {
		LOG_RETURN(SIG_ERR);
	}

	LOG_RETURN(oact.marcel_sa_handler);
}
#endif

#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, pmarcel_signal, signal, GLIBC_2_0);
#endif
DEF___C(void *,signal,(int sig, void * handler),(sig,handler))

DEF_LIBC(void *,__sysv_signal,(int sig, void * handler),(sig,handler))
DEF___LIBC(void *,__sysv_signal,(int sig, void * handler),(sig,handler))
#endif

#ifndef OSF_SYS
/***************************sigaction*****************************/
DEF_MARCEL_POSIX(int,sigaction,(int sig, const struct marcel_sigaction *act,
     struct marcel_sigaction *oact),(sig,act,oact),
{
	LOG_IN();
	ma_kernel_sigaction_t kact;
	void *handler;
	void *ohandler;

	if ((sig < 1) || (sig >= MARCEL_NSIG)) {
		mdebug
		    ("(p)marcel_sigaction : valeur sig (%d) invalide, mais renvoyons 0 quand même\n",
		    sig);
		//errno = EINVAL;
		LOG_RETURN(0);
	}

	if (sig == MARCEL_RESCHED_SIGNAL)
		marcel_fprintf(stderr, "!! warning: signal %d not supported\n", sig);

	if (!act) {
		/* Just reading the current sigaction */
		marcel_rwlock_rdlock(&rwlock);
		if (oact != NULL)
			*oact = csigaction[sig];
		marcel_rwlock_unlock(&rwlock);
		LOG_RETURN(0);
	}

	/* Really writing */
#ifdef SA_SIGINFO
	if (act->marcel_sa_flags & SA_SIGINFO)
		handler = act->marcel_sa_sigaction;
	else
#endif
		handler = act->marcel_sa_handler;

	if (handler == SIG_IGN) {
		if ((sig == SIGKILL) || (sig == SIGSTOP)) {
			mdebug("!! signal %d ne peut etre ignore\n", sig);
			errno = EINVAL;
			LOG_RETURN(-1);
		}
	} else if (handler != SIG_DFL)
		if ((sig == SIGKILL) || (sig == SIGSTOP)) {
			mdebug("!! signal %d ne peut etre capture\n", sig);
			errno = EINVAL;
			LOG_RETURN(-1);
		}

	/* Serialize sigaction operations */
	marcel_rwlock_wrlock(&rwlock);

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
		marcel_rwlock_unlock(&rwlock);
		LOG_RETURN(0);
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
		kact.sa_sigaction = handler;
#else
		kact.sa_handler = handler;
#endif
	else
#ifdef SA_SIGINFO
		kact.sa_sigaction = marcel_pidkill;
#else
		kact.sa_handler = marcel_pidkill;
#endif
	kact.sa_flags = SA_NODEFER |
#ifdef SA_SIGINFO
	    SA_SIGINFO |
#endif
	    (act->marcel_sa_flags & SA_RESTART);

	if (ma_kernel_sigaction(sig, &kact, NULL) == -1) {
		mdebug("!! syscall sigaction rate -> sig : %d\n", sig);
		mdebug("!! sortie erreur de marcel_sigaction\n");
		LOG_RETURN(-1);
	}

	/* After changing the kernel handler, we can unblock the signal on LWPs
	 * that should */
	if (ohandler == SIG_DFL && handler != SIG_DFL) {
		marcel_sigdelset(&ma_dfl_sigs, sig);
		sigdelset(&ma_dfl_ksigs, sig);
		update_lwps_blocked_signals(0);
	}

	marcel_rwlock_unlock(&rwlock);

	LOG_RETURN(0);
})

#ifdef MA__LIBPTHREAD
int lpt_sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	LOG_IN();

	int signo;
	int ret;
	struct marcel_sigaction marcel_act;
	struct marcel_sigaction marcel_oact;

	/* changement de structure act en marcel_act */
	if (act) {
		marcel_act.marcel_sa_flags = act->sa_flags;
#ifdef SA_SIGINFO
		if (act->sa_flags & SA_SIGINFO)
			marcel_act.marcel_sa_sigaction = act->sa_sigaction;
		else
#endif
			marcel_act.marcel_sa_handler = act->sa_handler;

		/* a cause du passage de sigset_t a marcel_sigset_t */
		marcel_sigemptyset(&marcel_act.marcel_sa_mask);
		for (signo = 1; signo < MARCEL_NSIG; signo++)
			if (sigismember(&act->sa_mask, signo))
				marcel_sigaddset(&marcel_act.marcel_sa_mask,
				    signo);
	}

	/* appel de marcel_sigaction */
	ret =
	    marcel_sigaction(sig, act ? &marcel_act : NULL,
	    oact ? &marcel_oact : NULL);

	/* on rechange tout a l'envers */
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

	LOG_RETURN(ret);
}
#endif

DEF_LIBC(int,sigaction,(int sig, const struct sigaction *act,
                     struct sigaction *oact),(sig,act,oact))
DEF___LIBC(int,sigaction,(int sig, const struct sigaction *act,
                     struct sigaction *oact),(sig,act,oact))
#endif

/***********************marcel_deliver_sig**********************/

/* Called in the context of a thread (either directly from sigmask or via
 * deviation when it should deliver some signals */
int marcel_deliver_sig(void)
{
	int sig;
	marcel_t cthread = marcel_self();

	ma_spin_lock_softirq(&cthread->siglock);

	cthread->delivering_sig = 1;
	cthread->restart_deliver_sig = 0;

deliver:
	for (sig = 1; sig < MARCEL_NSIG; sig++)
		if (marcel_sigismember(&cthread->sigpending, sig)) {
			if (!marcel_sigismember(&cthread->curmask, sig)) {
				/* signal non bloqué */
				marcel_sigdelset(&cthread->sigpending, sig);
				ma_spin_unlock_softirq(&cthread->siglock);
				ma_set_current_state(MA_TASK_RUNNING);
				marcel_call_function(sig);
				ma_spin_lock_softirq(&cthread->siglock);
			}
		}
	/* on regarde s'il reste des signaux à délivrer */
	if (cthread->restart_deliver_sig) {
		cthread->restart_deliver_sig = 0;
		goto deliver;
	}
	cthread->delivering_sig = 0;
	ma_spin_unlock_softirq(&cthread->siglock);

	LOG_RETURN(0);
}

/*************************marcel_call_function******************/

/* Actually deliver the signal: ignore if SIG_IGN, raise the signal to the
 * kernel if SIG_DFL, else just call the function */
int marcel_call_function(int sig)
{
	LOG_IN();

	if ((sig < 0) || (sig >= MARCEL_NSIG)) {
		mdebug("marcel_call_function : valeur sig (%d) invalide\n", sig);
		errno = EINVAL;
		LOG_RETURN(-1);
	}

	struct marcel_sigaction cact;
	marcel_rwlock_rdlock(&rwlock);
	cact = csigaction[sig];
	marcel_rwlock_unlock(&rwlock);

	void (*handler) (int);

	#ifdef SA_SIGINFO
	if ((cact.marcel_sa_flags & SA_SIGINFO))
		handler = (typeof(handler)) cact.marcel_sa_sigaction;
	else
	#endif
		handler = cact.marcel_sa_handler;

	   /* le signal courant doit etre masqué sauf avec SA_NODEFER */
	if (!(cact.marcel_sa_flags & SA_NODEFER))	//==0
		marcel_sigaddset(&cact.marcel_sa_mask, sig);

	if (handler == SIG_IGN)
		LOG_RETURN(0);

	if (handler == SIG_DFL) {
		if (kill(getpid(), sig) == -1)
			LOG_RETURN(-1);
		LOG_RETURN(0);
	}

	marcel_sigset_t oldmask;
	marcel_sigmask(SIG_BLOCK, &cact.marcel_sa_mask, &oldmask);

	if (!(cact.marcel_sa_flags & SA_RESTART))
		SELF_GETMEM(interrupted) = 1;

	      /* appel de la fonction */
	#ifdef SA_SIGINFO
	if ((cact.marcel_sa_flags & SA_SIGINFO)) {
		void (*action) (int sig, siginfo_t * info, void *uc) = (void *) handler;
		action(sig, &(marcel_self()->siginfo[sig]), NULL);
	} else
	#endif
		handler(sig);

	   /* on remet l'ancien mask */
	marcel_sigmask(SIG_SETMASK, &oldmask, NULL);

	LOG_RETURN(0);
}

/*******************ma_signals_init***********************/
__marcel_init void ma_signals_init(void)
{
	LOG_IN();
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
	LOG_OUT();
}

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

/****************autres fonctions******************/
DEF_MARCEL_POSIX(int,sighold,(int sig),(sig),
{
	LOG_IN();
	marcel_sigset_t set;
	marcel_sigemptyset(&set);
	marcel_sigaddset(&set, sig);
	int ret = marcel_sigmask(SIG_BLOCK, &set, NULL);
	if (ret) {
		errno = ret;
		LOG_RETURN(-1);
	}
	LOG_RETURN(0);
})

#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, pmarcel_sighold,
	              sighold, GLIBC_2_1);
#endif

DEF_MARCEL_POSIX(int,sigrelse,(int sig),(sig),
{
        LOG_IN();
        marcel_sigset_t set;
        marcel_sigemptyset(&set);
        marcel_sigaddset(&set,sig);
        int ret = marcel_sigmask(SIG_UNBLOCK,&set,NULL);
        if (ret) {
                errno = ret;
                LOG_RETURN(-1);
        }
        LOG_RETURN(0);
})

#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, pmarcel_sigrelse,
	              sigrelse, GLIBC_2_1);
#endif

#ifndef OSF_SYS
DEF_MARCEL_POSIX(int,sigignore,(int sig),(sig),
{
        LOG_IN();
        struct marcel_sigaction act;
        act.marcel_sa_handler = SIG_IGN;
        LOG_RETURN(marcel_sigaction(sig,&act,NULL));
})

#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, pmarcel_sigignore,
	              sigignore, GLIBC_2_1);
#endif
#endif

DEF_MARCEL_POSIX(int,sigpause,(int sig),(sig),
{
        LOG_IN();
        marcel_sigset_t set;
        marcel_sigemptyset(&set);
        marcel_sigaddset(&set,sig);
        int ret = marcel_sigsuspend(&set);
        if (ret != -1)
                mdebug("(p)marcel_sigpause : marcel_sigsuspend ne retourne pas -1\n");
        errno = EINTR;
        LOG_RETURN(-1);
})


#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, pmarcel_sigpause,
	              sigpause, GLIBC_2_0);
#endif

#ifndef OSF_SYS
void (*pmarcel_sigset(int sig,void (*dispo)(int)))
{
        LOG_IN();
        struct marcel_sigaction act,oact;
        act.marcel_sa_flags = 0; //TODO : vérifier quels flags il faut mettre
        act.marcel_sa_handler = dispo;

        marcel_sigset_t set,oset;
        marcel_sigemptyset(&set);

        if (dispo == SIG_HOLD)
                marcel_sigmask(SIG_BLOCK,&set,&oset);
        else
                marcel_sigmask(SIG_UNBLOCK,&set,&oset);
  
        int  ret = marcel_sigaction(sig,&act,&oact);
        if (ret != 0) {
                mdebug("pmarcel_sigset : sigaction ne retourne pas 0\n");
                LOG_RETURN(SIG_ERR);         }
        LOG_RETURN((marcel_sigismember(&oset,sig)?SIG_HOLD:oact.marcel_sa_handler));
}

#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, pmarcel_sigset,
	              sigset, GLIBC_2_1);
#endif
#endif

static void signal_init(ma_lwp_t lwp) {
	marcel_sigemptyset(&lwp->curmask);
}

static void signal_start(ma_lwp_t lwp) {
}

MA_DEFINE_LWP_NOTIFIER_START(signal, "signal",
			     signal_init, "Initialize signal masks",
			     signal_start, "Start signal handling");
MA_LWP_NOTIFIER_CALL_UP_PREPARE(signal, MA_INIT_MAIN_LWP);
MA_LWP_NOTIFIER_CALL_ONLINE(signal, MA_INIT_MAIN_LWP);

#endif
