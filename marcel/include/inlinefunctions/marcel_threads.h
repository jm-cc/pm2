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


/** Public inline **/
/*
 * Compare two marcel thread ids.
 */
static __tbx_inline__ int marcel_equal(marcel_t pid1, marcel_t pid2)
{
  return (pid1 == pid2);
}

static __tbx_inline__ void __marcel_some_thread_preemption_enable(marcel_t t)
{
#ifdef MA__DEBUG
	MA_BUG_ON(!THREAD_GETMEM(t, not_preemptible));
#endif
        ma_barrier();
	THREAD_GETMEM(t, not_preemptible)--;
}
static __tbx_inline__ void marcel_some_thread_preemption_enable(marcel_t t) {
	__marcel_some_thread_preemption_enable(t);
}

static __tbx_inline__ void __marcel_some_thread_preemption_disable(marcel_t t) {
	THREAD_GETMEM(t, not_preemptible)++;
        ma_barrier();
}

static __tbx_inline__ void marcel_some_thread_preemption_disable(marcel_t t) {
	__marcel_some_thread_preemption_disable(t);
}

static __tbx_inline__ int __marcel_some_thread_is_preemption_disabled(marcel_t t) {
	return THREAD_GETMEM(t, not_preemptible) != 0;
}

static __tbx_inline__ int marcel_some_thread_is_preemption_disabled(marcel_t t) {
	return __marcel_some_thread_is_preemption_disabled(t);
}


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
#ifdef MARCEL_MIGRATION_ENABLED

MARCEL_INLINE void marcel_disablemigration(marcel_t pid)
{
  pid->not_migratable++;
}

MARCEL_INLINE void marcel_enablemigration(marcel_t pid)
{
  pid->not_migratable--;
}

#endif /* MARCEL_MIGRATION_ENABLED */


#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_MARCEL_THREADS_H__ **/
