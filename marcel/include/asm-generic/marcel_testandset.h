
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
#include "tbx_compiler.h"
#section macros
#depend "asm/marcel_compareexchange.h[macros]"
#ifdef MA_HAVE_COMPAREEXCHANGE
#define MA_HAVE_TESTANDSET 1
#endif
#section marcel_macros
#depend "asm/marcel_compareexchange.h[macros]"
#ifdef MA_HAVE_COMPAREEXCHANGE
#define pm2_spinlock_testandset(spinlock) ma_cmpxchg(spinlock, 0, 1)
#define pm2_spinlock_release(spinlock) (void)ma_cmpxchg(spinlock, 1, 0)
#endif
#section marcel_variables
#ifndef MA_HAVE_COMPAREEXCHANGE
extern ma_spinlock_t testandset_spinlock;
#endif
#section marcel_functions
#ifndef MA_HAVE_COMPAREEXCHANGE
static __tbx_inline__ unsigned pm2_spinlock_testandset(volatile unsigned *spinlock) __tbx_deprecated__;
#endif
#section marcel_inline
#ifndef MA_HAVE_COMPAREEXCHANGE
#depend "linux_spinlock.h"
#depend "asm/linux_types.h"
static __tbx_inline__ unsigned __tbx_deprecated__ pm2_spinlock_testandset(volatile unsigned *spinlock)
{
  unsigned ret;
  ma_spin_lock_softirq(&testandset_spinlock);
  if (!(ret = *spinlock))
    *spinlock = 1;
  ma_spin_unlock_softirq(&testandset_spinlock);
  return ret;
}
#endif
#section marcel_macros
#ifndef MA_HAVE_COMPAREEXCHANGE
#define pm2_spinlock_release(spinlock) do { ma_mb(); (*(spinlock) = 0); } while(0)
#endif
