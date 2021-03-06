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


#ifndef __MARCEL_LPT_BARRIER_H__
#define __MARCEL_LPT_BARRIER_H__


#include "marcel_config.h"


#ifdef MARCEL_LIBPTHREAD


/** Public macros **/
#define LPT_BARRIER_SERIAL_THREAD (-1)


/** Public functions **/
extern int lpt_barrier_wait_begin(lpt_barrier_t * barrier);

extern int lpt_barrier_wait_end(lpt_barrier_t * barrier);

extern int lpt_barrier_init(lpt_barrier_t * __restrict barrier,
			    __const lpt_barrierattr_t * __restrict attr,
			    unsigned num) __THROW;

extern int lpt_barrier_destroy(lpt_barrier_t * barrier) __THROW;

extern int lpt_barrier_wait(lpt_barrier_t * __restrict barrier) __THROW;

extern int lpt_barrierattr_init(lpt_barrierattr_t * attr) __THROW;

extern int lpt_barrierattr_destroy(lpt_barrierattr_t * attr) __THROW;

extern int lpt_barrierattr_getpshared(__const lpt_barrierattr_t *
				      __restrict attr, int *__restrict pshared) __THROW;

extern int lpt_barrierattr_setpshared(lpt_barrierattr_t * attr, int pshared) __THROW;


#endif /** MARCEL_LIBPTHREAD **/


#endif /** __MARCEL_LPT_BARRIER_H__ **/
