
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
static __inline__ void
lpt_check_fastlock_type (void)
{
  char test[sizeof (long int) < sizeof (void *)
	    ? -1 : 1]
    __attribute__ ((__unused__));
}


/* Return true if LOCK is marked as taken.  */
#define MA_LPT_FASTLOCK_TAKEN(_lock)		\
  ((_lock)->__status & 2L)

/* If TAKEN is true, mark LOCK as taken, otherwise mark it as free.  */
#define MA_LPT_FASTLOCK_SET_STATUS(_lock, _taken)			\
  ((_taken)								\
   ? ma_test_and_set_bit (1, (unsigned long *) &(_lock)->__status)	\
   : ma_test_and_clear_bit (1, (unsigned long *) &(_lock)->__status))

/* Return the head of LOCK's wait list.  */
#define MA_LPT_FASTLOCK_WAIT_LIST(_lock)		\
  ((lpt_blockcell_t *) ((_lock)->__status & ~3L))

/* Set LOCK's wait list head to CELL, an `lpt_blockcell_t' pointer.  */
#define MA_LPT_FASTLOCK_SET_WAIT_LIST(_lock, _cell)			\
  do									\
    {									\
      /* CELL must be 4-byte aligned.  */				\
      MA_BUG_ON ((((uintptr_t) (_cell)) & 3L) != 0);			\
      (_lock)->__status = (((_lock)->__status) & 3L) | ((uintptr_t) (_cell)); \
    }									\
  while (0)
