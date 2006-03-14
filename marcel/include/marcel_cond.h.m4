dnl -*- linux-c -*-
include(scripts/marcel.m4)
dnl /***************************
dnl  * This is the original file
dnl  * =========================
dnl  ***************************/
/* This file has been autogenerated from __file__ */
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

#depend "linux_spinlock.h[macros]"

#section macros

REPLICATE([[dnl
#define PREFIX_COND_INITIALIZER \
  {.__data.__lock=MA_FASTLOCK_UNLOCKED, .__data.__waiting=0}
]], [[MARCEL PMARCEL]])/* pas LPT car dépendant de l'archi */

#section structures
#depend "marcel_fastlock.h[structures]"
#depend "marcel_threads.h[types]"


REPLICATE([[dnl
/* Attribute for conditionally variables.  */
struct prefix_condattr
{
	int value;
};
]])

REPLICATE([[dnl
typedef union
{
	struct prefix_condattr __data;
	long int __align;
} prefix_condattr_t;

/* Conditions (not abstract because of PREFIX_COND_INITIALIZER */
typedef union
{
        struct
        {
               /* Protect against concurrent access */
               struct _marcel_fastlock __lock;
               /* Threads waiting on this condition */
               p_marcel_task_t __waiting;
        } __data;
        long int __align;
} prefix_cond_t;
]], [[MARCEL PMARCEL]])/* pas LPT car dépendant de l'archi */


#section functions
#include <time.h>
#depend "marcel_alias.h[macros]"
#depend "marcel_mutex.h[types]"

REPLICATE([[dnl
/* Initialize condition variable COND using attributes ATTR, or use
   the default values if later is NULL.  */
extern int prefix_cond_init (prefix_cond_t *__restrict __cond,
                              __const prefix_condattr_t *__restrict
                              __cond_attr) __THROW;

/* Destroy condition variable COND.  */
extern int prefix_cond_destroy (prefix_cond_t *__cond) __THROW;

/* Wake up one thread waiting for condition variable COND.  */
extern int prefix_cond_signal (prefix_cond_t *__cond) __THROW;

/* Wake up all threads waiting for condition variables COND.  */
extern int prefix_cond_broadcast (prefix_cond_t *__cond) __THROW;

/* Wait for condition variable COND to be signaled or broadcast.
   MUTEX is assumed to be locked before.  */
extern int prefix_cond_wait (prefix_cond_t *__restrict __cond,
                              prefix_mutex_t *__restrict __mutex);

/* Wait for condition variable COND to be signaled or broadcast until
   ABSTIME.  MUTEX is assumed to be locked before.  ABSTIME is an
   absolute time specification; zero is the beginning of the epoch
   (00:00:00 GMT, January 1, 1970).  */
extern int prefix_cond_timedwait (prefix_cond_t *__restrict __cond,
                                   prefix_mutex_t *__restrict __mutex,
                                   __const struct timespec *__restrict
                                   __abstime);

/* Initialize condition variable attribute ATTR.  */
extern int prefix_condattr_init (prefix_condattr_t *__attr) __THROW;

/* Destroy condition variable attribute ATTR.  */
extern int prefix_condattr_destroy (prefix_condattr_t *__attr) __THROW;

/* Get the process-shared flag of the condition variable attribute ATTR.  */
extern int prefix_condattr_getpshared (__const prefix_condattr_t *
                                        __restrict __attr,
                                        int *__restrict __pshared) __THROW;

/* Set the process-shared flag of the condition variable attribute ATTR.  */
extern int prefix_condattr_setpshared (prefix_condattr_t *__attr,
                                        int __pshared) __THROW;

]],[[MARCEL PMARCEL LPT]])dnl END_REPLICATE
