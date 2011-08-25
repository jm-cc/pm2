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


#ifndef __SYS_MARCEL_KTHREAD_H__
#define __SYS_MARCEL_KTHREAD_H__


#include <signal.h>
#include "marcel_config.h"
#if defined(MA__LWPS) && !defined(MARCEL_DONT_USE_POSIX_THREADS)
# include <semaphore.h>
# include <pthread.h>
#endif
#include "sys/os/marcel_kthread.h"



#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal marcel_types **/
#ifdef MA__LWPS
typedef void *(*marcel_kthread_func_t) (void *arg);

# ifndef MARCEL_DONT_USE_POSIX_THREADS
#    define MARCEL_KTHREAD_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#    define MARCEL_KTHREAD_COND_INITIALIZER PTHREAD_COND_INITIALIZER
typedef pthread_t marcel_kthread_t;
typedef pthread_mutex_t marcel_kthread_mutex_t;
typedef sem_t marcel_kthread_sem_t;
typedef pthread_cond_t marcel_kthread_cond_t;
# endif
#endif				// MA__LWPS


/** Internal marcel_functions **/
#ifdef MA__LWPS
int marcel_gettid(void);

void marcel_kthread_create(marcel_kthread_t * pid, void *sp,
			   void *stack_base, marcel_kthread_func_t func, void *arg);

void marcel_kthread_join(marcel_kthread_t * pid);

void marcel_kthread_exit(void *retval);

marcel_kthread_t marcel_kthread_self(void);

void marcel_kthread_kill(marcel_kthread_t pid, int sig);

void marcel_kthread_mutex_init(marcel_kthread_mutex_t * lock);
void marcel_kthread_mutex_lock(marcel_kthread_mutex_t * lock);
void marcel_kthread_mutex_unlock(marcel_kthread_mutex_t * lock);
int marcel_kthread_mutex_trylock(marcel_kthread_mutex_t * lock);

void marcel_kthread_sem_init(marcel_kthread_sem_t * sem, int pshared, unsigned int value);
void marcel_kthread_sem_wait(marcel_kthread_sem_t * sem);
void marcel_kthread_sem_post(marcel_kthread_sem_t * sem);
int marcel_kthread_sem_trywait(marcel_kthread_sem_t * sem);
void marcel_kthread_cond_init(marcel_kthread_cond_t * cond);
void marcel_kthread_cond_signal(marcel_kthread_cond_t * cond);
void marcel_kthread_cond_broadcast(marcel_kthread_cond_t * cond);
void marcel_kthread_cond_wait(marcel_kthread_cond_t * cond,
			      marcel_kthread_mutex_t * mutex);
#endif				// MA__LWPS

void marcel_kthread_sigmask(int how, sigset_t * newmask, sigset_t * oldmask);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_MARCEL_KTHREAD_H__ **/
