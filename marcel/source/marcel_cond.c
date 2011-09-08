
/* This file has been autogenerated from source/nptl_cond.c.m4 */
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


#include "marcel.h"
#include "marcel_pmarcel.h"
#include <errno.h>


DEF_MARCEL_PMARCEL(int, condattr_init, (marcel_condattr_t * attr), (attr),
{
	MARCEL_LOG_IN();
	memset(attr, '\0', sizeof(*attr));
	MARCEL_LOG_RETURN(0);
})

DEF_MARCEL_PMARCEL(int, condattr_destroy, (marcel_condattr_t * attr TBX_UNUSED), (attr),
{
	MARCEL_LOG_IN();
	MARCEL_LOG_RETURN(0);
})

DEF_MARCEL(int, cond_init, (marcel_cond_t * __restrict cond, 
			    const marcel_condattr_t * __restrict attr TBX_UNUSED),
	   (cond, attr),
{
	MARCEL_LOG_IN();
	__marcel_init_lock(&cond->__data.__lock);
	cond->__data.__waiting = NULL;
	MARCEL_LOG_RETURN(0);
})

DEF_PMARCEL(int, cond_init, 
	    (pmarcel_cond_t * __restrict cond, const pmarcel_condattr_t * __restrict attr),
	    (cond, attr),
{
	MARCEL_LOG_IN();
	marcel_cond_init(cond, NULL);
#if HAVE_DECL_PTHREAD_CONDATTR_GETCLOCK
	if (attr)
		pmarcel_condattr_getclock(attr, &cond->__data.__clk_id);
#endif
	MARCEL_LOG_RETURN(0);
})

DEF_MARCEL_PMARCEL(int, cond_destroy, (marcel_cond_t * cond), (cond),
{
	MARCEL_LOG_IN();

	if (cond->__data.__waiting != NULL)
		MARCEL_LOG_RETURN(EBUSY);
	__marcel_destroy_lock(&cond->__data.__lock);
	
	MARCEL_LOG_RETURN(0);
})

DEF_MARCEL_PMARCEL(int, cond_signal, (marcel_cond_t * cond), (cond),
{
	MARCEL_LOG_IN();
	ma_fastlock_acquire(&cond->__data.__lock);
	__marcel_lock_signal(&cond->__data.__lock);
	ma_fastlock_release(&cond->__data.__lock);
	MARCEL_LOG_RETURN(0);
})

DEF_MARCEL_PMARCEL(int, cond_broadcast, (marcel_cond_t * cond), (cond),
{
	MARCEL_LOG_IN();
	
	ma_fastlock_acquire(&cond->__data.__lock);
	__marcel_lock_broadcast(&cond->__data.__lock, -1);
	ma_fastlock_release(&cond->__data.__lock);

	MARCEL_LOG_RETURN(0);
})

DEF_MARCEL(int, cond_wait, (marcel_cond_t * __restrict cond, marcel_mutex_t * __restrict mutex),
	   (cond, mutex),
{
	MARCEL_LOG_IN();

	/** release mutex and wait on cond */
	ma_fastlock_acquire(&cond->__data.__lock);
	marcel_mutex_unlock(mutex);
	__marcel_lock_wait(&cond->__data.__lock, ma_self(), 0);
	ma_fastlock_release(&cond->__data.__lock);

	/** reacquire mutex */
	marcel_mutex_lock(mutex);

	MARCEL_LOG_RETURN(0);
})

DEF_PMARCEL(int, cond_wait, (pmarcel_cond_t * __restrict cond, pmarcel_mutex_t * __restrict mutex),
	    (cond, mutex),
{
	MARCEL_LOG_IN();

	/** release the mutex and wait on cond */
	ma_fastlock_acquire(&cond->__data.__lock);
	pmarcel_mutex_unlock(mutex);
	__marcel_lock_wait(&cond->__data.__lock, ma_self(), MA_CHECK_CANCEL);
	ma_fastlock_release(&cond->__data.__lock);

	/** reacquire the mutex and check if I was canceled */
	pmarcel_mutex_lock(mutex);
#ifdef MARCEL_DEVIATION_ENABLED
	if (SELF_GETMEM(canceled) == MARCEL_IS_CANCELED)
		marcel_exit(PMARCEL_CANCELED);
#endif
	MARCEL_LOG_RETURN(0);
})

DEF_MARCEL(int, cond_timedwait, (marcel_cond_t * __restrict cond, marcel_mutex_t * __restrict mutex,
				 const struct timespec *__restrict abstime),
	   (cond, mutex, abstime),
{
	struct timeval now;;
	struct timeval tv;
	long timeout;
	int ret = 0;
	MARCEL_LOG_IN();

	tv.tv_sec = abstime->tv_sec;
	tv.tv_usec = abstime->tv_nsec / 1000;
	gettimeofday(&now, NULL);
	timeout = MA_TIMEVAL_TO_USEC(&tv) - MA_TIMEVAL_TO_USEC(&now);
	if (tbx_unlikely(timeout <= 0))
		MARCEL_LOG_RETURN(ETIMEDOUT);

	/** release mutex and wait on cond */
	ma_fastlock_acquire(&cond->__data.__lock);
	marcel_mutex_unlock(mutex);
	ret = __marcel_lock_timed_wait(&cond->__data.__lock, ma_self(), timeout, 0);
	ma_fastlock_release(&cond->__data.__lock);

	/** reacquire mutex */
	marcel_mutex_lock(mutex);

	MARCEL_LOG_RETURN(ret);
})

DEF_PMARCEL(int, cond_timedwait, (pmarcel_cond_t * __restrict cond, pmarcel_mutex_t * __restrict mutex,
				  const struct timespec *__restrict abstime),
	    (cond, mutex, abstime),
{
	long timeout;
	int ret;

	MARCEL_LOG_IN();

#if HAVE_DECL_PTHREAD_CONDATTR_GETCLOCK
	struct timespec now;
	clock_gettime(cond->__data.__clk_id, &now);
	timeout = MA_TIMESPEC_TO_USEC(abstime) - MA_TIMESPEC_TO_USEC(&now);
#else
	struct timeval now;
	struct timeval to;
	
	to.tv_sec = abstime->tv_sec;
	to.tv_usec = abstime->tv_nsec / 1000;
	gettimeofday(&now, NULL);
	timeout = MA_TIMEVAL_TO_USEC(&to) - MA_TIMEVAL_TO_USEC(&now);
#endif
	if (tbx_unlikely(timeout <= 0))
		MARCEL_LOG_RETURN(ETIMEDOUT);

	/** release the mutex and wait on cond */
	ma_fastlock_acquire(&cond->__data.__lock);
	pmarcel_mutex_unlock(mutex);
	ret = __marcel_lock_timed_wait(&cond->__data.__lock, ma_self(), timeout, MA_CHECK_CANCEL);
	ma_fastlock_release(&cond->__data.__lock);

	/** reacquire the mutex and check is I was canceled */
	pmarcel_mutex_lock(mutex);
#ifdef MARCEL_DEVIATION_ENABLED
	if (SELF_GETMEM(canceled) == MARCEL_IS_CANCELED)
		marcel_exit(PMARCEL_CANCELED);
#endif
	MARCEL_LOG_RETURN(ret);
})

DEF_PMARCEL(int, condattr_getpshared, (const pmarcel_condattr_t * __restrict attr, int *__restrict pshared),
	    (attr, pshared),
{
	MARCEL_LOG_IN();
	*pshared = MA_CONDATTR_GET_PSHARED(&attr->__data);
	MARCEL_LOG_RETURN(0);
})

DEF_PMARCEL(int, condattr_setpshared, (pmarcel_condattr_t * attr, int pshared),
	    (attr, pshared),
{
	MARCEL_LOG_IN();

	if (tbx_unlikely(pshared != PMARCEL_PROCESS_PRIVATE) && 
	    pshared != PMARCEL_PROCESS_SHARED)
		MARCEL_LOG_RETURN(EINVAL);

	/* For now it is not possible to share a mutex variable.  */
	if (tbx_unlikely(pshared != PMARCEL_PROCESS_PRIVATE))
		MARCEL_LOG_RETURN(ENOTSUP);

	MA_CONDATTR_SET_PSHARED(&attr->__data, pshared);

	MARCEL_LOG_RETURN(0);
})

#if HAVE_DECL_PTHREAD_CONDATTR_GETCLOCK
DEF_PMARCEL(int, condattr_getclock, 
	    (const pmarcel_condattr_t *__restrict attr, clockid_t *__restrict clock_id),
	    (attr, clock_id),
{
	MARCEL_LOG_IN();
	*clock_id = MA_CONDATTR_GET_CLKID(&attr->__data);
	MARCEL_LOG_RETURN(0);
})
#endif

#if HAVE_DECL_PTHREAD_CONDATTR_SETCLOCK
DEF_PMARCEL(int, condattr_setclock, (pmarcel_condattr_t * attr, clockid_t clock_id),
	    (attr, clock_id),
{
	struct timespec ts;

	MARCEL_LOG_IN();

#  ifndef CLOCK_MONOTONIC_RAW
#    define CLOCK_MONOTONIC_RAW CLOCK_MONOTONIC
#  endif	
	if (CLOCK_REALTIME != clock_id &&
	    CLOCK_MONOTONIC != clock_id &&
	    CLOCK_MONOTONIC_RAW != clock_id)
		MARCEL_LOG_RETURN(EINVAL);

	if (clock_getres(clock_id, &ts))
		MARCEL_LOG_RETURN(EINVAL);

	MA_CONDATTR_SET_CLKID(&attr->__data, clock_id);

	MARCEL_LOG_RETURN(0);
})
#endif
