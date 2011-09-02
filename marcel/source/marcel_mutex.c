
/* This file has been autogenerated from source/nptl_mutex.c.m4 */
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


int marcel_mutex_init(marcel_mutex_t * mutex,
		      const marcel_mutexattr_t * mutexattr TBX_UNUSED)
{
	__marcel_init_lock(&mutex->__data.__lock);
	return 0;
}

int marcel_mutex_destroy(marcel_mutex_t * mutex)
{
	__marcel_destroy_lock(&mutex->__data.__lock);
	return 0;
}

int marcel_mutex_lock(marcel_mutex_t * mutex)
{
	__marcel_lock(&mutex->__data.__lock, MARCEL_SELF);
	return 0;
}

int marcel_mutex_trylock(marcel_mutex_t * mutex)
{
	return __marcel_trylock(&mutex->__data.__lock);
}

int marcel_mutex_unlock(marcel_mutex_t * mutex)
{
	return __marcel_unlock(&mutex->__data.__lock);
}

int marcel_mutexattr_init(marcel_mutexattr_t * attr TBX_UNUSED)
{
	return 0;
}

int marcel_mutexattr_destroy(marcel_mutexattr_t * attr TBX_UNUSED)
{
	return 0;
}

int marcel_recursivemutex_init(marcel_recursivemutex_t * mutex,
			       const marcel_recursivemutexattr_t * mutexattr TBX_UNUSED)
{
	mutex->__data.owner = NULL;
	mutex->__data.__count = 0;
	__marcel_init_lock(&mutex->__data.__lock);
	return 0;
}

int marcel_recursivemutex_destroy(marcel_recursivemutex_t * mutex TBX_UNUSED)
{
	__marcel_destroy_lock(&mutex->__data.__lock);
	return 0;
}

int marcel_recursivemutex_lock(marcel_recursivemutex_t * mutex)
{
	if (mutex->__data.owner == ma_self()) {
		mutex->__data.__count++;
		return mutex->__data.__count + 1;
	} else
		mutex->__data.owner = ma_self();

	__marcel_lock(&mutex->__data.__lock, ma_self());
	/* and __count is 0 */
	return 1;
}

int marcel_recursivemutex_trylock(marcel_recursivemutex_t * mutex)
{
	if (mutex->__data.owner == ma_self()) {
		mutex->__data.__count ++;
		return mutex->__data.__count + 1;
	}

	if (!__marcel_trylock(&mutex->__data.__lock))
		return 0;

	mutex->__data.owner = ma_self();
	/* and __count is 0 */
	return 1;
}

int marcel_recursivemutex_unlock(marcel_recursivemutex_t * mutex)
{
	if (mutex->__data.__count) {
		mutex->__data.__count--;
		return mutex->__data.__count + 1;
	}
	
	mutex->__data.owner = NULL;
	__marcel_unlock(&mutex->__data.__lock);
	return 0;
}

int marcel_recursivemutexattr_init(marcel_recursivemutexattr_t * attr TBX_UNUSED)
{
	return 0;
}

int marcel_recursivemutexattr_destroy(marcel_recursivemutexattr_t * attr TBX_UNUSED)
{
	return 0;
}


#ifdef MA__IFACE_PMARCEL

static const pmarcel_mutexattr_t pmarcel_default_attr = {
	/* Default is a normal mutex, not shared between processes.  */
	.__data.mutexkind = PMARCEL_MUTEX_NORMAL
};

int pmarcel_mutex_init(pmarcel_mutex_t * mutex, const pmarcel_mutexattr_t * mutexattr)
{
	const pmarcel_mutexattr_t *imutexattr;

	MARCEL_LOG_IN();

	if (mutexattr)
		imutexattr = mutexattr;
	else
		imutexattr = &pmarcel_default_attr;

	memset(mutex, '\0', sizeof(pmarcel_mutex_t));
	mutex->__data.__kind = imutexattr->__data.mutexkind & ~0x80000000;
	__marcel_init_lock(&mutex->__data.__lock);

	MARCEL_LOG_RETURN(0);
}

int pmarcel_mutex_destroy(pmarcel_mutex_t * mutex TBX_UNUSED)
{
	MARCEL_LOG_IN();
	__marcel_destroy_lock(&mutex->__data.__lock);
	MARCEL_LOG_RETURN(0);
}

int pmarcel_mutex_lock(pmarcel_mutex_t * mutex)
{
	MARCEL_LOG_IN();

	switch (__builtin_expect(mutex->__data.__kind, PMARCEL_MUTEX_NORMAL)) {
	        case PMARCEL_MUTEX_RECURSIVE_NP:
			/* Check whether we already hold the mutex.  */
			if (mutex->__data.__owner == ma_self()) {
				/* Just bump the counter.  */
				if (__builtin_expect(mutex->__data.__count + 1 == 0, 0))
					/* Overflow of the counter.  */
					MARCEL_LOG_RETURN(EAGAIN);
				++mutex->__data.__count;
				MARCEL_LOG_RETURN(0);
			}
			break;

	        case PMARCEL_MUTEX_ERRORCHECK_NP:
			if (mutex->__data.__owner == ma_self())
				MARCEL_LOG_RETURN(EDEADLK);
			break;
	}

	/** lock mutex **/
	__marcel_lock(&mutex->__data.__lock, ma_self());

	/* Record the ownership.  */
	if (tbx_unlikely(mutex->__data.__kind != PMARCEL_MUTEX_NORMAL)) {
		mutex->__data.__count = 1;
		mutex->__data.__owner = ma_self();
	}
	++mutex->__data.__nusers;

	MARCEL_LOG_RETURN(0);
}

int pmarcel_mutex_trylock(pmarcel_mutex_t * mutex)
{
	MARCEL_LOG_IN();

	if (tbx_unlikely(mutex->__data.__kind == PMARCEL_MUTEX_RECURSIVE_NP)) {
		if (mutex->__data.__owner == ma_self()) {
			if (tbx_unlikely(mutex->__data.__count + 1 == 0))
				MARCEL_LOG_RETURN(EAGAIN);
			++mutex->__data.__count;
			MARCEL_LOG_RETURN(0);
		}
	}

	if (__marcel_trylock(&mutex->__data.__lock) != 0) {
		if (tbx_unlikely(mutex->__data.__kind != PMARCEL_MUTEX_NORMAL)) {
			/** record the ownership */
			mutex->__data.__owner = ma_self();
			mutex->__data.__count = 1;
		}
		++mutex->__data.__nusers;

		MARCEL_LOG_RETURN(0);
	}

	MARCEL_LOG_RETURN(EBUSY);
}

static int __pmarcel_mutex_blockcell(pmarcel_mutex_t * mutex, const struct timespec *abstime)
{
	struct timeval now, tv;
	long int timeout;
	int ret;

	/* must round-up */
	tv.tv_sec = abstime->tv_sec;
	tv.tv_usec = (abstime->tv_nsec + 999) / 1000;
	gettimeofday(&now, NULL);
	timeout = MA_TIMEVAL_TO_USEC(&tv) - MA_TIMEVAL_TO_USEC(&now);
	if (tbx_unlikely(timeout <= 0))
		MARCEL_LOG_RETURN(ETIMEDOUT);

	/** wait for the lock */
	ma_fastlock_acquire(&mutex->__data.__lock);
	ret = __marcel_lock_timed_wait(&mutex->__data.__lock, ma_self(), timeout, 0);
	ma_fastlock_release(&mutex->__data.__lock);
	MARCEL_LOG_RETURN(ret);
}

int pmarcel_mutex_timedlock(pmarcel_mutex_t * mutex, const struct timespec *abstime)
{
	MARCEL_LOG_IN();

	if (tbx_unlikely(abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000)) {
		MARCEL_LOG("pmarcel_mutex_timedlock : valeur temporelle invalide\n");
		MARCEL_LOG_RETURN(EINVAL);
	}

	switch (__builtin_expect(mutex->__data.__kind, PMARCEL_MUTEX_NORMAL)) {
	        case PMARCEL_MUTEX_RECURSIVE_NP:
			if (mutex->__data.__owner == ma_self()) {
				mutex->__data.__count++;
				MARCEL_LOG_RETURN(0);
			}
			break;

	        case PMARCEL_MUTEX_ERRORCHECK_NP:
			if (mutex->__data.__owner == ma_self())
				MARCEL_LOG_RETURN(EDEADLK);
			break;
	}

	if ( tbx_unlikely(mutex->__data.__nusers != 0 && 
			  __pmarcel_mutex_blockcell(mutex, abstime)))
		MARCEL_LOG_RETURN(ETIMEDOUT);

	__marcel_lock(&mutex->__data.__lock, NULL);
	
	/** Record the owership */
	if (tbx_unlikely(mutex->__data.__kind != PMARCEL_MUTEX_NORMAL)) {
		mutex->__data.__count = 1;
		mutex->__data.__owner = ma_self();
	}
	mutex->__data.__nusers++;

	MARCEL_LOG_RETURN(0);
}

int pmarcel_mutex_unlock(pmarcel_mutex_t * mutex)
{
	MARCEL_LOG_IN();

	if (tbx_unlikely(mutex->__data.__kind == PMARCEL_MUTEX_RECURSIVE_NP ||
			 mutex->__data.__kind == PMARCEL_MUTEX_ERRORCHECK_NP)) {
		if (mutex->__data.__owner != MARCEL_SELF)
			MARCEL_LOG_RETURN(EPERM);

		if (--mutex->__data.__count != 0)
			/* We still hold the mutex.  */
			MARCEL_LOG_RETURN(0);

		mutex->__data.__owner = 0;
	}

	/* Always reset the owner field.  */
	--mutex->__data.__nusers;

	/* Unlock.  */
	__marcel_unlock(&mutex->__data.__lock);

	MARCEL_LOG_RETURN(0);
}

/* We use bit 31 to signal whether the mutex is going to be
   process-shared or not.  By default it is zero, i.e., the
   mutex is not process-shared.  */
int pmarcel_mutexattr_init(pmarcel_mutexattr_t * attr)
{
	MARCEL_LOG_IN();
	attr->__data.mutexkind = PMARCEL_MUTEX_NORMAL;
	MARCEL_LOG_RETURN(0);
}

int pmarcel_mutexattr_destroy(pmarcel_mutexattr_t * attr TBX_UNUSED)
{
	MARCEL_LOG_IN();
	MARCEL_LOG_RETURN(0);
}

int pmarcel_mutexattr_settype(pmarcel_mutexattr_t * attr, int kind)
{
	MARCEL_LOG_IN();
	if (kind < PMARCEL_MUTEX_NORMAL || kind > PMARCEL_MUTEX_ADAPTIVE_NP) {
		MARCEL_LOG("pmarcel_mutexattr_settype : valeur kind(%d) invalide\n",
			   kind);
		MARCEL_LOG_RETURN(EINVAL);
	}
	attr->__data.mutexkind = (attr->__data.mutexkind & 0x80000000) | kind;
	MARCEL_LOG_RETURN(0);
}

int pmarcel_mutexattr_gettype(const pmarcel_mutexattr_t * __restrict attr,
			      int *__restrict kind)
{
	MARCEL_LOG_IN();
	*kind = attr->__data.mutexkind & ~0x80000000;
	MARCEL_LOG_RETURN(0);
}

int pmarcel_mutexattr_setpshared(pmarcel_mutexattr_t * attr, int pshared)
{
	MARCEL_LOG_IN();
	if (pshared != PMARCEL_PROCESS_PRIVATE &&
	    tbx_unlikely(pshared != PMARCEL_PROCESS_SHARED)) {
		MARCEL_LOG_RETURN(EINVAL);
	}
	/* For now it is not possible to share a mutex variable.  */
	if (pshared != MARCEL_PROCESS_PRIVATE) {
		MARCEL_LOG_RETURN(ENOTSUP);
	}

	if (pshared == PMARCEL_PROCESS_PRIVATE)
		attr->__data.mutexkind &= ~0x80000000;
	else
		attr->__data.mutexkind |= 0x80000000;

	MARCEL_LOG_RETURN(0);
}

int pmarcel_mutexattr_getpshared(const pmarcel_mutexattr_t * __restrict attr,
				 int *__restrict pshared)
{
	MARCEL_LOG_IN();
	*pshared = ((attr->__data.mutexkind & 0x80000000) != 0
		    ? PMARCEL_PROCESS_SHARED : PMARCEL_PROCESS_PRIVATE);

	MARCEL_LOG_RETURN(0);
}
#endif
