
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
#include "marcel_restart.h" // pour thread_self()

     /****************************************************************
      * CONDITIONS
      */
     
DEF_MARCEL_POSIX(int, condattr_init, (marcel_condattr_t *attr))
{
  return 0;
}
DEF_PTHREAD(condattr_init)

DEF_MARCEL_POSIX(int, condattr_destroy, (marcel_condattr_t *attr))
{
  return 0;
}
DEF_PTHREAD(condattr_destroy)

static marcel_condattr_t marcel_condattr_default = { 0, };

DEF_MARCEL_POSIX(int, cond_init, 
		 (marcel_cond_t *cond, 
		  __const marcel_condattr_t *attr))
{
  //LOG_IN();
  if(!attr) {
    attr = &marcel_condattr_default;
  }
  cond->__c_lock.__status = 0; //, 0};
  cond->__c_lock.__spinlock = 0; //, 0};
  cond->__c_waiting = NULL;
  //LOG_OUT();
  return 0;
}
DEF_PTHREAD(cond_init)

DEF_MARCEL_POSIX(int, cond_destroy, (marcel_cond_t *cond))
{
  LOG_IN();
  if (cond->__c_waiting != NULL) {
    LOG_OUT();
    return EBUSY;
  }
  LOG_OUT();
  return 0;
}
DEF_PTHREAD(cond_destroy)

DEF_MARCEL_POSIX(int, cond_signal, (marcel_cond_t *cond))
{
  LOG_IN();
  lock_task();
  __marcel_unlock(&cond->__c_lock);
  LOG_OUT();
  return 0;
}
DEF_PTHREAD(cond_signal)

DEF_MARCEL_POSIX(int, cond_broadcast, (marcel_cond_t *cond))
{
  LOG_IN();
  lock_task();
  marcel_lock_acquire(&cond->__c_lock.__spinlock);
  do {} while (__marcel_unlock_spinlocked(&cond->__c_lock));
  marcel_lock_release(&cond->__c_lock.__spinlock);
  unlock_task();
  LOG_OUT();
  return 0;
}
DEF_PTHREAD(cond_broadcast)

DEF_MARCEL_POSIX(int, cond_wait, (marcel_cond_t *cond, 
				  marcel_mutex_t *mutex))
{
  LOG_IN();
  lock_task();

  marcel_lock_acquire(&mutex->__m_lock.__spinlock);
  marcel_lock_acquire(&cond->__c_lock.__spinlock);
  __marcel_unlock_spinlocked(&mutex->__m_lock);
  marcel_lock_release(&mutex->__m_lock.__spinlock);
  __marcel_block_spinlocked(&cond->__c_lock, marcel_self()); /* Fait le release et le unlock_task() */

  marcel_mutex_lock(mutex);
  LOG_OUT();
  return 0;
}
DEF_PTHREAD(cond_wait)

DEF_MARCEL_POSIX(int, cond_timedwait, 
		 (marcel_cond_t *cond, marcel_mutex_t *mutex,
		  const struct timespec *abstime))
{
  struct timeval now, tv;
  unsigned long timeout;
  int r = 0;

  LOG_IN();
  tv.tv_sec = abstime->tv_sec;
  tv.tv_usec = abstime->tv_nsec / 1000;

  gettimeofday(&now, NULL);

  if(timercmp(&tv, &now, <=)) {
    LOG_OUT();
    return ETIMEDOUT;
  }

  timeout = ((tv.tv_sec*1e6 + tv.tv_usec) -
	     (now.tv_sec*1e6 + now.tv_usec)) / 1000;

  lock_task();

  marcel_lock_acquire(&mutex->__m_lock.__spinlock);
  marcel_lock_acquire(&cond->__c_lock.__spinlock);

  __marcel_unlock_spinlocked(&mutex->__m_lock);


  {
    blockcell c;
    __marcel_register_spinlocked(&cond->__c_lock, marcel_self(), &c);
    marcel_lock_release(&cond->__c_lock.__spinlock);
    if (__marcel_tempo_give_hand(timeout, &c.blocked, NULL)) {
      if (__marcel_unregister_spinlocked(&cond->__c_lock, &c)) {
	pm2debug("Strange, we should be in the queue !!! (%s:%d)\n", __FILE__, __LINE__);
      }
      r=ETIMEDOUT;
    }
  }

  marcel_mutex_lock(mutex);
  LOG_OUT();

  return r;
}
DEF_PTHREAD(cond_timedwait)
DEF___PTHREAD(cond_timedwait)
