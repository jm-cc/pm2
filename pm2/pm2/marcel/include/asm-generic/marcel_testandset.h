
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

#section marcel_macros
#depend "asm/marcel_compareexchange[macros]"
#ifdef MA_HAVE_COMPAREEXCHANGE
#define MA_HAVE_TESTANDSET 1
#define pm2_spinlock_testandset(spinlock) ma_cmpxchg(spinlock, 0, 1)
#define pm2_spinlock_release(spinlock) ma_cmpxchg(spinlock, 1, 0)
#else
#section marcel_variables
extern ma_spinlock_t testandset_spinlock;
#section marcel_functions
static __inline__ unsigned pm2_spinlock_testandset(volatile unsigned *spinlock) __tbx_deprecated__;
static __inline__ unsigned pm2_spinlock_release(volatile unsigned *spinlock) __tbx_deprecated__;
#section marcel_inline
#depend "linux_spinlock.h"
#depend "asm/linux_types.h"
static __inline__ unsigned __tbx_deprecated__ pm2_spinlock_testandset(volatile unsigned *spinlock)
{
  unsigned ret;
  ma_spin_lock_softirq(&testandset_spinlock);
  if (!(ret = *spinlock))
    *spinlock = 1;
  ma_spin_unlock_softirq(&testandset_spinlock);
  return ret;
}
#define pm2_spinlock_release(spinlock) (*(spinlock) = 0)
#endif
