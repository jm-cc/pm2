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

#ifndef _MARCEL_PTHREAD_H
#define _MARCEL_PTHREAD_H	1
#ifndef DARWIN_SYS
#include <features.h>
#endif
#include "tbx_compiler.h"

#warning toto
/* Function for handling threads.  */

/* Create a thread with given attributes ATTR (or default attributes
   if ATTR is NULL), and call function START_ROUTINE with given
   arguments ARG.  */
extern int marcel_create (marcel_t *__restrict __threadp,
			   __const marcel_attr_t *__restrict __attr,
			   void *(*__start_routine) (void *),
			   void *__restrict __arg) __THROW;

/* Obtain the identifier of the current thread.  */
extern marcel_t marcel_self (void) __THROW;

/* Compare two thread identifiers.  */
extern int marcel_equal (marcel_t __thread1, marcel_t __thread2) __THROW;

/* Terminate calling thread.  */
extern void marcel_exit (void *__retval)
     __THROW TBX_NORETURN;

/* Make calling thread wait for termination of the thread TH.  The
   exit status of the thread is stored in *THREAD_RETURN, if THREAD_RETURN
   is not NULL.  */
extern int marcel_join (marcel_t __th, void **__thread_return) __THROW;

/* Indicate that the thread TH is never to be joined with MARCEL_JOIN.
   The resources of TH will therefore be freed immediately when it
   terminates, instead of waiting for another thread to perform MARCEL_JOIN
   on it.  */
extern int marcel_detach (marcel_t __th) __THROW;


/* Functions for handling attributes.  */

/* Initialize thread attribute *ATTR with default attributes
   (detachstate is MARCEL_JOINABLE, scheduling policy is SCHED_OTHER,
    no user-provided stack).  */
extern int marcel_attr_init (marcel_attr_t *__attr) __THROW;

/* Destroy thread attribute *ATTR.  */
extern int marcel_attr_destroy (marcel_attr_t *__attr) __THROW;

/* Set the `detachstate' attribute in *ATTR according to DETACHSTATE.  */
extern int marcel_attr_setdetachstate (marcel_attr_t *__attr,
					int __detachstate) __THROW;

/* Return in *DETACHSTATE the `detachstate' attribute in *ATTR.  */
extern int marcel_attr_getdetachstate (__const marcel_attr_t *__attr,
					int *__detachstate) __THROW;

/* Set scheduling parameters (priority, etc) in *ATTR according to PARAM.  */
extern int marcel_attr_setschedparam (marcel_attr_t *__restrict __attr,
				       __const struct sched_param *__restrict
				       __param) __THROW;

/* Return in *PARAM the scheduling parameters of *ATTR.  */
extern int marcel_attr_getschedparam (__const marcel_attr_t *__restrict
				       __attr,
				       struct sched_param *__restrict __param)
     __THROW;

/* Set scheduling policy in *ATTR according to POLICY.  */
extern int marcel_attr_setschedpolicy (marcel_attr_t *__attr, int __policy)
     __THROW;

/* Return in *POLICY the scheduling policy of *ATTR.  */
extern int marcel_attr_getschedpolicy (__const marcel_attr_t *__restrict
					__attr, int *__restrict __policy)
     __THROW;

/* Set scheduling inheritance mode in *ATTR according to INHERIT.  */
extern int marcel_attr_setinheritsched (marcel_attr_t *__attr,
					 int __inherit) __THROW;

/* Return in *INHERIT the scheduling inheritance mode of *ATTR.  */
extern int marcel_attr_getinheritsched (__const marcel_attr_t *__restrict
					 __attr, int *__restrict __inherit)
     __THROW;

/* Set scheduling contention scope in *ATTR according to SCOPE.  */
extern int marcel_attr_setscope (marcel_attr_t *__attr, int __scope)
     __THROW;

/* Return in *SCOPE the scheduling contention scope of *ATTR.  */
extern int marcel_attr_getscope (__const marcel_attr_t *__restrict __attr,
				  int *__restrict __scope) __THROW;

#ifdef __USE_UNIX98
/* Set the size of the guard area at the bottom of the thread.  */
extern int marcel_attr_setguardsize (marcel_attr_t *__attr,
				      size_t __guardsize) __THROW;

/* Get the size of the guard area at the bottom of the thread.  */
extern int marcel_attr_getguardsize (__const marcel_attr_t *__restrict
				      __attr, size_t *__restrict __guardsize)
     __THROW;
#endif

/* Set the starting address of the stack of the thread to be created.
   Depending on whether the stack grows up or down the value must either
   be higher or lower than all the address in the memory block.  The
   minimal size of the block must be MARCEL_STACK_SIZE.  */
extern int marcel_attr_setstackaddr (marcel_attr_t *__attr,
				      void *__stackaddr) __THROW;

/* Return the previously set address for the stack.  */
extern int marcel_attr_getstackaddr (__const marcel_attr_t *__restrict
				      __attr, void **__restrict __stackaddr)
     __THROW;

#ifdef __USE_XOPEN2K
/* The following two interfaces are intended to replace the last two.  They
   require setting the address as well as the size since only setting the
   address will make the implementation on some architectures impossible.  */
extern int marcel_attr_setstack (marcel_attr_t *__attr, void *__stackaddr,
				  size_t __stacksize) __THROW;

/* Return the previously set address for the stack.  */
extern int marcel_attr_getstack (__const marcel_attr_t *__restrict __attr,
				  void **__restrict __stackaddr,
				  size_t *__restrict __stacksize) __THROW;
#endif

/* Add information about the minimum stack size needed for the thread
   to be started.  This size must never be less than MARCEL_STACK_SIZE
   and must also not exceed the system limits.  */
extern int marcel_attr_setstacksize (marcel_attr_t *__attr,
				      size_t __stacksize) __THROW;

/* Return the currently used minimal stack size.  */
extern int marcel_attr_getstacksize (__const marcel_attr_t *__restrict
				      __attr, size_t *__restrict __stacksize)
     __THROW;

#ifdef __USE_GNU
/* Get thread attributes corresponding to the already running thread TH.  */
extern int marcel_getattr_np (marcel_t __th, marcel_attr_t *__attr) __THROW;
#endif

/* Functions for scheduling control.  */

/* Set the scheduling parameters for TARGET_THREAD according to POLICY
   and *PARAM.  */
extern int marcel_setschedparam (marcel_t __target_thread, int __policy,
				  __const struct sched_param *__param)
     __THROW;

/* Return in *POLICY and *PARAM the scheduling parameters for TARGET_THREAD.  */
extern int marcel_getschedparam (marcel_t __target_thread,
				  int *__restrict __policy,
				  struct sched_param *__restrict __param)
     __THROW;

#ifdef __USE_UNIX98
/* Determine level of concurrency.  */
extern int marcel_getconcurrency (void) __THROW;

/* Set new concurrency level to LEVEL.  */
extern int marcel_setconcurrency (int __level) __THROW;
#endif

#ifdef __USE_GNU
/* Yield the processor to another thread or process.
   This function is similar to the POSIX `sched_yield' function but
   might be differently implemented in the case of a m-on-n thread
   implementation.  */
extern int marcel_yield (void) __THROW;
#endif

/* Functions for mutex handling.  */

/* Initialize MUTEX using attributes in *MUTEX_ATTR, or use the
   default values if later is NULL.  */
extern int marcel_mutex_init (marcel_mutex_t *__restrict __mutex,
			       __const marcel_mutexattr_t *__restrict
			       __mutex_attr) __THROW;

/* Destroy MUTEX.  */
extern int marcel_mutex_destroy (marcel_mutex_t *__mutex) __THROW;

/* Try to lock MUTEX.  */
extern int marcel_mutex_trylock (marcel_mutex_t *__mutex) __THROW;

/* Wait until lock for MUTEX becomes available and lock it.  */
extern int marcel_mutex_lock (marcel_mutex_t *__mutex) __THROW;

#ifdef __USE_XOPEN2K
/* Wait until lock becomes available, or specified time passes. */
extern int marcel_mutex_timedlock (marcel_mutex_t *__restrict __mutex,
				    __const struct timespec *__restrict
				    __abstime) __THROW;
#endif

/* Unlock MUTEX.  */
extern int marcel_mutex_unlock (marcel_mutex_t *__mutex) __THROW;


/* Functions for handling mutex attributes.  */

/* Initialize mutex attribute object ATTR with default attributes
   (kind is MARCEL_MUTEX_TIMED_NP).  */
extern int marcel_mutexattr_init (marcel_mutexattr_t *__attr) __THROW;

/* Destroy mutex attribute object ATTR.  */
extern int marcel_mutexattr_destroy (marcel_mutexattr_t *__attr) __THROW;

/* Get the process-shared flag of the mutex attribute ATTR.  */
extern int marcel_mutexattr_getpshared (__const marcel_mutexattr_t *
					 __restrict __attr,
					 int *__restrict __pshared) __THROW;

/* Set the process-shared flag of the mutex attribute ATTR.  */
extern int marcel_mutexattr_setpshared (marcel_mutexattr_t *__attr,
					 int __pshared) __THROW;

#ifdef __USE_UNIX98
/* Set the mutex kind attribute in *ATTR to KIND (either MARCEL_MUTEX_NORMAL,
   MARCEL_MUTEX_RECURSIVE, MARCEL_MUTEX_ERRORCHECK, or
   MARCEL_MUTEX_DEFAULT).  */
extern int marcel_mutexattr_settype (marcel_mutexattr_t *__attr, int __kind)
     __THROW;

/* Return in *KIND the mutex kind attribute in *ATTR.  */
extern int marcel_mutexattr_gettype (__const marcel_mutexattr_t *__restrict
				      __attr, int *__restrict __kind) __THROW;
#endif


/* Functions for handling conditional variables.  */

/* Initialize condition variable COND using attributes ATTR, or use
   the default values if later is NULL.  */
extern int marcel_cond_init (marcel_cond_t *__restrict __cond,
			      __const marcel_condattr_t *__restrict
			      __cond_attr) __THROW;

/* Destroy condition variable COND.  */
extern int marcel_cond_destroy (marcel_cond_t *__cond) __THROW;

/* Wake up one thread waiting for condition variable COND.  */
extern int marcel_cond_signal (marcel_cond_t *__cond) __THROW;

/* Wake up all threads waiting for condition variables COND.  */
extern int marcel_cond_broadcast (marcel_cond_t *__cond) __THROW;

/* Wait for condition variable COND to be signaled or broadcast.
   MUTEX is assumed to be locked before.  */
extern int marcel_cond_wait (marcel_cond_t *__restrict __cond,
			      marcel_mutex_t *__restrict __mutex) __THROW;

/* Wait for condition variable COND to be signaled or broadcast until
   ABSTIME.  MUTEX is assumed to be locked before.  ABSTIME is an
   absolute time specification; zero is the beginning of the epoch
   (00:00:00 GMT, January 1, 1970).  */
extern int marcel_cond_timedwait (marcel_cond_t *__restrict __cond,
				   marcel_mutex_t *__restrict __mutex,
				   __const struct timespec *__restrict
				   __abstime) __THROW;

/* Functions for handling condition variable attributes.  */

/* Initialize condition variable attribute ATTR.  */
extern int marcel_condattr_init (marcel_condattr_t *__attr) __THROW;

/* Destroy condition variable attribute ATTR.  */
extern int marcel_condattr_destroy (marcel_condattr_t *__attr) __THROW;

/* Get the process-shared flag of the condition variable attribute ATTR.  */
extern int marcel_condattr_getpshared (__const marcel_condattr_t *
					__restrict __attr,
					int *__restrict __pshared) __THROW;

/* Set the process-shared flag of the condition variable attribute ATTR.  */
extern int marcel_condattr_setpshared (marcel_condattr_t *__attr,
					int __pshared) __THROW;


#ifdef __USE_UNIX98
/* Functions for handling read-write locks.  */

/* Initialize read-write lock RWLOCK using attributes ATTR, or use
   the default values if later is NULL.  */
extern int marcel_rwlock_init (marcel_rwlock_t *__restrict __rwlock,
				__const marcel_rwlockattr_t *__restrict
				__attr) __THROW;

/* Destroy read-write lock RWLOCK.  */
extern int marcel_rwlock_destroy (marcel_rwlock_t *__rwlock) __THROW;

/* Acquire read lock for RWLOCK.  */
extern int marcel_rwlock_rdlock (marcel_rwlock_t *__rwlock) __THROW;

/* Try to acquire read lock for RWLOCK.  */
extern int marcel_rwlock_tryrdlock (marcel_rwlock_t *__rwlock) __THROW;

#ifdef __USE_XOPEN2K
/* Try to acquire read lock for RWLOCK or return after specfied time.  */
extern int marcel_rwlock_timedrdlock (marcel_rwlock_t *__restrict __rwlock,
				       __const struct timespec *__restrict
				       __abstime) __THROW;
#endif

/* Acquire write lock for RWLOCK.  */
extern int marcel_rwlock_wrlock (marcel_rwlock_t *__rwlock) __THROW;

/* Try to acquire write lock for RWLOCK.  */
extern int marcel_rwlock_trywrlock (marcel_rwlock_t *__rwlock) __THROW;

#ifdef __USE_XOPEN2K
/* Try to acquire write lock for RWLOCK or return after specfied time.  */
extern int marcel_rwlock_timedwrlock (marcel_rwlock_t *__restrict __rwlock,
				       __const struct timespec *__restrict
				       __abstime) __THROW;
#endif

/* Unlock RWLOCK.  */
extern int marcel_rwlock_unlock (marcel_rwlock_t *__rwlock) __THROW;


/* Functions for handling read-write lock attributes.  */

/* Initialize attribute object ATTR with default values.  */
extern int marcel_rwlockattr_init (marcel_rwlockattr_t *__attr) __THROW;

/* Destroy attribute object ATTR.  */
extern int marcel_rwlockattr_destroy (marcel_rwlockattr_t *__attr) __THROW;

/* Return current setting of process-shared attribute of ATTR in PSHARED.  */
extern int marcel_rwlockattr_getpshared (__const marcel_rwlockattr_t *
					  __restrict __attr,
					  int *__restrict __pshared) __THROW;

/* Set process-shared attribute of ATTR to PSHARED.  */
extern int marcel_rwlockattr_setpshared (marcel_rwlockattr_t *__attr,
					  int __pshared) __THROW;

/* Return current setting of reader/writer preference.  */
extern int marcel_rwlockattr_getkind_np (__const marcel_rwlockattr_t *__attr,
					  int *__pref) __THROW;

/* Set reader/write preference.  */
extern int marcel_rwlockattr_setkind_np (marcel_rwlockattr_t *__attr,
					  int __pref) __THROW;
#endif

#ifdef __USE_XOPEN2K
/* The IEEE Std. 1003.1j-2000 introduces functions to implement
   spinlocks.  */

/* Initialize the spinlock LOCK.  If PSHARED is nonzero the spinlock can
   be shared between different processes.  */
extern int marcel_spin_init (marcel_spinlock_t *__lock, int __pshared)
     __THROW;

/* Destroy the spinlock LOCK.  */
extern int marcel_spin_destroy (marcel_spinlock_t *__lock) __THROW;

/* Wait until spinlock LOCK is retrieved.  */
extern int marcel_spin_lock (marcel_spinlock_t *__lock) __THROW;

/* Try to lock spinlock LOCK.  */
extern int marcel_spin_trylock (marcel_spinlock_t *__lock) __THROW;

/* Release spinlock LOCK.  */
extern int marcel_spin_unlock (marcel_spinlock_t *__lock) __THROW;


/* Barriers are a also a new feature in 1003.1j-2000. */

extern int marcel_barrier_init (marcel_barrier_t *__restrict __barrier,
				 __const marcel_barrierattr_t *__restrict
				 __attr, unsigned int __count) __THROW;

extern int marcel_barrier_destroy (marcel_barrier_t *__barrier) __THROW;

extern int marcel_barrierattr_init (marcel_barrierattr_t *__attr) __THROW;

extern int marcel_barrierattr_destroy (marcel_barrierattr_t *__attr) __THROW;

extern int marcel_barrierattr_getpshared (__const marcel_barrierattr_t *
					   __restrict __attr,
					   int *__restrict __pshared) __THROW;

extern int marcel_barrierattr_setpshared (marcel_barrierattr_t *__attr,
					   int __pshared) __THROW;

extern int marcel_barrier_wait (marcel_barrier_t *__barrier) __THROW;
#endif


/* Functions for handling thread-specific data.  */

/* Create a key value identifying a location in the thread-specific
   data area.  Each thread maintains a distinct thread-specific data
   area.  DESTR_FUNCTION, if non-NULL, is called with the value
   associated to that key when the key is destroyed.
   DESTR_FUNCTION is not called if the value associated is NULL when
   the key is destroyed.  */
extern int marcel_key_create (marcel_key_t *__key,
			       void (*__destr_function) (void *)) __THROW;

/* Destroy KEY.  */
extern int marcel_key_delete (marcel_key_t __key) __THROW;

/* Store POINTER in the thread-specific data slot identified by KEY. */
extern int marcel_setspecific (marcel_key_t __key,
				__const void *__pointer) __THROW;

/* Return current value of the thread-specific data slot identified by KEY.  */
extern void *marcel_getspecific (marcel_key_t __key) __THROW;


/* Functions for handling initialization.  */


/* Functions for handling cancellation.  */

/* Set cancelability state of current thread to STATE, returning old
   state in *OLDSTATE if OLDSTATE is not NULL.  */
extern int marcel_setcancelstate (int __state, int *__oldstate) __THROW;

/* Set cancellation state of current thread to TYPE, returning the old
   type in *OLDTYPE if OLDTYPE is not NULL.  */
extern int marcel_setcanceltype (int __type, int *__oldtype) __THROW;

/* Cancel THREAD immediately or at the next possibility.  */
extern int marcel_cancel (marcel_t __cancelthread) __THROW;

/* Test for pending cancellation for the current thread and terminate
   the thread as per marcel_exit(MARCEL_CANCELED) if it has been
   cancelled.  */
extern void marcel_testcancel (void) __THROW;


#ifdef __USE_XOPEN2K
/* Get ID of CPU-time clock for thread THREAD_ID.  */
extern int marcel_getcpuclockid (marcel_t __thread_id,
				  clockid_t *__clock_id) __THROW;
#endif


/* Functions for handling signals.  */
#include <bits/marcel_sigthread.h>


/* Functions for handling process creation and process execution.  */

/* Install handlers to be called when a new process is created with FORK.
   The PREPARE handler is called in the parent process just before performing
   FORK. The PARENT handler is called in the parent process just after FORK.
   The CHILD handler is called in the child process.  Each of the three
   handlers can be NULL, meaning that no handler needs to be called at that
   point.
   MARCEL_ATFORK can be called several times, in which case the PREPARE
   handlers are called in LIFO order (last added with MARCEL_ATFORK,
   first called before FORK), and the PARENT and CHILD handlers are called
   in FIFO (first added, first called).  */

extern int marcel_atfork (void (*__prepare) (void),
			   void (*__parent) (void),
			   void (*__child) (void)) __THROW;

/* Terminate all threads in the program except the calling process.
   Should be called just before invoking one of the exec*() functions.  */

extern void marcel_kill_other_threads_np (void) __THROW;


#endif	/* marcel_pthread.h */
