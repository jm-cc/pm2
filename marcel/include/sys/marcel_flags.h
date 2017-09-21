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


#ifndef __SYS_MARCEL_FLAGS_H__
#define __SYS_MARCEL_FLAGS_H__


#include "tbx_compiler.h"


#ifdef MARCEL_DEBUG
#  define MA__DEBUG
#endif

#ifdef MARCEL_MONO
#  define __SYS_MARCEL_MONO 1
#else
#  define __SYS_MARCEL_MONO 0
#endif

#ifdef MARCEL_SMP
#  define __SYS_MARCEL_SMP 1
#else
#  define __SYS_MARCEL_SMP 0
#endif

#ifdef MARCEL_NUMA
#  define __SYS_MARCEL_NUMA 1
#else
#  define __SYS_MARCEL_NUMA 0
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
/* MA__SELF_VAR_TLS : if set, store __ma_self in TLS (kernel or marcel) */
#ifdef MA__SELF_VAR_TLS
#  undef MA__SELF_VAR_TLS
#endif

#ifdef MA__BUBBLES
#  undef MA__BUBBLES
#endif

#ifdef MA__SIGNAL_NODEFER
#  undef MA__SIGNAL_NODEFER
#endif

#ifdef MA__SIGNAL_RESETHAND
#  undef MA__SIGNAL_RESETHAND
#endif


/*
 * Options chosen by the User, activating options implemented by Marcel
 */

/* In mono, we can just store self in a global variable, and no need for TLS */
#ifdef MARCEL_MONO
#  define MA__SELF_VAR
#  undef MA__SELF_VAR_TLS
#endif

/* SMP/NUMA needs kernel threads */
#if defined(MARCEL_SMP) || defined(MARCEL_NUMA)
#  define MA__LWPS
#endif

#if defined(MARCEL_NUMA)
#  define MA__NUMA
   /* Always enable bubbles in NUMA */
#endif

#ifdef MARCEL_BUBBLES
#  define MA__BUBBLES
#endif

#ifdef MARCEL_POSIX
#  define MA__IFACE_PMARCEL
#endif

/* If we do not use posix kernel threads, we can implement TLS ourselves
 */
#if defined(MARCEL_DONT_USE_POSIX_THREADS)
#  define MA__PROVIDE_TLS
#endif

/* If we want a libpthread, we need lpt_* functions and the whole libpthread stuff */
#ifdef MARCEL_LIBPTHREAD
#  define MA__IFACE_LPT
#  define MA__LIBPTHREAD
#endif

#if defined(MARCEL_USE_TLS)
#  define MA__USE_TLS
#endif

/* If we use the system TLS or provide our own TLS, we can use it for
 * marcel_self */
#if (defined(MA__USE_TLS) && defined(HAVE__TLS_SUPPORT)) || defined(MA__PROVIDE_TLS)
#  define MA__SELF_VAR
#  define MA__SELF_VAR_TLS
#endif

/* If we have kernel threads and do not provide libpthread ourselves, we need to fix the LWP libpthread state in the timer handler */
#if defined(MA__LWPS) && !defined(MA__LIBPTHREAD)
#  define MA__INTERRUPTS_FIX_LWP
#endif

/* We may not want timer signals */
#if defined(MA_DO_NOT_LAUNCH_SIGNAL_TIMER)
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

#if HAVE_DECL_SA_NODEFER
#  define MA__SIGNAL_NODEFER SA_NODEFER
#elif HAVE_DECL_SA_NOMASK
#  define MA__SIGNAL_NODEFER SA_NOMASK
#else
#  define MA__SIGNAL_NODEFER 0
#endif

#if HAVE_DECL_SA_RESETHAND
#  define MA__SIGNAL_RESETHAND SA_RESETHAND
#elif HAVE_DECL_SA_ONESHOT
#  define MA__SIGNAL_RESETHAND SA_ONESHOT
#else
#  define MA__SIGNAL_RESETHAND 0
#endif


#include "sys/os/marcel_flags.h"


#endif /** __SYS_MARCEL_FLAGS_H__ **/
