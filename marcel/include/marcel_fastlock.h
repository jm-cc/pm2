
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
#define MA_MARCEL_FASTLOCK_UNLOCKED {.__status=0, .__spinlock=MA_SPIN_LOCK_UNLOCKED}
#define MA_PMARCEL_FASTLOCK_UNLOCKED MA_MARCEL_FASTLOCK_UNLOCKED
/* This must remain 0 to keep ABI compatibility with static initializers.  */
#define MA_LPT_FASTLOCK_UNLOCKED { .__status = 0 }

#section types
typedef int __marcel_atomic_lock_t;

#section structures
#depend "linux_spinlock.h[types]"

#include <stdint.h>

/* Fast locks (not abstract because mutexes and conditions aren't abstract). */
struct _marcel_fastlock
{
  long int __status;   /* "Free" or "taken" or head of waiting list */
  ma_spinlock_t __spinlock;  /* Used by compare_and_swap emulation. Also,
			  adaptive SMP lock stores spin count here. */
};

#define _pmarcel_fastlock _marcel_fastlock

/* Same as _marcel_fastlock, but constrainted to a long int so as to fit it
 * tight in lpt-ABI-compatible structures.
 * In that case we have to use ma_bit_spin_lock, which is significantly slower
 * that ma_spin_lock
 */
struct _lpt_fastlock
{
  /* Bit 0: Spinlock, accessed via `lpt_lock_acquire ()' and
            `lpt_lock_release ()'.

     Bit 1: Status, 1 means "taken", 0 means "free".

     Remaining bits: Head of the waiting list (i.e., address of the first
            `lpt_blockcell_t').  This assumes that `lpt_blockcell_t's are
            4-byte aligned.  */
  long int __status;
};

/* Make sure that the `__status' field of `_lpt_fastlock' is large enough to
   hold a pointer.  */
MA_VERIFY (sizeof (long int) >= sizeof (void *));


/* Return true if LOCK is marked as taken.  */
#define MA_LPT_FASTLOCK_TAKEN(_lock)		\
  ((_lock)->__status & 2L)

/* Return true if LOCK is busy (taken and possibly a wait list) */
#define MA_MARCEL_FASTLOCK_BUSY(_lock)		\
  ((_lock)->__status != 0L)

/* If TAKEN is true, mark LOCK as taken, otherwise mark it as free.
 * Assumes the lock is taken.  */
#define MA_LPT_FASTLOCK_SET_STATUS(_lock, _taken)			\
  do { \
    /* Make sure it's locked */						\
    MA_BUG_ON (((_lock)->__status & 1) != 1);		\
    (_lock)->__status = (((_taken) & 1) << 1) | 1;			\
  } while (0)
#define MA_MARCEL_FASTLOCK_SET_STATUS(_lock, _taken)			\
    (_lock)->__status = (_taken)

/* Return the head of LOCK's wait list.  */
#define MA_LPT_FASTLOCK_WAIT_LIST(_lock)		\
  ((lpt_blockcell_t *) ((_lock)->__status & ~3L))
#define MA_MARCEL_FASTLOCK_WAIT_LIST(_lock)		\
  ((blockcell *) ((_lock)->__status & ~1L))

/* Set LOCK's wait list head to CELL, an `lpt_blockcell_t' pointer.
 * Assumes the lock is taken.
 * Also sets status to held.  */
#define MA_LPT_FASTLOCK_SET_WAIT_LIST(_lock, _cell)			\
  do									\
    {									\
      /* CELL must be 4-byte aligned.  */				\
      MA_BUG_ON ((((uintptr_t) (_cell)) & 3L) != 0);			\
      /* Make sure it's locked */		\
      MA_BUG_ON (((_lock)->__status & 1L) != 1);			\
      (_lock)->__status = ((_lock)->__status & 3L) | ((uintptr_t) (_cell)); \
    }									\
  while (0)
#define MA_MARCEL_FASTLOCK_SET_WAIT_LIST(_lock, _cell)			\
  do									\
    {									\
      /* CELL must be 2-byte aligned.  */				\
      MA_BUG_ON ((((uintptr_t) (_cell)) & 1L) != 0);			\
      (_lock)->__status = 1 | ((uintptr_t) (_cell)); \
    }									\
  while (0)
