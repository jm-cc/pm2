
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

#ifndef MARCEL_KERNEL_THREADS_IS_DEF
#define MARCEL_KERNEL_THREADS_IS_DEF

#include <signal.h>

#include "sys/marcel_flags.h"

#ifdef MA__SMP

typedef void * (*marcel_kthread_func_t)(void *arg);

#ifdef MARCEL_DONT_USE_POSIX_THREADS

#ifdef SOLARIS_SYS

#include <thread.h>
typedef thread_t marcel_kthread_t;

#elif LINUX_SYS

#include <sys/types.h>
#include <unistd.h>
typedef pid_t marcel_kthread_t;

#else

#error CANNOT AVOID USING PTHREADS ON THIS ARCHITECTURE. SORRY.

#endif

#else // MARCEL_DONT_USE_POSIX_THREADS

#include <pthread.h>

typedef pthread_t marcel_kthread_t;

#endif

void marcel_kthread_create(marcel_kthread_t *pid,
			   marcel_kthread_func_t func, void *arg);

void marcel_kthread_join(marcel_kthread_t pid);

void marcel_kthread_exit(void *retval);

marcel_kthread_t marcel_kthread_self(void);

void marcel_kthread_sigmask(int how, sigset_t *newmask, sigset_t *oldmask);

#endif // MA__SMP

#endif

