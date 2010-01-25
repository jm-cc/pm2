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


#ifndef __INLINEFUNCTIONS_MARCEL_TIMER_H__
#define __INLINEFUNCTIONS_MARCEL_TIMER_H__


#include "marcel_timer.h"


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
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


#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_MARCEL_TIMER_H__ **/
