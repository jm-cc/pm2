
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
#include <errno.h>
#include "marcel_fastlock.h"

int marcel_mutex_init (marcel_mutex_t *mutex, 
		const marcel_mutexattr_t * mutexattr TBX_UNUSED) {
	mdebug("initializing mutex %p by %p\n", mutex, marcel_self());
	__marcel_init_lock(&mutex->__data.__lock);
	return 0;
}
int marcel_mutex_destroy(marcel_mutex_t * mutex TBX_UNUSED) {
	return 0;
}
int marcel_mutex_lock(marcel_mutex_t * mutex) {
	struct marcel_task *id = MARCEL_SELF;
	__marcel_lock(&mutex->__data.__lock, id);
	return 0;
}
int marcel_mutex_trylock(marcel_mutex_t * mutex) {
	return __marcel_trylock(&mutex->__data.__lock);
}
int marcel_mutex_unlock(marcel_mutex_t * mutex) {
	return __marcel_unlock(&mutex->__data.__lock);
}
int marcel_mutexattr_init(marcel_mutexattr_t * attr TBX_UNUSED)	{
	return 0;
}
int marcel_mutexattr_destroy(marcel_mutexattr_t * attr TBX_UNUSED)	{
	return 0;
}

int marcel_recursivemutex_init (marcel_recursivemutex_t *mutex, 
		const marcel_recursivemutexattr_t * mutexattr TBX_UNUSED) {
	mdebug("initializing mutex %p by %p\n", mutex, marcel_self());
	mutex->__data.owner = NULL;
	mutex->__data.__count = 0;
	__marcel_init_lock(&mutex->__data.__lock);
	return 0;
}
int marcel_recursivemutex_destroy(marcel_recursivemutex_t * mutex TBX_UNUSED) {
	return 0;
}
int marcel_recursivemutex_lock(marcel_recursivemutex_t * mutex) {
	struct marcel_task *id = MARCEL_SELF;
	if (mutex->__data.owner == id) {
		mutex->__data.__count++;
		return mutex->__data.__count + 1;
	}
	__marcel_lock(&mutex->__data.__lock, id);
	mutex->__data.owner = id;
	/* and __count is 0 */
	return 1;
}
int marcel_recursivemutex_trylock(marcel_recursivemutex_t * mutex) {
	struct marcel_task *id = MARCEL_SELF;
	if (mutex->__data.owner == id) {
		mutex->__data.__count++;
		return mutex->__data.__count + 1;
	}
	if (!__marcel_trylock(&mutex->__data.__lock))
		return 0;
	mutex->__data.owner = id;
	/* and __count is 0 */
	return 1;
}
int marcel_recursivemutex_unlock(marcel_recursivemutex_t * mutex) {
	if (mutex->__data.__count) {
		mutex->__data.__count--;
		return mutex->__data.__count + 1;
	}
	mutex->__data.owner = NULL;
	__marcel_unlock(&mutex->__data.__lock);
	return 0;
}
int marcel_recursivemutexattr_init(marcel_recursivemutexattr_t * attr TBX_UNUSED)	{
	return 0;
}
int marcel_recursivemutexattr_destroy(marcel_recursivemutexattr_t * attr TBX_UNUSED)	{
	return 0;
}

#ifdef MA__IFACE_PMARCEL
static const pmarcel_mutexattr_t pmarcel_default_attr = {
	/* Default is a normal mutex, not shared between processes.  */
	.__data.mutexkind = PMARCEL_MUTEX_NORMAL
};

int pmarcel_mutex_init (pmarcel_mutex_t *mutex, 
		const pmarcel_mutexattr_t * mutexattr) {
	const pmarcel_mutexattr_t *imutexattr = mutexattr ?: &pmarcel_default_attr;
	LOG_IN();
	memset (mutex, '\0', sizeof(pmarcel_mutex_t));
	mutex->__data.__kind = imutexattr->__data.mutexkind & ~0x80000000;
	__pmarcel_init_lock(&mutex->__data.__lock);
	LOG_RETURN(0);
}
int pmarcel_mutex_destroy(pmarcel_mutex_t * mutex TBX_UNUSED) {
	LOG_IN();
	LOG_RETURN(0);
}
int pmarcel_mutex_lock(pmarcel_mutex_t * mutex) {
	struct marcel_task *id = MARCEL_SELF;
	LOG_IN();
	switch (__builtin_expect (mutex->__data.__kind, PMARCEL_MUTEX_TIMED_NP)) {
		case PMARCEL_MUTEX_RECURSIVE_NP:
			/* Check whether we already hold the mutex.  */
			if (mutex->__data.__owner == id) {
				/* Just bump the counter.  */
				if (__builtin_expect (mutex->__data.__count + 1 == 0, 0))
					/* Overflow of the counter.  */
					LOG_RETURN(EAGAIN);
				++mutex->__data.__count;
				LOG_RETURN(0);
			}

			/* We have to get the mutex.  */
			__pmarcel_lock(&mutex->__data.__lock, id);

			mutex->__data.__count = 1;
			break;

		case PMARCEL_MUTEX_ERRORCHECK_NP:
			if (mutex->__data.__owner == id)
				LOG_RETURN(EDEADLK);

		default:

		case PMARCEL_MUTEX_TIMED_NP:
		case PMARCEL_MUTEX_ADAPTIVE_NP:
			/* Normal mutex.  */
			__pmarcel_lock(&mutex->__data.__lock, id);
			break;
	}
	/* Record the ownership.  */
	MA_BUG_ON (mutex->__data.__owner != 0);
	mutex->__data.__owner = id;
	++mutex->__data.__nusers;

	LOG_RETURN(0);
}
int pmarcel_mutex_trylock(pmarcel_mutex_t * mutex) {
	struct marcel_task *id;
	LOG_IN();
	switch (__builtin_expect (mutex->__data.__kind, PMARCEL_MUTEX_TIMED_NP)) {
		case PMARCEL_MUTEX_RECURSIVE_NP:
			id = MARCEL_SELF;
			if (mutex->__data.__owner == id) {
				if (tbx_unlikely (mutex->__data.__count + 1 == 0))
					LOG_RETURN(EAGAIN);
				++mutex->__data.__count;
				LOG_RETURN(0);
			}

			if (__pmarcel_trylock(&mutex->__data.__lock) != 0) {
				mutex->__data.__owner = id;
				mutex->__data.__count = 1;
				++mutex->__data.__nusers;
				return 0;
			}
			break;

		case PMARCEL_MUTEX_ERRORCHECK_NP:
		default:
			/* Correct code cannot set any other type.  */
		case PMARCEL_MUTEX_TIMED_NP:
		case PMARCEL_MUTEX_ADAPTIVE_NP:
			/* Normal mutex.  */
			if (__pmarcel_trylock(&mutex->__data.__lock) != 0) {
				/* Record the ownership.  */
				mutex->__data.__owner = MARCEL_SELF;
				++mutex->__data.__nusers;	
				LOG_RETURN(0);
			}
	}
	LOG_RETURN(EBUSY);
}
static int pmarcel_mutex_blockcell(pmarcel_mutex_t * mutex,const struct timespec *abstime) {
	struct timeval now, tv;
	unsigned long int timeout;
	blockcell c;
	/* il faut arrondir au sup�rieur */
	tv.tv_sec = abstime->tv_sec;
	tv.tv_usec =(abstime->tv_nsec + 999) / 1000;
	gettimeofday(&now, NULL);
	if(timercmp(&tv, &now, <=)) {
		mdebug("pmarcel_mutex_blockcell : valeur temporelle invalide\n");
		LOG_RETURN(ETIMEDOUT);
	}

	timeout = (((tv.tv_sec*1e6 + tv.tv_usec) -
				(now.tv_sec*1e6 + now.tv_usec)) + marcel_gettimeslice()-1)/marcel_gettimeslice();

	ma_fastlock_acquire(&mutex->__data.__lock); 

	__pmarcel_register_spinlocked(&mutex->__data.__lock,
			marcel_self(), &c);

	//tant que c'est bloqu� et qu'il y a du temps...
	while(c.blocked && timeout) {
		ma_set_current_state(MA_TASK_INTERRUPTIBLE);
		ma_fastlock_release(&mutex->__data.__lock);
		timeout = ma_schedule_timeout(timeout+1);
		ma_fastlock_acquire(&mutex->__data.__lock);
	}
	// si c'est encore bloqu� (cad le temps est �coul�)
	if (c.blocked) {
		if (__pmarcel_unregister_spinlocked(&mutex->__data.__lock, &c)) {
			pm2debug("Strange, we should be in the queue !!! (%s:%d)\n", __FILE__, __LINE__);
		}
		//on sort	
		ma_fastlock_release(&mutex->__data.__lock);
		LOG_RETURN(ETIMEDOUT);
	}
	ma_fastlock_release(&mutex->__data.__lock);
	LOG_RETURN(0);
}
int pmarcel_mutex_timedlock(pmarcel_mutex_t * mutex,const struct timespec *abstime) {
	marcel_t cthread;
	int ret;
	LOG_IN();

	cthread = marcel_self();

	if (__builtin_expect (abstime->tv_nsec, 0) < 0
			|| __builtin_expect (abstime->tv_nsec, 0) >= 1000000000) {
		mdebug("pmarcel_mutex_timedlock : valeur temporelle invalide\n");       
		LOG_RETURN(EINVAL);
	}

	switch(mutex->__data.__kind) {

		case PMARCEL_MUTEX_RECURSIVE_NP:
			cthread = cthread;
			if (mutex->__data.__owner == cthread) {
				mutex->__data.__count++;
				LOG_RETURN(0);
			}
			if (mutex->__data.__nusers == 0)
				mutex->__data.__count = 1;
			else {
				ret = pmarcel_mutex_blockcell(mutex,abstime);
				if (ret)
					LOG_RETURN(ETIMEDOUT);
				else
					mutex->__data.__count = 1;
			}
			break;

		case PMARCEL_MUTEX_ERRORCHECK_NP:
			cthread = cthread;
			if (mutex->__data.__owner == cthread) 
				LOG_RETURN(EDEADLK);
			if (mutex->__data.__nusers != 0) {
				ret = pmarcel_mutex_blockcell(mutex,abstime);
				if (ret)
					LOG_RETURN(ETIMEDOUT);
			}
			break;

		case PMARCEL_MUTEX_ADAPTIVE_NP:
		case PMARCEL_MUTEX_TIMED_NP:
			if (mutex->__data.__nusers != 0) {
				ret = pmarcel_mutex_blockcell(mutex,abstime);
				if (ret)
					LOG_RETURN(ETIMEDOUT);
			}
			break;

		default:
			LOG_RETURN(EINVAL);
	}

	mutex->__data.__nusers ++;
	mutex->__data.__owner = cthread;
	__pmarcel_lock(&mutex->__data.__lock, NULL);
	LOG_RETURN(0);
}
int pmarcel_mutex_unlock_usercnt(pmarcel_mutex_t * mutex, int decr) {
	LOG_IN();

	switch (__builtin_expect (mutex->__data.__kind, PMARCEL_MUTEX_TIMED_NP)) {
		case PMARCEL_MUTEX_RECURSIVE_NP:
			if (mutex->__data.__owner != MARCEL_SELF)
				LOG_RETURN(EPERM);

			if (--mutex->__data.__count != 0)
				/* We still hold the mutex.  */
				LOG_RETURN(0);
			break;

		case PMARCEL_MUTEX_ERRORCHECK_NP:
			if (mutex->__data.__owner != MARCEL_SELF)
				LOG_RETURN(EPERM);
			break;

		default:
			/* Correct code cannot set any other type.  */
		case PMARCEL_MUTEX_TIMED_NP:
		case PMARCEL_MUTEX_ADAPTIVE_NP:
			/* Normal mutex.  Nothing special to do.  */
			break;
	}

	/* Always reset the owner field.  */
	mutex->__data.__owner = 0;
	if (decr)
		/* One less user.  */
		--mutex->__data.__nusers;

	/* Unlock.  */
	__pmarcel_unlock(&mutex->__data.__lock);

	LOG_RETURN(0);
}

int pmarcel_mutex_unlock(pmarcel_mutex_t * mutex) {
	LOG_IN();
	LOG_RETURN(pmarcel_mutex_unlock_usercnt (mutex, 1));
}
/* We use bit 31 to signal whether the mutex is going to be
   process-shared or not.  By default it is zero, i.e., the
   mutex is not process-shared.  */
int pmarcel_mutexattr_init(pmarcel_mutexattr_t * attr) {
	LOG_IN();
	attr->__data.mutexkind = PMARCEL_MUTEX_NORMAL;
	LOG_RETURN(0);
}
int pmarcel_mutexattr_destroy(pmarcel_mutexattr_t * attr TBX_UNUSED) {
	LOG_IN();
	LOG_RETURN(0);
}
int pmarcel_mutexattr_settype(pmarcel_mutexattr_t * attr, int kind) {
	LOG_IN();
	if (kind < PMARCEL_MUTEX_NORMAL || kind > PMARCEL_MUTEX_ADAPTIVE_NP) {
		mdebug("pmarcel_mutexattr_settype : valeur kind(%d) invalide\n",kind);
		LOG_RETURN(EINVAL);
	}
	attr->__data.mutexkind = (attr->__data.mutexkind & 0x80000000) | kind;
	LOG_RETURN(0);
}
int pmarcel_mutexattr_gettype(const pmarcel_mutexattr_t * __restrict attr,
		int * __restrict kind) {
	LOG_IN();
	*kind = attr->__data.mutexkind & ~0x80000000;
	LOG_RETURN(0);
}
int pmarcel_mutexattr_setpshared(pmarcel_mutexattr_t * attr, int pshared) {
	LOG_IN();
	if (pshared != PMARCEL_PROCESS_PRIVATE
			&& __builtin_expect (pshared != PMARCEL_PROCESS_SHARED, 0)) {
		LOG_RETURN(EINVAL);
	}
	/* For now it is not possible to share a mutex variable.  */
	if (pshared != MARCEL_PROCESS_PRIVATE) {
		LOG_RETURN(ENOTSUP);
	}

	if (pshared == PMARCEL_PROCESS_PRIVATE)
		attr->__data.mutexkind &= ~0x80000000;
	else
		attr->__data.mutexkind |= 0x80000000;

	LOG_RETURN(0);
}
int pmarcel_mutexattr_getpshared(const pmarcel_mutexattr_t * __restrict attr,
		int * __restrict pshared) {
	LOG_IN();
	*pshared = ((attr->__data.mutexkind & 0x80000000) != 0
			? PMARCEL_PROCESS_SHARED : PMARCEL_PROCESS_PRIVATE);

	LOG_RETURN(0);
}
#endif
