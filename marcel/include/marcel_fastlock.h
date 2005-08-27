
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

#section macros
#define MA_FASTLOCK_UNLOCKED {.__status=0, .__spinlock=MA_SPIN_LOCK_UNLOCKED}
#define MA_LPT_FASTLOCK_UNLOCKED {.__status=0, .__spinlock=0}

#section types
typedef int __marcel_atomic_lock_t;

#section structures
#depend "linux_spinlock.h[marcel_structures]"
/* Fast locks (not abstract because mutexes and conditions aren't abstract). */
struct _marcel_fastlock
{
  long int __status;   /* "Free" or "taken" or head of waiting list */
  ma_spinlock_t __spinlock;  /* Used by compare_and_swap emulation. Also,
			  adaptive SMP lock stores spin count here. */
};

struct _lpt_fastlock
{
  long int __status;   /* "Free" or "taken" or head of waiting list */
  long int __spinlock;  /* Used by compare_and_swap emulation. Also,
			  adaptive SMP lock stores spin count here. */
};
