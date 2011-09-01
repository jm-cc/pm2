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


#ifndef __MARCEL_TIMER_H__
#define __MARCEL_TIMER_H__


#include "tbx_compiler.h"
#include "marcel_config.h"
#include "asm/linux_atomic.h"
#include "sys/marcel_lwp.h"


/** Public macros **/
/* Timer tick period, in us */
#define MA_JIFFIES_PER_TIMER_TICK   1
#define MARCEL_MIN_TIME_SLICE	    80000
#if MARCEL_MIN_TIME_SLICE < 10000 || MARCEL_MIN_TIME_SLICE >= 1000000
#  error " Marcel timeslice value should be in [10000; 1000000["
#endif

/* Timer time and signal */
#ifdef MA__TIMER
#  if defined(MA__LWPS) && (!defined(SA_SIGINFO) || !defined(SI_KERNEL))
/* no way to distinguish between a signal from the kernel or from another LWP */
#    define MA_BOGUS_SIGINFO_CODE
#  endif

/*  Signal utilisé pour la premption automatique */
#  define MARCEL_TIMER_SIGNAL       SIGALRM
#  ifdef MA_BOGUS_SIGINFO_CODE
#    define MARCEL_TIMER_USERSIGNAL SIGVTALRM
#  else
#    define MARCEL_TIMER_USERSIGNAL MARCEL_TIMER_SIGNAL
#  endif
#  define MARCEL_ITIMER_TYPE        ITIMER_REAL
#endif /** MA__TIMER **/
#define MARCEL_RESCHED_SIGNAL       SIGXFSZ


/** Public data types **/
typedef int marcel_time_t;


/** Public functions **/
/* Set the preemption timer granularity in microseconds */
void marcel_settimeslice(unsigned long microsecs);
/* Get the preemption timer granularity in microseconds */
unsigned long marcel_gettimeslice(void);
/* Return the number of milliseconds elapsed since Marcel start */
unsigned long marcel_clock(void);

/* always call disable_interrupts before calling exec*() */
void marcel_sig_enable_interrupts(void);
void marcel_sig_disable_interrupts(void);


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal macros **/
#define MA_LWP_RESCHED(lwp) \
	marcel_kthread_kill((lwp)->pid, MARCEL_RESCHED_SIGNAL)
/** \brief Return the number of microseconds corresponding to \param ts, a
 * `struct timespec' pointer.  */
#define MA_TIMESPEC_TO_USEC(_ts)			\
	((_ts)->tv_sec * 1000000 + (_ts)->tv_nsec / 1000)
/** \brief Return the number of microseconds corresponding to \param tv, a
 * `struct timeval' pointer.  */
#define MA_TIMEVAL_TO_USEC(_tv)			\
	((_tv)->tv_sec * 1000000 + (_tv)->tv_usec)

#ifdef SYS_nanosleep
#  define ma_nanosleep(request, remain) syscall(SYS_nanosleep, (request), (remain))
#else
#  ifndef MA__LIBPTHREAD
#    define ma_nanosleep(request, remain)	\
	do {					\
		marcel_extlib_protect();	\
		nanosleep((request), (remain));	\
		marcel_extlib_unprotect();	\
	} while (0);
#  else
#    error "ma_nanosleep not defined"
#  endif
#endif


/** Internal global variables **/
extern ma_atomic_t __ma_preemption_disabled;


/** Internal functions **/
void marcel_sig_exit(void);
void marcel_sig_nanosleep(void);
void marcel_sig_pause(void);
void marcel_sig_create_timer(ma_lwp_t lwp);
void __ma_sig_enable_interrupts(void);
void __ma_sig_disable_interrupts(void);
void marcel_sig_reset_timer(void);
void marcel_sig_stop_itimer(void);
void marcel_sig_reset_perlwp_timer(void);
void marcel_sig_stop_perlwp_itimer(void);

static __tbx_inline__ void disable_preemption(void);
static __tbx_inline__ void enable_preemption(void);
static __tbx_inline__ unsigned int preemption_enabled(void);
static __tbx_inline__ unsigned int ma_jiffies_from_us(unsigned int microsecs);


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_TIMER_H__ **/
