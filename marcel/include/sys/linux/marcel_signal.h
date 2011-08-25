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


#ifndef __SYS_LINUX_MARCEL_SIGNAL_H__
#define __SYS_LINUX_MARCEL_SIGNAL_H__


#include <sys/syscall.h>
#include <unistd.h>
#include "tbx_compiler.h"
#include "marcel_config.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


#if defined(MA__LIBPTHREAD) && defined(MARCEL_SIGNALS_ENABLED)
/*  L'interface noyau de linux pour sigaction n'est _pas_ la même que cette de la glibc ! */
#  ifndef SA_RESTORER
#    define SA_RESTORER 0x04000000
#  endif

#  ifdef X86_64_ARCH
#    define MA_KERNEL_SIGACTION_RESTORER	      \
	if (_act) {				      \
		_act->sa_flags |= SA_RESTORER;	      \
		_act->sa_restorer = &__ma_restore_rt; \
	}
#  else
#    define MA_KERNEL_SIGACTION_RESTORER
#  endif

#  undef sa_handler
#  undef sa_sigaction
typedef struct {
	union {
		__sighandler_t sa_handler;
		void (*sa_sigaction) (int, siginfo_t *, void *);
	} __sigaction_handler;
	unsigned long sa_flags;
	void (*sa_restorer) (void);
	sigset_t sa_mask;
} ma_sigaction_t;
#  define sa_handler __sigaction_handler.sa_handler
#  define sa_sigaction __sigaction_handler.sa_sigaction

#  define ma_sigaction(num, act, oact)					\
({									\
	ma_sigaction_t *_act = (act);					\
	MA_KERNEL_SIGACTION_RESTORER;					\
	syscall(SYS_rt_sigaction, num, _act, oact, _NSIG/8);		\
})
#  define ma_sigprocmask(how, nset, oset)			\
	syscall(SYS_rt_sigprocmask, how, nset, oset, _NSIG/8)
#  define ma_kill(pid, sig)			\
	syscall(SYS_kill, pid, sig)
#  define ma_sigsuspend(mask) \
	syscall(SYS_rt_sigsuspend,mask,_NSIG/8)
#  define ma_setitimer(which,val,oval)	\
	syscall(SYS_setitimer,which,val,oval)
#  ifdef SYS_signal
#    define ma_signal(sig,handler) \
	syscall(SYS_signal,sig,handler)
#  endif
#endif


/** Internal functions **/
#if defined(MA__LIBPTHREAD) && defined(MARCEL_SIGNALS_ENABLED)
#  ifdef X86_64_ARCH
void __ma_restore_rt(void);
#  endif
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_LINUX_MARCEL_SIGNAL_H__ **/
