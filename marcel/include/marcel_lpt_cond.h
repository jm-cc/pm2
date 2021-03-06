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


#ifndef __MARCEL_LPT_COND_H__
#define __MARCEL_LPT_COND_H__


#include <time.h>
#include "marcel_config.h"
#include "sys/marcel_fastlock.h"
#include "marcel_mutex.h"
#include "marcel_threads.h"


/** Public data structures **/
#ifdef MA__IFACE_LPT
/* Attribute for conditionally variables.  */
struct lpt_condattr {
	int value;
};
#endif


/** Public functions **/
#ifdef MA__IFACE_LPT
extern int lpt_cond_init(lpt_cond_t * __restrict __cond,
			 __const lpt_condattr_t * __restrict __cond_attr) __THROW;
extern int lpt_cond_destroy(lpt_cond_t * __cond) __THROW;
extern int lpt_cond_signal(lpt_cond_t * __cond) __THROW;
extern int lpt_cond_broadcast(lpt_cond_t * __cond) __THROW;
extern int lpt_cond_wait(lpt_cond_t * __restrict __cond,
			 lpt_mutex_t * __restrict __mutex);
extern int lpt_cond_timedwait(lpt_cond_t * __restrict __cond,
			      lpt_mutex_t * __restrict __mutex,
			      __const struct timespec *__restrict __abstime);
extern int lpt_condattr_init(lpt_condattr_t * __attr) __THROW;
extern int lpt_condattr_destroy(lpt_condattr_t * __attr) __THROW;
extern int lpt_condattr_getpshared(__const lpt_condattr_t *
				   __restrict __attr, int *__restrict __pshared) __THROW;
extern int lpt_condattr_setpshared(lpt_condattr_t * __attr, int __pshared) __THROW;

#endif


#endif /** __MARCEL_LPT_COND_H__ **/
