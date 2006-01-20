
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

#section common
/*
 * Similar to:
 * include/asm-i386/spinlock.h
 */

#section types
/*
 * Your basic SMP spinlocks, allowing only a single CPU anywhere
 */

#if defined(MA__LWPS) || !defined(MA_HAVE_COMPAREEXCHANGE)
#depend "asm/marcel_testandset.h[macros]"
#ifdef MA_HAVE_TESTANDSET
typedef volatile unsigned ma_spinlock_t;
#else
#ifdef MARCEL_DONT_USE_POSIX_THREADS
#warning "Can't avoid using pthreads with no native locking primitive"
#warning "Please define pm2_compareexchange in marcel/include/asm-yourarch/marcel_compareexchange.h"
#error "or define pm2_spinlock_testandset in marcel/include/asm-yourarch/marcel_testandset.h"
#endif
#include <pthread.h>
typedef pthread_mutex_t ma_spinlock_t;
#endif
#endif

#section macros
#if defined(MA__LWPS) || !defined(MA_HAVE_COMPAREEXCHANGE)
#depend "asm/marcel_testandset.h[macros]"
#ifdef MA_HAVE_TESTANDSET
#define MA_SPIN_LOCK_UNLOCKED 0

#define ma_spin_lock_init(x)	do { *(x) = (ma_spinlock_t) MA_SPIN_LOCK_UNLOCKED; } while(0)
#else
#include <pthread.h>
#define MA_SPIN_LOCK_UNLOCKED PTHREAD_MUTEX_INITIALIZER
#define ma_spin_lock_init(x) pthread_mutex_init(x,NULL);
#endif
#endif

#section marcel_macros
/*
 * Simple spin lock operations.  There are two variants, one clears IRQ's
 * on the local processor, one does not.
 *
 * We make no fairness assumptions. They have a cost.
 */

#if defined(MA__LWPS) || !defined(MA_HAVE_COMPAREEXCHANGE)

#depend "asm/marcel_testandset.h[macros]"
#ifdef MA_HAVE_TESTANDSET
#define ma_spin_is_locked(x)	(!!*(x))
#define ma_spin_unlock_wait(x)	do { ma_barrier(); } while(ma_spin_is_locked(x))

#define _ma_raw_spin_unlock(x) pm2_spinlock_release(x)
#define _ma_raw_spin_trylock(x) (!pm2_spinlock_testandset(x))
#define _ma_raw_spin_lock(x) do { } while (!(_ma_raw_spin_trylock(x)))
#else
#ifdef MARCEL_DONT_USE_POSIX_THREADS
#error "Can't avoid using Posix Threads library if neither cmpxchg nor testandset are implemented !"
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
#endif

#endif /* MA__LWPS */
#section marcel_inline
