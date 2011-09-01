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


#ifndef __SYS_MARCEL_FASTLOCK_H__
#define __SYS_MARCEL_FASTLOCK_H__


#include <time.h>
#include "marcel_config.h"
#include "tbx.h"
#include "sys/linux_spinlock.h"
#include "sys/marcel_debug.h"


/** Public data structures **/
/* Fast locks (not abstract because mutexes and conditions aren't abstract). */
struct _marcel_fastlock {
	ma_atomic_t   __status;         /* "Free" or "taken" or head of waiting list */
	void         *__waiting;	/* Head of waiting list */
	ma_spinlock_t __spinlock;	/* Used by compare_and_swap emulation. Also,
					   adaptive SMP lock stores spin count here. */
};

#define _pmarcel_fastlock _marcel_fastlock

/* Same as _marcel_fastlock, but constrainted to a long int so as to fit it
 * tight in lpt-ABI-compatible structures.
 */
struct _lpt_fastlock {
	/* Bit 0: 1 means "initialized"
	   Remaining bits: marcel_fastlock pointer */
	struct _marcel_fastlock *__mlock;
};

/* Make sure that the `__status' field of `_lpt_fastlock' is large enough to
   hold a pointer.  */
MA_VERIFY(sizeof(long int) >= sizeof(void *));


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macro **/
#define MA_MARCEL_FASTLOCK_UNLOCKED					\
	{ .__status = MA_ATOMIC_INIT(0), .__waiting = NULL, .__spinlock = MA_SPIN_LOCK_UNLOCKED}
#define MA_PMARCEL_FASTLOCK_UNLOCKED MA_MARCEL_FASTLOCK_UNLOCKED
/* This must remain 0 to keep ABI compatibility with static initializers.  */
#define MA_LPT_FASTLOCK_UNLOCKED { .__mlock = NULL }

/** __marcel_lock_(timed_)wait flag options */
#define MA_CHECK_INTR     (1)
#define MA_CHECK_CANCEL   (MA_CHECK_INTR<<1)

/** Access _ma_fastlock structure from _lpt_fastlock */
#define LPT_FASTLOCK_2_MA_FASTLOCK(lock) ((lock)->__mlock)

/* Return the head of LOCK's wait list.  */
#define MA_LPT_FASTLOCK_WAIT_LIST(_lock)		\
	MA_MARCEL_FASTLOCK_WAIT_LIST(LPT_FASTLOCK_2_MA_FASTLOCK(_lock))
#define MA_MARCEL_FASTLOCK_WAIT_LIST(_lock)	\
	((blockcell *) ((_lock)->__waiting))

/* Set LOCK's wait list head to CELL */
#define MA_LPT_FASTLOCK_SET_WAIT_LIST(_lock, _cell)			\
	MA_MARCEL_FASTLOCK_SET_WAIT_LIST(LPT_FASTLOCK_2_MA_FASTLOCK(_lock), _cell)
#define MA_MARCEL_FASTLOCK_SET_WAIT_LIST(_lock, _cell)			\
	(_lock)->__waiting = _cell;					\


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_MARCEL_FASTLOCK_H__ **/
