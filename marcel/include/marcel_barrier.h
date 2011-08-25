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


#include "marcel_config.h"
#include "asm/linux_atomic.h"
#include "sys/marcel_types.h"
#include "sys/marcel_utils.h"
#include "sys/marcel_fastlock.h"
#include "marcel_mutex.h"
#include "marcel_cond.h"


/** Public macros **/
#define MARCEL_BARRIER_SERIAL_THREAD (-1)
#define PMARCEL_BARRIER_SERIAL_THREAD (MARCEL_BARRIER_SERIAL_THREAD)

#define MARCEL_BARRIER_INITIALIZER(count) { \
	.init_count	= (count),\
	.lock		= MA_MARCEL_FASTLOCK_UNLOCKED, \
	.leftB		= MA_ATOMIC_INIT(count), \
	.leftE		= MA_ATOMIC_INIT(0), \
	.mode		= MARCEL_BARRIER_SLEEP_MODE, \
}
#define PMARCEL_BARRIER_INITIALIZER(count) MARCEL_BARRIER_INITIALIZER(count)


/** Public data types **/
typedef enum marcel_barrier_mode {
	MARCEL_BARRIER_SLEEP_MODE,
	MARCEL_BARRIER_YIELD_MODE
} marcel_barrier_mode_t, pmarcel_barrier_mode_t;

typedef struct marcel_barrierattr {
	int pshared;
	marcel_barrier_mode_t mode;
} marcel_barrierattr_t;

typedef struct {
	unsigned int init_count;
	struct _marcel_fastlock lock;
	ma_atomic_t leftB;
	ma_atomic_t leftE;
	marcel_barrier_mode_t mode;
} marcel_barrier_t;


/** Public functions **/
DEC_MARCEL(int, barrier_setcount, (marcel_barrier_t * barrier, unsigned int count));
DEC_MARCEL(int, barrier_addcount, (marcel_barrier_t * barrier, int addcount));
DEC_MARCEL(int, barrier_getcount, (__const marcel_barrier_t * barrier, unsigned int *count));
DEC_MARCEL(int, barrier_wait_begin, (marcel_barrier_t * barrier));
DEC_MARCEL(int, barrier_wait_end, (marcel_barrier_t * barrier));
DEC_MARCEL(int, barrier_init, (marcel_barrier_t * __restrict barrier,
			       __const marcel_barrierattr_t * __restrict attr,
			       unsigned num) __THROW);
DEC_MARCEL(int, barrier_destroy, (marcel_barrier_t * barrier) __THROW);
DEC_MARCEL(int, barrier_wait, (marcel_barrier_t * __restrict barrier) __THROW);
DEC_MARCEL(int, barrierattr_init, (marcel_barrierattr_t * attr) __THROW);
DEC_MARCEL(int, barrierattr_destroy, (marcel_barrierattr_t * attr) __THROW);
DEC_MARCEL(int, barrierattr_getpshared, (__const marcel_barrierattr_t *__restrict attr,
					 int *__restrict pshared) __THROW);
DEC_MARCEL(int, barrierattr_setpshared, (marcel_barrierattr_t * attr, int pshared) __THROW);
DEC_MARCEL(int, barrierattr_getmode, (__const marcel_barrierattr_t *__restrict attr,
				      marcel_barrier_mode_t * __restrict mode) __THROW);
DEC_MARCEL(int, barrierattr_setmode, (marcel_barrierattr_t * attr, marcel_barrier_mode_t mode) __THROW);


#endif /** __MARCEL_BARRIER_H__ **/
