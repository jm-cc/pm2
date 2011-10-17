/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __MARCEL_SIGNAL_H__
#define __MARCEL_SIGNAL_H__


#include <signal.h>
#include <setjmp.h>
#include <sys/time.h>
#include "sys/os/marcel_signal.h"
#include "marcel_config.h"
#include "sys/marcel_alias.h"
#include "sys/marcel_types.h"



/** Public macros **/
#define MARCEL_NSIG 32

/* Define EMPTY and FULL signal set */
#define MARCEL_SIGSET_EMPTY 0
#define MARCEL_SIGSET_FULL (0xffffffffUL - marcel_sigoneset(SIGKILL) - marcel_sigoneset(SIGSTOP))

/* Return a mask that includes the bit for SIG only.  */
#define marcel_sigoneset(sig) \
	(1UL << ((sig) - 1))

#define marcel_sigemptyset(set)						\
	do {								\
		*(set) = MARCEL_SIGSET_EMPTY;				\
	} while (0);

#define marcel_sigfillset(set)			\
	do {					\
		*(set) = MARCEL_SIGSET_FULL;	\
	} while (0);

#define marcel_sigisemptyset(set)			\
	({ *(set) == MARCEL_SIGSET_EMPTY; })

#define marcel_sigisfullset(set)		\
	({ *(set) == MARCEL_SIGSET_FULL; })

#define marcel_sigismember(set,sig)			\
	({ unsigned int mask = marcel_sigoneset(sig);	\
	   ((*(set)&mask) ? 1 : 0); })

#define marcel_sigaddset(set,sig)				\
	do {							\
		unsigned int mask = marcel_sigoneset(sig);	\
		marcel_sigset_t *__set = (set);			\
		*__set = *__set|mask;				\
	} while (0);

#define marcel_sigdelset(set,sig)				\
	do {							\
		unsigned int mask = marcel_sigoneset(sig);	\
		marcel_sigset_t *__set = (set);			\
		if ((*__set&mask) == mask)			\
			*__set = *__set&~mask;			\
	} while (0);

#define marcel_signotset(dest,set)		\
	do {					\
		*(dest) = ~*(set);		\
	} while (0);

#define marcel_sigandset(dest, left, right)      \
	do {					 \
		*(dest) = *(left)&*(right);	 \
	} while (0);

#define marcel_sigorset(dest, left, right)	\
	do {					\
		*(dest) = *(left)|*(right);	\
	} while (0);

#define marcel_sigorismember(left,right,sig)  \
	({ marcel_sigset_t aura;			\
	   marcel_sigorset(&aura,(left),(right));	\
	   marcel_sigismember(&aura,(sig)); })

#define marcel_signandset(dest, left, right)			\
	do {							\
		marcel_sigset_t notright;			\
		marcel_signotset(&notright,(right));		\
		marcel_sigandset((dest), (left), &(notright));	\
	} while (0);

#define marcel_signandisempty(left,right)     \
	({ marcel_sigset_t nand;	      \
	   marcel_signandset(&nand,(left),(right));	\
	   marcel_sigisemptyset(&nand); })

#define marcel_signandismember(left,right,sig)      \
	({ marcel_sigset_t nand;                    \
	   marcel_signandset(&nand,(left),(right));	\
	   marcel_sigismember(&nand,(sig)); })

#define marcel_sigequalset(left,right)        \
	(*(left)==*(right))

#define marcel_sig_omnislash_ismember(feu,glace,foudre,sig) \
({						    \
	marcel_sigset_t antifeu;			    \
	marcel_sigset_t antifoudre;			    \
	marcel_signotset(&antifeu,(feu));		    \
	marcel_signotset(&antifoudre,(foudre));		    \
	marcel_sigset_t rufus;				    \
	marcel_sigset_t sephiroth;				\
	marcel_sigandset(&rufus,(glace),&antifoudre);		\
	marcel_sigandset(&sephiroth,&antifeu,&rufus);		\
	marcel_sigismember(&sephiroth,sig);			\
})
#define marcel_sigomnislash     marcel_sig_omnislash_ismember


/** Public data structures **/
struct marcel_sigaction {
	void (*marcel_sa_handler) (int);
	marcel_sigset_t marcel_sa_mask;
	int marcel_sa_flags;
#ifdef SA_SIGINFO
	void (*marcel_sa_sigaction) (int, siginfo_t *, void *);
#endif
};


/** Public functions **/
typedef void (*ma_sighandler_t) (int);

DEC_MARCEL(int, pause, (void) __THROW);

DEC_MARCEL(unsigned int, alarm, (unsigned int nb_sec));

DEC_MARCEL(int, getitimer, (int which, struct itimerval * value));

DEC_MARCEL(int, setitimer,
	   (int which, const struct itimerval * value, struct itimerval * ovalue));

DEC_MARCEL(int, kill, (marcel_t thread, int sig) __THROW);

DEC_MARCEL(int, sigqueue, (marcel_t *thread, int sig, union sigval value) __THROW);

DEC_MARCEL(int, raise, (int sig) __THROW);

DEC_MARCEL(int, sigmask, (int how, __const marcel_sigset_t * set, marcel_sigset_t * oset) __THROW);
DEC_LPT(int, sigmask, (int how, __const sigset_t * set, sigset_t * oset))

DEC_MARCEL(int, sigpending, (marcel_sigset_t * set) __THROW);
DEC_LPT(int, sigpending, (sigset_t * set))

DEC_MARCEL(int, sigtimedwait,
	   (const marcel_sigset_t * __restrict set, siginfo_t * __restrict info,
	    const struct timespec * __restrict timeout) __THROW);
DEC_LPT(int, sigtimedwait, (__const sigset_t * __restrict set, 
			    siginfo_t * __restrict info,
			    __const struct timespec *__restrict timeout))


DEC_MARCEL(int, sigwait,
	   (const marcel_sigset_t * __restrict set, int *__restrict sig) __THROW);
DEC_LPT(int, sigwait, (const sigset_t * __restrict set, int *__restrict sig) __THROW)

DEC_MARCEL(int, sigwaitinfo,
	   (const marcel_sigset_t * __restrict set, siginfo_t * __restrict info) __THROW);
DEC_LPT(int, sigwaitinfo, (__const sigset_t * __restrict set, siginfo_t * __restrict info))

DEC_MARCEL(int, sigsuspend, (const marcel_sigset_t * sigmask) __THROW);
DEC_LPT(int, sigsuspend, (const sigset_t * sigmask))

DEC_MARCEL(int TBX_RETURNS_TWICE, sigsetjmp, (sigjmp_buf env, int savemask) __THROW);

DEC_MARCEL(void TBX_NORETURN, siglongjmp, (sigjmp_buf env, int val) __THROW);

DEC_MARCEL(void *, signal, (int sig, void *handler) __THROW);

DEC_MARCEL(int, sigaction, (int sig, const struct marcel_sigaction * act,
			    struct marcel_sigaction * oact) __THROW);
DEC_LPT(int, sigaction, (int sig, const struct sigaction *act, struct sigaction *oact))


DEC_LPT(int, sigprocmask, (int how, __const sigset_t * set, sigset_t * oset))
DEC_LPT(void*, __sysv_signal, (int sig, void * handler))

DEC_MARCEL(int, sighold, (int sig) __THROW);

DEC_MARCEL(int, sigrelse, (int sig) __THROW);

DEC_MARCEL(int, sigignore, (int sig) __THROW);

int marcel___sigpause(int sig_or_mask, int is_sig);
DEC_MARCEL(int, sigpause, (int mask) __THROW);
 
DEC_MARCEL(int, xpg_sigpause, (int sig) __THROW);

void (*pmarcel_sigset(int sig, void (*dispo) (int)));

int marcel_deliver_sig(void);

int marcel_call_function(int sig);

int marcel_distribwait_sigext(siginfo_t * info);

int marcel_distribwait_thread(siginfo_t * info, marcel_t thread);

/**************************marcel_sigset_t*********************/
void marcel_testset(__const marcel_sigset_t * set, char *what);


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#if !defined(MA__LIBPTHREAD) || !defined(MARCEL_SIGNALS_ENABLED)
typedef struct sigaction ma_sigaction_t;
#define ma_sigaction(num, act, oact) sigaction(num, act, oact)
#define ma_sigprocmask(how, nset, oset) sigprocmask(how, nset, oset)
#define ma_kill(pid, sig) kill(pid, sig)
#define ma_sigsuspend(mask) sigsuspend(mask)
#define ma_setitimer(which,val,oval) setitimer(which, val, oval)
#define ma_signal(sig, handler) signal(sig, handler)
#endif

#ifdef MARCEL_SIGNALS_ENABLED
#define MA_GET_INTERRUPTED() SELF_GETMEM(interrupted)
#define MA_SET_INTERRUPTED(v) SELF_SETMEM(interrupted,(v))
#else
#define MA_GET_INTERRUPTED() 0
#define MA_SET_INTERRUPTED(v) (void)0
#endif


/** Internal data types **/
typedef struct ma_twblocked_struct ma_twblocked_t;


/** Internal data structures **/
struct ma_twblocked_struct {
	marcel_t t;
	struct ma_twblocked_struct *next_twblocked;
};


/** Internal global variable **/
#ifdef MARCEL_SIGNALS_ENABLED
extern sigset_t ma_thr_ksigmask;
#endif


/** Internal functions **/
void ma_update_lwp_blocked_signals(void);
int  ma_distribwait_sigext(siginfo_t * info);
#ifndef MARCEL_SIGNALS_ENABLED
#define ma_update_lwp_blocked_signals() ((void)0)
#endif

/* Called on thread termination */
void ma_signals_init(void);
int ma_savesigs(sigjmp_buf env, int savemask);
void ma_sigexit(void);
#ifndef MARCEL_SIGNALS_ENABLED
#define ma_sigexit() ((void)0)
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_SIGNAL_H__ **/
