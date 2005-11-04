m4_dnl -*- linux-c -*-
m4_include(scripts/marcel.m4)
dnl /***************************
dnl  * This is the original file
dnl  * =========================
dnl  ***************************/
/* This file has been autogenerated from m4___file__ */
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
#define MARCEL_MUTEX_INITIALIZER \
  {.__data.__lock=MA_FASTLOCK_UNLOCKED}


#section macros
REPLICATE([[dnl
/* Single execution handling.  */
#define PREFIX_ONCE_INIT 0
]])

#section types
REPLICATE([[dnl
/* Once-only execution */
typedef int prefix_once_t;
]], [[MARCEL PMARCEL]])/* pas LPT car d�pendant de l'archi */


REPLICATE([[dnl
/* Mutex types.  */
enum
{
  PREFIX_MUTEX_TIMED_NP,
  PREFIX_MUTEX_RECURSIVE_NP,
  PREFIX_MUTEX_ERRORCHECK_NP,
  PREFIX_MUTEX_ADAPTIVE_NP
//#ifdef __USE_UNIX98
  ,
  PREFIX_MUTEX_NORMAL = PREFIX_MUTEX_TIMED_NP,
  PREFIX_MUTEX_RECURSIVE = PREFIX_MUTEX_RECURSIVE_NP,
  PREFIX_MUTEX_ERRORCHECK = PREFIX_MUTEX_ERRORCHECK_NP,
  PREFIX_MUTEX_DEFAULT = PREFIX_MUTEX_NORMAL
//#endif
//#ifdef __USE_GNU
  /* For compatibility.  */
  , PREFIX_MUTEX_FAST_NP = PREFIX_MUTEX_TIMED_NP
//#endif
};
]], [[PMARCEL LPT]])/* pas MARCEL car il n'a pas tout ces modes */

enum {
  MARCEL_MUTEX_NORMAL,
};

REPLICATE([[dnl
/* Process shared or private flag.  */
enum
{
  PREFIX_PROCESS_PRIVATE,
#define PREFIX_PROCESS_PRIVATE PREFIX_PROCESS_PRIVATE
  PREFIX_PROCESS_SHARED
#define PREFIX_PROCESS_SHARED  PREFIX_PROCESS_SHARED
};
]], [[PMARCEL LPT]])/* pas MARCEL car il n'a pas ces modes */

#include <asm/linux_types.h>
REPLICATE([[dnl
/* Mutex initializers.  */
#define PREFIX_MUTEX_INITIALIZER \
  { }
#ifdef __USE_GNU
# if MA_BITS_PER_LONG == 64
#  define PREFIX_RECURSIVE_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, 0, PREFIX_MUTEX_RECURSIVE_NP } }
#  define PREFIX_ERRORCHECK_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, 0, PREFIX_MUTEX_ERRORCHECK_NP } }
#  define PREFIX_ADAPTIVE_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, 0, PREFIX_MUTEX_ADAPTIVE_NP } }
# else
#  define PREFIX_RECURSIVE_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, PREFIX_MUTEX_RECURSIVE_NP } }
#  define PREFIX_ERRORCHECK_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, PREFIX_MUTEX_ERRORCHECK_NP } }
#  define PREFIX_ADAPTIVE_MUTEX_INITIALIZER_NP \
  { { 0, 0, 0, PREFIX_MUTEX_ADAPTIVE_NP } }
# endif
#endif
]],[[PMARCEL LPT]])/* pas PTHREAD car d�j� dans pthread.h, pas MARCEL car il a son propre initializer */

#section structures
#depend "marcel_threads.h[types]"

REPLICATE([[dnl
/* Attribute for mutex.  */
struct prefix_mutexattr
{
	int mutexkind;
};

typedef union
{
	struct prefix_mutexattr __data;
	long int __align;
} prefix_mutexattr_t;
]],[[MARCEL PMARCEL]])/* pas LPT car d�pendant de l'archi */

typedef union
{
	struct
	{
		unsigned int __nusers;
		unsigned int __count;
		marcel_t __owner;
		int __kind;
		struct _marcel_fastlock __lock;
	} __data;
	long int __align;
} pmarcel_mutex_t;

typedef union
{
	struct
	{
		struct _marcel_fastlock __lock;
	} __data;
	long int __align;
} marcel_mutex_t;

#section functions
#include <sys/time.h>
REPLICATE([[dnl
/* Initialize a mutex.  */
extern int prefix_mutex_init (prefix_mutex_t *__mutex,
                               __const prefix_mutexattr_t *__mutexattr)
     __THROW;

/* Destroy a mutex.  */
extern int prefix_mutex_destroy (prefix_mutex_t *__mutex) __THROW;

/* Try locking a mutex.  */
extern int prefix_mutex_trylock (prefix_mutex_t *_mutex) __THROW;

/* Lock a mutex.  */
extern int prefix_mutex_lock (prefix_mutex_t *__mutex) __THROW;

#ifdef __USE_XOPEN2K
/* Wait until lock becomes available, or specified time passes. */
extern int prefix_mutex_timedlock (prefix_mutex_t *__restrict __mutex,
                                    __const struct timespec *__restrict
                                    __abstime) __THROW;
#endif

/* Unlock a mutex.  */
extern int prefix_mutex_unlock (prefix_mutex_t *__mutex) __THROW;


/* Functions for handling mutex attributes.  */

/* Initialize mutex attribute object ATTR with default attributes
   (kind is PREFIX_MUTEX_TIMED_NP).  */
extern int prefix_mutexattr_init (prefix_mutexattr_t *__attr) __THROW;

/* Destroy mutex attribute object ATTR.  */
extern int prefix_mutexattr_destroy (prefix_mutexattr_t *__attr) __THROW;

/* Get the process-shared flag of the mutex attribute ATTR.  */
extern int prefix_mutexattr_getpshared (__const prefix_mutexattr_t *
                                         __restrict __attr,
                                         int *__restrict __pshared) __THROW;

/* Set the process-shared flag of the mutex attribute ATTR.  */
extern int prefix_mutexattr_setpshared (prefix_mutexattr_t *__attr,
                                         int __pshared) __THROW;

//#ifdef __USE_UNIX98
/* Return in *KIND the mutex kind attribute in *ATTR.  */
extern int prefix_mutexattr_gettype (__const prefix_mutexattr_t *__restrict
                                      __attr, int *__restrict __kind) __THROW;

/* Set the mutex kind attribute in *ATTR to KIND (either PREFIX_MUTEX_NORMAL,
   PREFIX_MUTEX_RECURSIVE, PREFIX_MUTEX_ERRORCHECK, or
   PREFIX_MUTEX_DEFAULT).  */
extern int prefix_mutexattr_settype (prefix_mutexattr_t *__attr, int __kind)
     __THROW;
//#endif

/* Functions for handling initialization.  */

/* Guarantee that the initialization function INIT_ROUTINE will be called
   only once, even if prefix_once is executed several times with the
   same ONCE_CONTROL argument. ONCE_CONTROL must point to a static or
   extern variable initialized to PREFIX_ONCE_INIT.  */
extern int prefix_once (prefix_once_t *__once_control,
                         void (*__init_routine) (void)) __THROW;
]],[[MARCEL PMARCEL LPT]])dnl END_REPLICATE



REPLICATE([[dnl
extern void __prefix_once_fork_prepare (void);
extern void __prefix_once_fork_parent (void);
extern void __prefix_once_fork_child (void);
extern void __pmarcel_once_fork_prepare (void);
extern void __pmarcel_once_fork_parent (void);
extern void __pmarcel_once_fork_child (void);
]],[[PTHREAD]])
