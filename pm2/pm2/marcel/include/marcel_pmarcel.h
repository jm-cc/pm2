/* Copyright (C) 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _PMARCEL_H
#define _PMARCEL_H	1

#include <features.h>
#include <sched.h>
#include <time.h>

#define __need_sigset_t
#include <signal.h>
#undef __need_sigset_t
#include <bits/pmarceltypes.h>

#include "tbx_compiler.h"

/* Detach state.  */
enum
{
  PMARCEL_CREATE_JOINABLE,
#define PMARCEL_CREATE_JOINABLE	PMARCEL_CREATE_JOINABLE
  PMARCEL_CREATE_DETACHED
#define PMARCEL_CREATE_DETACHED	PMARCEL_CREATE_DETACHED
};


/* Mutex handling.  */

#define PMARCEL_MUTEX_INITIALIZER \
  { }

#define PMARCEL_RECURSIVE_MUTEX_INITIALIZER_NP \
  { .__data = { .__kind = PMARCEL_MUTEX_RECURSIVE_NP } }

#define PMARCEL_RWLOCK_INITIALIZER \
  { }

#define PMARCEL_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP \
  { .__data = { .__flags = PMARCEL_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP } }

enum
{
  PMARCEL_MUTEX_TIMED_NP,
  PMARCEL_MUTEX_RECURSIVE_NP,
  PMARCEL_MUTEX_ERRORCHECK_NP,
  PMARCEL_MUTEX_ADAPTIVE_NP
#ifdef __USE_UNIX98
  ,
  PMARCEL_MUTEX_NORMAL = PMARCEL_MUTEX_TIMED_NP,
  PMARCEL_MUTEX_RECURSIVE = PMARCEL_MUTEX_RECURSIVE_NP,
  PMARCEL_MUTEX_ERRORCHECK = PMARCEL_MUTEX_ERRORCHECK_NP,
  PMARCEL_MUTEX_DEFAULT = PMARCEL_MUTEX_NORMAL
#endif
#ifdef __USE_GNU
  /* For compatibility.  */
  , PMARCEL_MUTEX_FAST_NP = PMARCEL_MUTEX_TIMED_NP
#endif
};


/* Read-write lock types.  */
#ifdef __USE_UNIX98
enum
{
  PMARCEL_RWLOCK_PREFER_READER_NP,
  PMARCEL_RWLOCK_PREFER_WRITER_NP,
  PMARCEL_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP,
  PMARCEL_RWLOCK_DEFAULT_NP = PMARCEL_RWLOCK_PREFER_READER_NP
};
#endif  /* Unix98 */


/* Scheduler inheritance.  */
enum
{
  PMARCEL_INHERIT_SCHED,
#define PMARCEL_INHERIT_SCHED   PMARCEL_INHERIT_SCHED
  PMARCEL_EXPLICIT_SCHED
#define PMARCEL_EXPLICIT_SCHED  PMARCEL_EXPLICIT_SCHED
};


/* Scope handling.  */
enum
{
  PMARCEL_SCOPE_SYSTEM,
#define PMARCEL_SCOPE_SYSTEM    PMARCEL_SCOPE_SYSTEM
  PMARCEL_SCOPE_PROCESS
#define PMARCEL_SCOPE_PROCESS   PMARCEL_SCOPE_PROCESS
};


/* Process shared or private flag.  */
enum
{
  PMARCEL_PROCESS_PRIVATE,
#define PMARCEL_PROCESS_PRIVATE PMARCEL_PROCESS_PRIVATE
  PMARCEL_PROCESS_SHARED
#define PMARCEL_PROCESS_SHARED  PMARCEL_PROCESS_SHARED
};



/* Conditional variable handling.  */
#define PMARCEL_COND_INITIALIZER { }


/* Cleanup buffers */
struct _pmarcel_cleanup_buffer
{
  void (*__routine) (void *);             /* Function to call.  */
  void *__arg;                            /* Its argument.  */
  int __canceltype;                       /* Saved cancellation type. */
  struct _pmarcel_cleanup_buffer *__prev; /* Chaining of cleanup functions.  */
};

/* Cancellation */
enum
{
  PMARCEL_CANCEL_ENABLE,
#define PMARCEL_CANCEL_ENABLE   PMARCEL_CANCEL_ENABLE
  PMARCEL_CANCEL_DISABLE
#define PMARCEL_CANCEL_DISABLE  PMARCEL_CANCEL_DISABLE
};
enum
{
  PMARCEL_CANCEL_DEFERRED,
#define PMARCEL_CANCEL_DEFERRED	PMARCEL_CANCEL_DEFERRED
  PMARCEL_CANCEL_ASYNCHRONOUS
#define PMARCEL_CANCEL_ASYNCHRONOUS	PMARCEL_CANCEL_ASYNCHRONOUS
};
#define PMARCEL_CANCELED ((void *) -1)


/* Single execution handling.  */
#define PMARCEL_ONCE_INIT 0


#ifdef __USE_XOPEN2K
/* Value returned by 'pmarcel_barrier_wait' for one of the threads after
   the required number of threads have called this function.
   -1 is distinct from 0 and all errno constants */
# define PMARCEL_BARRIER_SERIAL_THREAD -1
#endif


__BEGIN_DECLS

/* Create a new thread, starting with execution of START-ROUTINE
   getting passed ARG.  Creation attributed come from ATTR.  The new
   handle is stored in *NEWTHREAD.  */
extern int pmarcel_create (pmarcel_t *__restrict __newthread,
			   __const pmarcel_attr_t *__restrict __attr,
			   void *(*__start_routine) (void *),
			   void *__restrict __arg) __THROW;

/* Terminate calling thread.  */
extern void pmarcel_exit (void *__retval)
     __THROW TBX_NORETURN;

/* Make calling thread wait for termination of the thread TH.  The
   exit status of the thread is stored in *THREAD_RETURN, if THREAD_RETURN
   is not NULL.  */
extern int pmarcel_join (pmarcel_t __th, void **__thread_return) __THROW;

#ifdef __USE_GNU
/* Check whether thread TH has terminated.  If yes return the status of
   the thread in *THREAD_RETURN, if THREAD_RETURN is not NULL.  */
extern int pmarcel_tryjoin_np (pmarcel_t __th, void **__thread_return) __THROW;

/* Make calling thread wait for termination of the thread TH, but only
   until TIMEOUT.  The exit status of the thread is stored in
   *THREAD_RETURN, if THREAD_RETURN is not NULL.  */
extern int pmarcel_timedjoin_np (pmarcel_t __th, void **__thread_return,
				 __const struct timespec *__abstime) __THROW;
#endif

/* Indicate that the thread TH is never to be joined with PMARCEL_JOIN.
   The resources of TH will therefore be freed immediately when it
   terminates, instead of waiting for another thread to perform PMARCEL_JOIN
   on it.  */
extern int pmarcel_detach (pmarcel_t __th) __THROW;


/* Obtain the identifier of the current thread.  */
extern pmarcel_t pmarcel_self (void) __THROW TBX_CONST;

/* Compare two thread identifiers.  */
extern int pmarcel_equal (pmarcel_t __thread1, pmarcel_t __thread2) __THROW;


/* Thread attribute handling.  */

/* Initialize thread attribute *ATTR with default attributes
   (detachstate is PMARCEL_JOINABLE, scheduling policy is SCHED_OTHER,
    no user-provided stack).  */
extern int pmarcel_attr_init (pmarcel_attr_t *__attr) __THROW;

/* Destroy thread attribute *ATTR.  */
extern int pmarcel_attr_destroy (pmarcel_attr_t *__attr) __THROW;

/* Get detach state attribute.  */
extern int pmarcel_attr_getdetachstate (__const pmarcel_attr_t *__attr,
					int *__detachstate) __THROW;

/* Set detach state attribute.  */
extern int pmarcel_attr_setdetachstate (pmarcel_attr_t *__attr,
					int __detachstate) __THROW;


/* Get the size of the guard area created for stack overflow protection.  */
extern int pmarcel_attr_getguardsize (__const pmarcel_attr_t *__attr,
				      size_t *__guardsize) __THROW;

/* Set the size of the guard area created for stack overflow protection.  */
extern int pmarcel_attr_setguardsize (pmarcel_attr_t *__attr,
				      size_t __guardsize) __THROW;


/* Return in *PARAM the scheduling parameters of *ATTR.  */
extern int pmarcel_attr_getschedparam (__const pmarcel_attr_t *__restrict
				       __attr,
				       struct sched_param *__restrict __param)
     __THROW;

/* Set scheduling parameters (priority, etc) in *ATTR according to PARAM.  */
extern int pmarcel_attr_setschedparam (pmarcel_attr_t *__restrict __attr,
				       __const struct sched_param *__restrict
				       __param) __THROW;

/* Return in *POLICY the scheduling policy of *ATTR.  */
extern int pmarcel_attr_getschedpolicy (__const pmarcel_attr_t *__restrict
					__attr, int *__restrict __policy)
     __THROW;

/* Set scheduling policy in *ATTR according to POLICY.  */
extern int pmarcel_attr_setschedpolicy (pmarcel_attr_t *__attr, int __policy)
     __THROW;

/* Return in *INHERIT the scheduling inheritance mode of *ATTR.  */
extern int pmarcel_attr_getinheritsched (__const pmarcel_attr_t *__restrict
					 __attr, int *__restrict __inherit)
     __THROW;

/* Set scheduling inheritance mode in *ATTR according to INHERIT.  */
extern int pmarcel_attr_setinheritsched (pmarcel_attr_t *__attr,
					 int __inherit) __THROW;


/* Return in *SCOPE the scheduling contention scope of *ATTR.  */
extern int pmarcel_attr_getscope (__const pmarcel_attr_t *__restrict __attr,
				  int *__restrict __scope) __THROW;

/* Set scheduling contention scope in *ATTR according to SCOPE.  */
extern int pmarcel_attr_setscope (pmarcel_attr_t *__attr, int __scope)
     __THROW;

/* Return the previously set address for the stack.  */
extern int pmarcel_attr_getstackaddr (__const pmarcel_attr_t *__restrict
				      __attr, void **__restrict __stackaddr)
     __THROW __attribute_deprecated__;

/* Set the starting address of the stack of the thread to be created.
   Depending on whether the stack grows up or down the value must either
   be higher or lower than all the address in the memory block.  The
   minimal size of the block must be PMARCEL_STACK_SIZE.  */
extern int pmarcel_attr_setstackaddr (pmarcel_attr_t *__attr,
				      void *__stackaddr)
     __THROW __attribute_deprecated__;

/* Return the currently used minimal stack size.  */
extern int pmarcel_attr_getstacksize (__const pmarcel_attr_t *__restrict
				      __attr, size_t *__restrict __stacksize)
     __THROW;

/* Add information about the minimum stack size needed for the thread
   to be started.  This size must never be less than PMARCEL_STACK_SIZE
   and must also not exceed the system limits.  */
extern int pmarcel_attr_setstacksize (pmarcel_attr_t *__attr,
				      size_t __stacksize) __THROW;

#ifdef __USE_XOPEN2K
/* Return the previously set address for the stack.  */
extern int pmarcel_attr_getstack (__const pmarcel_attr_t *__restrict __attr,
				  void **__restrict __stackaddr,
				  size_t *__restrict __stacksize) __THROW;

/* The following two interfaces are intended to replace the last two.  They
   require setting the address as well as the size since only setting the
   address will make the implementation on some architectures impossible.  */
extern int pmarcel_attr_setstack (pmarcel_attr_t *__attr, void *__stackaddr,
				  size_t __stacksize) __THROW;
#endif

#ifdef __USE_GNU
/* Get thread attributes corresponding to the already running thread TH.  */
extern int pmarcel_getattr_np (pmarcel_t __th, pmarcel_attr_t *__attr) __THROW;
#endif


/* Functions for scheduling control.  */

/* Set the scheduling parameters for TARGET_THREAD according to POLICY
   and *PARAM.  */
extern int pmarcel_setschedparam (pmarcel_t __target_thread, int __policy,
				  __const struct sched_param *__param)
     __THROW;

/* Return in *POLICY and *PARAM the scheduling parameters for TARGET_THREAD. */
extern int pmarcel_getschedparam (pmarcel_t __target_thread,
				  int *__restrict __policy,
				  struct sched_param *__restrict __param)
     __THROW;


#ifdef __USE_UNIX98
/* Determine level of concurrency.  */
extern int pmarcel_getconcurrency (void) __THROW;

/* Set new concurrency level to LEVEL.  */
extern int pmarcel_setconcurrency (int __level) __THROW;
#endif

#ifdef __USE_GNU
/* Yield the processor to another thread or process.
   This function is similar to the POSIX `sched_yield' function but
   might be differently implemented in the case of a m-on-n thread
   implementation.  */
extern int pmarcel_yield (void) __THROW;
#endif


/* Functions for handling initialization.  */

/* Guarantee that the initialization function INIT_ROUTINE will be called
   only once, even if pmarcel_once is executed several times with the
   same ONCE_CONTROL argument. ONCE_CONTROL must point to a static or
   extern variable initialized to PMARCEL_ONCE_INIT.  */
extern int pmarcel_once (pmarcel_once_t *__once_control,
			 void (*__init_routine) (void)) __THROW;


/* Functions for handling cancellation.  */

/* Set cancelability state of current thread to STATE, returning old
   state in *OLDSTATE if OLDSTATE is not NULL.  */
extern int pmarcel_setcancelstate (int __state, int *__oldstate) __THROW;

/* Set cancellation state of current thread to TYPE, returning the old
   type in *OLDTYPE if OLDTYPE is not NULL.  */
extern int pmarcel_setcanceltype (int __type, int *__oldtype) __THROW;

/* Cancel THREAD immediately or at the next possibility.  */
extern int pmarcel_cancel (pmarcel_t __th) __THROW;

/* Test for pending cancellation for the current thread and terminate
   the thread as per pmarcel_exit(PMARCEL_CANCELED) if it has been
   cancelled.  */
extern void pmarcel_testcancel (void) __THROW;


/* Install a cleanup handler: ROUTINE will be called with arguments ARG
   when the thread is cancelled or calls pmarcel_exit.  ROUTINE will also
   be called with arguments ARG when the matching pmarcel_cleanup_pop
   is executed with non-zero EXECUTE argument.

   pmarcel_cleanup_push and pmarcel_cleanup_pop are macros and must always
   be used in matching pairs at the same nesting level of braces.  */
#define pmarcel_cleanup_push(routine,arg) \
  { struct _pmarcel_cleanup_buffer _buffer;				      \
    _pmarcel_cleanup_push (&_buffer, (routine), (arg));

extern void _pmarcel_cleanup_push (struct _pmarcel_cleanup_buffer *__buffer,
				   void (*__routine) (void *), void *__arg)
     __THROW;

/* Remove a cleanup handler installed by the matching pmarcel_cleanup_push.
   If EXECUTE is non-zero, the handler function is called. */
#define pmarcel_cleanup_pop(execute) \
    _pmarcel_cleanup_pop (&_buffer, (execute)); }

extern void _pmarcel_cleanup_pop (struct _pmarcel_cleanup_buffer *__buffer,
				  int __execute) __THROW;

#ifdef __USE_GNU
/* Install a cleanup handler as pmarcel_cleanup_push does, but also
   saves the current cancellation type and sets it to deferred
   cancellation.  */
# define pmarcel_cleanup_push_defer_np(routine,arg) \
  { struct _pmarcel_cleanup_buffer _buffer;				      \
    _pmarcel_cleanup_push_defer (&_buffer, (routine), (arg));

extern void _pmarcel_cleanup_push_defer (struct _pmarcel_cleanup_buffer *__buffer,
					 void (*__routine) (void *),
					 void *__arg) __THROW;

/* Remove a cleanup handler as pmarcel_cleanup_pop does, but also
   restores the cancellation type that was in effect when the matching
   pmarcel_cleanup_push_defer was called.  */
# define pmarcel_cleanup_pop_restore_np(execute) \
  _pmarcel_cleanup_pop_restore (&_buffer, (execute)); }

extern void _pmarcel_cleanup_pop_restore (struct _pmarcel_cleanup_buffer *__buffer,
					  int __execute) __THROW;
#endif


/* Mutex handling.  */

/* Initialize a mutex.  */
extern int pmarcel_mutex_init (pmarcel_mutex_t *__mutex,
			       __const pmarcel_mutexattr_t *__mutexattr)
     __THROW;

/* Destroy a mutex.  */
extern int pmarcel_mutex_destroy (pmarcel_mutex_t *__mutex) __THROW;

/* Try locking a mutex.  */
extern int pmarcel_mutex_trylock (pmarcel_mutex_t *_mutex) __THROW;

/* Lock a mutex.  */
extern int pmarcel_mutex_lock (pmarcel_mutex_t *__mutex) __THROW;

#ifdef __USE_XOPEN2K
/* Wait until lock becomes available, or specified time passes. */
extern int pmarcel_mutex_timedlock (pmarcel_mutex_t *__restrict __mutex,
                                    __const struct timespec *__restrict
                                    __abstime) __THROW;
#endif

/* Unlock a mutex.  */
extern int pmarcel_mutex_unlock (pmarcel_mutex_t *__mutex) __THROW;


/* Functions for handling mutex attributes.  */

/* Initialize mutex attribute object ATTR with default attributes
   (kind is PMARCEL_MUTEX_TIMED_NP).  */
extern int pmarcel_mutexattr_init (pmarcel_mutexattr_t *__attr) __THROW;

/* Destroy mutex attribute object ATTR.  */
extern int pmarcel_mutexattr_destroy (pmarcel_mutexattr_t *__attr) __THROW;

/* Get the process-shared flag of the mutex attribute ATTR.  */
extern int pmarcel_mutexattr_getpshared (__const pmarcel_mutexattr_t *
					 __restrict __attr,
					 int *__restrict __pshared) __THROW;

/* Set the process-shared flag of the mutex attribute ATTR.  */
extern int pmarcel_mutexattr_setpshared (pmarcel_mutexattr_t *__attr,
					 int __pshared) __THROW;

#ifdef __USE_UNIX98
/* Return in *KIND the mutex kind attribute in *ATTR.  */
extern int pmarcel_mutexattr_gettype (__const pmarcel_mutexattr_t *__restrict
				      __attr, int *__restrict __kind) __THROW;

/* Set the mutex kind attribute in *ATTR to KIND (either PMARCEL_MUTEX_NORMAL,
   PMARCEL_MUTEX_RECURSIVE, PMARCEL_MUTEX_ERRORCHECK, or
   PMARCEL_MUTEX_DEFAULT).  */
extern int pmarcel_mutexattr_settype (pmarcel_mutexattr_t *__attr, int __kind)
     __THROW;
#endif


#ifdef __USE_UNIX98
/* Functions for handling read-write locks.  */

/* Initialize read-write lock RWLOCK using attributes ATTR, or use
   the default values if later is NULL.  */
extern int pmarcel_rwlock_init (pmarcel_rwlock_t *__restrict __rwlock,
				__const pmarcel_rwlockattr_t *__restrict
				__attr) __THROW;

/* Destroy read-write lock RWLOCK.  */
extern int pmarcel_rwlock_destroy (pmarcel_rwlock_t *__rwlock) __THROW;

/* Acquire read lock for RWLOCK.  */
extern int pmarcel_rwlock_rdlock (pmarcel_rwlock_t *__rwlock) __THROW;

/* Try to acquire read lock for RWLOCK.  */
extern int pmarcel_rwlock_tryrdlock (pmarcel_rwlock_t *__rwlock) __THROW;

# ifdef __USE_XOPEN2K
/* Try to acquire read lock for RWLOCK or return after specfied time.  */
extern int pmarcel_rwlock_timedrdlock (pmarcel_rwlock_t *__restrict __rwlock,
				       __const struct timespec *__restrict
				       __abstime) __THROW;
# endif

/* Acquire write lock for RWLOCK.  */
extern int pmarcel_rwlock_wrlock (pmarcel_rwlock_t *__rwlock) __THROW;

/* Try to acquire write lock for RWLOCK.  */
extern int pmarcel_rwlock_trywrlock (pmarcel_rwlock_t *__rwlock) __THROW;

# ifdef __USE_XOPEN2K
/* Try to acquire write lock for RWLOCK or return after specfied time.  */
extern int pmarcel_rwlock_timedwrlock (pmarcel_rwlock_t *__restrict __rwlock,
				       __const struct timespec *__restrict
				       __abstime) __THROW;
# endif

/* Unlock RWLOCK.  */
extern int pmarcel_rwlock_unlock (pmarcel_rwlock_t *__rwlock) __THROW;


/* Functions for handling read-write lock attributes.  */

/* Initialize attribute object ATTR with default values.  */
extern int pmarcel_rwlockattr_init (pmarcel_rwlockattr_t *__attr) __THROW;

/* Destroy attribute object ATTR.  */
extern int pmarcel_rwlockattr_destroy (pmarcel_rwlockattr_t *__attr) __THROW;

/* Return current setting of process-shared attribute of ATTR in PSHARED.  */
extern int pmarcel_rwlockattr_getpshared (__const pmarcel_rwlockattr_t *
					  __restrict __attr,
					  int *__restrict __pshared) __THROW;

/* Set process-shared attribute of ATTR to PSHARED.  */
extern int pmarcel_rwlockattr_setpshared (pmarcel_rwlockattr_t *__attr,
					  int __pshared) __THROW;

/* Return current setting of reader/writer preference.  */
extern int pmarcel_rwlockattr_getkind_np (__const pmarcel_rwlockattr_t *
					  __restrict __attr,
					  int *__restrict __pref) __THROW;

/* Set reader/write preference.  */
extern int pmarcel_rwlockattr_setkind_np (pmarcel_rwlockattr_t *__attr,
					  int __pref) __THROW;
#endif


/* Functions for handling conditional variables.  */

/* Initialize condition variable COND using attributes ATTR, or use
   the default values if later is NULL.  */
extern int pmarcel_cond_init (pmarcel_cond_t *__restrict __cond,
			      __const pmarcel_condattr_t *__restrict
			      __cond_attr) __THROW;

/* Destroy condition variable COND.  */
extern int pmarcel_cond_destroy (pmarcel_cond_t *__cond) __THROW;

/* Wake up one thread waiting for condition variable COND.  */
extern int pmarcel_cond_signal (pmarcel_cond_t *__cond) __THROW;

/* Wake up all threads waiting for condition variables COND.  */
extern int pmarcel_cond_broadcast (pmarcel_cond_t *__cond) __THROW;

/* Wait for condition variable COND to be signaled or broadcast.
   MUTEX is assumed to be locked before.  */
extern int pmarcel_cond_wait (pmarcel_cond_t *__restrict __cond,
			      pmarcel_mutex_t *__restrict __mutex) __THROW;

/* Wait for condition variable COND to be signaled or broadcast until
   ABSTIME.  MUTEX is assumed to be locked before.  ABSTIME is an
   absolute time specification; zero is the beginning of the epoch
   (00:00:00 GMT, January 1, 1970).  */
extern int pmarcel_cond_timedwait (pmarcel_cond_t *__restrict __cond,
				   pmarcel_mutex_t *__restrict __mutex,
				   __const struct timespec *__restrict
				   __abstime) __THROW;

/* Functions for handling condition variable attributes.  */

/* Initialize condition variable attribute ATTR.  */
extern int pmarcel_condattr_init (pmarcel_condattr_t *__attr) __THROW;

/* Destroy condition variable attribute ATTR.  */
extern int pmarcel_condattr_destroy (pmarcel_condattr_t *__attr) __THROW;

/* Get the process-shared flag of the condition variable attribute ATTR.  */
extern int pmarcel_condattr_getpshared (__const pmarcel_condattr_t *
                                        __restrict __attr,
                                        int *__restrict __pshared) __THROW;

/* Set the process-shared flag of the condition variable attribute ATTR.  */
extern int pmarcel_condattr_setpshared (pmarcel_condattr_t *__attr,
                                        int __pshared) __THROW;



#ifdef __USE_XOPEN2K
/* Functions to handle spinlocks.  */

/* Initialize the spinlock LOCK.  If PSHARED is nonzero the spinlock can
   be shared between different processes.  */
extern int pmarcel_spin_init (pmarcel_spinlock_t *__lock, int __pshared)
     __THROW;

/* Destroy the spinlock LOCK.  */
extern int pmarcel_spin_destroy (pmarcel_spinlock_t *__lock) __THROW;

/* Wait until spinlock LOCK is retrieved.  */
extern int pmarcel_spin_lock (pmarcel_spinlock_t *__lock) __THROW;

/* Try to lock spinlock LOCK.  */
extern int pmarcel_spin_trylock (pmarcel_spinlock_t *__lock) __THROW;

/* Release spinlock LOCK.  */
extern int pmarcel_spin_unlock (pmarcel_spinlock_t *__lock) __THROW;


/* Functions to handle barriers.  */

/* Initialize BARRIER with the attributes in ATTR.  The barrier is
   opened when COUNT waiters arrived.  */
extern int pmarcel_barrier_init (pmarcel_barrier_t *__restrict __barrier,
				 __const pmarcel_barrierattr_t *__restrict
				 __attr, unsigned int __count) __THROW;

/* Destroy a previously dynamically initialized barrier BARRIER.  */
extern int pmarcel_barrier_destroy (pmarcel_barrier_t *__barrier) __THROW;

/* Wait on barrier BARRIER.  */
extern int pmarcel_barrier_wait (pmarcel_barrier_t *__barrier) __THROW;


/* Initialize barrier attribute ATTR.  */
extern int pmarcel_barrierattr_init (pmarcel_barrierattr_t *__attr) __THROW;

/* Destroy previously dynamically initialized barrier attribute ATTR.  */
extern int pmarcel_barrierattr_destroy (pmarcel_barrierattr_t *__attr) __THROW;

/* Get the process-shared flag of the barrier attribute ATTR.  */
extern int pmarcel_barrierattr_getpshared (__const pmarcel_barrierattr_t *
					   __restrict __attr,
					   int *__restrict __pshared) __THROW;

/* Set the process-shared flag of the barrier attribute ATTR.  */
extern int pmarcel_barrierattr_setpshared (pmarcel_barrierattr_t *__attr,
                                           int __pshared) __THROW;
#endif


/* Functions for handling thread-specific data.  */

/* Create a key value identifying a location in the thread-specific
   data area.  Each thread maintains a distinct thread-specific data
   area.  DESTR_FUNCTION, if non-NULL, is called with the value
   associated to that key when the key is destroyed.
   DESTR_FUNCTION is not called if the value associated is NULL when
   the key is destroyed.  */
extern int pmarcel_key_create (pmarcel_key_t *__key,
			       void (*__destr_function) (void *)) __THROW;

/* Destroy KEY.  */
extern int pmarcel_key_delete (pmarcel_key_t __key) __THROW;

/* Return current value of the thread-specific data slot identified by KEY.  */
extern void *pmarcel_getspecific (pmarcel_key_t __key) __THROW;

/* Store POINTER in the thread-specific data slot identified by KEY. */
extern int pmarcel_setspecific (pmarcel_key_t __key,
				__const void *__pointer) __THROW;


/* Install a cleanup handler: ROUTINE will be called with arguments ARG
   when the thread is cancelled or calls pmarcel_exit.  ROUTINE will also
   be called with arguments ARG when the matching pmarcel_cleanup_pop
   is executed with non-zero EXECUTE argument.
   pmarcel_cleanup_push and pmarcel_cleanup_pop are macros and must always
   be used in matching pairs at the same nesting level of braces. */
#define pmarcel_cleanup_push(routine,arg) \
  { struct _pmarcel_cleanup_buffer _buffer;				      \
    _pmarcel_cleanup_push (&_buffer, (routine), (arg));

extern void _pmarcel_cleanup_push (struct _pmarcel_cleanup_buffer *__buffer,
				   void (*__routine) (void *),
				   void *__arg) __THROW;

/* Remove a cleanup handler installed by the matching pmarcel_cleanup_push.
   If EXECUTE is non-zero, the handler function is called. */
#define pmarcel_cleanup_pop(execute) \
    _pmarcel_cleanup_pop (&_buffer, (execute)); }

extern void _pmarcel_cleanup_pop (struct _pmarcel_cleanup_buffer *__buffer,
				  int __execute) __THROW;


#ifdef __USE_GNU
/* Install a cleanup handler as pmarcel_cleanup_push does, but also
   saves the current cancellation type and set it to deferred cancellation.  */
# define pmarcel_cleanup_push_defer_np(routine,arg) \
  { struct _pmarcel_cleanup_buffer _buffer;				      \
    _pmarcel_cleanup_push_defer (&_buffer, (routine), (arg));

extern void _pmarcel_cleanup_push_defer (struct _pmarcel_cleanup_buffer *__buffer,
					 void (*__routine) (void *),
					 void *__arg) __THROW;

/* Remove a cleanup handler as pmarcel_cleanup_pop does, but also
   restores the cancellation type that was in effect when the matching
   pmarcel_cleanup_push_defer was called.  */
# define pmarcel_cleanup_pop_restore_np(execute) \
  _pmarcel_cleanup_pop_restore (&_buffer, (execute)); }

extern void _pmarcel_cleanup_pop_restore (struct _pmarcel_cleanup_buffer *__buffer,
					  int __execute) __THROW;
#endif


#ifdef __USE_XOPEN2K
/* Get ID of CPU-time clock for thread THREAD_ID.  */
extern int pmarcel_getcpuclockid (pmarcel_t __thread_id,
				  clockid_t *__clock_id) __THROW;
#endif


/* Install handlers to be called when a new process is created with FORK.
   The PREPARE handler is called in the parent process just before performing
   FORK. The PARENT handler is called in the parent process just after FORK.
   The CHILD handler is called in the child process.  Each of the three
   handlers can be NULL, meaning that no handler needs to be called at that
   point.
   PMARCEL_ATFORK can be called several times, in which case the PREPARE
   handlers are called in LIFO order (last added with PMARCEL_ATFORK,
   first called before FORK), and the PARENT and CHILD handlers are called
   in FIFO (first added, first called).  */

extern int pmarcel_atfork (void (*__prepare) (void),
			   void (*__parent) (void),
			   void (*__child) (void)) __THROW;

__END_DECLS

#endif	/* pmarcel.h */
