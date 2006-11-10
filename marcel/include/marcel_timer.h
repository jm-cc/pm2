
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

#section common
#include "tbx_compiler.h"
#section types
typedef int marcel_time_t;

#section macros
#define MA_JIFFIES_PER_TIMER_TICK 1

#section macros
#define JIFFIES_FROM_US(microsecs) \
  ((microsecs)*MA_JIFFIES_PER_TIMER_TICK/marcel_gettimeslice())

#section functions
void marcel_settimeslice(unsigned long microsecs);
unsigned long marcel_gettimeslice(void);
unsigned long marcel_clock(void);

#ifndef MA__TIMER
#define marcel_gettimeslice() 10000L
#endif

#section functions
/* always call disable_interrupts before calling exec*() */
void marcel_sig_enable_interrupts(void);
void marcel_sig_disable_interrupts(void);

#section marcel_functions
void marcel_sig_exit(void);
void marcel_sig_pause(void);
void marcel_sig_nanosleep(void);
void marcel_sig_reset_timer(void);
void marcel_sig_stop_itimer(void);
#ifndef MA__TIMER
#define marcel_sig_pause() (void)0
#define marcel_sig_enable_interrupts() (void)0
#define marcel_sig_disable_interrupts() (void)0
#define marcel_sig_stop_itimer() (void)0
#endif

#section marcel_functions
static __tbx_inline__ void disable_preemption(void);
static __tbx_inline__ void enable_preemption(void);
static __tbx_inline__ unsigned int preemption_enabled(void);
#section marcel_variables
extern TBX_EXTERN ma_atomic_t __ma_preemption_disabled;
#section marcel_inline
static __tbx_inline__ void disable_preemption(void)
{
	ma_atomic_inc(&__ma_preemption_disabled);
}

static __tbx_inline__ void enable_preemption(void)
{
	ma_atomic_dec(&__ma_preemption_disabled);
}

static __tbx_inline__ unsigned int preemption_enabled(void)
{
	return ma_atomic_read(&__ma_preemption_disabled) == 0;
}

#section marcel_macros
#define MA_LWP_RESCHED(lwp) marcel_kthread_kill((lwp)->pid, MARCEL_RESCHED_SIGNAL)
