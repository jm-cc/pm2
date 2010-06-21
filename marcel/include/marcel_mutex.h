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


#ifndef __MARCEL_MUTEX_H__
#define __MARCEL_MUTEX_H__


#include <sys/time.h>
#include <asm/linux_types.h>
#include "sys/marcel_flags.h"
#include "marcel_types.h"
#include "marcel_fastlock.h"


/** Public macros **/
#define MARCEL_MUTEX_INITIALIZER \
  {.__data = {.__lock=MA_MARCEL_FASTLOCK_UNLOCKED}}
#define MARCEL_RECURSIVEMUTEX_INITIALIZER \
  {.__data = {.__lock=MA_MARCEL_FASTLOCK_UNLOCKED}}

/* pour pthread_mutexattr_setprotocol */
#define PMARCEL_PRIO_NONE 0
#define PMARCEL_PRIO_INHERIT 1
#define PMARCEL_PRIO_PROTECT 2

#define MARCEL_ONCE_INIT 0
#ifdef MA__IFACE_PMARCEL
#  define PMARCEL_ONCE_INIT 0
#endif


/** Public data structures **/
/* Attribute for mutex.  */
struct marcel_mutexattr {
};
struct marcel_recursivemutexattr {
};

#ifdef MA__IFACE_PMARCEL
struct pmarcel_mutexattr {
	int mutexkind;
};
#endif


/** Public data types **/
enum {
	MARCEL_MUTEX_NORMAL
};
#ifdef MA__IFACE_PMARCEL
/* Mutex types.  */
enum {
	PMARCEL_MUTEX_TIMED_NP = MARCEL_MUTEX_NORMAL,
	PMARCEL_MUTEX_RECURSIVE_NP,
	PMARCEL_MUTEX_ERRORCHECK_NP,
	PMARCEL_MUTEX_ADAPTIVE_NP,

	PMARCEL_MUTEX_NORMAL = PMARCEL_MUTEX_TIMED_NP,
	PMARCEL_MUTEX_RECURSIVE = PMARCEL_MUTEX_RECURSIVE_NP,
	PMARCEL_MUTEX_ERRORCHECK = PMARCEL_MUTEX_ERRORCHECK_NP,
	PMARCEL_MUTEX_DEFAULT = PMARCEL_MUTEX_NORMAL,

	/* For compatibility.  */
	PMARCEL_MUTEX_FAST_NP = PMARCEL_MUTEX_TIMED_NP
};
#endif

/* Process shared or private flag.  */
enum {
	MARCEL_PROCESS_PRIVATE,
#define MARCEL_PROCESS_PRIVATE MARCEL_PROCESS_PRIVATE
	MARCEL_PROCESS_SHARED
#define MARCEL_PROCESS_SHARED  MARCEL_PROCESS_SHARED
};
#ifdef MA__IFACE_PMARCEL
enum {
	PMARCEL_PROCESS_PRIVATE,
#  define PMARCEL_PROCESS_PRIVATE PMARCEL_PROCESS_PRIVATE
	PMARCEL_PROCESS_SHARED
#  define PMARCEL_PROCESS_SHARED  PMARCEL_PROCESS_SHARED
};
#endif

#ifdef MA__IFACE_PMARCEL
/* Mutex initializers.  */
#  define PMARCEL_MUTEX_INITIALIZER \
  {.__data = {.__lock=MA_PMARCEL_FASTLOCK_UNLOCKED}}
#  define PMARCEL_RECURSIVE_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, PMARCEL_MUTEX_RECURSIVE_NP, MA_PMARCEL_FASTLOCK_UNLOCKED } }
#  define PMARCEL_ERRORCHECK_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, PMARCEL_MUTEX_ERRORCHECK_NP, MA_PMARCEL_FASTLOCK_UNLOCKED } }
#  define PMARCEL_ADAPTIVE_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, PMARCEL_MUTEX_ADAPTIVE_NP, MA_PMARCEL_FASTLOCK_UNLOCKED } }
#endif

/* pas PTHREAD car déjà dans pthread.h, pas MARCEL car il a son propre initializer */
typedef union {
	struct marcel_mutexattr __data;
	long int __align;
} marcel_mutexattr_t;
typedef union {
	struct marcel_recursivemutexattr __data;
	long int __align;
} marcel_recursivemutexattr_t;

#ifdef MA__IFACE_PMARCEL
union __pmarcel_mutexattr_t {
	struct pmarcel_mutexattr __data;
	long int __align;
};
#endif

typedef union {
	struct {
		unsigned int __count;
		marcel_t owner;
		struct _marcel_fastlock __lock;
	} __data;
	long int __align;
} marcel_recursivemutex_t;

#ifdef MA__IFACE_PMARCEL
union __pmarcel_mutex_t {
	struct {
		unsigned int __nusers;
		unsigned int __count;
		marcel_t __owner;
		int __kind;
		struct _marcel_fastlock __lock;
	} __data;
	long int __align;
};
#endif


/** Public functions **/
/* Initialize a mutex.  */
extern int marcel_mutex_init(marcel_mutex_t * __restrict __mutex,
			     __const marcel_mutexattr_t * __restrict __mutexattr) __THROW;

/* Destroy a mutex.  */
extern int marcel_mutex_destroy(marcel_mutex_t * __mutex) __THROW;

/* Try locking a mutex.  */
extern int marcel_mutex_trylock(marcel_mutex_t * _mutex) __THROW;

/* Lock a mutex.  */
extern int marcel_mutex_lock(marcel_mutex_t * __mutex) __THROW;

/* Wait until lock becomes available, or specified time passes. */
extern int marcel_mutex_timedlock(marcel_mutex_t * __restrict __mutex,
				  __const struct timespec *__restrict __abstime) __THROW;

/* Unlock a mutex.  */
extern int marcel_mutex_unlock(marcel_mutex_t * __mutex) __THROW;


/* Functions for handling mutex attributes.  */

/* Initialize mutex attribute object ATTR with default attributes
   (kind is MARCEL_MUTEX_TIMED_NP).  */
extern int marcel_mutexattr_init(marcel_mutexattr_t * __attr) __THROW;

/* Destroy mutex attribute object ATTR.  */
extern int marcel_mutexattr_destroy(marcel_mutexattr_t * __attr) __THROW;


/* Initialize a mutex.  */
extern int marcel_recursivemutex_init(marcel_recursivemutex_t * __restrict __mutex,
				      __const marcel_recursivemutexattr_t *
				      __restrict __mutexattr) __THROW;

/* Destroy a mutex.  */
extern int marcel_recursivemutex_destroy(marcel_recursivemutex_t * __mutex) __THROW;

/* Try locking a mutex.  Return nesting level (number of times we now have it.)  */
extern int marcel_recursivemutex_trylock(marcel_recursivemutex_t * _mutex) __THROW;

/* Lock a mutex.  Return nesting level (number of times we now have it.)  */
extern int marcel_recursivemutex_lock(marcel_recursivemutex_t * __mutex) __THROW;

/* Wait until lock becomes available, or specified time passes. */
extern int marcel_recursivemutex_timedlock(marcel_recursivemutex_t * __restrict __mutex,
					   __const struct timespec *__restrict
					   __abstime) __THROW;

/* Unlock a mutex.  Return nesting level (number of times we still have it.)  */
extern int marcel_recursivemutex_unlock(marcel_recursivemutex_t * __mutex) __THROW;


/* Functions for handling mutex attributes.  */

/* Initialize mutex attribute object ATTR with default attributes
   (kind is PMARCEL_MUTEX_RECURSIVE_NP).  */
extern int marcel_recursivemutexattr_init(marcel_recursivemutexattr_t * __attr) __THROW;

/* Destroy mutex attribute object ATTR.  */
extern int marcel_recursivemutexattr_destroy(marcel_recursivemutexattr_t *
					     __attr) __THROW;

/* PART PMARCEL */
/*#line 146 "include/marcel_mutex.h.m4"*/
#ifdef MA__IFACE_PMARCEL
extern int pmarcel_mutex_init(pmarcel_mutex_t * __restrict __mutex,
			      __const pmarcel_mutexattr_t * __restrict __mutexattr)
    __THROW;
extern int pmarcel_mutex_destroy(pmarcel_mutex_t * __mutex) __THROW;
extern int pmarcel_mutex_trylock(pmarcel_mutex_t * _mutex) __THROW;
extern int pmarcel_mutex_lock(pmarcel_mutex_t * __mutex) __THROW;
extern int pmarcel_mutex_timedlock(pmarcel_mutex_t * __restrict __mutex,
				   __const struct timespec *__restrict __abstime) __THROW;
extern int pmarcel_mutex_unlock(pmarcel_mutex_t * __mutex) __THROW;
extern int pmarcel_mutex_unlock_usercnt(pmarcel_mutex_t * mutex, int decr) __THROW;
extern int pmarcel_mutexattr_init(pmarcel_mutexattr_t * __attr) __THROW;
extern int pmarcel_mutexattr_destroy(pmarcel_mutexattr_t * __attr) __THROW;
extern int pmarcel_mutexattr_getpshared(__const pmarcel_mutexattr_t *
					__restrict __attr,
					int *__restrict __pshared) __THROW;
extern int pmarcel_mutexattr_setpshared(pmarcel_mutexattr_t * __attr,
					int __pshared) __THROW;
extern int pmarcel_mutexattr_gettype(__const pmarcel_mutexattr_t * __restrict
				     __attr, int *__restrict __kind) __THROW;
extern int pmarcel_mutexattr_settype(pmarcel_mutexattr_t * __attr, int __kind) __THROW;
#endif

#ifdef MARCEL_ONCE_ENABLED
/* Functions for handling initialization.  */

/* Guarantee that the initialization function INIT_ROUTINE will be called
   only once, even if marcel_once is executed several times with the
   same ONCE_CONTROL argument. ONCE_CONTROL must point to a static or
   extern variable initialized to MARCEL_ONCE_INIT.  */
extern int marcel_once(marcel_once_t * __once_control,
		       void (*__init_routine) (void)) __THROW;
#  ifdef MA__IFACE_PMARCEL
extern int pmarcel_once(pmarcel_once_t * __once_control, void (*__init_routine) (void));
#  endif
#endif				/* MARCEL_ONCE_ENABLED */


#endif /** __MARCEL_MUTEX_H__ **/
