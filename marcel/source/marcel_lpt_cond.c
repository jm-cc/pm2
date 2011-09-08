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
#include <errno.h>


#ifdef MA__IFACE_LPT
int lpt_condattr_init(lpt_condattr_t * attr)
{
	MARCEL_LOG_IN();
	memset(attr, '\0', sizeof(*attr));
	MA_BUG_ON(sizeof(lpt_condattr_t) > __SIZEOF_LPT_CONDATTR_T);
	MARCEL_LOG_RETURN(0);
}
DEF_LIBPTHREAD(int, condattr_init, (pthread_condattr_t * attr), (attr))
DEF___LIBPTHREAD(int, condattr_init, (pthread_condattr_t * attr), (attr))

int lpt_condattr_destroy(lpt_condattr_t * attr TBX_UNUSED)
{
	MARCEL_LOG_IN();
	/* Nothing to be done.  */
	MARCEL_LOG_RETURN(0);
}
DEF_LIBPTHREAD(int, condattr_destroy, (pthread_condattr_t * attr), (attr))
DEF___LIBPTHREAD(int, condattr_destroy, (pthread_condattr_t * attr), (attr))

int lpt_condattr_getpshared(const lpt_condattr_t * __restrict attr, int *__restrict pshared)
{
	MARCEL_LOG_IN();
	*pshared = MA_CONDATTR_GET_PSHARED((const struct lpt_condattr *)attr);
	MARCEL_LOG_RETURN(0);
}
DEF_LIBPTHREAD(int, condattr_getpshared, (pthread_condattr_t * __restrict attr, int *__restrict pshared), (attr, pshared))

int lpt_condattr_setpshared(lpt_condattr_t * attr, int pshared)
{
	MARCEL_LOG_IN();
	if (pshared != LPT_PROCESS_PRIVATE  && 
	    __builtin_expect(pshared != LPT_PROCESS_SHARED, 0)) {
		MARCEL_LOG("lpt_condattr_setpshared : valeur pshared(%d)  invalide\n", pshared);
		MARCEL_LOG_RETURN(EINVAL);
	}

	/* For now it is not possible to share a mutex variable.  */
	if (pshared != LPT_PROCESS_PRIVATE) {
		MA_NOT_SUPPORTED("shared condition");
		MARCEL_LOG_RETURN(ENOTSUP);
	}

	MA_CONDATTR_SET_PSHARED((struct lpt_condattr *)attr, pshared);
	MARCEL_LOG_RETURN(0);
}
DEF_LIBPTHREAD(int, condattr_setpshared, (pthread_condattr_t * attr, int pshared), (attr, pshared))

#if HAVE_DECL_PTHREAD_CONDATTR_GETCLOCK
int lpt_condattr_getclock(const lpt_condattr_t *__restrict attr, clockid_t *__restrict clock_id)
{
	MARCEL_LOG_IN();
	*clock_id = MA_CONDATTR_GET_CLKID((const struct lpt_condattr *)attr);
	MARCEL_LOG_RETURN(0);
}
DEF_LIBPTHREAD(int, condattr_getclock, 
	       (pthread_condattr_t *__restrict attr, clockid_t *__restrict clock_id),
	       (attr, clock_id))
#endif

#if HAVE_DECL_PTHREAD_CONDATTR_SETCLOCK
int lpt_condattr_setclock(lpt_condattr_t * attr, clockid_t clock_id)
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

	MA_CONDATTR_SET_CLKID((struct lpt_condattr *)attr, clock_id);

	MARCEL_LOG_RETURN(0);
}
DEF_LIBPTHREAD(int, condattr_setclock, (pthread_condattr_t *attr, clockid_t clock_id), 
	       (attr, clock_id))
#endif


int lpt_cond_init(lpt_cond_t * __restrict cond, const lpt_condattr_t * __restrict attr)
{
	const struct _lpt_fastlock initlock = MA_LPT_FASTLOCK_UNLOCKED;

	MARCEL_LOG_IN();

	cond->__data.__lock = initlock;
	cond->__data.__waiting = NULL;
	if (attr)
		lpt_condattr_getclock(attr, &cond->__data.__clk_id);

	MARCEL_LOG_RETURN(0);
}
versioned_symbol(libpthread, lpt_cond_init, pthread_cond_init, GLIBC_2_3_2);

int lpt_cond_destroy(lpt_cond_t * cond)
{
	MARCEL_LOG_IN();

	if (cond->__data.__waiting != NULL)
		MARCEL_LOG_RETURN(EBUSY);
	__lpt_destroy_lock(&cond->__data.__lock);

	MARCEL_LOG_RETURN(0);
}
versioned_symbol(libpthread, lpt_cond_destroy, pthread_cond_destroy, GLIBC_2_3_2);

int lpt_cond_signal(lpt_cond_t * cond)
{
	MARCEL_LOG_IN();
	lpt_fastlock_acquire(&cond->__data.__lock);
	__lpt_lock_signal(&cond->__data.__lock);
	lpt_fastlock_release(&cond->__data.__lock);
	MARCEL_LOG_RETURN(0);
}
versioned_symbol(libpthread, lpt_cond_signal, pthread_cond_signal, GLIBC_2_3_2);

int lpt_cond_broadcast(lpt_cond_t * cond)
{
	MARCEL_LOG_IN();

	lpt_fastlock_acquire(&cond->__data.__lock);
	__lpt_lock_broadcast(&cond->__data.__lock, -1);
	lpt_fastlock_release(&cond->__data.__lock);

	MARCEL_LOG_RETURN(0);
}
versioned_symbol(libpthread, lpt_cond_broadcast, pthread_cond_broadcast, GLIBC_2_3_2);

int lpt_cond_wait(lpt_cond_t * __restrict cond, lpt_mutex_t * __restrict mutex)
{
	MARCEL_LOG_IN();

	/** release the mutex and wait on cond */
	lpt_fastlock_acquire(&cond->__data.__lock);
	lpt_mutex_unlock(mutex);
	__lpt_lock_wait(&cond->__data.__lock, ma_self(), MA_CHECK_CANCEL);
	lpt_fastlock_release(&cond->__data.__lock);

	/** reacquire the mutex and check if I was canceled */
	lpt_mutex_lock(mutex);
#ifdef MARCEL_DEVIATION_ENABLED
	if (SELF_GETMEM(canceled) == MARCEL_IS_CANCELED)
                marcel_exit(PMARCEL_CANCELED);
#endif
	MARCEL_LOG_RETURN(0);
}
versioned_symbol(libpthread, lpt_cond_wait, pthread_cond_wait, GLIBC_2_3_2);

int lpt_cond_timedwait(lpt_cond_t * __restrict cond, lpt_mutex_t * __restrict mutex,
		       const struct timespec *__restrict abstime)
{
	MARCEL_LOG_IN();

	long timeout;
	int ret;

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
	lpt_fastlock_acquire(&cond->__data.__lock);
	lpt_mutex_unlock(mutex);
	ret = __lpt_lock_timed_wait(&cond->__data.__lock, ma_self(), timeout, MA_CHECK_CANCEL);
	lpt_fastlock_release(&cond->__data.__lock);

	/** reacquire the mutex and check if I was canceled */
	lpt_mutex_lock(mutex);
#ifdef MARCEL_DEVIATION_ENABLED
        if (SELF_GETMEM(canceled) == MARCEL_IS_CANCELED)
                marcel_exit(PMARCEL_CANCELED);
#endif
	MARCEL_LOG_RETURN(ret);
}
versioned_symbol(libpthread, lpt_cond_timedwait, pthread_cond_timedwait, GLIBC_2_3_2);
#endif


#if defined(MA__LIBPTHREAD) && MA_GLIBC_VERSION_MINIMUM < 20302
DEF_STRONG_ALIAS(lpt_cond_init, __old_pthread_cond_init)
compat_symbol(libpthread, __old_pthread_cond_init, pthread_cond_init, GLIBC_2_0);
DEF_STRONG_ALIAS(lpt_cond_destroy, __old_pthread_cond_destroy)
compat_symbol(libpthread, __old_pthread_cond_destroy, pthread_cond_destroy, GLIBC_2_0);
DEF_STRONG_ALIAS(lpt_cond_signal, __old_pthread_cond_signal)
compat_symbol(libpthread, __old_pthread_cond_signal, pthread_cond_signal, GLIBC_2_0);
DEF_STRONG_ALIAS(lpt_cond_broadcast, __old_pthread_cond_broadcast)
compat_symbol(libpthread, __old_pthread_cond_broadcast, pthread_cond_broadcast, GLIBC_2_0);
DEF_STRONG_ALIAS(lpt_cond_wait, __old_pthread_cond_wait)
compat_symbol(libpthread, __old_pthread_cond_wait, pthread_cond_wait, GLIBC_2_0);
DEF_STRONG_ALIAS(lpt_cond_timedwait, __old_pthread_cond_timedwait)
compat_symbol(libpthread, __old_pthread_cond_timedwait, pthread_cond_timedwait, GLIBC_2_0);
#endif
