
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
#depend "asm-generic/linux_rwlock.h[]"

#section types
/*
 * Your basic SMP spinlocks, allowing only a single CPU anywhere
 */

#ifdef MA__LWPS
#depend "asm/marcel_testandset.h[macros]"
#ifdef MA_HAVE_TESTANDSET
typedef volatile unsigned ma_spinlock_t;
#else
#include <pthread.h>
typedef pthread_mutex_t mutex;
#endif
#endif

#section macros
#ifdef MA__LWPS
#depend "asm/marcel_testandset.h[macros]"
#ifdef MA_HAVE_TESTANDSET
#define MA_SPIN_LOCK_UNLOCKED 0

#define ma_spin_lock_init(x)	do { *(x) = (ma_spinlock_t) MA_SPIN_LOCK_UNLOCKED; } while(0)
#else
#include <pthread.h>
#define MA_SPIN_LOCK_UNLOCKED PTHREAD_MUTEX_INITIALIZER
#define ma_spin_lock_init(x) pthread_mutex_init(&x,NULL);
#endif
#endif

#section marcel_macros
/*
 * Simple spin lock operations.  There are two variants, one clears IRQ's
 * on the local processor, one does not.
 *
 * We make no fairness assumptions. They have a cost.
 */

#ifdef MA__LWPS

#depend "asm/marcel_testandset.h[macros]"
#ifdef MA_HAVE_TESTANDSET
#define ma_spin_is_locked(x)	(*(x) == 1)
#define ma_spin_unlock_wait(x)	do { ma_barrier(); } while(ma_spin_is_locked(x))

#define _ma_raw_spin_unlock(x) pm2_spinlock_release(x)
#define _ma_raw_spin_trylock(x) (!pm2_spinlock_testandset(x))
#define _ma_raw_spin_lock(x) do { } while (!(_ma_raw_spin_trylock(x)))
#else
#ifdef MARCEL_DONT_USE_POSIX_THREADS
#error "Can't avoid using Posix Threads library if neither cmpxchg nor testandset are implemented !"
#endif
#define ma_spin_is_locked(x)	({ int ___ret; \
				pthread_mutex_t *___px = &(x); \
				if (!(___ret = pthread_mutex_trylock(___px))) \
				  pthread_mutex_unlock(___px); \
				___ret != 0; })
#define ma_spin_unlock_wait(x)	do { } while(ma_spin_is_locked(x))

#define _ma_raw_spin_unlock(x) pthread_mutex_lock(&(x))
#define _ma_raw_spin_trylock(x) (!(pthread_mutex_trylock(&(x))))
#define _ma_raw_spin_lock(x) do { if (!(ma_raw_spin_lock(&(x)))) \
				MA_BUG() } \
				while (0)
#endif

#endif /* MA__LWPS */
