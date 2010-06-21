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


#ifndef __MARCEL_BARRIER_H__
#define __MARCEL_BARRIER_H__


#include "asm/linux_atomic.h"
#include "sys/marcel_flags.h"
#include "marcel_types.h"
#include "marcel_utils.h"
#include "marcel_fastlock.h"
#include "marcel_threads.h"
#include "marcel_mutex.h"
#include "marcel_cond.h"


/** Public macros **/
#define MARCEL_BARRIER_SERIAL_THREAD (-1)
#ifdef MA__IFACE_PMARCEL
#  define PMARCEL_BARRIER_SERIAL_THREAD (MARCEL_BARRIER_SERIAL_THREAD)
#endif

#ifdef MA_BARRIER_USE_MUTEX
#define MARCEL_BARRIER_INITIALIZER(count) { \
	.init_count	= (count),\
        .m              = MARCEL_MUTEX_INITIALIZER,		\
        .c              = MARCEL_COND_INITIALIZER,		\
	.leftB		= MA_ATOMIC_INIT(count), \
	.leftE		= MA_ATOMIC_INIT(0), \
	.mode		= MA_BARRIER_SLEEP_MODE, \
}
#else
#define MARCEL_BARRIER_INITIALIZER(count) { \
	.init_count	= (count),\
	.lock		= (struct _marcel_fastlock) MA_MARCEL_FASTLOCK_UNLOCKED, \
	.leftB		= MA_ATOMIC_INIT(count), \
	.leftE		= MA_ATOMIC_INIT(0), \
	.mode		= MA_BARRIER_SLEEP_MODE, \
}
#endif

#ifdef MA__IFACE_PMARCEL
#  define PMARCEL_BARRIER_INITIALIZER(count) MARCEL_BARRIER_INITIALIZER(count)
#endif


/** Public data types **/
typedef struct marcel_barrierattr {
	int pshared;
	ma_barrier_mode_t mode;
} marcel_barrierattr_t;
#ifdef MA__IFACE_PMARCEL
typedef marcel_barrierattr_t pmarcel_barrierattr_t;
#endif


/** Public functions **/
int marcel_barrier_setcount(marcel_barrier_t * barrier, unsigned int count);
int marcel_barrier_addcount(marcel_barrier_t * barrier, int addcount);
int marcel_barrier_getcount(__const marcel_barrier_t * barrier, unsigned int *count);
int marcel_barrier_wait_begin(marcel_barrier_t * barrier);
int marcel_barrier_wait_end(marcel_barrier_t * barrier);

#ifdef MA__IFACE_PMARCEL
int pmarcel_barrier_wait_begin(pmarcel_barrier_t * barrier);
int pmarcel_barrier_wait_end(pmarcel_barrier_t * barrier);
#endif

DEC_MARCEL_PMARCEL(int, barrier_init, (marcel_barrier_t * __restrict barrier,
				       __const marcel_barrierattr_t * __restrict attr,
				       unsigned num) __THROW);
DEC_MARCEL_PMARCEL(int, barrier_destroy, (marcel_barrier_t * barrier) __THROW);
DEC_MARCEL_PMARCEL(int, barrier_wait, (marcel_barrier_t * __restrict barrier) __THROW);
DEC_MARCEL_PMARCEL(int, barrierattr_init, (marcel_barrierattr_t * attr) __THROW);
DEC_MARCEL_PMARCEL(int, barrierattr_destroy, (marcel_barrierattr_t * attr) __THROW);
DEC_MARCEL_PMARCEL(int, barrierattr_getpshared, (__const marcel_barrierattr_t *__restrict attr,
						 int *__restrict pshared) __THROW);
DEC_MARCEL_PMARCEL(int, barrierattr_setpshared, (marcel_barrierattr_t * attr, int pshared) __THROW);
DEC_MARCEL_PMARCEL(int, barrierattr_getmode, (__const marcel_barrierattr_t *__restrict attr,
					    ma_barrier_mode_t * __restrict mode) __THROW);
DEC_MARCEL_PMARCEL(int, barrierattr_setmode, (marcel_barrierattr_t * attr, ma_barrier_mode_t mode) __THROW);


#endif /** __MARCEL_BARRIER_H__ **/
