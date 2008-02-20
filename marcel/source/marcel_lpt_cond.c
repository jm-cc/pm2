 
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
#include <errno.h>
#include "marcel_fastlock.h"

#define ma_timercmp(a, b, CMP) \
  (((a)->tv_sec != (b)->tv_sec)? \
   ((a)->tv_sec CMP (b)->tv_sec): \
   ((a)->tv_usec CMP (b)->tv_usec))

#ifdef MA__LIBPTHREAD
DEF_LIBPTHREAD(int, condattr_init,
	  (pthread_condattr_t * attr),
	  (attr))
DEF___LIBPTHREAD(int, condattr_init,
	  (pthread_condattr_t * attr),
	  (attr))
DEF_LIBPTHREAD(int, condattr_destroy,
	  (pthread_condattr_t * attr),
	  (attr))
DEF___LIBPTHREAD(int, condattr_destroy,
	  (pthread_condattr_t * attr),
	  (attr))
DEF_LIBPTHREAD(int, condattr_setpshared,
	  (pthread_condattr_t * attr, int pshared),
	  (attr, pshared))
versioned_symbol (libpthread, lpt_cond_init,
                  pthread_cond_init, GLIBC_2_3_2);
versioned_symbol (libpthread, lpt_cond_destroy,
                  pthread_cond_destroy, GLIBC_2_3_2);
versioned_symbol (libpthread, lpt_cond_signal,
                  pthread_cond_signal, GLIBC_2_3_2);
versioned_symbol (libpthread, lpt_cond_broadcast,
                  pthread_cond_broadcast, GLIBC_2_3_2);
versioned_symbol (libpthread, lpt_cond_wait,
                  pthread_cond_wait, GLIBC_2_3_2);
versioned_symbol (libpthread, lpt_cond_timedwait,
                  pthread_cond_timedwait, GLIBC_2_3_2);
#  if MA_GLIBC_VERSION_MINIMUM < 20302
strong_alias (lpt_cond_init, __old_pthread_cond_init)
compat_symbol (libpthread, __old_pthread_cond_init, pthread_cond_init,
               GLIBC_2_0);
strong_alias (lpt_cond_destroy, __old_pthread_cond_destroy)
compat_symbol (libpthread, __old_pthread_cond_destroy, pthread_cond_destroy,
               GLIBC_2_0);
strong_alias (lpt_cond_signal, __old_pthread_cond_signal)
compat_symbol (libpthread, __old_pthread_cond_signal, pthread_cond_signal,
               GLIBC_2_0);
strong_alias (lpt_cond_broadcast, __old_pthread_cond_broadcast)
compat_symbol (libpthread, __old_pthread_cond_broadcast,
               pthread_cond_broadcast, GLIBC_2_0);
strong_alias (lpt_cond_wait, __old_pthread_cond_wait)
compat_symbol (libpthread, __old_pthread_cond_wait, pthread_cond_wait,
               GLIBC_2_0);
strong_alias (lpt_cond_timedwait, __old_pthread_cond_timedwait)
compat_symbol (libpthread, __old_pthread_cond_timedwait,
               pthread_cond_timedwait, GLIBC_2_0);
#  endif
#endif
#ifdef MA__IFACE_LPT
int lpt_condattr_init (lpt_condattr_t *attr) {
        LOG_IN();
	memset (attr, '\0', sizeof (*attr));
	MA_BUG_ON (sizeof (lpt_condattr_t) > __SIZEOF_LPT_CONDATTR_T);
		  LOG_RETURN(0);
}
int lpt_condattr_destroy (lpt_condattr_t *attr)
{
        LOG_IN();
        /* Nothing to be done.  */
        LOG_RETURN(0);      
}
int lpt_condattr_getpshared (const lpt_condattr_t * __restrict attr,
	int* __restrict pshared)
{
        LOG_IN();
	*pshared = ((const struct lpt_condattr *) attr)->value & 1;
        LOG_RETURN(0);
}
int lpt_condattr_setpshared (lpt_condattr_t *attr, int pshared)
{
	LOG_IN();
	if (pshared != LPT_PROCESS_PRIVATE
	    && __builtin_expect(pshared != LPT_PROCESS_SHARED, 0)) {
		mdebug
		    ("lpt_condattr_setpshared : valeur pshared(%d)  invalide\n",
		    pshared);
		LOG_RETURN(EINVAL);
	}

	/* For now it is not possible to share a mutex variable.  */
	if (pshared != LPT_PROCESS_PRIVATE) {
		fprintf(stderr,
		    "lpt_condattr_setpshared : shared condition requested!\n");
		LOG_RETURN(ENOTSUP);
	}

	int *valuep = &((struct lpt_condattr *) attr)->value;
	*valuep = (*valuep & ~1) | (pshared != LPT_PROCESS_PRIVATE);

	LOG_RETURN(0);
}
int lpt_cond_init (lpt_cond_t * __restrict cond,
	const lpt_condattr_t * __restrict attr)
{
	LOG_IN();
	cond->__data.__lock =
	    (struct _lpt_fastlock) MA_LPT_FASTLOCK_UNLOCKED;
	cond->__data.__waiting = NULL;
	LOG_RETURN(0);
}
int lpt_cond_destroy (lpt_cond_t *cond)
{
        LOG_IN();
        if (cond->__data.__waiting != NULL)
                LOG_RETURN(EBUSY);
        LOG_RETURN(0);
}
int lpt_cond_signal (lpt_cond_t *cond)
{
	LOG_IN();
	__lpt_unlock(&cond->__data.__lock);
	LOG_RETURN(0);
}
int lpt_cond_broadcast (lpt_cond_t *cond)
{
	LOG_IN();
	lpt_lock_acquire(&cond->__data.__lock.__spinlock);
	do { } while (__lpt_unlock_spinlocked(&cond->__data.__lock));
	lpt_lock_release(&cond->__data.__lock.__spinlock);
	LOG_RETURN(0);
}
int lpt_cond_wait (lpt_cond_t * __restrict cond,
	lpt_mutex_t * __restrict mutex)
{
	LOG_IN();
	lpt_lock_acquire(&mutex->__data.__lock.__spinlock);
	lpt_lock_acquire(&cond->__data.__lock.__spinlock);
	mutex->__data.__owner = 0;
	__lpt_unlock_spinlocked(&mutex->__data.__lock);
	lpt_lock_release(&mutex->__data.__lock.__spinlock);
	{
		blockcell c;

		__lpt_register_spinlocked(&cond->__data.__lock,
		    marcel_self(), &c);

		mdebug("blocking %p (cell %p) in lpt_cond_wait %p\n",
		    marcel_self(), &c, cond);
		INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(c.blocked,
		    lpt_lock_release(&cond->__data.__lock.__spinlock),
		    lpt_lock_acquire(&cond->__data.__lock.__spinlock));
		lpt_lock_release(&cond->__data.__lock.__spinlock);
		mdebug("unblocking %p (cell %p) in lpt_cond_wait %p\n",
		    marcel_self(), &c, cond);
	}
	lpt_mutex_lock(mutex);
	LOG_RETURN(0);
}

int lpt_cond_timedwait(lpt_cond_t * __restrict cond,
	lpt_mutex_t * __restrict mutex,
	const struct timespec * __restrict abstime)
{
	LOG_IN();
	struct timeval now, tv;
	unsigned long timeout;
	int ret = 0;

	tv.tv_sec = abstime->tv_sec;
	tv.tv_usec = abstime->tv_nsec / 1000;

	gettimeofday(&now, NULL);

	if (ma_timercmp(&tv, &now, <=)) {
		LOG_RETURN(ETIMEDOUT);
	}

	timeout = JIFFIES_FROM_US(((tv.tv_sec * 1e6 + tv.tv_usec) -
		(now.tv_sec * 1e6 + now.tv_usec)));

	lpt_lock_acquire(&mutex->__data.__lock.__spinlock);
	lpt_lock_acquire(&cond->__data.__lock.__spinlock);
	mutex->__data.__owner = 0;
	__lpt_unlock_spinlocked(&mutex->__data.__lock);
	lpt_lock_release(&mutex->__data.__lock.__spinlock);
	{
		blockcell c;
		__lpt_register_spinlocked(&cond->__data.__lock,
		    marcel_self(), &c);

		mdebug("blocking %p (cell %p) in lpt_cond_timedwait %p\n",
		    marcel_self(), &c, cond);
		while (c.blocked && timeout) {
			ma_set_current_state(MA_TASK_INTERRUPTIBLE);
			lpt_lock_release(&cond->__data.__lock.__spinlock);
			timeout = ma_schedule_timeout(timeout);
			lpt_lock_acquire(&cond->__data.__lock.__spinlock);
		}
		if (c.blocked) {
			if (__lpt_unregister_spinlocked(&cond->__data.__lock,
				&c)) {
				pm2debug
				    ("Strange, we should be in the queue !!! (%s:%d)\n",
				    __FILE__, __LINE__);
			}
			ret = ETIMEDOUT;
		}
		lpt_lock_release(&cond->__data.__lock.__spinlock);
		mdebug("unblocking %p (cell %p) in lpt_cond_timedwait %p\n",
		    marcel_self(), &c, cond);
	}

	lpt_mutex_lock(mutex);
	LOG_RETURN(ret);
}
#endif



