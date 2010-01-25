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


#ifndef __ASM_GENERIC_LINUX_SPINLOCK_H__
#define __ASM_GENERIC_LINUX_SPINLOCK_H__


#include <unistd.h>
#include "sys/marcel_flags.h"
#include "marcel_config.h"
#include "asm/marcel_compareexchange.h"
#include "marcel_utils.h"
#if defined(MA__LWPS) || !defined(MA_HAVE_COMPAREEXCHANGE)
#include "asm/marcel_testandset.h"
#if !defined(MA_HAVE_TESTANDSET) && _POSIX_SPIN_LOCKS <= 0
#include <pthread.h>
#endif
#endif 


/** Public macros **/
/* Test and set implementation */
#if defined(MA__LWPS) || !defined(MA_HAVE_COMPAREEXCHANGE)
#ifdef MA_HAVE_TESTANDSET
#define MA_SPIN_LOCK_UNLOCKED 0
#define ma_spin_lock_init(x)	do { *(x) = (ma_spinlock_t) MA_SPIN_LOCK_UNLOCKED; } while(0)

/* Posix spinlock implementation */
#elif _POSIX_SPIN_LOCKS > 0
#define MA_SPIN_LOCK_UNLOCKED 0 /* hoping this is ok */
#define ma_spin_lock_init(x)	pthread_spin_init(x, 0)

/* No spin implementation, use posix mutexes */
#else
#ifdef MARCEL_DONT_USE_POSIX_THREADS
#warning "Can't avoid using pthreads with no native locking primitive"
#warning "Please define pm2_compareexchange in marcel/include/asm-yourarch/marcel_compareexchange.h"
#error "or define pm2_spinlock_testandset in marcel/include/asm-yourarch/marcel_testandset.h"
#endif
#define MA_SPIN_LOCK_UNLOCKED PTHREAD_MUTEX_INITIALIZER
#define ma_spin_lock_init(x) pthread_mutex_init(x,NULL);
#endif /* type of LOCK */
#endif /* LWPS & !HAVE_COMPAREEXCHANGE*/


/** Public data types **/
/* Test and set implementation */
#if defined(MA__LWPS) || !defined(MA_HAVE_COMPAREEXCHANGE)
#ifdef MA_HAVE_TESTANDSET
typedef volatile unsigned ma_spinlock_t;

/* Posix spinlock implementation */
#elif _POSIX_SPIN_LOCKS > 0
typedef pthread_spinlock_t ma_spinlock_t;

/* No spin implementation, use posix mutexes */
#else
#ifdef MARCEL_DONT_USE_POSIX_THREADS
#warning "Can't avoid using pthreads with no native locking primitive"
#warning "Please define pm2_compareexchange in marcel/include/asm-yourarch/marcel_compareexchange.h"
#error "or define pm2_spinlock_testandset in marcel/include/asm-yourarch/marcel_testandset.h"
#endif
typedef pthread_mutex_t ma_spinlock_t;
#endif /* type of LOCK */
#endif /* LWPS & !HAVE_COMPAREEXCHANGE*/


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
/* Test and set implementation */
#if defined(MA__LWPS) || !defined(MA_HAVE_COMPAREEXCHANGE)
#ifdef MA_HAVE_TESTANDSET
#define ma_spin_is_locked(x)	(!!*(x))
#define ma_spin_unlock_wait(x)	do { ma_barrier(); } while(ma_spin_is_locked(x))

#define _ma_raw_spin_unlock(x) pm2_spinlock_release(x)
#define _ma_raw_spin_trylock(x) (!pm2_spinlock_testandset(x))
#define _ma_raw_spin_lock(x) do { } while (!(_ma_raw_spin_trylock(x)))

/* Posix spinlock implementation */
#elif _POSIX_SPIN_LOCKS > 0
#define ma_spin_is_locked(x)	(pthread_spin_trylock(x) ? 1 : pthread_spin_unlock(x), 0) /* yes, a bit silly... */
#define ma_spin_unlock_wait(x)	do { ma_barrier(); } while(ma_spin_is_locked(x))

#define _ma_raw_spin_unlock(x)  pthread_spin_unlock(x)
#define _ma_raw_spin_trylock(x) (!pthread_spin_trylock(x))
#define _ma_raw_spin_lock(x)    pthread_spin_lock(x)

/* No spin implementation, use posix mutexes */
#else
#ifdef MARCEL_DONT_USE_POSIX_THREADS
#warning "Can't avoid using pthreads with no native locking primitive"
#warning "Please define pm2_compareexchange in marcel/include/asm-yourarch/marcel_compareexchange.h"
#error "or define pm2_spinlock_testandset in marcel/include/asm-yourarch/marcel_testandset.h"
#endif
#define ma_spin_is_locked(x)	({ int ___ret; \
				pthread_mutex_t *___px = (x); \
				if (!(___ret = pthread_mutex_trylock(___px))) \
				  pthread_mutex_unlock(___px); \
				___ret != 0; })
#define ma_spin_unlock_wait(x)	do { } while(ma_spin_is_locked(x))

#define _ma_raw_spin_unlock(x) pthread_mutex_unlock((x))
#define _ma_raw_spin_trylock(x) (!(pthread_mutex_trylock((x))))
#include <stdlib.h>
#define _ma_raw_spin_lock(x) do { if (pthread_mutex_lock((x))) \
				abort() } \
				while (0)

#endif /* type of LOCK */
#endif /* LWPS & !HAVE_COMPAREEXCHANGE*/


#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_GENERIC_LINUX_SPINLOCK_H__ **/
