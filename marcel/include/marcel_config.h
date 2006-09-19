 
/* This file has been autogenerated from include/marcel_mutex.h.m4 */
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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

/* ========== customization =========== */

#define MAX_KEY_SPECIFIC	20

#define MARCEL_MAX_USER_SCHED	16

#define THREAD_THRESHOLD_LOW	1

#define MARCEL_MAXNAMESIZE	32

#define MARCEL_ALIGN		64

#define MA_PER_LWP_ROOM		32768
#define MA_PER_LEVEL_ROOM	4096

#define MA_TLS_AREA_SIZE        ((unsigned long) 4096)

#ifdef MA__LWPS
#define MARCEL_NBMAXCPUS	(sizeof(unsigned long)*8)
#ifdef MA__NUMA
#define MARCEL_NBMAXCORES	16
#define MARCEL_NBMAXDIES	8
#define MARCEL_NBMAXNODES	8
#endif
#endif

#ifdef MA__LWPS
#define MARCEL_NBMAXVPSUP	2
#else
/* don't change this */
#define MARCEL_NBMAXVPSUP	0
#endif

#define MARCEL_LEVEL_DEFAULT	0
#define MARCEL_LEVEL_KEEPCLOSED	INT_MAX

//#define MARCEL_GANG_SCHEDULER

#if defined(MA__IFACE_LPT)
#define MA_MAX_USER_RT_PRIO	99
#elif defined(MA__IFACE_PMARCEL)
#define MA_MAX_USER_RT_PRIO	32
#else
#define MA_MAX_USER_RT_PRIO	9
#endif
#define MA_MAX_SYS_RT_PRIO	9
#define MA_MAX_NICE		0

/* TODO: utiliser la priorit� pour le calculer */
#define MARCEL_TASK_TIMESLICE	1
#define MARCEL_BUBBLE_TIMESLICE	10

#define MARCEL_SCHED_DEFAULT	MARCEL_SCHED_BALANCE

/* ========== timer =================== */

/* Unit� : microsecondes */
#define MARCEL_MIN_TIME_SLICE		10000
#define MARCEL_DEFAULT_TIME_SLICE	MARCEL_MIN_TIME_SLICE

#include <signal.h>
#ifdef MA__TIMER
#if defined(MA__LWPS) && (!defined(SA_SIGINFO) || defined(OSF_SYS) || defined(AIX_SYS) || defined(DARWIN_SYS))
/* no way to distinguish between a signal from the kernel or from another LWP */
#define MA_BOGUS_SIGINFO_CODE
#endif
/*  Signal utilis� pour la premption automatique */
#ifdef USE_VIRTUAL_TIMER
#  define MARCEL_TIMER_SIGNAL       SIGVTALRM
#  ifdef MA_BOGUS_SIGINFO_CODE
#    define MARCEL_TIMER_USERSIGNAL SIGALRM
#  else
#    define MARCEL_TIMER_USERSIGNAL MARCEL_TIMER_SIGNAL
#  endif
#  define MARCEL_ITIMER_TYPE        ITIMER_VIRTUAL
#else
#  define MARCEL_TIMER_SIGNAL       SIGALRM
#  ifdef MA_BOGUS_SIGINFO_CODE
#    define MARCEL_TIMER_USERSIGNAL SIGVTALRM
#  else
#    define MARCEL_TIMER_USERSIGNAL MARCEL_TIMER_SIGNAL
#  endif
#  define MARCEL_ITIMER_TYPE        ITIMER_REAL
#endif
#endif /* MA__TIMER */
#define MARCEL_RESCHED_SIGNAL     SIGXFSZ
