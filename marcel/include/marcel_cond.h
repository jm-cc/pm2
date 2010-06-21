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
#include "marcel_types.h"
#include "marcel_mutex.h"
#include "marcel_fastlock.h"
#include "marcel_threads.h"


/** Public macros **/
#define MARCEL_COND_INITIALIZER \
  {.__data = {.__lock=MA_MARCEL_FASTLOCK_UNLOCKED, .__waiting=0} }

#ifdef MA__IFACE_PMARCEL
#  define PMARCEL_COND_INITIALIZER MARCEL_COND_INITIALIZER
#endif


/** Public data structures **/
struct marcel_condattr {
	int value;
};


/** Public data types **/
union __marcel_condattr_t {
	struct marcel_condattr __data;
	long int __align;
};


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

/* Get the process-shared flag of the condition variable attribute ATTR.  */
extern int marcel_condattr_getpshared(__const marcel_condattr_t *
				      __restrict __attr,
				      int *__restrict __pshared) __THROW;

/* Set the process-shared flag of the condition variable attribute ATTR.  */
extern int marcel_condattr_setpshared(marcel_condattr_t * __attr, int __pshared) __THROW;


#ifdef MA__IFACE_PMARCEL
extern int pmarcel_cond_init(pmarcel_cond_t * __restrict __cond,
			     __const pmarcel_condattr_t * __restrict __cond_attr) __THROW;
extern int pmarcel_cond_destroy(pmarcel_cond_t * __cond) __THROW;
extern int pmarcel_cond_signal(pmarcel_cond_t * __cond) __THROW;
extern int pmarcel_cond_broadcast(pmarcel_cond_t * __cond) __THROW;
extern int pmarcel_cond_wait(pmarcel_cond_t * __restrict __cond,
			     pmarcel_mutex_t * __restrict __mutex);
extern int pmarcel_cond_timedwait(pmarcel_cond_t * __restrict __cond,
				  pmarcel_mutex_t * __restrict __mutex,
				  __const struct timespec *__restrict __abstime);
extern int pmarcel_condattr_init(pmarcel_condattr_t * __attr) __THROW;
extern int pmarcel_condattr_destroy(pmarcel_condattr_t * __attr) __THROW;
extern int pmarcel_condattr_getpshared(__const pmarcel_condattr_t *
				       __restrict __attr,
				       int *__restrict __pshared) __THROW;
extern int pmarcel_condattr_setpshared(pmarcel_condattr_t * __attr,
				       int __pshared) __THROW;
#endif


#endif /** __MARCEL_COND_H__ **/
