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

/* Max number of marcel_key_create() calls */
#ifdef MA__IFACE_PMARCEL
#define MAX_KEY_SPECIFIC	128
#else
#define MAX_KEY_SPECIFIC	20
#endif

/* Max number of marcel_schedpolicy_create() calls */
#define MARCEL_MAX_USER_SCHED	16

/* Threshold for MARCEL_SCHED_AFFINITY policy */
#define THREAD_THRESHOLD_LOW	1

/* Max size for thread names */
#define MARCEL_MAXNAMESIZE	32

/* Alignment of marcel_t */
#define MARCEL_ALIGN		64

/* Size of stacks, must be a power of two, and be at least twice as much
 * as PTHREAD_STACK_MIN */
/* #define MARCEL_THREAD_SLOT_SIZE 0x10000UL */

/* Room for per-something variables */
#define MA_PER_LWP_ROOM		32768
#define MA_PER_LEVEL_ROOM	4096

/* Room for Exec TLS (not dynamic) */
#ifndef MA_TLS_AREA_SIZE
#define MA_TLS_AREA_SIZE        4096UL
#endif

/* Max number of architecture elements */
#ifdef MA__LWPS
#  ifndef MARCEL_NBMAXCPUS /* not enforced in the flavor */
#    define MARCEL_NBMAXCPUS	32
#  endif
#  ifdef MA__NUMA
#    ifndef MARCEL_NBMAXNODES /* not enforced in the flavor */
#      define MARCEL_NBMAXNODES	8
#    endif
#  endif
#else
#  define MARCEL_NBMAXCPUS	1
#endif

/* Max number of marcel_add_lwp() calls */
#ifdef MARCEL_NBMAXVPSUP
#  undef MARCEL_NBMAXVPSUP
#endif
#define MARCEL_NBMAXVPSUP	10

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
#define MARCEL_SCHED_DEFAULT	MARCEL_SCHED_SHARED

/* Max number of threads in cache, for each topology level */
#define MARCEL_THREAD_CACHE_MAX 100

/* ========== timer =================== */

/* Timer tick period, in µs */
#ifdef AIX_SYS
/* AIX has troubles with automatic preemption... */
#define MARCEL_MIN_TIME_SLICE		100000
#else
#define MARCEL_MIN_TIME_SLICE		10000
#endif
#define MARCEL_DEFAULT_TIME_SLICE	MARCEL_MIN_TIME_SLICE

/* Don't mask signals during signal handler (avoid two syscalls per timer preemption, but may lead to handler reentrancy) */
#define MA_SIGNAL_NOMASK

/* Timer time and signal */
#include <signal.h>
#ifdef MA__TIMER
#if defined(MA__LWPS) && (!defined(SA_SIGINFO) || defined(OSF_SYS) || defined(AIX_SYS) || defined(DARWIN_SYS))
/* no way to distinguish between a signal from the kernel or from another LWP */
#define MA_BOGUS_SIGINFO_CODE
#endif
/*  Signal utilisé pour la premption automatique */
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

/* don't change this */
#ifndef MA__LWPS
#undef MARCEL_NBMAXVPSUP
#define MARCEL_NBMAXVPSUP	0
#endif
