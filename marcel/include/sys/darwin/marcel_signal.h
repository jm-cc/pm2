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


#ifndef __SYS_DARWIN_MARCEL_SIGNAL_H__
#define __SYS_DARWIN_MARCEL_SIGNAL_H__


#include <sys/syscall.h>
#include <unistd.h>
#include "tbx_compiler.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


#if defined(MA__LIBPTHREAD) && defined(MARCEL_SIGNALS_ENABLED)
#  undef sa_handler
#  undef sa_sigaction
typedef struct sigaction ma_sigaction_t;

#  define ma_sigaction(num, act, oact)					\
({									\
        ma_sigaction_t *_act = (act);						\
	syscall(SYS_sigaction, num, _act, oact);
})
#  define ma_sigprocmask(how, nset, oset)	\
        syscall(SYS_sigprocmask, how, nset, oset)
#  define ma_kill(pid, sig)			\
	syscall(SYS_kill, pid, sig)
#  define ma_sigsuspend(mask) \
        syscall(SYS_sigsuspend, mask)
#  define ma_setitimer(which, val, oval)	\
	syscall(SYS_setitimer, which, val, oval)
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_DARWIN_MARCEL_SIGNAL_H__ **/
