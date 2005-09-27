m4_dnl -*- linux-c -*-
m4_include(scripts/marcel.m4)
dnl /***************************
dnl  * This is the original file
dnl  * =========================
dnl  ***************************/
/* This file has been autogenerated from m4___file__ */
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
#include <pthread.h>

#include "marcel_fastlock.h"
//#include "marcel_restart.h" // pour thread_self()

#ifdef MA__POSIX_FUNCTIONS_NAMES
static inline _pmarcel_descr thread_self() {
  return (_pmarcel_descr)marcel_self();
}
#endif

DEFINE_MODES

/****************************************************************
 * MUTEX
 */

     /**************/
     /* mutex_init */
     /**************/
PRINT_PTHREAD([[dnl
DEF_LIBPTHREAD(int, mutex_init,
	  (pthread_mutex_t * mutex, const pthread_mutexattr_t * mutex_attr),
	  (mutex, mutex_attr))
DEF___LIBPTHREAD(int, mutex_init,
	  (pthread_mutex_t * mutex, const pthread_mutexattr_t * mutex_attr),
	  (mutex, mutex_attr))
]])

REPLICATE_CODE([[dnl
int prefix_mutex_init (prefix_mutex_t *mutex, 
		       const prefix_mutexattr_t * mutexattr)
{
	mdebug("initializing mutex %p by %p\n", 
	       mutex, marcel_self());

        __marcel_init_lock(&mutex->__data.__lock);
	return 0;
}
]], [[MARCEL]])

REPLICATE_CODE([[dnl
static const struct prefix_mutexattr prefix_default_attr =
{
	/* Default is a normal mutex, not shared between processes.  */
	.__mutexkind = PREFIX_MUTEX_NORMAL
};

int prefix_mutex_init (prefix_mutex_t *mutex, 
		       const prefix_mutexattr_t * mutexattr)
{
	const struct prefix_mutexattr *imutexattr;

	mdebug("initializing mutex %p by %p\n", 
	       mutex, marcel_self());

#if MA__MODE == MA__MODE_LPT
	MA_BUG_ON (sizeof (lpt_mutex_t) > __SIZEOF_LPT_MUTEX_T);
#endif

	imutexattr = (const struct prefix_mutexattr *) mutexattr 
		?: &prefix_default_attr;

	/* Clear the whole variable.  */
	memset (mutex, '\0', sizeof(prefix_mutex_t));

	/* Copy the values from the attribute.  */
	mutex->__data.__kind = imutexattr->__mutexkind & ~0x80000000;
	/* Default values: mutex not used yet.  */
	// mutex->__count = 0;        already done by memset
	// mutex->__owner = 0;        already done by memset
	// mutex->__nusers = 0;       already done by memset
	// mutex->__spins = 0;        already done by memset
	if (tbx_unlikely(mutex->__data.__kind)) {
		RAISE(NOT_IMPLEMENTED);
	}
	
        __prefix_init_lock(&mutex->__data.__lock);
	return 0;
}
]], [[PMARCEL LPT]])

     /*****************/
     /* mutex_destroy */
     /*****************/
PRINT_PTHREAD([[dnl
DEF_LIBPTHREAD(int, mutex_destroy, (pthread_mutex_t * mutex), (mutex))
DEF___LIBPTHREAD(int, mutex_destroy, (pthread_mutex_t * mutex), (mutex))
]])

REPLICATE_CODE([[dnl
int prefix_mutex_destroy(prefix_mutex_t * mutex)
{
#if MA__MODE == MA__MODE_LPT || MA__MODE == MA__MODE_PMARCEL
  if (mutex->__data.__nusers != 0)
    return EBUSY;
#endif
  return 0;
}
]],[[MARCEL PMARCEL LPT]])


     /**************/
     /* mutex_lock */
     /**************/
PRINT_PTHREAD([[dnl
DEF_LIBPTHREAD(int, mutex_lock, (pthread_mutex_t * mutex), (mutex))
DEF___LIBPTHREAD(int, mutex_lock, (pthread_mutex_t * mutex), (mutex))
]])

REPLICATE_CODE([[dnl
int prefix_mutex_lock(prefix_mutex_t * mutex)
{
	struct marcel_task *id = MARCEL_SELF;
        __marcel_lock(&mutex->__data.__lock, id);
        return 0;
}
]], [[MARCEL]])

REPLICATE_CODE([[dnl
int prefix_mutex_lock(prefix_mutex_t * mutex)
{
	struct marcel_task *id = MARCEL_SELF;

#if MA__MODE == MA__MODE_LPT
	MA_BUG_ON (sizeof (mutex->__size) < sizeof (mutex->__data));
#endif
	switch (__builtin_expect (mutex->__data.__kind, PREFIX_MUTEX_TIMED_NP))
	{
		/* Recursive mutex.  */
	case PREFIX_MUTEX_RECURSIVE_NP:
		/* Check whether we already hold the mutex.  */
		if (mutex->__data.__owner == id)
		{
			/* Just bump the counter.  */
			if (__builtin_expect (mutex->__data.__count + 1 == 0, 0))
				/* Overflow of the counter.  */
				return EAGAIN;
			
			++mutex->__data.__count;
			
			return 0;
		}
		
		/* We have to get the mutex.  */
		__prefix_lock(&mutex->__data.__lock, id);
		
		mutex->__data.__count = 1;
		break;
		
		/* Error checking mutex.  */
	case PREFIX_MUTEX_ERRORCHECK_NP:
		/* Check whether we already hold the mutex.  */
		if (mutex->__data.__owner == id)
			return EDEADLK;
		
		/* FALLTHROUGH */
		
	default:
		/* Correct code cannot set any other type.  */
	case PREFIX_MUTEX_TIMED_NP:
	case PREFIX_MUTEX_ADAPTIVE_NP:
		/* Normal mutex.  */
		__prefix_lock(&mutex->__data.__lock, id);
		break;
		
	}
	
	/* Record the ownership.  */
	MA_BUG_ON (mutex->__data.__owner != 0);
	mutex->__data.__owner = id;
	++mutex->__data.__nusers;
	
	return 0;
}
]], [[PMARCEL LPT]])

     /*****************/
     /* mutex_trylock */
     /*****************/
PRINT_PTHREAD([[dnl
DEF_LIBPTHREAD(int, mutex_trylock, (pthread_mutex_t * mutex), (mutex))
DEF___LIBPTHREAD(int, mutex_trylock, (pthread_mutex_t * mutex), (mutex))
]])

REPLICATE_CODE([[dnl
int prefix_mutex_trylock(prefix_mutex_t * mutex)
{
        return __marcel_trylock(&mutex->__data.__lock);
}
]], [[MARCEL]])

REPLICATE_CODE([[dnl
int prefix_mutex_trylock(prefix_mutex_t * mutex)
{
	struct marcel_task *id;

	switch (__builtin_expect (mutex->__data.__kind, PREFIX_MUTEX_TIMED_NP))
	{
		/* Recursive mutex.  */
	case PREFIX_MUTEX_RECURSIVE_NP:
		id = MARCEL_SELF;
		/* Check whether we already hold the mutex.  */
		if (mutex->__data.__owner == id)
		{
			/* Just bump the counter.  */
			if (tbx_unlikely (mutex->__data.__count + 1 == 0))
				/* Overflow of the counter.  */
				return EAGAIN;
			
			++mutex->__data.__count;
			return 0;
		}
		
		if (__prefix_trylock(&mutex->__data.__lock) != 0)
		{
			/* Record the ownership.  */
			mutex->__data.__owner = id;
			mutex->__data.__count = 1;
			++mutex->__data.__nusers;
			return 0;
		}
		break;
		
	case PREFIX_MUTEX_ERRORCHECK_NP:
		/* Error checking mutex.  We do not check for deadlocks.  */
	default:
		/* Correct code cannot set any other type.  */
	case PREFIX_MUTEX_TIMED_NP:
	case PREFIX_MUTEX_ADAPTIVE_NP:
		/* Normal mutex.  */
		if (__prefix_trylock(&mutex->__data.__lock) != 0)
		{
			/* Record the ownership.  */
			mutex->__data.__owner = MARCEL_SELF;
			++mutex->__data.__nusers;
			
			return 0;
		}
	}
	
	return EBUSY;
}
]], [[PMARCEL LPT]])

#if 0
     /*******************/
     /* mutex_timedlock */
     /*******************/
DEF_POSIX(int, mutex_timedlock, (pmarcel_mutex_t *mutex,
				 const struct timespec *abstime),
		(mutex, abstime),
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
DEF_PTHREAD(int, mutex_timedlock, (pthread_mutex_t *mutex,
				 const struct timespec *abstime),
		(mutex, abstime))
DEF___PTHREAD(int, mutex_timedlock, (pthread_mutex_t *mutex,
				 const struct timespec *abstime),
		(mutex, abstime))
#endif

     /****************/
     /* mutex_unlock */
     /****************/
PRINT_PTHREAD([[dnl
DEF_LIBPTHREAD(int, mutex_unlock, (pthread_mutex_t * mutex), (mutex))
DEF___LIBPTHREAD(int, mutex_unlock, (pthread_mutex_t * mutex), (mutex))
]])

REPLICATE_CODE([[dnl
int prefix_mutex_unlock(prefix_mutex_t * mutex)
{
        return __marcel_unlock(&mutex->__data.__lock);
}
]], [[MARCEL]])

REPLICATE_CODE([[dnl
int prefix_mutex_unlock_usercnt(prefix_mutex_t * mutex, int decr)
{
	switch (__builtin_expect (mutex->__data.__kind, PREFIX_MUTEX_TIMED_NP))
	{
	case PREFIX_MUTEX_RECURSIVE_NP:
		/* Recursive mutex.  */
		if (mutex->__data.__owner != MARCEL_SELF)
			return EPERM;
		
		if (--mutex->__data.__count != 0)
			/* We still hold the mutex.  */
			return 0;
		break;
		
	case PREFIX_MUTEX_ERRORCHECK_NP:
		/* Error checking mutex.  */
		if (mutex->__data.__owner != MARCEL_SELF
		    //|| ! lll_mutex_islocked (mutex->__data.__lock)
			)
			return EPERM;
		break;
		
	default:
		/* Correct code cannot set any other type.  */
	case PREFIX_MUTEX_TIMED_NP:
	case PREFIX_MUTEX_ADAPTIVE_NP:
		/* Normal mutex.  Nothing special to do.  */
		break;
	}
	
	/* Always reset the owner field.  */
	mutex->__data.__owner = 0;
	if (decr)
		/* One less user.  */
		--mutex->__data.__nusers;
	
	/* Unlock.  */
	__prefix_unlock(&mutex->__data.__lock);
	
	return 0;
}
 
int prefix_mutex_unlock(prefix_mutex_t * mutex)
{
  return prefix_mutex_unlock_usercnt (mutex, 1);
}
]], [[PMARCEL LPT]])

     /******************/
     /* mutexattr_init */
     /******************/
PRINT_PTHREAD([[dnl
DEF_LIBPTHREAD(int, mutexattr_init, (pthread_mutexattr_t *attr), (attr))
DEF___LIBPTHREAD(int, mutexattr_init, (pthread_mutexattr_t *attr), (attr))
]])

REPLICATE_CODE([[dnl
int prefix_mutexattr_init(prefix_mutexattr_t * attr)
{
        return 0;
}
]], [[MARCEL]])

REPLICATE_CODE([[dnl
int prefix_mutexattr_init(prefix_mutexattr_t * attr)
{
#if MA__MODE == MA__MODE_LPT
	if (sizeof (struct prefix_mutexattr) != sizeof (prefix_mutexattr_t))
		memset (attr, '\0', sizeof (*attr));
#endif

	/* We use bit 31 to signal whether the mutex is going to be
	   process-shared or not.  By default it is zero, i.e., the
	   mutex is not process-shared.  */
	((struct prefix_mutexattr *) attr)->__mutexkind = PREFIX_MUTEX_NORMAL;
	
	return 0;
}
]], [[PMARCEL LPT]])

     /*********************/
     /* mutexattr_destroy */
     /*********************/
PRINT_PTHREAD([[dnl
DEF_LIBPTHREAD(int, mutexattr_destroy, (pthread_mutexattr_t *attr), (attr))
DEF___LIBPTHREAD(int, mutexattr_destroy, (pthread_mutexattr_t *attr), (attr))
]])

REPLICATE_CODE([[dnl
int prefix_mutexattr_destroy(prefix_mutexattr_t * attr)
{
        return 0;
}
]],[[MARCEL PMARCEL LPT]])


#if 0
     /****************************/
     /* mutex_settype/setkind_np */
     /****************************/
DEF_POSIX(int, mutexattr_settype, (pmarcel_mutexattr_t *attr, int kind),
		(attr, kind),
{
  if (kind != MARCEL_MUTEX_ADAPTIVE_NP
      && kind != MARCEL_MUTEX_RECURSIVE_NP
      && kind != MARCEL_MUTEX_ERRORCHECK_NP
      && kind != MARCEL_MUTEX_TIMED_NP)
    return EINVAL;
  attr->__mutexkind = kind;
  return 0;
})
DEF_PTHREAD_WEAK(int, mutexattr_settype, (pthread_mutexattr_t *attr, int kind),
		(attr, kind))
DEF___PTHREAD(int, mutexattr_settype, (pthread_mutexattr_t *attr, int kind),
		(attr, kind))

DEF_ALIAS_POSIX(int, mutexattr_setkind_np, (pthread_mutex_attr_t *attr,
			int kind), (attr, kind))
DEF_STRONG_T(int, LOCAL_POSIX_NAME(mutexattr_settype),
	     LOCAL_POSIX_NAME(mutexattr_setkind_np),
	     (pthread_mutex_attr_t *attr, int kind), (attr, kind))
#ifdef MA__PTHREAD_FUNCTIONS
  extern __typeof__(pthread_mutexattr_settype) pthread_mutexattr_setkind_np;
#endif
DEF_PTHREAD_WEAK(int, mutexattr_setkind_np, (pthread_mutex_attr_t *attr,
			int kind), (attr, kind))
DEF___PTHREAD(int, mutexattr_setkind_np, (pthread_mutex_attr_t *attr,
			int kind), (attr, kind))

     /****************************/
     /* mutex_gettype/getkind_np */
     /****************************/
DEF_POSIX(int, mutexattr_gettype, (const pmarcel_mutexattr_t *attr, int *kind),
		(attr, kind),
{
  *kind = attr->__mutexkind;
  return 0;
})
DEF_PTHREAD_WEAK(int, mutexattr_gettype, (const pthread_mutexattr_t *attr,
			int *kind), (attr, kind))
DEF___PTHREAD(int, mutexattr_gettype, (const pthread_mutexattr_t *attr,
			int *kind), (attr, kind))

DEF_ALIAS_POSIX(int, mutexattr_getkind_np, (cont pthread_mutexattr_t *attr,
			int *kind), (attr, kind))
DEF_STRONG_T(int, LOCAL_POSIX_NAME(mutexattr_gettype),
	     LOCAL_POSIX_NAME(mutexattr_getkind_np),
	     (cont pthread_mutexattr_t *attr,
			int *kind), (attr, kind))
#ifdef MA__PTHREAD_FUNCTIONS
  extern __typeof__(pthread_mutexattr_gettype) pthread_mutexattr_getkind_np;
#endif
DEF_PTHREAD_WEAK(int, mutexattr_getkind_np, (cont pthread_mutexattr_t *attr,
			int *kind), (attr, kind))
DEF___PTHREAD(int, mutexattr_getkind_np, (cont pthread_mutexattr_t *attr,
			int *kind), (attr, kind))


     /********************/
     /* mutex_getpshared */
     /********************/
/* certains pthread.h n'ont pas encore cette d�finition */
extern int pthread_mutexattr_getpshared (__const pthread_mutexattr_t *
                                         __restrict __attr,
                                         int *__restrict __pshared) __THROW;
DEF_POSIX(int, mutexattr_getpshared, (const pmarcel_mutexattr_t *attr,
				      int *pshared), (attr, pshared),
{
  *pshared = MARCEL_PROCESS_PRIVATE;
  return 0;
})
DEF_PTHREAD_WEAK(int, mutexattr_getpshared, (const pthread_mutexattr_t *attr,
				      int *pshared), (attr, pshared))
DEF___PTHREAD(int, mutexattr_getpshared, (const pthread_mutexattr_t *attr,
				      int *pshared), (attr, pshared))

     /********************/
     /* mutex_setpshared */
     /********************/
/* certains pthread.h n'ont pas encore cette d�finition */
extern int pthread_mutexattr_setpshared (pthread_mutexattr_t *__attr,
                                         int __pshared) __THROW;
DEF_POSIX(int, mutexattr_setpshared, (pmarcel_mutexattr_t *attr, int pshared),
		(attr, pshared),
{
  if (pshared != MARCEL_PROCESS_PRIVATE && pshared != MARCEL_PROCESS_SHARED)
    return EINVAL;

  /* For now it is not possible to shared a conditional variable.  */
  if (pshared != MARCEL_PROCESS_PRIVATE)
    return ENOSYS;

  return 0;
})
DEF_PTHREAD_WEAK(int, mutexattr_setpshared, (pthread_mutexattr_t *attr,
			int pshared), (attr, pshared))
DEF___PTHREAD(int, mutexattr_setpshared, (pthread_mutexattr_t *attr,
			int pshared), (attr, pshared))

#endif

     /****************************************************************
      * ONCE-ONLY EXECUTION
      */
#include <limits.h>

// XXX Vince, � corriger. On n'a pas lpt_mutex sur toutes les archis pour l'instant, donc j'ai mis un pmarcel_mutex pour que �a marchouille.
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

REPLICATE_CODE([[dnl
int prefix_once(prefix_once_t * once_control, 
	void (*init_routine)(void))
{
	/* flag for doing the condition broadcast outside of mutex */
	int state_changed;

	marcel_init_section(MA_INIT_MAIN_LWP);

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
]],[[MARCEL PMARCEL LPT]])

PRINT_PTHREAD([[dnl
DEF_LIBPTHREAD(int, once, (pthread_once_t *once_control,
	                   void (*init_routine)(void)),
	       (once_control, init_routine))
DEF___LIBPTHREAD(int, once, (pthread_once_t *once_control,
	                   void (*init_routine)(void)),
	       (once_control, init_routine))
]])


/*
 * Handle the state of the marcel_once mechanism across forks.  The
 * once_masterlock is acquired in the parent process prior to a fork to ensure
 * that no thread is in the critical region protected by the lock.  After the
 * fork, the lock is released. In the child, the lock and the condition
 * variable are simply reset.  The child also increments its generation
 * counter which lets marcel_once calls detect stale IN_PROGRESS states
 * and reset them back to NEVER.
 */

//#ifdef MA__POSIX_BEHAVIOUR
//#define __DEF___PTHREAD(name) \
//  DEF_STRONG_T(__PTHREAD_NAME(name), \
//               LOCAL_POSIX_NAME(name), __PTHREAD_NAME(name))


REPLICATE_CODE([[dnl
void prefix_once_fork_prepare(void)
{
  marcel_mutex_lock(&once_masterlock);
}

void prefix_once_fork_parent(void)
{
  marcel_mutex_unlock(&once_masterlock);
}

void prefix_once_fork_child(void)
{
  marcel_mutex_init(&once_masterlock, NULL);
  marcel_cond_init(&once_finished, NULL);
  if (fork_generation <= INT_MAX - 4)
    fork_generation += 4;	/* leave least significant two bits zero */
  else
    fork_generation = 0;
}
]],[[LPT]])

PRINT_PTHREAD([[dnl
//__DEF___PTHREAD(void, once_fork_prepare, (void), ())
//__DEF___PTHREAD(void, once_fork_parent, (void), ())
//__DEF___PTHREAD(void, once_fork_child, (void), ())
DEF___LIBPTHREAD(void, once_fork_prepare, (void), ())
DEF___LIBPTHREAD(void, once_fork_parent, (void), ())
DEF___LIBPTHREAD(void, once_fork_child, (void), ())
]])
//#endif
