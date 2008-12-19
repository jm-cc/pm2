
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

#ifndef MARCEL_FLAGS_EST_DEF
#define MARCEL_FLAGS_EST_DEF

#ifdef PM2DEBUG
#  define MA__DEBUG
#endif

#ifdef MARCEL_MONO
#  undef MARCEL_MONO
#  define MARCEL_MONO 1
#endif

#ifdef MARCEL_SMP
#  undef MARCEL_SMP
#  define MARCEL_SMP 1
#endif

#ifdef MARCEL_NUMA
#  undef MARCEL_NUMA
#  define MARCEL_NUMA 1
#endif

#if ((MARCEL_MONO + MARCEL_SMP + MARCEL_NUMA) == 0)
#  error No MARCEL_... defined. Choose between MONO SMP and NUMA Marcel options in the flavor
//#define MARCEL_MONO 1
#endif

#if ((MARCEL_MONO + MARCEL_SMP + MARCEL_NUMA) > 1)
#  error You must define at most one of MARCEL_MONO, MARCEL_SMP or MARCEL_NUMA Marcel options in the flavor
#endif


/*
 * Options implemented by Marcel
 */

/* MA__LWPS : if set, we use several kernel threads running concurrently.
 * */
#ifdef MA__LWPS
#  undef MA__LWPS
#endif

/* MA__TIMER : if set, we use the unix timer (signals) for periodic
 * preemption.
 * */
#ifndef MA__TIMER
#  define MA__TIMER
#endif

/* MA__INTERRUPTS_FIX_LWP : if set, we need to fix the libpthread LWP
 * state in the timer handler thanks to
 * MA_ARCH_(SWITCHTO|INTERRUPT_(ENTER|EXIT))_LWP_FIX
 * Assumes MA__LWPS && !MA__LIBPTHREAD
 * */
#ifdef MA__INTERRUPTS_FIX_LWP
#  undef MA__INTERRUPTS_FIX_LWP
#endif

/* MA__IFACE_PMARCEL : if set, we define pmarcel_* structures and symbols
 * (API-compatible with POSIX)
 * */
#ifdef MA__IFACE_PMARCEL
#  undef MA__IFACE_PMARCEL
#endif

/* MA__IFACE_LPT : if set, we define lpt_* structures and symbols
 * (ABI-compatible with libpthread)
 * */
#ifdef MA__IFACE_LPT
#  undef MA__IFACE_LPT
#endif

/* MA__LIBPTHREAD : if set, we define pthread_* functions and everything
 * needed for a libpthread.
 * */
#ifdef MA__LIBPTHREAD
#  undef MA__LIBPTHREAD
#endif

/* MA__FUT_RECORD_TID : if set, we record tids in traces.
 * */
#ifdef MA__FUT_RECORD_TID
#  undef MA__FUT_RECORD_TID
#endif

/* MA__PROVIDE_TLS : if set, we provide per-marcel-thread TLS for the application and glibc etc. */
#ifdef MA__PROVIDE_TLS
#  undef MA__PROVIDE_TLS
#endif

/* MA__USE_TLS : if set, use TLS provided by system */
#ifdef MA__USE_TLS
#  undef MA__USE_TLS
#endif

/* MA__SELF_VAR : if set, store marcel_self in a variable (kernel or marcel TLS) */
#ifdef MA__SELF_VAR
#  undef MA__SELF_VAR
#endif



/*
 * Options chosen by the User, activating options implemented by Marcel
 */

/* In mono, we can just store self in a global variable */
#ifdef MARCEL_MONO
#  define MA__SELF_VAR
#  undef MA__USE_TLS
#endif

/* SMP/NUMA needs kernel threads */
#if defined(MARCEL_SMP) || defined(MARCEL_NUMA)
#  define MA__LWPS
#endif

#if defined(MARCEL_NUMA)
#  define MA__NUMA
   /* Always enable bubbles in NUMA */
#  define MA__BUBBLES
#endif

#if defined(MARCEL_NUMA_MEMORY)
#  define MA__NUMA_MEMORY
#endif

#ifdef MARCEL_POSIX
#  define MA__IFACE_PMARCEL
#endif

/* If we do not use posix kernel threads, we can implement TLS ourselves
 */
#ifdef MARCEL_DONT_USE_POSIX_THREADS
#  define MA__PROVIDE_TLS
#endif

/* If we want a libpthread, we need lpt_* functions and the whole libpthread stuff */
#ifdef MARCEL_LIBPTHREAD
#  define MA__IFACE_LPT
#  define MA__LIBPTHREAD
#endif

#ifdef MARCEL_USE_TLS
#  define MA__SELF_VAR
#  define MA__USE_TLS
#endif


/* If we have kernel threads and do not provide libpthread ourselves, we need to fix the LWP libpthread state in the timer handler */
#if defined(MA__LWPS) && !defined(MA__LIBPTHREAD)
#  define MA__INTERRUPTS_FIX_LWP
#endif

/* We may not want timer signals */
#if defined(MA_DO_NOT_LAUNCH_SIGNAL_TIMER) || defined(__MINGW32__)
#ifdef PM2_DEV
#warning NO SIGNAL TIMER ENABLE
#warning I hope you are debugging
#endif
#  undef MA__TIMER
#endif

/* Actually we need to always record tids, as we need them to detect
 * interruptions between the switch_to record and the actual thread switch
 * */
#define MA__FUT_RECORD_TID

/* Not all systems have the .subsection assembly directive */
#ifdef MA__HAS_SUBSECTION
#  undef MA__HAS_SUBSECTION
#endif

#if defined(LINUX_SYS) || defined(GNU_SYS)
#  define MA__HAS_SUBSECTION
#endif

#endif /* MARCEL_FLAGS_EST_DEF */

