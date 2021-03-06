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


#ifndef __MARCEL_LPT_MUTEX_H__
#define __MARCEL_LPT_MUTEX_H__


#include <sys/time.h>
#include <asm/linux_types.h>
#include "marcel_config.h"
#include "sys/marcel_utils.h"
#include "marcel_threads.h"


/** Public macros **/
#ifdef MA__IFACE_LPT
#define LPT_ONCE_INIT 0
#endif


/** Public data types **/
#ifdef MA__IFACE_LPT
/* Mutex types.  */
enum {
	LPT_MUTEX_TIMED_NP,
	LPT_MUTEX_RECURSIVE_NP,
	LPT_MUTEX_ERRORCHECK_NP,
	LPT_MUTEX_ADAPTIVE_NP,

	LPT_MUTEX_NORMAL = LPT_MUTEX_TIMED_NP,
	LPT_MUTEX_RECURSIVE = LPT_MUTEX_RECURSIVE_NP,
	LPT_MUTEX_ERRORCHECK = LPT_MUTEX_ERRORCHECK_NP,
	LPT_MUTEX_DEFAULT = LPT_MUTEX_NORMAL,

	/* For compatibility.  */
	LPT_MUTEX_FAST_NP = LPT_MUTEX_TIMED_NP
};
/* Process shared or private flag.  */
enum {
	LPT_PROCESS_PRIVATE,
#define LPT_PROCESS_PRIVATE LPT_PROCESS_PRIVATE
	LPT_PROCESS_SHARED
#define LPT_PROCESS_SHARED  LPT_PROCESS_SHARED
};
#endif

#ifdef MA__IFACE_LPT
/* Mutex initializers.  */
#  define LPT_MUTEX_INITIALIZER \
  {.__data = {.__lock=MA_LPT_FASTLOCK_UNLOCKED}}
#  if MA_BITS_PER_LONG == 64
#    define LPT_RECURSIVE_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, 0, LPT_MUTEX_RECURSIVE_NP, MA_LPT_FASTLOCK_UNLOCKED } }
#    define LPT_ERRORCHECK_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, 0, LPT_MUTEX_ERRORCHECK_NP, MA_LPT_FASTLOCK_UNLOCKED } }
#    define LPT_ADAPTIVE_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, 0, LPT_MUTEX_ADAPTIVE_NP, MA_LPT_FASTLOCK_UNLOCKED } }
#  else
#    define LPT_RECURSIVE_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, LPT_MUTEX_RECURSIVE_NP, MA_LPT_FASTLOCK_UNLOCKED } }
#    define LPT_ERRORCHECK_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, LPT_MUTEX_ERRORCHECK_NP, MA_LPT_FASTLOCK_UNLOCKED } }
#    define LPT_ADAPTIVE_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, LPT_MUTEX_ADAPTIVE_NP, MA_LPT_FASTLOCK_UNLOCKED } }
#  endif
#endif


/** Public functions **/
#ifdef MA__IFACE_LPT
extern int lpt_mutex_init(lpt_mutex_t * __restrict __mutex,
			  __const lpt_mutexattr_t * __restrict __mutexattr) __THROW;
extern int lpt_mutex_destroy(lpt_mutex_t * __mutex) __THROW;
extern int lpt_mutex_trylock(lpt_mutex_t * _mutex) __THROW;
extern int lpt_mutex_lock(lpt_mutex_t * __mutex) __THROW;
extern int lpt_mutex_timedlock(lpt_mutex_t * __restrict __mutex,
			       __const struct timespec *__restrict __abstime) __THROW;
extern int lpt_mutex_unlock(lpt_mutex_t * __mutex) __THROW;
extern int lpt_mutexattr_init(lpt_mutexattr_t * __attr) __THROW;
extern int lpt_mutexattr_destroy(lpt_mutexattr_t * __attr) __THROW;
extern int lpt_mutexattr_getpshared(__const lpt_mutexattr_t *
				    __restrict __attr, int *__restrict __pshared) __THROW;
extern int lpt_mutexattr_setpshared(lpt_mutexattr_t * __attr, int __pshared) __THROW;
extern int lpt_mutexattr_gettype(__const lpt_mutexattr_t * __restrict
				 __attr, int *__restrict __kind) __THROW;
extern int lpt_mutexattr_settype(lpt_mutexattr_t * __attr, int __kind) __THROW;
#endif
#ifdef MARCEL_ONCE_ENABLED
#  ifdef MA__IFACE_LPT
extern int lpt_once(lpt_once_t * __once_control, void (*__init_routine) (void)) __THROW;
extern void lpt_once_fork_prepare(void);
extern void lpt_once_fork_parent(void);
extern void lpt_once_fork_child(void);
#  endif
#  ifdef MA__LIBPTHREAD
extern void __pthread_once_fork_prepare(void);
extern void __pthread_once_fork_parent(void);
extern void __pthread_once_fork_child(void);
extern void __pmarcel_once_fork_prepare(void);
extern void __pmarcel_once_fork_parent(void);
extern void __pmarcel_once_fork_child(void);
#  endif
#endif				/* MARCEL_ONCE_ENABLED */


#endif /** __MARCEL_LPT_MUTEX_H__ **/
