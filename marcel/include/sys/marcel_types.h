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


#ifndef __SYS_MARCEL_TYPES_H__
#define __SYS_MARCEL_TYPES_H__


#include "tbx_compiler.h"
#include "tbx_types.h"
#include "marcel_config.h"


/** Public data types **/
// Task
typedef struct ma_task marcel_task_t;
typedef marcel_task_t* marcel_t;
typedef marcel_t lpt_t;
typedef struct marcel_sched_param marcel_sched_param_t;

// Topology
#ifdef MA__LWPS
#  define MA_NR_VPS (MARCEL_NBMAXCPUS+MARCEL_NBMAXVPSUP)
#else
#  define MA_NR_VPS 1
#endif

/* \brief Virtual processor set: defines the set of "allowed" LWP for a given thread */
#if !defined(MA__LWPS) || MA_NR_VPS <= 32
#  if SIZEOF_UINT_FAST32_T > 0
typedef uint_fast32_t marcel_vpset_t;
#  else
typedef uint32_t marcel_vpset_t;
#  endif
#elif MA_NR_VPS <= 64
typedef uint64_t marcel_vpset_t;
#else
/* size and count of subsets within a set */
#define MA_VPSUBSET_SIZE		(8*sizeof(long))
#define MA_VPSUBSET_COUNT		((MA_NR_VPS+MA_VPSUBSET_SIZE-1)/MA_VPSUBSET_SIZE)
typedef struct {
	unsigned long s[MA_VPSUBSET_COUNT];
} marcel_vpset_t;
#endif
/** \brief Type of a topology level */
typedef struct marcel_topo_level marcel_topo_level_t;

// Stats
typedef char marcel_stats_t[MARCEL_STATS_ROOM] TBX_ALIGNED;

// Marcel types
#include "asm/nptl_types.h"
typedef struct marcel_spinlock marcel_spinlock_t;
typedef struct __marcel_attr_s marcel_attr_t;
typedef struct marcel_rwlockattr marcel_rwlockattr_t;
typedef lpt_rwlock_t marcel_rwlock_t;
typedef struct semaphor_struct marcel_sem_t;
typedef unsigned int marcel_sigset_t;
typedef unsigned int marcel_key_t;
typedef int marcel_once_t;


#ifdef __MARCEL_KERNEL__


// LWPS
typedef struct marcel_lwp* ma_lwp_t;


#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_MARCEL_TYPES_H__ **/

