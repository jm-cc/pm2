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


#ifndef __MARCEL_TYPES_H__
#define __MARCEL_TYPES_H__


#include "tbx_compiler.h"
#include "tbx_intdef.h"
#include "sys/marcel_flags.h"
#include "marcel_config.h"


/** Public data types **/
// Task
typedef struct ma_task marcel_task_t;
typedef marcel_task_t *p_marcel_task_t;
typedef p_marcel_task_t marcel_t, lpt_t;
typedef struct marcel_sched_param marcel_sched_param_t;


// LWPS
typedef struct marcel_lwp *ma_lwp_t;


// Topology
/* \brief Virtual processor set: defines the set of "allowed" LWP for a given thread */
#if !defined(MA__LWPS) || MARCEL_NBMAXCPUS <= 32
typedef uint32_t marcel_vpset_t;	/* FIXME: uint_fast32_t if available? */
#elif MARCEL_NBMAXCPUS <= 64
typedef uint64_t marcel_vpset_t;
#else
/* size and count of subsets within a set */
#define MA_VPSUBSET_SIZE		(8*sizeof(long))
#define MA_VPSUBSET_COUNT		((MARCEL_NBMAXCPUS+MA_VPSUBSET_SIZE-1)/MA_VPSUBSET_SIZE)
typedef struct {
	unsigned long s[MA_VPSUBSET_COUNT];
} marcel_vpset_t;
#endif
typedef marcel_vpset_t pmarcel_cpu_set_t;
/** \brief Type of a topology level */
typedef struct marcel_topo_level marcel_topo_level_t;


// Stats
#ifdef MARCEL_STATS_ENABLED
typedef char ma_stats_t[MARCEL_STATS_ROOM] TBX_ALIGNED;
#endif


// Lock
/* Attribute for read-write locks (from glibc's
 * `sysdeps/unix/sysv/linux/internaltypes.h').  */
struct marcel_rwlockattr {
	int __lockkind;
	int __pshared;
};




#include "asm/nptl_types.h"


// Cond types
typedef union __marcel_condattr_t marcel_condattr_t;
/* Conditions (not abstract because of MARCEL_COND_INITIALIZER */
typedef union {
	struct {
		/* Protect against concurrent access */
		struct _marcel_fastlock __lock;
		/* Threads waiting on this condition */
		p_marcel_task_t __waiting;
	} __data;
	long int __align;
} marcel_cond_t;


// Mutex types
typedef union {
	struct {
		struct _marcel_fastlock __lock;
	} __data;
	long int __align;
} marcel_mutex_t;


// Sem types
typedef struct semcell_struct semcell;
struct semaphor_struct {
	int value;
	struct semcell_struct *first, *last;
	ma_spinlock_t lock;	/* For preventing concurrent access from multiple LWPs */
};


// Barrier types
#define MA_BARRIER_USE_MUTEX 0
typedef enum marcel_barrier_mode {
	MA_BARRIER_SLEEP_MODE,
	MA_BARRIER_YIELD_MODE
} ma_barrier_mode_t;

typedef struct {
	unsigned int init_count;
#ifdef  MA_BARRIER_USE_MUTEX
	marcel_mutex_t m;
	marcel_cond_t c;
#else
	struct _marcel_fastlock lock;
#endif
	ma_atomic_t leftB;
	ma_atomic_t leftE;
	ma_barrier_mode_t mode;
} marcel_barrier_t;


// Pmarcel types
typedef unsigned long int pmarcel_t;
typedef struct marcel_spinlock marcel_spinlock_t, pmarcel_spinlock_t;
typedef struct __marcel_attr_s marcel_attr_t, pmarcel_attr_t;
typedef struct marcel_rwlockattr marcel_rwlockattr_t, pmarcel_rwlockattr_t;
typedef lpt_rwlock_t marcel_rwlock_t, pmarcel_rwlock_t;
typedef struct semaphor_struct marcel_sem_t, pmarcel_sem_t;
typedef unsigned int marcel_sigset_t, pmarcel_sigset_t;

#ifdef MARCEL_KEYS_ENABLED
/* Keys for thread-specific data */
typedef unsigned int marcel_key_t, pmarcel_key_t;
#endif				/* MARCEL_KEYS_ENABLED */

#ifdef MA__IFACE_PMARCEL
typedef union __pmarcel_mutex_t pmarcel_mutex_t;
typedef union __pmarcel_mutexattr_t pmarcel_mutexattr_t;
typedef marcel_condattr_t pmarcel_condattr_t;
typedef marcel_cond_t pmarcel_cond_t;
typedef marcel_barrier_t pmarcel_barrier_t;
#endif				/* MA__IFACE_PMARCEL */

#ifdef MARCEL_ONCE_ENABLED
typedef int marcel_once_t;
#  ifdef MA__IFACE_PMARCEL
typedef marcel_once_t pmarcel_once_t;
#  endif
#endif				/* MARCEL_ONCE_ENABLED */


#endif /** __MARCEL_TYPES_H__ **/
