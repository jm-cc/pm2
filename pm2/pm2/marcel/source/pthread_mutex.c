
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
//#include "marcel_restart.h" // pour thread_self()

#ifdef MA__POSIX_FUNCTIONS_NAMES
static inline _pmarcel_descr thread_self() {
  return (_pmarcel_descr)marcel_self();
}
#endif

/****************************************************************
 * MUTEX
 */

     /**************/
     /* mutex_init */
     /**************/
static marcel_mutexattr_t marcel_mutexattr_default = {0,};

DEF_MARCEL(int, mutex_init, 
	   (marcel_mutex_t *mutex, __const marcel_mutexattr_t *attr))
{
  //LOG_IN();
  mdebug("initializing mutex %p by %p in lock %p\n", 
	 mutex, marcel_self(), &mutex->__m_lock);
  if(!attr) {
    attr = &marcel_mutexattr_default;
  }
  __marcel_init_lock(&mutex->__m_lock);
  //LOG_OUT();
  return 0;
}
DEF_POSIX(int, mutex_init,
	  (pmarcel_mutex_t * mutex, const pmarcel_mutexattr_t * mutex_attr),
{
  __marcel_init_lock((struct _marcel_fastlock *)&mutex->__m_lock);
  mutex->__m_kind =
    mutex_attr == NULL ? MARCEL_MUTEX_TIMED_NP : mutex_attr->__mutexkind;
  mutex->__m_count = 0;
  mutex->__m_owner = NULL;
  return 0;
})
DEF_PTHREAD(mutex_init)
DEF___PTHREAD(mutex_init)
	  
     /*****************/
     /* mutex_destroy */
     /*****************/
DEF_MARCEL(int, mutex_destroy, (marcel_mutex_t *mutex))
{
  LOG_IN();
  if (mutex->__m_lock.__status != 0) {
    LOG_OUT();
    return EBUSY;
  }
  LOG_OUT();
  return 0;
}
DEF_POSIX(int, mutex_destroy, (pmarcel_mutex_t * mutex), 
{
  switch (mutex->__m_kind) {
  case MARCEL_MUTEX_ADAPTIVE_NP:
  case MARCEL_MUTEX_RECURSIVE_NP:
    if ((mutex->__m_lock.__status & 1) != 0)
      return EBUSY;
    return 0;
  case MARCEL_MUTEX_ERRORCHECK_NP:
  case MARCEL_MUTEX_TIMED_NP:
    if (mutex->__m_lock.__status != 0)
      return EBUSY;
    return 0;
  default:
    return EINVAL;
  }
})
DEF_PTHREAD(mutex_destroy)
DEF___PTHREAD(mutex_destroy)

     /**************/
     /* mutex_lock */
     /**************/
DEF_MARCEL(int, mutex_lock, (marcel_mutex_t *mutex))
{
  LOG_IN();
  lock_task();

  __marcel_lock(&mutex->__m_lock, marcel_self());

  LOG_OUT();
  return 0;
}
DEF_POSIX(int, mutex_lock, (pmarcel_mutex_t * mutex),
{
  _pmarcel_descr self;

  switch(mutex->__m_kind) {
  case MARCEL_MUTEX_ADAPTIVE_NP:
    __pmarcel_lock((struct _marcel_fastlock*)&mutex->__m_lock, NULL);
    return 0;
  case MARCEL_MUTEX_RECURSIVE_NP:
    self = thread_self();
    if (mutex->__m_owner == self) {
      mutex->__m_count++;
      return 0;
    }
    __pmarcel_lock((struct _marcel_fastlock*)&mutex->__m_lock, (marcel_t)self);
    mutex->__m_owner = self;
    mutex->__m_count = 0;
    return 0;
  case MARCEL_MUTEX_ERRORCHECK_NP:
    self = thread_self();
    if (mutex->__m_owner == self) {
      return EDEADLK;
    }
    __pmarcel_alt_lock((struct _marcel_fastlock*)&mutex->__m_lock,
		      (marcel_t)self);
    mutex->__m_owner = self;
    return 0;
  case MARCEL_MUTEX_TIMED_NP:
    __pmarcel_alt_lock((struct _marcel_fastlock*)&mutex->__m_lock, NULL);
    return 0;
  default:
    return EINVAL;
  }
})
DEF_PTHREAD(mutex_lock)
DEF___PTHREAD(mutex_lock)

     /*****************/
     /* mutex_trylock */
     /*****************/
DEF_MARCEL(int, mutex_trylock, (marcel_mutex_t *mutex))
{
  int ret;
  LOG_IN();
  lock_task();

  ret=__marcel_trylock(&mutex->__m_lock);
  LOG_OUT();
  return ret;
}
DEF_POSIX(int, mutex_trylock, (pmarcel_mutex_t * mutex),
{
  _pmarcel_descr self;
  int retcode;

  switch(mutex->__m_kind) {
  case MARCEL_MUTEX_ADAPTIVE_NP:
    retcode = __pmarcel_trylock((struct _marcel_fastlock *)&mutex->__m_lock);
    return (retcode==0);
  case MARCEL_MUTEX_RECURSIVE_NP:
    self = thread_self();
    if (mutex->__m_owner == self) {
      mutex->__m_count++;
      return 0;
    }
    retcode = __pmarcel_trylock((struct _marcel_fastlock *)&mutex->__m_lock);
    if (retcode != 0) {
      mutex->__m_owner = self;
      mutex->__m_count = 0;
    }
    return (retcode==0);
  case MARCEL_MUTEX_ERRORCHECK_NP:
    retcode = __pmarcel_alt_trylock((struct _marcel_fastlock*)&mutex->__m_lock);
    if (retcode != 0) {
      mutex->__m_owner = thread_self();
    }
    return (retcode==0);
  case MARCEL_MUTEX_TIMED_NP:
    retcode = __pmarcel_alt_trylock((struct _marcel_fastlock*)&mutex->__m_lock);
    return (retcode==0);
  default:
    return EINVAL;
  }
})
DEF_PTHREAD(mutex_trylock)
DEF___PTHREAD(mutex_trylock)

#if 0
     /*******************/
     /* mutex_timedlock */
     /*******************/
DEF_POSIX(int, mutex_timedlock, (pmarcel_mutex_t *mutex,
				 const struct timespec *abstime),
{
  marcel_descr self;
  int res;

  if (__builtin_expect (abstime->tv_nsec, 0) < 0
      || __builtin_expect (abstime->tv_nsec, 0) >= 1000000000)
    return EINVAL;

  switch(mutex->__m_kind) {
  case MARCEL_MUTEX_ADAPTIVE_NP:
    __pmarcel_lock(&mutex->__m_lock, NULL);
    return 0;
  case MARCEL_MUTEX_RECURSIVE_NP:
    self = thread_self();
    if (mutex->__m_owner == self) {
      mutex->__m_count++;
      return 0;
    }
    __pmarcel_lock(&mutex->__m_lock, self);
    mutex->__m_owner = self;
    mutex->__m_count = 0;
    return 0;
  case MARCEL_MUTEX_ERRORCHECK_NP:
    self = thread_self();
    if (mutex->__m_owner == self) return EDEADLK;
    res = __pmarcel_alt_timedlock(&mutex->__m_lock, self, abstime);
    if (res != 0)
      {
	mutex->__m_owner = self;
	return 0;
      }
    return ETIMEDOUT;
  case MARCEL_MUTEX_TIMED_NP:
    /* Only this type supports timed out lock. */
    return (__pmarcel_alt_timedlock(&mutex->__m_lock, NULL, abstime)
	    ? 0 : ETIMEDOUT);
  default:
    return EINVAL;
  }
})
DEF_PTHREAD(mutex_timedlock)
DEF___PTHREAD(mutex_timedlock)
#endif

     /****************/
     /* mutex_unlock */
     /****************/
DEF_MARCEL(int, mutex_unlock, (marcel_mutex_t *mutex))
{
  int ret;
  LOG_IN();
  lock_task();
  ret=__marcel_unlock(&mutex->__m_lock);
  LOG_OUT();
  return ret;
}
DEF_POSIX(int, mutex_unlock, (pmarcel_mutex_t * mutex),
{
  switch (mutex->__m_kind) {
  case MARCEL_MUTEX_ADAPTIVE_NP:
    __pmarcel_unlock((struct _marcel_fastlock*)&mutex->__m_lock);
    return 0;
  case MARCEL_MUTEX_RECURSIVE_NP:
    if (mutex->__m_owner != thread_self()) {
      return EPERM;
    }
    if (mutex->__m_count > 0) {
      mutex->__m_count--;
      return 0;
    }
    mutex->__m_owner = NULL;
    __pmarcel_unlock((struct _marcel_fastlock*)&mutex->__m_lock);
    return 0;
  case MARCEL_MUTEX_ERRORCHECK_NP:
    if (mutex->__m_owner != thread_self() || mutex->__m_lock.__status == 0) {
      return EPERM;
    }
    mutex->__m_owner = NULL;
    __pmarcel_alt_unlock((struct _marcel_fastlock*)&mutex->__m_lock);
    return 0;
  case MARCEL_MUTEX_TIMED_NP:
    __pmarcel_alt_unlock((struct _marcel_fastlock*)&mutex->__m_lock);
    return 0;
  default:
    return EINVAL;
  }
})
DEF_PTHREAD(mutex_unlock)
DEF___PTHREAD(mutex_unlock)

     /******************/
     /* mutexattr_init */
     /******************/
DEF_MARCEL(int, mutexattr_init, (marcel_mutexattr_t *attr))
{
  return 0;
}
DEF_POSIX(int, mutexattr_init, (pmarcel_mutexattr_t *attr),
{
  attr->__mutexkind = MARCEL_MUTEX_TIMED_NP;
  return 0;
})
DEF_PTHREAD(mutexattr_init)
DEF___PTHREAD(mutexattr_init)

     /*********************/
     /* mutexattr_destroy */
     /*********************/
DEF_MARCEL_POSIX(int, mutexattr_destroy, (marcel_mutexattr_t *attr))
{
  return 0;
}
DEF_PTHREAD(mutexattr_destroy)
DEF___PTHREAD(mutexattr_destroy)

     /****************************/
     /* mutex_settype/setkind_np */
     /****************************/
DEF_POSIX(int, mutexattr_settype, (pmarcel_mutexattr_t *attr, int kind),
{
  if (kind != MARCEL_MUTEX_ADAPTIVE_NP
      && kind != MARCEL_MUTEX_RECURSIVE_NP
      && kind != MARCEL_MUTEX_ERRORCHECK_NP
      && kind != MARCEL_MUTEX_TIMED_NP)
    return EINVAL;
  attr->__mutexkind = kind;
  return 0;
})
DEF_PTHREAD_WEAK(mutexattr_settype)
DEF___PTHREAD(mutexattr_settype)

DEF_ALIAS_POSIX(mutexattr_setkind_np)
DEF_STRONG_T(LOCAL_POSIX_NAME(mutexattr_setkind_np),
	     LOCAL_POSIX_NAME(mutexattr_settype),
	     LOCAL_POSIX_NAME(mutexattr_setkind_np))
#ifdef MA__PTHREAD_FUNCTIONS
  extern typeof(pthread_mutexattr_settype) pthread_mutexattr_setkind_np;
#endif
DEF_PTHREAD_WEAK(mutexattr_setkind_np)
DEF___PTHREAD(mutexattr_setkind_np)

     /****************************/
     /* mutex_gettype/getkind_np */
     /****************************/
DEF_POSIX(int, mutexattr_gettype, (const pmarcel_mutexattr_t *attr, int *kind),
{
  *kind = attr->__mutexkind;
  return 0;
})
DEF_PTHREAD_WEAK(mutexattr_gettype)
DEF___PTHREAD(mutexattr_gettype)

DEF_ALIAS_POSIX(mutexattr_getkind_np)
DEF_STRONG_T(LOCAL_POSIX_NAME(mutexattr_getkind_np),
	     LOCAL_POSIX_NAME(mutexattr_gettype),
	     LOCAL_POSIX_NAME(mutexattr_getkind_np))
#ifdef MA__PTHREAD_FUNCTIONS
  extern typeof(pthread_mutexattr_gettype) pthread_mutexattr_getkind_np;
#endif
DEF_PTHREAD_WEAK(mutexattr_getkind_np)
DEF___PTHREAD(mutexattr_getkind_np)


     /********************/
     /* mutex_getpshared */
     /********************/
DEF_POSIX(int, mutexattr_getpshared, (const pmarcel_mutexattr_t *attr,
				      int *pshared),
{
  *pshared = MARCEL_PROCESS_PRIVATE;
  return 0;
})
DEF_PTHREAD_WEAK(mutexattr_getpshared)
DEF___PTHREAD(mutexattr_getpshared)

     /********************/
     /* mutex_setpshared */
     /********************/
DEF_POSIX(int, mutexattr_setpshared, (pmarcel_mutexattr_t *attr, int pshared),
{
  if (pshared != MARCEL_PROCESS_PRIVATE && pshared != MARCEL_PROCESS_SHARED)
    return EINVAL;

  /* For now it is not possible to shared a conditional variable.  */
  if (pshared != MARCEL_PROCESS_PRIVATE)
    return ENOSYS;

  return 0;
})
DEF_PTHREAD_WEAK(mutexattr_setpshared)
DEF___PTHREAD(mutexattr_setpshared)


     /****************************************************************
      * ONCE-ONLY EXECUTION
      */
#include <limits.h>

static marcel_mutex_t once_masterlock = MARCEL_MUTEX_INITIALIZER;
static marcel_cond_t once_finished = MARCEL_COND_INITIALIZER;
static int fork_generation = 0;	/* Child process increments this after fork. */

enum { NEVER = 0, IN_PROGRESS = 1, DONE = 2 };

/* If a thread is canceled while calling the init_routine out of
   marcel once, this handler will reset the once_control variable
   to the NEVER state. */

static void marcel_once_cancelhandler(void *arg)
{
    marcel_once_t *once_control = arg;

    marcel_mutex_lock(&once_masterlock);
    *once_control = NEVER;
    marcel_mutex_unlock(&once_masterlock);
    marcel_cond_broadcast(&once_finished);
}

DEF_MARCEL_POSIX(int, once, (marcel_once_t * once_control, 
                             void (*init_routine)(void)))
{
  /* flag for doing the condition broadcast outside of mutex */
  int state_changed;

  /* Test without locking first for speed */
  if (*once_control == DONE) {
    READ_MEMORY_BARRIER();
    return 0;
  }
  /* Lock and test again */

  state_changed = 0;

  marcel_mutex_lock(&once_masterlock);

  /* If this object was left in an IN_PROGRESS state in a parent
     process (indicated by stale generation field), reset it to NEVER. */
  if ((*once_control & 3) == IN_PROGRESS && (*once_control & ~3) != fork_generation)
    *once_control = NEVER;

  /* If init_routine is being called from another routine, wait until
     it completes. */
  while ((*once_control & 3) == IN_PROGRESS) {
    marcel_cond_wait(&once_finished, &once_masterlock);
  }
  /* Here *once_control is stable and either NEVER or DONE. */
  if (*once_control == NEVER) {
    *once_control = IN_PROGRESS | fork_generation;
    marcel_mutex_unlock(&once_masterlock);
    marcel_cleanup_push(marcel_once_cancelhandler, once_control);
    init_routine();
    marcel_cleanup_pop(0);
    marcel_mutex_lock(&once_masterlock);
    WRITE_MEMORY_BARRIER();
    *once_control = DONE;
    state_changed = 1;
  }
  marcel_mutex_unlock(&once_masterlock);

  if (state_changed)
    marcel_cond_broadcast(&once_finished);

  return 0;
}
DEF_PTHREAD(once)
DEF___PTHREAD(once)

/*
 * Handle the state of the marcel_once mechanism across forks.  The
 * once_masterlock is acquired in the parent process prior to a fork to ensure
 * that no thread is in the critical region protected by the lock.  After the
 * fork, the lock is released. In the child, the lock and the condition
 * variable are simply reset.  The child also increments its generation
 * counter which lets marcel_once calls detect stale IN_PROGRESS states
 * and reset them back to NEVER.
 */

#ifdef MA__POSIX_BEHAVIOUR
#define __DEF___PTHREAD(name) \
  DEF_STRONG_T(__PTHREAD_NAME(name), \
               POSIX_NAME(name), __PTHREAD_NAME(name))

void __pmarcel_once_fork_prepare(void)
{
  pmarcel_mutex_lock(&once_masterlock);
}
__DEF___PTHREAD(once_fork_prepare)

void __pmarcel_once_fork_parent(void)
{
  pmarcel_mutex_unlock(&once_masterlock);
}
__DEF___PTHREAD(once_fork_parent)

void __pmarcel_once_fork_child(void)
{
  pmarcel_mutex_init(&once_masterlock, NULL);
  pmarcel_cond_init(&once_finished, NULL);
  if (fork_generation <= INT_MAX - 4)
    fork_generation += 4;	/* leave least significant two bits zero */
  else
    fork_generation = 0;
}
__DEF___PTHREAD(once_fork_child)

#endif
