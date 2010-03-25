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


#ifdef __MARCEL_KERNEL__
#include "tbx_compiler.h"
#include "sys/marcel_flags.h"
#include "asm/linux_atomic.h"
#include "sys/marcel_lwp.h"
#endif


/** Public macros **/
#define MA_JIFFIES_PER_TIMER_TICK 1

#define JIFFIES_FROM_US(microsecs) \
  ((microsecs)*MA_JIFFIES_PER_TIMER_TICK/marcel_gettimeslice())


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


/** Internal macros **/
#define MA_LWP_RESCHED(lwp) marcel_kthread_kill((lwp)->pid, MARCEL_RESCHED_SIGNAL)

/** \brief Return the number of microseconds corresponding to \param ts, a
 * `struct timespec' pointer.  */
#define MA_TIMESPEC_TO_USEC(_ts)								\
  ((_ts)->tv_sec * 1e6 + (_ts)->tv_nsec / 1e3)

/** \brief Return the number of jiffies corresponding to \param ts, a pointer
 * to a `struct timespec' denoting a time interval.  */
#define MA_TIMESPEC_TO_JIFFIES(_ts)							\
  (JIFFIES_FROM_US (MA_TIMESPEC_TO_USEC (_ts)))


/** Internal global variables **/
extern TBX_EXTERN ma_atomic_t __ma_preemption_disabled;


/** Internal functions **/
void marcel_sig_exit(void);
void marcel_sig_pause(void);
void marcel_sig_nanosleep(void);
void marcel_sig_create_timer(ma_lwp_t lwp);
void marcel_sig_reset_timer(void);
void marcel_sig_stop_itimer(void);
#ifndef MA__TIMER
#define marcel_sig_stop_itimer() (void)0
#endif

static __tbx_inline__ void disable_preemption(void);
static __tbx_inline__ void enable_preemption(void);
static __tbx_inline__ unsigned int preemption_enabled(void);


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_TIMER_H__ **/
