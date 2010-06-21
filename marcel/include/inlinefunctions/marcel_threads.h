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


#ifndef __INLINEFUNCTIONS_MARCEL_THREADS_H__
#define __INLINEFUNCTIONS_MARCEL_THREADS_H__


#include "marcel_threads.h"


/*
 * Compare two marcel thread ids.
 */
static __tbx_inline__ int marcel_equal(marcel_t pid1, marcel_t pid2)
{
	return (pid1 == pid2);
}


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


static __tbx_inline__ void ma_some_thread_preemption_enable(marcel_t t)
{
#ifdef MA__DEBUG
	MA_BUG_ON(!THREAD_GETMEM(t, not_preemptible));
#endif
	ma_barrier();
	THREAD_GETMEM(t, not_preemptible)--;
}

static __tbx_inline__ void ma_some_thread_preemption_disable(marcel_t t)
{
	THREAD_GETMEM(t, not_preemptible)++;
	ma_barrier();
}

static __tbx_inline__ int ma_some_thread_is_preemption_disabled(marcel_t t)
{
	return THREAD_GETMEM(t, not_preemptible) != 0;
}


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_MARCEL_THREADS_H__ **/
