
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

#section common

#depend "linux_spinlock.h[macros]"

#section macros

#define MARCEL_COND_INITIALIZER {.__c_lock=MA_FASTLOCK_UNLOCKED,.__c_waiting=0}

#section types
typedef struct __marcel_condattr_s marcel_condattr_t;
typedef struct __marcel_cond_s marcel_cond_t;

#section structures
#depend "marcel_fastlock.h[structures]"
#depend "marcel_threads.h[types]"
/* Attribute for conditionally variables.  */
struct __marcel_condattr_s
{
  int __dummy;
};

/* Conditions (not abstract because of MARCEL_COND_INITIALIZER */
struct __marcel_cond_s
{
  struct _marcel_fastlock __c_lock; /* Protect against concurrent access */
  p_marcel_task_t __c_waiting;      /* Threads waiting on this condition */
};

#section functions
#include <sys/time.h>
#depend "marcel_alias.h[macros]"
#depend "marcel_mutex.h[types]"

DEC_MARCEL(int, condattr_init, (marcel_condattr_t *attr) __THROW);
DEC_MARCEL(int, condattr_destroy, (marcel_condattr_t *attr) __THROW);

DEC_MARCEL(int, cond_init, (marcel_cond_t *cond,
			    __const marcel_condattr_t *attr) __THROW);
DEC_MARCEL(int, cond_destroy, (marcel_cond_t *cond) __THROW);

DEC_MARCEL(int, cond_signal, (marcel_cond_t *cond) __THROW);
DEC_MARCEL(int, cond_broadcast, (marcel_cond_t *cond) __THROW);

DEC_MARCEL(int, cond_wait, (marcel_cond_t *cond,
			    marcel_mutex_t *mutex) __THROW);
DEC_MARCEL(int, cond_timedwait, (marcel_cond_t *cond, marcel_mutex_t *mutex,
				 const struct timespec *abstime) __THROW);


DEC_POSIX(int, condattr_init, (pmarcel_condattr_t *attr));
DEC_POSIX(int, condattr_destroy, (pmarcel_condattr_t *attr));

DEC_POSIX(int, cond_init, (pmarcel_cond_t *cond,
			   __const pmarcel_condattr_t *attr));
DEC_POSIX(int, cond_destroy, (pmarcel_cond_t *cond));

DEC_POSIX(int, cond_signal, (pmarcel_cond_t *cond));
DEC_POSIX(int, cond_broadcast, (pmarcel_cond_t *cond));

DEC_POSIX(int, cond_wait, (pmarcel_cond_t *cond,
			   pmarcel_mutex_t *mutex));
DEC_POSIX(int, cond_timedwait, (pmarcel_cond_t *cond, pmarcel_mutex_t *mutex,
				const struct timespec *abstime));
#section common
