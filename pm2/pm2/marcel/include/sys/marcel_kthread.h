
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

#section marcel_types
#ifdef MA__SMP
typedef void * (*marcel_kthread_func_t)(void *arg);

# ifndef MARCEL_DONT_USE_POSIX_THREADS
#   include <pthread.h>
typedef pthread_t marcel_kthread_t;
# else
#   ifdef SOLARIS_SYS
#     include <thread.h>
typedef thread_t marcel_kthread_t;
#   elif LINUX_SYS
#     include <sys/types.h>
#     include <unistd.h>
typedef pid_t marcel_kthread_t;
#   else
#     error CANNOT AVOID USING PTHREADS ON THIS ARCHITECTURE. SORRY.
#   endif
# endif
#endif // MA__SMP

#section marcel_functions
#ifdef MA__SMP
void marcel_kthread_create(marcel_kthread_t *pid, void *sp,
			   marcel_kthread_func_t func, void *arg);

void marcel_kthread_join(marcel_kthread_t pid);

void marcel_kthread_exit(void *retval);

marcel_kthread_t marcel_kthread_self(void);

void marcel_kthread_sigmask(int how, sigset_t *newmask, sigset_t *oldmask);

void marcel_kthread_kill(marcel_kthread_t pid, int sig);
#endif // MA__SMP


