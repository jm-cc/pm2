
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
typedef volatile unsigned ma_spinlock_t;
#endif

#section macros
#ifdef MA__LWPS
#define MA_SPIN_LOCK_UNLOCKED 0

#define ma_spin_lock_init(x)	do { *(x) = (ma_spinlock_t) MA_SPIN_LOCK_UNLOCKED; } while(0)
#endif

#section marcel_macros
/*
 * Simple spin lock operations.  There are two variants, one clears IRQ's
 * on the local processor, one does not.
 *
 * We make no fairness assumptions. They have a cost.
 */

#ifdef MA__LWPS

#define ma_spin_is_locked(x)	(*(x) == 1)
#define ma_spin_unlock_wait(x)	do { ma_barrier(); } while(ma_spin_is_locked(x))

#define _ma_raw_spin_unlock(x) pm2_spinlock_release(x)
#define _ma_raw_spin_trylock(x) (!pm2_spinlock_testandset(x))
#define _ma_raw_spin_lock(x) do { } while (!(_ma_raw_spin_trylock(x)))

#endif /* MA__LWPS */
