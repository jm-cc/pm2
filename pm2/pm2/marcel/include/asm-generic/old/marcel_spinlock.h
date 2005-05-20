
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

#section default [no-include-in-main,no-include-in-master-section,no-write-section]

#section all [write-section]
#define MARCEL_SPIN_ITERATIONS 100
#define MARCEL_SPIN_LOCK_UNLOCKED            0

#include "asm/marcel_testandset.h"
#include "sys/marcel_archdep.h" /* Pour SCHED_YIELD() */

typedef unsigned marcel_spinlock_t;

__tbx_inline__ static void __marcel_raw_spin_init(marcel_spinlock_t *lock)
{
  *lock = MARCEL_SPIN_LOCK_UNLOCKED;
}

__tbx_inline__ static void __marcel_raw_spin_lock(marcel_spinlock_t *lock)
{
  unsigned counter = 0;

  while(pm2_spinlock_testandset(lock)) {
    if(++counter > MARCEL_SPIN_ITERATIONS) {
      SCHED_YIELD();
      counter=0;
    }
  }
}

__tbx_inline__ static int __marcel_raw_spin_trylock(marcel_spinlock_t *lock)
{
  return !pm2_spinlock_testandset(lock);
}

__tbx_inline__ static void __marcel_raw_spin_unlock(marcel_spinlock_t *lock)
{
  pm2_spinlock_release(lock);
}

__tbx_inline__ static int __marcel_raw_spin_is_locked(marcel_spinlock_t *lock)
{
  return *lock;
}


