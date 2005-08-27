
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

#include "marcel_for_pthread.h"

#include <errno.h>

#include "marcel_fastlock.h"

     /****************************************************************
      * CONDITIONS
      */
     
#if 0
DEF_MARCEL_POSIX(int, condattr_init, (marcel_condattr_t *attr), (attr))
{
  return 0;
}
DEF_PTHREAD(int, condattr_init, (pthread_condattr_t *attr), (attr))

DEF_MARCEL_POSIX(int, condattr_destroy, (marcel_condattr_t *attr), (attr))
{
  return 0;
}
DEF_PTHREAD(int, condattr_destroy, (pthread_condattr_t *attr), (attr))

static marcel_condattr_t marcel_condattr_default = { 0, };

DEF_MARCEL_POSIX(int, cond_init, 
		 (marcel_cond_t *cond, 
		  __const marcel_condattr_t *attr),
		 (cond, attr))
{
  //LOG_IN();
  if(!attr) {
    attr = &marcel_condattr_default;
  }
  cond->__c_lock = (struct _marcel_fastlock) MA_FASTLOCK_UNLOCKED;
  cond->__c_waiting = NULL;
  //LOG_OUT();
  return 0;
}
DEF_PTHREAD(int, cond_init, 
		 (pthread_cond_t *cond, 
		  __const pthread_condattr_t *attr),
		 (cond, attr))

DEF_MARCEL_POSIX(int, cond_destroy, (marcel_cond_t *cond), (cond))
{
  LOG_IN();
  if (cond->__c_waiting != NULL) {
    LOG_OUT();
    return EBUSY;
  }
  LOG_OUT();
  return 0;
}
DEF_PTHREAD(int, cond_destroy, (pthread_cond_t *cond), (cond))

DEF_MARCEL_POSIX(int, cond_signal, (marcel_cond_t *cond), (cond))
{
  LOG_IN();
  __marcel_unlock(&cond->__c_lock);
  LOG_OUT();
  return 0;
}
DEF_PTHREAD(int, cond_signal, (pthread_cond_t *cond), (cond))

DEF_MARCEL_POSIX(int, cond_broadcast, (marcel_cond_t *cond), (cond))
{
  LOG_IN();
  marcel_lock_acquire(&cond->__c_lock.__spinlock);
  do {} while (__marcel_unlock_spinlocked(&cond->__c_lock));
  marcel_lock_release(&cond->__c_lock.__spinlock);
  LOG_OUT();
  return 0;
}
DEF_PTHREAD(int, cond_broadcast, (pthread_cond_t *cond), (cond))

DEF_MARCEL_POSIX(int, cond_wait, (marcel_cond_t *cond, 
				  marcel_mutex_t *mutex), (cond, mutex))
{
  LOG_IN();

  marcel_lock_acquire(&mutex->__m_lock.__spinlock);
  marcel_lock_acquire(&cond->__c_lock.__spinlock);
  __marcel_unlock_spinlocked(&mutex->__m_lock);
  marcel_lock_release(&mutex->__m_lock.__spinlock);
  {
	  blockcell c;
	  
	  __marcel_register_spinlocked(&cond->__c_lock, marcel_self(), &c);

	  mdebug("blocking %p (cell %p) in cond_wait %p\n", marcel_self(), &c,
		 cond);
	  INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(
		  c.blocked, 
		  marcel_lock_release(&cond->__c_lock.__spinlock),
		  marcel_lock_acquire(&cond->__c_lock.__spinlock));
	  marcel_lock_release(&cond->__c_lock.__spinlock);
	  mdebug("unblocking %p (cell %p) in cond_wait %p\n", marcel_self(), &c,
		 cond);
  }

  marcel_mutex_lock(mutex);
  LOG_OUT();
  return 0;
}
DEF_PTHREAD(int, cond_wait, (pthread_cond_t *cond, 
				  pthread_mutex_t *mutex), (cond, mutex))

DEF_MARCEL_POSIX(int, cond_timedwait, 
		 (marcel_cond_t *cond, marcel_mutex_t *mutex,
		  const struct timespec *abstime), (cond, mutex, abstime))
{
	struct timeval now, tv;
	unsigned long timeout;
	int ret = 0;

	LOG_IN();
	tv.tv_sec = abstime->tv_sec;
	tv.tv_usec = abstime->tv_nsec / 1000;
	
	gettimeofday(&now, NULL);
	
	if(timercmp(&tv, &now, <=)) {
		LOG_OUT();
		return ETIMEDOUT;
	}
	
	timeout = JIFFIES_FROM_US(((tv.tv_sec*1e6 + tv.tv_usec) -
				   (now.tv_sec*1e6 + now.tv_usec)));
	
	marcel_lock_acquire(&mutex->__m_lock.__spinlock);
	marcel_lock_acquire(&cond->__c_lock.__spinlock);
	__marcel_unlock_spinlocked(&mutex->__m_lock);
	marcel_lock_release(&mutex->__m_lock.__spinlock);
	{
		blockcell c;
		
		__marcel_register_spinlocked(&cond->__c_lock, marcel_self(), &c);
		
		mdebug("blocking %p (cell %p) in cond_wait %p\n", marcel_self(), &c,
		       cond);
#ifdef PM2_DEV
#warning not managing work in cond_timedwait
#endif
		while(c.blocked && timeout) {
			ma_set_current_state(MA_TASK_INTERRUPTIBLE);
			marcel_lock_release(&cond->__c_lock.__spinlock);
			timeout=ma_schedule_timeout(timeout);
			marcel_lock_acquire(&cond->__c_lock.__spinlock);
		}
		if (c.blocked) {
			if (__marcel_unregister_spinlocked(&cond->__c_lock, &c)) {
				pm2debug("Strange, we should be in the queue !!! (%s:%d)\n", __FILE__, __LINE__);
			}
			ret=ETIMEDOUT;
		}
		marcel_lock_release(&cond->__c_lock.__spinlock);
		mdebug("unblocking %p (cell %p) in cond_wait %p\n", marcel_self(), &c,
		       cond);
	}
	
	marcel_mutex_lock(mutex);
	LOG_OUT();
	return ret;
}
DEF_PTHREAD(int, cond_timedwait, 
		 (pthread_cond_t *cond, pthread_mutex_t *mutex,
		  const struct timespec *abstime), (cond, mutex, abstime))
DEF___PTHREAD(int, cond_timedwait, 
		 (pthread_cond_t *cond, pthread_mutex_t *mutex,
		  const struct timespec *abstime), (cond, mutex, abstime))
#endif
