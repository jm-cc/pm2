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

#ifndef _BITS_MARCEL_SIGTHREAD_H
#define _BITS_MARCEL_SIGTHREAD_H	1

#if !defined _SIGNAL_H && !defined _MARCEL_H
# error "Never include this file directly.  Use <marcel_pthread.h> instead"
#endif

/* Functions for handling signals. */

/* Modify the signal mask for the calling thread.  The arguments have
   the same meaning as for sigprocmask(2). */
extern int marcel_sigmask (int __how,
			    __const __sigset_t *__restrict __newmask,
			    __sigset_t *__restrict __oldmask)__THROW;

/* Send signal SIGNO to the given thread. */
extern int marcel_kill (marcel_t thread, int __signo) __THROW;

#endif	/* bits/marcel_sigthread.h */
