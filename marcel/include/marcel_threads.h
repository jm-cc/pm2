/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __MARCEL_THREADS_H__
#define __MARCEL_THREADS_H__


#include <unistd.h>
#include <time.h>
#include "sys/marcel_flags.h"
#include "marcel_alias.h"
#include "marcel_attr.h"
#include "marcel_types.h"
#ifdef MA__IFACE_PMARCEL
#include "marcel_pmarcel.h"
#endif


/** Public macros **/
/* #define STANDARD_MAIN */
#define MARCEL_ONCE_INIT 0

#define MAX_ATEXIT_FUNCS	5

#define MARCEL_THREAD_ISALIVE(thread) \
   (((thread)->state != MA_TASK_DEAD)||(!(thread)->detached))


/** pmarcel version of thread id comparison (as a macro).
 */
#define pmarcel_equal(t1,t2) ((t1) == (t2))


#ifdef STANDARD_MAIN
/** Structure of main thread when STANDARD_MAIN is defined.
 */
extern marcel_task_t __main_thread_struct;
#  define __main_thread  (&__main_thread_struct)
#else
/** Structure of main thread.
 */
extern marcel_task_t *__main_thread;
#endif


/** Public data types **/
/* Thread creation attribute constants.

   Note: #defines of the same name than enum entries are there for
   consistency with /usr/include/pthread.h and to enable the use of
   #ifdef on them
 */

/** Relationship with parent thread.
 */
enum
{
  MARCEL_CREATE_JOINABLE,
#define MARCEL_CREATE_JOINABLE	MARCEL_CREATE_JOINABLE
  MARCEL_CREATE_DETACHED
#define MARCEL_CREATE_DETACHED	MARCEL_CREATE_DETACHED
};

#define MARCEL_CREATE_DETACHSTATE_INVALID -1

/** Scheduling policy and parameters inheritance.
 */
enum
{
  MARCEL_INHERIT_SCHED,
#define MARCEL_INHERIT_SCHED	MARCEL_INHERIT_SCHED
  MARCEL_EXPLICIT_SCHED
#define MARCEL_EXPLICIT_SCHED	MARCEL_EXPLICIT_SCHED
};

#define MARCEL_INHERITSCHED_INVALID -1

/** Scheduling contention scope.
 */
enum
{
  MARCEL_SCOPE_SYSTEM,
#define MARCEL_SCOPE_SYSTEM	MARCEL_SCOPE_SYSTEM
  MARCEL_SCOPE_PROCESS
#define MARCEL_SCOPE_PROCESS	MARCEL_SCOPE_PROCESS
};

#define MARCEL_SCOPE_INVALID -1

/** Cancellation capability.
 */
enum
{
  MARCEL_CANCEL_ENABLE,
#define MARCEL_CANCEL_ENABLE	MARCEL_CANCEL_ENABLE
  MARCEL_CANCEL_DISABLE
#define MARCEL_CANCEL_DISABLE	MARCEL_CANCEL_DISABLE
};

/** Cancellation mode.
 */
enum
{
  MARCEL_CANCEL_DEFERRED,
#define MARCEL_CANCEL_DEFERRED	MARCEL_CANCEL_DEFERRED
  MARCEL_CANCEL_ASYNCHRONOUS
#define MARCEL_CANCEL_ASYNCHRONOUS	MARCEL_CANCEL_ASYNCHRONOUS
};

/** Return value of canceled threads.
 */
#define MARCEL_CANCELED ((void *) -1)

/** Thread handler generic type.
 */
typedef any_t (*marcel_func_t)(any_t);

#ifdef MARCEL_CLEANUP_ENABLED
/** Clean-up handler type.
 */
typedef void (*cleanup_func_t)(any_t);
#endif /* MARCEL_CLEANUP_ENABLED */

#ifdef MARCEL_ATEXIT_ENABLED
/** At-exit clean-up handler.
 */
typedef void (*marcel_atexit_func_t)(any_t);
#endif /* MARCEL_ATEXIT_ENABLED */

#ifdef MARCEL_POSTEXIT_ENABLED
/** Post exit clean-up handler.
 */
typedef void (*marcel_postexit_func_t)(any_t);
#endif /* MARCEL_POSTEXIT_ENABLED */

#ifdef MARCEL_MIGRATION_ENABLED

/* =========== migration =========== */
/** Sending-side migration handler.
 */
typedef void (*transfert_func_t)(marcel_t t, unsigned long depl, unsigned long blksize, void *arg);

/** Receiving-side migration handler.
 */
typedef void (*post_migration_func_t)(void *arg);

#endif /* MARCEL_MIGRATION_ENABLED */


/** Public data structures **/
#ifdef MARCEL_CLEANUP_ENABLED
/** Linked list of buffers for thread clean-up handlers.
 */
/* NOTE: ABI-compatible with NPTL, see its __pthread_cleanup_buffer */
struct _marcel_cleanup_buffer
{
  void (*__routine) (void *);		  /* Function to call.  */
  void *__arg;				  /* Its argument.  */
  int __canceltype;			  /* Saved cancellation type. */
  struct _marcel_cleanup_buffer *__prev; /* Chaining of cleanup functions.  */
};
#endif /* MARCEL_CLEANUP_ENABLED */


/** Public functions **/
/** Create a thread.
 */
DEC_MARCEL_POSIX(int,create,(marcel_t * __restrict pid,
		  __const marcel_attr_t * __restrict attr, 
		  marcel_func_t func, any_t __restrict arg) __THROW);

/** Create a thread but dont schedule it.
 */
int marcel_create_dontsched(marcel_t * __restrict pid,
		  __const marcel_attr_t * __restrict attr, 
		  marcel_func_t func, any_t __restrict arg) __THROW;


/** Posix join.
 */
DEC_MARCEL_POSIX(int, join, (marcel_t pid, any_t *status) __THROW);

/** Posix exit.
 */
DEC_MARCEL_POSIX(void TBX_NORETURN, exit, (any_t val) __THROW);

/** Posix detach.
 */
DEC_MARCEL_POSIX(int, detach, (marcel_t pid) __THROW);

/** Posix cancel.
 */
#ifdef MARCEL_DEVIATION_ENABLED
DEC_MARCEL_POSIX(int, cancel, (marcel_t pid) __THROW);
#endif /* MARCEL_DEVIATION_ENABLED */

static __tbx_inline__ int marcel_equal(marcel_t pid1, marcel_t pid2);

/** Posix setconcurrency.
 */
DEC_MARCEL_POSIX(int, setconcurrency, (int newlevel) __THROW);

/** Posix getconcurrency.
 */
DEC_MARCEL_POSIX(int, getconcurrency, (void) __THROW);

#ifdef MARCEL_DEVIATION_ENABLED
/** Posix setcancelstate.
 */
DEC_MARCEL_POSIX(int, setcancelstate,(int state, int *oldstate) __THROW);

/** Posix setcanceltype.
 */
DEC_MARCEL_POSIX(int, setcanceltype,(int type, int *oldtype) __THROW);

/** Posix testcancel.
 */
DEC_MARCEL_POSIX(void, testcancel,(void) __THROW);
#endif /* MARCEL_DEVIATION_ENABLED */

/** GNU setaffinity_np.
 */
DEC_MARCEL_POSIX(int, setaffinity_np, (marcel_t pid, size_t cpusetsize, const pmarcel_cpu_set_t *cpuset));

/** GNU getaffinity_np.
 */
DEC_MARCEL_POSIX(int, getaffinity_np, (marcel_t pid, size_t cpusetsize, pmarcel_cpu_set_t *cpuset));

/** pmarcel specific asynchronous cancel enable/disable.

    Notes:
    - why two specific functions for the pmarcel case?
    - why the fastcall convention?
    - why the "__" prefix? Are those function for internal use only?
    - why not __ma_ instead of __pmarcel_ prefix?

    - BECAUSE it's stolen from the libpthread source.
 */
int fastcall __pmarcel_enable_asynccancel (void);
void fastcall __pmarcel_disable_asynccancel(int old);

#if !defined(MA__IFACE_PMARCEL) || !defined(MARCEL_DEVIATION_ENABLED)
#  define __pmarcel_enable_asynccancel() 0
#  define __pmarcel_disable_asynccancel(old) (void)(old)
#endif

/** Posix setschedprio.
 */
DEC_MARCEL_POSIX(int, setschedprio,(marcel_t thread,int prio) __THROW);

#if 0
/** Posix-only setschedprio.
    Notes:
    Some systems may not have the definition.
 */
int pthread_setschedprio(pthread_t thread, int prio) __THROW;
#endif

/** Set the scheduling params of the specified thread.
 */
DEC_MARCEL_POSIX(int, setschedparam,(marcel_t thread, int policy,
                                     __const struct marcel_sched_param *__restrict param) __THROW);

/** Get the scheduling params of the specified thread.
 */
DEC_MARCEL_POSIX(int, getschedparam,(marcel_t thread, int *__restrict policy,
                                     struct marcel_sched_param *__restrict param) __THROW);

#ifdef _POSIX_CPUTIME
#  if _POSIX_CPUTIME >= 0
DEC_POSIX(int, getcpuclockid, (pmarcel_t thread, clockid_t *clock_id) __THROW);
#  endif
#endif

/** Prepare a set of threads for a subsequent migration.
 */
void marcel_freeze(marcel_t *pids, int nb);

/** Resume a set of threads after a migration.
 */
void marcel_unfreeze(marcel_t *pids, int nb);

#ifdef MARCEL_USERSPACE_ENABLED
/** Get the ptr to the user space area reserved for the thread.
 */
void marcel_getuserspace(marcel_t __restrict pid,
		void * __restrict * __restrict user_space);

/** Unlock a thread created with some user space area reserved and
    waiting for this area to be filled.
 */
void marcel_run(marcel_t __restrict pid, any_t __restrict arg);
#endif /* MARCEL_USERSPACE_ENABLED */

/* ========== callbacks ============ */

#ifdef MARCEL_POSTEXIT_ENABLED
/** Setup a post-exit clean-up handler
 * Called after complete termination of the thread, i.e. the stack is not used any more.
 */
void marcel_postexit(marcel_postexit_func_t, any_t);
#endif /* MARCEL_POSTEXIT_ENABLED */

#ifdef MARCEL_ATEXIT_ENABLED
/** Setup a at-exit clean-up handler
 * Called in the context of the thread just before its complete termination.
 */
void marcel_atexit(marcel_atexit_func_t, any_t);
#endif /* MARCEL_ATEXIT_ENABLED */

/*
  - why a regular prototype + a macro for the marcel_thread_preemption_enable series of functions?
  - BECAUSE that provides a nice prototype for the doxygen documentation.
  - why both "__marcel_some" prefix and "marcel_some" prefix where the "marcel_some" prefix does not seem to add anything?
  - probably BECAUSE of historical reasons that no longer apply.
 */

/** Enable automatic preemption of threads.
 */
void marcel_thread_preemption_enable(void);

/** Disable automatic preemption of threads.
 */
void marcel_thread_preemption_disable(void);

/** Return whether automatic preemption of threads is currently
    disabled (1) or enabled (0).
 */
int marcel_thread_is_preemption_disabled(void);

static __tbx_inline__ void marcel_some_thread_preemption_enable(marcel_t t);
static __tbx_inline__ void marcel_some_thread_preemption_disable(marcel_t t);
static __tbx_inline__ int marcel_some_thread_is_preemption_disabled(marcel_t t);

#define	marcel_thread_preemption_enable() marcel_some_thread_preemption_enable(MARCEL_SELF)
#define	marcel_thread_preemption_disable() marcel_some_thread_preemption_disable(MARCEL_SELF)
#define	marcel_thread_is_preemption_disabled() marcel_some_thread_is_preemption_disabled(MARCEL_SELF)

#ifdef MA__DEBUG

extern void marcel_print_thread(marcel_t pid);
extern void marcel_print_jmp_buf(char *name, jmp_buf buf);

#endif /* MA__DEBUG */



#ifdef MARCEL_CLEANUP_ENABLED
#undef NAME_PREFIX
#define NAME_PREFIX _

/** Posix cleanup_push.
 */
DEC_MARCEL_POSIX(void, cleanup_push,(struct _marcel_cleanup_buffer * __restrict __buffer,
				     cleanup_func_t func, any_t __restrict arg) __THROW);
/** Posix cleanup_pop.
 */
DEC_MARCEL_POSIX(void, cleanup_pop,(struct _marcel_cleanup_buffer *__buffer,
				    tbx_bool_t execute) __THROW);
#undef NAME_PREFIX
#define NAME_PREFIX
#endif /* MARCEL_CLEANUP_ENABLED */

#ifdef MARCEL_SUSPEND_ENABLED
/** Temporarily suspend execution of the given thread.
    - why keep this feature that is used only in a single example and uses a
    GETMEM entry?

    - Hum... It have to admit it was only implemented for test
      purposes (to check if the implementation of semaphores is
      compliant with the behavior of 'marcel_deviate'). These
      functions, along with the corresponding GETMEM entries, can
      definitely be deleted.

    Note: suspending a thread only suspends the current execution of the
    thread. A signal can still be handled by the thread for instance.
 */
void marcel_suspend(marcel_t pid);

/** Resume a formerly suspended thread.
 */
void marcel_resume(marcel_t pid);
#endif /* MARCEL_SUSPEND_ENABLED */

#ifdef MARCEL_MIGRATION_ENABLED

/** Disable preemptive migration for the given thread.
 */
MARCEL_INLINE void marcel_disablemigration(marcel_t pid);
/** Enable preemptive migration for the given thread.
 */
MARCEL_INLINE void marcel_enablemigration(marcel_t pid);
/** Set sending-side migration handler and settings.
 */
void marcel_begin_hibernation(marcel_t __restrict t, transfert_func_t transf, void * __restrict arg, tbx_bool_t fork);

/** Set receive-side migration handler and settings.
 */
void marcel_end_hibernation(marcel_t __restrict t, post_migration_func_t f, void * __restrict arg);

#endif /* MARCEL_MIGRATION_ENABLED */

/** Give a name to the thread, for debugging purpose.
 */
int marcel_setname(marcel_t __restrict pid, const char * __restrict name);

/** Give the name of the thread.
 */
int marcel_getname(marcel_t __restrict pid, char * __restrict name, size_t n);

#ifdef MARCEL_CLEANUP_ENABLED
/** Install a cleanup handler. ROUTINE will be called with arguments ARG
    when the thread is cancelled or calls marcel_exit.  ROUTINE will also
    be called with arguments ARG when the matching marcel_cleanup_pop
    is executed with non-zero EXECUTE argument.
*/
extern void _marcel_cleanup_push (struct _marcel_cleanup_buffer *__buffer,
				   void (*__routine) (void *),
				   void *__arg) __THROW;

/** Remove a cleanup handler installed by the matching _marcel_cleanup_push.
    If EXECUTE is non-zero, the handler function is called.
*/
extern void _marcel_cleanup_pop (struct _marcel_cleanup_buffer *__buffer,
                                 tbx_bool_t __execute) __THROW;

/** Install a clean-up handler with a private clean-up buffer.
    Note: marcel_cleanup_push and marcel_cleanup_pop are macros and must always
    be used in matching pairs at the same nesting level of braces.
*/
#define marcel_cleanup_push(routine,arg) \
{       struct _marcel_cleanup_buffer _buffer; \
    _marcel_cleanup_push (&_buffer, (routine), (arg));

/** Remove a clean-up handler installed by the matching marcel_cleanup_push.
 */
#define marcel_cleanup_pop(execute) \
    _marcel_cleanup_pop (&_buffer, (execute)); }

/** Install a cleanup handler as marcel_cleanup_push does, but also
    saves the current cancellation type and set it to deferred cancellation.
*/
extern void _marcel_cleanup_push_defer (struct _marcel_cleanup_buffer *__buffer,
					 void (*__routine) (void *),
					 void *__arg) __THROW;

/** Remove a cleanup handler as marcel_cleanup_pop does, but also
    restores the cancellation type that was in effect when the matching
    _marcel_cleanup_push_defer was called.
*/
extern void _marcel_cleanup_pop_restore (struct _marcel_cleanup_buffer *__buffer,
					  int __execute) __THROW;

/** Install a clean-up handler and save the current cancellation type
    with a private clean-up buffer.
 */
# define marcel_cleanup_push_defer_np(routine,arg) \
{       struct _marcel_cleanup_buffer _buffer; \
    _marcel_cleanup_push_defer (&_buffer, (routine), (arg));

/** Remove a clean-up handler and restore the cancellation type that was in effect when the matching
    marcel_cleanup_push_defer_np was called.
 */
# define marcel_cleanup_pop_restore_np(execute) \
  _marcel_cleanup_pop_restore (&_buffer, (execute)); }

#endif /* MARCEL_CLEANUP_ENABLED */

#ifdef MARCEL_ONCE_ENABLED
/* Guarantee that the initialization function INIT_ROUTINE will be called
   only once, even if marcel_once is executed several times with the
   same ONCE_CONTROL argument. ONCE_CONTROL must point to a static or
   extern variable initialized to MARCEL_ONCE_INIT.  */
/* extern int marcel_once (marcel_once_t *__once_control, */
/* 			 void (*__init_routine) (void)) __THROW; */
#endif /* MARCEL_ONCE_ENABLED */

#ifdef PROFILE
extern void ma_thread_record_hw_sample(marcel_t t, unsigned long hw_id, unsigned long long hw_val);
#endif


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
/** Return whether the current thread is preemptible (1) or not (0).
 */
#define ma_thread_preemptible() (!SELF_GETMEM(not_preemptible))


/** Internal functions **/
/** Create a special thread such as internal threads or idle threads.
    Note: the actual code does not seem to make any difference with marcel_create_dontsched
 */
int marcel_create_special(marcel_t * __restrict pid,
			  __const marcel_attr_t * __restrict attr, 
			  marcel_func_t func, any_t __restrict arg) __THROW;

/** Exit function that should never return.
 */
void marcel_exit_special(any_t val) __THROW TBX_NORETURN;

/** Exit function that may return.
 */
void marcel_exit_canreturn(any_t val) __THROW;

/** Clean-up a terminated thread.
 */
void marcel_funerals(marcel_t t);


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_THREADS_H__ **/
