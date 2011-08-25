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


#ifndef __SYS_LINUX_MARCEL_KTHREAD_H__
#define __SYS_LINUX_MARCEL_KTHREAD_H__


#include "sys/marcel_flags.h"
#if defined(MA__LWPS) && defined(MARCEL_DONT_USE_POSIX_THREADS)
# include <sys/types.h>
# include <unistd.h>
#endif				/* MA__LWPS */
#include "asm/linux_atomic.h"


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal marcel_types **/
#if defined(MA__LWPS) && defined(MARCEL_DONT_USE_POSIX_THREADS)
typedef pid_t marcel_kthread_t;
typedef ma_atomic_t marcel_kthread_sem_t;
typedef marcel_kthread_sem_t marcel_kthread_mutex_t;
#define MARCEL_KTHREAD_MUTEX_INITIALIZER MA_ATOMIC_INIT(1)
typedef int marcel_kthread_cond_t;
#define MARCEL_KTHREAD_COND_INITIALIZER 0
#endif


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_LINUX_MARCEL_KTHREAD_H__ **/
