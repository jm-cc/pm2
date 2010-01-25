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


#ifndef __MARCEL_FUTEX_H__
#define __MARCEL_FUTEX_H__


#include "sys/marcel_flags.h"
#include "marcel_fastlock.h"


/** Public macros **/
#define MARCEL_FUTEX_INITIALIZER { .__lock = MA_MARCEL_FASTLOCK_UNLOCKED }


/** Public data types **/
/* TODO: instead of a fastlock, use a batch requeuing mechanism */
typedef struct {
	struct _marcel_fastlock __lock;
} marcel_futex_t;


/** Public functions **/
/* Similar to Linux' futex(2), except that it avoids the use of a giant hash
 * table... */

/* Check whether *addr & mask is still equal to val, and if so, queue the
 * thread on the futex and wait for somebody to wake it.
 *
 * timeout is in jiffies (use marcel_gettimeslice() to convert from us), or
 * MA_MAX_SCHEDULE_TIMEOUT to request indefinite sleep.
 */
extern int marcel_futex_wait(marcel_futex_t *futex, unsigned long *addr, unsigned long mask, unsigned long val, signed long timeout);

/* Wakes nb threads queued on the futex. UINT_MAX will wake all of them. */
extern int marcel_futex_wake(marcel_futex_t *futex, unsigned nb);


#endif /** __MARCEL_FUTEX_H__ **/
