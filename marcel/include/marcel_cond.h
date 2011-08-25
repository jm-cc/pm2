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


#ifndef __MARCEL_COND_H__
#define __MARCEL_COND_H__


#include <time.h>
#include "sys/marcel_flags.h"
#include "sys/marcel_types.h"
#include "sys/marcel_fastlock.h"
#include "asm/nptl_types.h"
#include "marcel_mutex.h"


/** Public macros **/
#ifdef CLOCK_REALTIME
#  define MARCEL_COND_INITIALIZER \
	{.__data = {.__lock=MA_MARCEL_FASTLOCK_UNLOCKED, .__waiting=0, .__clk_id=CLOCK_REALTIME} }
#else
#  define MARCEL_COND_INITIALIZER \
	{.__data = {.__lock=MA_MARCEL_FASTLOCK_UNLOCKED, .__waiting=0} }
#endif
#ifdef MA__IFACE_PMARCEL
#  define PMARCEL_COND_INITIALIZER MARCEL_COND_INITIALIZER
#endif


/** Public data structures **/
union __marcel_condattr_t {
	struct {
		int value;
	} __data;
	long int __align;
};
typedef union __marcel_condattr_t marcel_condattr_t;

/* Conditions (not abstract because of MARCEL_COND_INITIALIZER */
typedef union {
	struct {
		/* Protect against concurrent access */
		struct _marcel_fastlock __lock;
		/* Threads waiting on this condition */
		marcel_t __waiting;
#ifdef CLOCK_REALTIME
		/* Clock-id for cond_timedwait abstime */
		clockid_t __clk_id;
#endif
	} __data;
	long int __align;
} marcel_cond_t;


/** Public functions **/
/* Initialize condition variable COND using attributes ATTR, or use
   the default values if later is NULL.  */
extern int marcel_cond_init(marcel_cond_t * __restrict __cond,
			    __const marcel_condattr_t * __restrict __cond_attr) __THROW;

/* Destroy condition variable COND.  */
extern int marcel_cond_destroy(marcel_cond_t * __cond) __THROW;

/* Wake up one thread waiting for condition variable COND.  */
extern int marcel_cond_signal(marcel_cond_t * __cond) __THROW;

/* Wake up all threads waiting for condition variables COND.  */
extern int marcel_cond_broadcast(marcel_cond_t * __cond) __THROW;

/* Wait for condition variable COND to be signaled or broadcast.
   MUTEX is assumed to be locked before.  */
extern int marcel_cond_wait(marcel_cond_t * __restrict __cond,
			    marcel_mutex_t * __restrict __mutex);

/* Wait for condition variable COND to be signaled or broadcast until
   ABSTIME.  MUTEX is assumed to be locked before.  ABSTIME is an
   absolute time specification; zero is the beginning of the epoch
   (00:00:00 GMT, January 1, 1970).  */
extern int marcel_cond_timedwait(marcel_cond_t * __restrict __cond,
				 marcel_mutex_t * __restrict __mutex,
				 __const struct timespec *__restrict __abstime);

/* Initialize condition variable attribute ATTR.  */
extern int marcel_condattr_init(marcel_condattr_t * __attr) __THROW;

/* Destroy condition variable attribute ATTR.  */
extern int marcel_condattr_destroy(marcel_condattr_t * __attr) __THROW;


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#define MA_CONDATTR_GET_PSHARED(attr)          ((attr)->value & 1)
#define MA_CONDATTR_SET_PSHARED(attr, pshared) (attr)->value = (((attr)->value & ~1) | (pshared != PMARCEL_PROCESS_SHARED))
#define MA_CONDATTR_GET_CLKID(attr)            ((attr)->value >> 1)
#define MA_CONDATTR_SET_CLKID(attr, clk_id)    (attr)->value = (MA_CONDATTR_GET_PSHARED(attr) | (clk_id << 1))


TBX_VISIBILITY_PUSH_DEFAULT
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_COND_H__ **/
