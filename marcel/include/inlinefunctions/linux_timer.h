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


#ifndef __INLINEFUNCTIONS_LINUX_TIMER_H__
#define __INLINEFUNCTIONS_LINUX_TIMER_H__


#include "linux_timer.h"


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
/***
 * ma_init_timer - initialize a timer.
 * @timer: the timer to be initialized
 *
 * init_timer() must be done to a timer prior calling *any* of the
 * other timer functions.
 */
static __tbx_inline__ void ma_init_timer(struct ma_timer_list * timer)
{
	timer->entry.next = NULL;
	timer->base = &__ma_get_lwp_var(tvec_bases).t_base;
#ifdef PM2_DEBUG
	timer->magic = MA_TIMER_MAGIC;
#endif
}

/***
 * ma_timer_pending - is a timer pending?
 * @timer: the timer in question
 *
 * ma_timer_pending will tell whether a given timer is currently pending,
 * or not. Callers must ensure serialization wrt. other operations done
 * to this timer, eg. interrupt contexts, or other CPUs on SMP.
 *
 * return value: 1 if the timer is pending, 0 if not.
 */
static __tbx_inline__ int ma_timer_pending(const struct ma_timer_list * timer)
{
	return timer->entry.next != NULL;
}

/***
 * ma_add_timer - start a timer
 * @timer: the timer to be added
 *
 * The kernel will do a ->function(->data) callback from the
 * timer interrupt at the ->expired point in the future. The
 * current time is 'jiffies'.
 *
 * The timer's ->expired, ->function (and if the handler uses it, ->data)
 * fields must be set prior calling this function.
 *
 * Timers with an ->expired field in the past will be executed in the next
 * timer tick.
 */
static __tbx_inline__ void ma_add_timer(struct ma_timer_list * timer)
{
	__ma_mod_timer(timer, timer->expires);
}


#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_LINUX_TIMER_H__ **/
