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


#ifndef __MARCEL_CONFIG_H__
#define __MARCEL_CONFIG_H__


#include "sys/marcel_flags.h"


/* Max number of marcel_schedpolicy_create() calls */
#define MARCEL_MAX_USER_SCHED	16

/* Threshold for MARCEL_SCHED_AFFINITY policy */
#define THREAD_THRESHOLD_LOW	1

/* Max size for thread names */
#define MARCEL_MAXNAMESIZE	32

/* Alignment of marcel_t */
#define MARCEL_ALIGN		64

/* Room for per-something variables */
#define MA_PER_LWP_ROOM		32768
#define MA_PER_LEVEL_ROOM	4096

/* lpt_tcb_t padding: sizeof(struct pthread) = 2288 (glibc-2.11.2) */
#define MA_NPTL_PTHREAD_SIZE    3072UL

/* Max number of marcel_add_lwp() calls */
#ifndef MA__LWPS
#define MARCEL_NBMAXVPSUP	0
#else
#define MARCEL_NBMAXVPSUP	10
#endif

/* Default scheduling level */
#define MARCEL_LEVEL_DEFAULT	0
/* Level of bubble recursion from which they are kept closed */
#define MARCEL_LEVEL_KEEPCLOSED	INT_MAX

/* Room for threads/bubbles statistics */
/* There are 9 defined marcel statistics for now, one of them points
   to an array of MARCEL_NBMAXNODES items. */
#ifdef MA__NUMA
#define MARCEL_STATS_ROOM	((9 + MARCEL_NBMAXNODES) * sizeof (long))
#else
#define MARCEL_STATS_ROOM	(9 * sizeof (long))
#endif

/* Number of distinct real-time priorities */
#if defined(MA__IFACE_LPT)
#define MA_MAX_USER_RT_PRIO	99
#elif defined(MA__IFACE_PMARCEL)
#define MA_MAX_USER_RT_PRIO	32
#else
#define MA_MAX_USER_RT_PRIO	9
#endif
/* Same, for PM2's own usage */
#define MA_MAX_SYS_RT_PRIO	9
/* Number of Nice prioritiés */
#define MA_MAX_NICE		0

/* TODO: utiliser la priorité pour le calculer */
/* Preemption timeslice for threads in timer ticks */
#define MARCEL_TASK_TIMESLICE	1
/* Timeslice for bubbles in timer ticks */
#define MARCEL_BUBBLE_TIMESLICE	10

/* Default LWP choice policy */
#define MARCEL_SCHED_DEFAULT	MARCEL_SCHED_BALANCE


#endif /** __MARCEL_CONFIG_H__ **/
