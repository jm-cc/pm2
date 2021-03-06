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


#ifndef __ASM_X86_64_NPTL_TYPES_H__
#define __ASM_X86_64_NPTL_TYPES_H__


#include <time.h>
#include <stddef.h>
#include "sys/marcel_debug.h"
#include "sys/marcel_fastlock.h"
#include "sys/marcel_types.h"


/** Public macros **/
#define __SIZEOF_LPT_ATTR_T 56
#define __SIZEOF_LPT_MUTEX_T 40
#define __OFFSETOF_LPT_MUTEX_KIND 16
#define __SIZEOF_LPT_MUTEXATTR_T 4
#define __SIZEOF_LPT_COND_T 48
#define __SIZEOF_LPT_CONDATTR_T 4
#define __SIZEOF_LPT_RWLOCK_T 56
#define __OFFSETOF_LPT_RWLOCK_FLAGS 48
#define __SIZEOF_LPT_RWLOCKATTR_T 8
#define __SIZEOF_LPT_BARRIER_T 32
#define __SIZEOF_LPT_BARRIERATTR_T 4


/** Public data structures **/
typedef union {
	char __size[__SIZEOF_LPT_ATTR_T];
	long int __align;
} lpt_attr_t;

/* Data structures for mutex handling.  The structure of the attribute
   type is not exposed on purpose.  */
typedef union {
	struct {
		unsigned int __nusers;
		unsigned int __count;
		marcel_t __owner;
		/* KIND must stay at this position in the structure to maintain
		   binary compatibility with static initializers.  */
		int __kind;
		struct _lpt_fastlock __lock;
	} __data;
	char __size[__SIZEOF_LPT_MUTEX_T];
	long int __align;
} lpt_mutex_t;

struct lpt_mutexattr {
	int mutexkind;
};

typedef union {
	struct lpt_mutexattr __data;
	char __size[__SIZEOF_LPT_MUTEXATTR_T];
	int __align;
} lpt_mutexattr_t;


/* Data structure for conditional variable handling.  The structure of
 * the attribute type is not exposed on purpose.
 *
 * NB: We will store clk_id from condattr_t into __nwaiters field 
 *     without doing bit operations like in glibc. We do not need to 
 *     store  other information. It is compliant with the static
 *     initializer ... per-default clock is CLOCK_REALTIME (= 0) */
typedef union {
	struct {
		struct _lpt_fastlock __lock;
		marcel_t __waiting;
#ifdef CLOCK_REALTIME
		unsigned long __unused_pad[3];
		clockid_t __clk_id; //__nwaiters field in glibc
#endif
	} __data;
	char __size[__SIZEOF_LPT_COND_T];
	__extension__ long long int __align;
} lpt_cond_t;

typedef union {
	char __size[__SIZEOF_LPT_CONDATTR_T];
	int __align;
} lpt_condattr_t;


/* Keys for thread-specific data */
typedef unsigned int lpt_key_t;


/* Once-only execution */
typedef int lpt_once_t;


/* Data structure for read-write lock variable handling.  The
   structure of the attribute type is not exposed on purpose.  */
typedef union {
	struct {
		struct _lpt_fastlock __lock;
		unsigned int __nr_readers;
		int __shared;
		struct _lpt_fastlock __readers_wakeup;
		struct _lpt_fastlock __writer_wakeup;
		unsigned int __nr_readers_queued;
		unsigned int __nr_writers_queued;
		marcel_t __writer;
		/* FLAGS must stay at this position in the structure to maintain
		   binary compatibility with static initializers.  */
		unsigned int __flags;
	} __data;
	char __size[__SIZEOF_LPT_RWLOCK_T];
	long int __align;
} lpt_rwlock_t;

typedef union {
	char __size[__SIZEOF_LPT_RWLOCKATTR_T];
	long int __align;
} lpt_rwlockattr_t;


/* POSIX spinlock data type.  */
typedef volatile int lpt_spinlock_t;

/* POSIX barriers data type. */
struct lpt_barrier {
	unsigned int init_count;
	struct _lpt_fastlock lock;
	ma_atomic_t leftB;
	ma_atomic_t leftE;
};


/* POSIX barriers data type.  The structure of the type is
   deliberately not exposed.  */
typedef union {
	char __size[__SIZEOF_LPT_BARRIER_T];
	long int __align;
} lpt_barrier_t;

struct lpt_barrierattr {
	int pshared;
};

typedef union {
	char __size[__SIZEOF_LPT_BARRIERATTR_T];
	int __align;
} lpt_barrierattr_t;


/* Check that the size of our data structures is compatible with those of
   NPTL.  */
MA_VERIFY(sizeof(lpt_attr_t) <= __SIZEOF_LPT_ATTR_T);
MA_VERIFY(sizeof(lpt_mutex_t) <= __SIZEOF_LPT_MUTEX_T);
MA_VERIFY(offsetof(lpt_mutex_t, __data.__kind) == __OFFSETOF_LPT_MUTEX_KIND);
MA_VERIFY(sizeof(lpt_mutexattr_t) <= __SIZEOF_LPT_MUTEXATTR_T);
MA_VERIFY(sizeof(lpt_cond_t) <= __SIZEOF_LPT_COND_T);
MA_VERIFY(sizeof(lpt_condattr_t) <= __SIZEOF_LPT_CONDATTR_T);
MA_VERIFY(sizeof(lpt_rwlock_t) <= __SIZEOF_LPT_RWLOCK_T);
MA_VERIFY(offsetof(lpt_rwlock_t, __data.__flags) == __OFFSETOF_LPT_RWLOCK_FLAGS);
MA_VERIFY(sizeof(lpt_rwlockattr_t) <= __SIZEOF_LPT_RWLOCKATTR_T);
MA_VERIFY(sizeof(lpt_barrier_t) <= __SIZEOF_LPT_BARRIER_T);
MA_VERIFY(sizeof(lpt_barrierattr_t) <= __SIZEOF_LPT_BARRIERATTR_T);


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#define __ma_cleanup_fct_attribute


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __ASM_X86_64_NPTL_TYPES_H__ **/
