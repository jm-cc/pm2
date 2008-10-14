
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

#section marcel_types
typedef struct ma_twblocked_struct ma_twblocked_t;

#section marcel_structures
struct ma_twblocked_struct {
	marcel_t t;
	marcel_sigset_t waitset;
	int *waitsig;
	siginfo_t *waitinfo;
	struct ma_twblocked_struct *next_twblocked;
};

#section structures
struct marcel_sigaction {
	void (*marcel_sa_handler) (int);
	marcel_sigset_t marcel_sa_mask;
	int marcel_sa_flags;
	void (*marcel_sa_sigaction) (int, siginfo_t *, void *);
};

#section marcel_macros
#define MARCEL_NSIG 32

#if defined(MA__LIBPTHREAD) && defined(MARCEL_SIGNALS_ENABLED)
#ifdef LINUX_SYS
#include <sys/syscall.h>
#include <unistd.h>
/*  L'interface noyau de linux pour sigaction n'est _pas_ la même que cette de la glibc ! */
#ifndef SA_RESTORER
#define SA_RESTORER 0x04000000
#endif
#ifdef X86_64_ARCH
void __ma_restore_rt(void);
#define MA_KERNEL_SIGACTION_RESTORER \
	if (_act) { \
		_act->sa_flags |= SA_RESTORER; \
		_act->sa_restorer = &__ma_restore_rt; \
	}
#else
#define MA_KERNEL_SIGACTION_RESTORER
#endif
#define ma_kernel_sigaction(num, act, oact) ({ \
	ma_kernel_sigaction_t *_act = (act); \
	MA_KERNEL_SIGACTION_RESTORER; \
	syscall(SYS_rt_sigaction, num, _act, oact, _NSIG / 8); \
})
typedef struct ma_kernel_sigaction {
      union {
#undef sa_handler
            __sighandler_t sa_handler;
#define sa_handler __sigaction_handler.sa_handler
#undef sa_sigaction
            void (*sa_sigaction) (int, siginfo_t *, void *);
#define sa_sigaction __sigaction_handler.sa_sigaction
      } __sigaction_handler;
      unsigned long sa_flags;
      void (*sa_restorer) (void);
      sigset_t sa_mask;
} ma_kernel_sigaction_t;
#else
#error Need to know how to send signals directly to kernel
#endif
#else
typedef struct sigaction ma_kernel_sigaction_t;
#define ma_kernel_sigaction(num, act, oact) sigaction(num, act, oact)
#endif

#section functions
#depend "marcel_alias.h[macros]"
#include <signal.h>
#include <setjmp.h>
typedef void (*ma_sighandler_t)(int);

int pmarcel_pause(void);
DEC_MARCEL_POSIX(int,pause,(void) __THROW);

unsigned int pmarcel_alarm(unsigned int nb_sec);
DEC_MARCEL_POSIX(unsigned int, alarm,(unsigned int nb_sec));

int pmarcel_getitimer(int which, struct itimerval *value);
DEC_MARCEL_POSIX(int, getitimer, (int which, struct itimerval *value));

int pmarcel_setitimer(int which, const struct itimerval *value, struct itimerval *ovalue);
DEC_MARCEL_POSIX(int, setitimer, (int which, const struct itimerval *value, struct itimerval *ovalue));

int pmarcel_kill(pmarcel_t thread, int sig);
DEC_MARCEL_POSIX(int, kill, (marcel_t thread, int sig) __THROW);

int pmarcel_raise(int sig);
DEC_MARCEL_POSIX(int, raise, (int sig) __THROW);

int pmarcel_sigmask(int how, __const marcel_sigset_t *set, marcel_sigset_t *oset);
DEC_MARCEL_POSIX(int, sigmask, (int how, __const marcel_sigset_t *set, marcel_sigset_t *oset) __THROW);

int pmarcel_sigpending(marcel_sigset_t *set);
DEC_MARCEL_POSIX(int, sigpending, (marcel_sigset_t *set) __THROW);

int pmarcel_sigtimedwait(const marcel_sigset_t *__restrict set, siginfo_t *__restrict info,const struct timespec *__restrict timeout);

DEC_MARCEL_POSIX(int, sigtimedwait, (const marcel_sigset_t *__restrict set, siginfo_t *__restrict info,const struct timespec *__restrict timeout) __THROW);

int pmarcel_sigwait(const marcel_sigset_t *__restrict set, int *__restrict sig);
DEC_MARCEL_POSIX(int, sigwait, (const marcel_sigset_t *__restrict set, int *__restrict sig) __THROW);

int pmarcel_sigwaitinfo(const marcel_sigset_t *__restrict set, siginfo_t *__restrict info);

DEC_MARCEL_POSIX(int, sigwaitinfo, (const marcel_sigset_t *__restrict set, siginfo_t *__restrict info) __THROW);

int pmarcel_sigsuspend(const marcel_sigset_t *sigmask);
DEC_MARCEL_POSIX(int,sigsuspend, (const marcel_sigset_t *sigmask) __THROW);

int TBX_RETURNS_TWICE pmarcel_sigsetjmp(sigjmp_buf env, int savemask);
DEC_MARCEL_POSIX(int TBX_RETURNS_TWICE, sigsetjmp, (sigjmp_buf env, int savemask) __THROW);

void TBX_NORETURN pmarcel_siglongjmp(sigjmp_buf env, int val);
DEC_MARCEL_POSIX(void TBX_NORETURN, siglongjmp, (sigjmp_buf env, int val) __THROW);

void *pmarcel_signal(int sig, void *handler);
DEC_MARCEL_POSIX(void *,signal,(int sig, void *handler) __THROW);

#ifdef OSF_SYS
/* Hack around gcc's signal.h stupid #define sigaction _Esigaction */
#undef sigaction
#define sigaction(a,b,c) _Esigaction(a,b,c)
#endif

int pmarcel_sigaction(int sig, const struct marcel_sigaction *act,struct marcel_sigaction *oact);
DEC_MARCEL_POSIX(int,sigaction,(int sig, const struct marcel_sigaction *act,
                     struct marcel_sigaction *oact) __THROW);

int marcel_deliver_sig(void);
int marcel_call_function(int sig);
int marcel_distribwait_sigext(siginfo_t *info);
int marcel_distribwait_thread(siginfo_t *info,marcel_t thread);
void ma_signals_init(void);

/**************************marcel_sigset_t*********************/

#section types
typedef unsigned int marcel_sigset_t, pmarcel_sigset_t;

#section marcel_macros

/* Return a mask that includes the bit for SIG only.  */
#define ma_sigmask(sig) \
  (1UL << ((sig) - 1))

#define marcel_sigemptyset(set) \
  ({ *(set) = 0; \
     0;})

#define MARCEL_SIGSET_FULL (0xffffffffUL - ma_sigmask(SIGKILL) - ma_sigmask(SIGSTOP))

#define marcel_sigfillset(set) \
  ({ *(set) = MARCEL_SIGSET_FULL;    \
     0; })

#define marcel_sigisemptyset(set)          \
  ({ (*(set) == 0); })

#define marcel_sigisfullset(set)           \
  ({ (*(set) == MARCEL_SIGSET_FULL); })

#define marcel_sigismember(set,sig)        \
  ({ unsigned int mask = ma_sigmask(sig);  \
    ((*(set)&mask) ? 1 : 0); })

#define marcel_sigaddset(set,sig)          \
  ({ unsigned int mask = ma_sigmask(sig);  \
     marcel_sigset_t *__set = (set);       \
     *__set = *__set|mask;                 \
     0; })
     
#define marcel_sigdelset(set,sig)          \
  ({ unsigned int mask = ma_sigmask(sig);  \
     marcel_sigset_t *__set = (set);       \
     if ((*__set&mask) == mask)            \
         *__set = *__set&~mask;            \
     0; })


#define marcel_signotset(dest,set)         \
  ({ *(dest) = ~*(set);                    \
     0; })

#define marcel_sigandset(dest, left, right)      \
  ({ *(dest) = *(left)&*(right);           \
     0; })

#define marcel_sigorset(dest, left, right) \
  ({ *(dest) = *(left)|*(right);           \
     0; })

#define marcel_sigorismember(left,right,sig)  \
  ({ marcel_sigset_t aura;                    \
     marcel_sigorset(&aura,(left),(right));   \
     marcel_sigismember(&aura,(sig)); })

#define marcel_signandset(dest, left, right)       \
  ({ marcel_sigset_t notright;                     \
     marcel_signotset(&notright,(right));          \
           marcel_sigandset((dest), (left), &(notright)); \
     0; }) 

#define marcel_signandisempty(left,right)     \
  ({ marcel_sigset_t nand;                    \
     marcel_signandset(&nand,(left),(right)); \
     marcel_sigisemptyset(&nand); })

#define marcel_signandismember(left,right,sig)      \
  ({ marcel_sigset_t nand;                    \
     marcel_signandset(&nand,(left),(right)); \
     marcel_sigismember(&nand,(sig)); })

#define marcel_sigequalset(left,right)        \
  (*(left)==*(right)) 

#define marcel_sig_omnislash_ismember(feu,glace,foudre,sig) \
  ({ marcel_sigset_t antifeu;                      \
     marcel_sigset_t antifoudre;                   \
     marcel_signotset(&antifeu,(feu));             \
     marcel_signotset(&antifoudre,(foudre));       \
     marcel_sigset_t rufus;                        \
     marcel_sigset_t sephiroth;                    \
     marcel_sigandset(&rufus,(glace),&antifoudre); \
     marcel_sigandset(&sephiroth,&antifeu,&rufus); \
     marcel_sigismember(&sephiroth,sig);           \
   })

#define marcel_sigomnislash    marcel_sig_omnislash_ismember

#define pmarcel_sigemptyset     marcel_sigemptyset
#define pmarcel_sigfillset      marcel_sigfillset
#define pmarcel_sigismember     marcel_sigismember
#define pmarcel_sigaddset       marcel_sigaddset
#define pmarcel_sigisemptyset   marcel_sigisemptyset
#define pmarcel_sigdelset       marcel_sigdelset
#define pmarcel_signotset       marcel_signotset
#define pmarcel_sigandset       marcel_sigandset
#define pmarcel_sigorset        marcel_sigorset
#define pmarcel_sigorismember   marcel_sigorismember
#define pmarcel_signandset      marcel_signandset
#define pmarcel_signandisempty  marcel_signandisempty
#define pmarcel_signandismember marcel_signandismember
#define pmarcel_sigequalset     marcel_sigequalset

#ifdef MARCEL_SIGNALS_ENABLED
#define MA_GET_INTERRUPTED() SELF_GETMEM(interrupted)
#define MA_SET_INTERRUPTED(v) SELF_SETMEM(interrupted,(v))
#else
#define MA_GET_INTERRUPTED() 0
#define MA_SET_INTERRUPTED(v) (void)0
#endif

#section functions
void marcel_testset(__const marcel_sigset_t * set,char *what);

/****************autres fonctions******************/
int pmarcel_sighold(int sig);
DEC_MARCEL_POSIX(int,sighold,(int sig) __THROW);

int pmarcel_sigrelse(int sig);
DEC_MARCEL_POSIX(int,sigrelse,(int sig) __THROW);

int pmarcel_sigignore(int sig);
DEC_MARCEL_POSIX(int,sigignore,(int sig) __THROW);

int pmarcel_sigpause(int sig);
DEC_MARCEL_POSIX(int,sigpause,(int sig) __THROW);

void (*pmarcel_sigset(int sig,void (*dispo)(int)));
