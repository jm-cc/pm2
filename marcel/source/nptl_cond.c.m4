dnl -*- linux-c -*-
include(scripts/marcel.m4)
dnl /***************************
dnl  * This is the original file
dnl  * =========================
dnl  ***************************/
/* This file has been autogenerated from __file__ */
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
#if defined(MA__IFACE_PMARCEL) || defined(MA__IFACE_LPT) || defined(MA__LIBPTHREAD)
#include <pthread.h>
#endif

#include "marcel_fastlock.h"

/****************************************************************
 * CONDITIONS
 */
 
/*****************/
/* condattr_init */
/*****************/
PRINT_PTHREAD([[dnl
DEF_LIBPTHREAD(int, condattr_init,
	  (pthread_condattr_t * attr),
	  (attr))
DEF___LIBPTHREAD(int, condattr_init,
	  (pthread_condattr_t * attr),
	  (attr))
]])

REPLICATE_CODE([[dnl
int prefix_condattr_init (prefix_condattr_t *attr)
{
	memset (attr, '\0', sizeof (*attr));
#if MA__MODE == MA__MODE_LPT
	MA_BUG_ON (sizeof (lpt_condattr_t) > __SIZEOF_LPT_CONDATTR_T);
#endif
	return 0;
}
]])

/********************/
/* condattr_destroy */
/********************/
PRINT_PTHREAD([[dnl
DEF_LIBPTHREAD(int, condattr_destroy,
	  (pthread_condattr_t * attr),
	  (attr))
DEF___LIBPTHREAD(int, condattr_destroy,
	  (pthread_condattr_t * attr),
	  (attr))
]])

REPLICATE_CODE([[dnl
int prefix_condattr_destroy (prefix_condattr_t *attr)
{
        /* Nothing to be done.  */
        return 0;
}
]])

/***********************/
/* condattr_getpshared */
/***********************/
PRINT_PTHREAD([[dnl
DEF_LIBPTHREAD(int, condattr_getpshared,
	  (pthread_condattr_t * __restrict attr, int* __restrict pshared),
	  (attr, pshared))
]])

REPLICATE_CODE([[dnl
int prefix_condattr_getpshared (const prefix_condattr_t * __restrict attr,
	int* __restrict pshared)
{
{
	*pshared = ((const struct prefix_condattr *) attr)->value & 1;
        return 0;
}}
]], [[PMARCEL LPT]])

/***********************/
/* condattr_setpshared */
/***********************/
PRINT_PTHREAD([[dnl
DEF_LIBPTHREAD(int, condattr_setpshared,
	  (pthread_condattr_t * attr, int pshared),
	  (attr, pshared))
]])

REPLICATE_CODE([[dnl
int prefix_condattr_setpshared (prefix_condattr_t *attr, int pshared)
{
        if (pshared != PTHREAD_PROCESS_PRIVATE
            && __builtin_expect (pshared != PTHREAD_PROCESS_SHARED, 0))
                return EINVAL;

	/* For now it is not possible to share a mutex variable.  */
	if (pshared != MARCEL_PROCESS_PRIVATE) {
		pm2debug("Argh: shared condition requested!\n");
		return ENOSYS;
	}

        int *valuep = &((struct prefix_condattr *) attr)->value;

        *valuep = (*valuep & ~1) | (pshared != PTHREAD_PROCESS_PRIVATE);

        return 0;
}
]], [[PMARCEL LPT]])


/*************/
/* cond_init */
/*************/

PRINT_PTHREAD([[dnl
versioned_symbol (libpthread, lpt_cond_init,
                  pthread_cond_init, GLIBC_2_3_2);
strong_alias (lpt_cond_init, __old_pthread_cond_init)
compat_symbol (libpthread, __old_pthread_cond_init, pthread_cond_init,
               GLIBC_2_0);
]])

REPLICATE_CODE([[dnl
int prefix_cond_init (prefix_cond_t * __restrict cond,
	const prefix_condattr_t * __restrict attr)
{
  cond->__data.__lock = (struct _marcel_fastlock) MA_FASTLOCK_UNLOCKED;
  cond->__data.__waiting = NULL;

  return 0;
}
]], [[MARCEL PMARCEL]])

REPLICATE_CODE([[dnl
int prefix_cond_init (prefix_cond_t * __restrict cond,
	const prefix_condattr_t * __restrict attr)
{
  cond->__data.__lock = (struct _prefix_fastlock) MA_LPT_FASTLOCK_UNLOCKED;
  cond->__data.__waiting = NULL;

  return 0;
}
]], [[LPT]])

/****************/
/* cond_destroy */
/****************/

PRINT_PTHREAD([[dnl
versioned_symbol (libpthread, lpt_cond_destroy,
                  pthread_cond_destroy, GLIBC_2_3_2);
strong_alias (lpt_cond_destroy, __old_pthread_cond_destroy)
compat_symbol (libpthread, __old_pthread_cond_destroy, pthread_cond_destroy,
               GLIBC_2_0);
]])

REPLICATE_CODE([[dnl
int prefix_cond_destroy (prefix_cond_t *cond)
{
  if (cond->__data.__waiting != NULL) {
    /* TODO: pas tr�s POSIX �a. Voir l'impl�mentation dans nptl */
    return EBUSY;
  }
  return 0;
}
]])

/***************/
/* cond_signal */
/***************/

PRINT_PTHREAD([[dnl
versioned_symbol (libpthread, lpt_cond_signal,
                  pthread_cond_signal, GLIBC_2_3_2);
strong_alias (lpt_cond_signal, __old_pthread_cond_signal)
compat_symbol (libpthread, __old_pthread_cond_signal, pthread_cond_signal,
               GLIBC_2_0);
]])

REPLICATE_CODE([[dnl
int prefix_cond_signal (prefix_cond_t *cond)
{
  __prefix_unlock(&cond->__data.__lock);
  return 0;
}
]])

/******************/
/* cond_broadcast */
/******************/

PRINT_PTHREAD([[dnl
versioned_symbol (libpthread, lpt_cond_broadcast,
                  pthread_cond_broadcast, GLIBC_2_3_2);
strong_alias (lpt_cond_broadcast, __old_pthread_cond_broadcast)
compat_symbol (libpthread, __old_pthread_cond_broadcast,
               pthread_cond_broadcast, GLIBC_2_0);
]])

REPLICATE_CODE([[dnl
int prefix_cond_broadcast (prefix_cond_t *cond)
{
  prefix_lock_acquire(&cond->__data.__lock.__spinlock);
  do {} while (__prefix_unlock_spinlocked(&cond->__data.__lock));
  prefix_lock_release(&cond->__data.__lock.__spinlock);
  return 0;
}
]])

/*************/
/* cond_wait */
/*************/

PRINT_PTHREAD([[dnl
versioned_symbol (libpthread, lpt_cond_wait,
                  pthread_cond_wait, GLIBC_2_3_2);
strong_alias (lpt_cond_wait, __old_pthread_cond_wait)
compat_symbol (libpthread, __old_pthread_cond_wait, pthread_cond_wait,
               GLIBC_2_0);
]])

REPLICATE_CODE([[dnl
int prefix_cond_wait (prefix_cond_t * __restrict cond,
	prefix_mutex_t * __restrict mutex)
{
  prefix_lock_acquire(&mutex->__data.__lock.__spinlock);
  prefix_lock_acquire(&cond->__data.__lock.__spinlock);
  __prefix_unlock_spinlocked(&mutex->__data.__lock);
  prefix_lock_release(&mutex->__data.__lock.__spinlock);
  {
	  blockcell c;
	  
	  __prefix_register_spinlocked(&cond->__data.__lock, 
		  marcel_self(), &c);

	  mdebug("blocking %p (cell %p) in prefix_cond_wait %p\n", 
		  marcel_self(), &c, cond);
	  INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(
		  c.blocked, 
		  prefix_lock_release(&cond->__data.__lock.__spinlock),
		  prefix_lock_acquire(&cond->__data.__lock.__spinlock));
	  prefix_lock_release(&cond->__data.__lock.__spinlock);
	  mdebug("unblocking %p (cell %p) in prefix_cond_wait %p\n",
		  marcel_self(), &c, cond);
  }

  prefix_mutex_lock(mutex);
  return 0;
}
]])


/******************/
/* cond_timedwait */
/******************/

PRINT_PTHREAD([[dnl
versioned_symbol (libpthread, lpt_cond_timedwait,
                  pthread_cond_timedwait, GLIBC_2_3_2);
strong_alias (lpt_cond_timedwait, __old_pthread_cond_timedwait)
compat_symbol (libpthread, __old_pthread_cond_timedwait,
               pthread_cond_timedwait, GLIBC_2_0);
]])

#define ma_timercmp(a, b, CMP) \
  (((a)->tv_sec != (b)->tv_sec)? \
   ((a)->tv_sec CMP (b)->tv_sec): \
   ((a)->tv_usec CMP (b)->tv_usec))

REPLICATE_CODE([[dnl
int prefix_cond_timedwait(prefix_cond_t * __restrict cond,
	prefix_mutex_t * __restrict mutex,
	const struct timespec * __restrict abstime)
{
	struct timeval now, tv;
	unsigned long timeout;
	int ret = 0;

	LOG_IN();
	tv.tv_sec = abstime->tv_sec;
	tv.tv_usec = abstime->tv_nsec / 1000;
	
	gettimeofday(&now, NULL);
	
	if(ma_timercmp(&tv, &now, <=)) {
		LOG_OUT();
		return ETIMEDOUT;
	}
	
	timeout = JIFFIES_FROM_US(((tv.tv_sec*1e6 + tv.tv_usec) -
				   (now.tv_sec*1e6 + now.tv_usec)));
	
	prefix_lock_acquire(&mutex->__data.__lock.__spinlock);
	prefix_lock_acquire(&cond->__data.__lock.__spinlock);
	__prefix_unlock_spinlocked(&mutex->__data.__lock);
	prefix_lock_release(&mutex->__data.__lock.__spinlock);
	{
		blockcell c;
		
		__prefix_register_spinlocked(&cond->__data.__lock,
		                             marcel_self(), &c);
		
		mdebug("blocking %p (cell %p) in prefix_cond_timedwait %p\n",
		       marcel_self(), &c, cond);
#ifdef PM2_DEV
#warning not managing work in cond_timedwait
#endif
		while(c.blocked && timeout) {
			ma_set_current_state(MA_TASK_INTERRUPTIBLE);
			prefix_lock_release(&cond->__data.__lock.__spinlock);
			timeout=ma_schedule_timeout(timeout);
			prefix_lock_acquire(&cond->__data.__lock.__spinlock);
		}
		if (c.blocked) {
			if (__prefix_unregister_spinlocked(&cond->__data.__lock, &c)) {
				pm2debug("Strange, we should be in the queue !!! (%s:%d)\n", __FILE__, __LINE__);
			}
			ret=ETIMEDOUT;
		}
		prefix_lock_release(&cond->__data.__lock.__spinlock);
		mdebug("unblocking %p (cell %p) in prefix_cond_timedwait %p\n",
		       marcel_self(), &c, cond);
	}
	
	prefix_mutex_lock(mutex);
	LOG_OUT();
	return ret;
}
]])

