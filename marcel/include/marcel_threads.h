
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


/****************************************************************/
#section types
#include <time.h>

#section macros

/* #define STANDARD_MAIN */
#define MARCEL_ONCE_INIT 0

#section types
enum
{
  MARCEL_CREATE_JOINABLE,
#define MARCEL_CREATE_JOINABLE	MARCEL_CREATE_JOINABLE
  MARCEL_CREATE_DETACHED
#define MARCEL_CREATE_DETACHED	MARCEL_CREATE_DETACHED
};

#define MARCEL_CREATE_DETACHSTATE_INVALID -1

enum
{
  MARCEL_INHERIT_SCHED,
#define MARCEL_INHERIT_SCHED	MARCEL_INHERIT_SCHED
  MARCEL_EXPLICIT_SCHED
#define MARCEL_EXPLICIT_SCHED	MARCEL_EXPLICIT_SCHED
};

#define MARCEL_INHERITSCHED_INVALID -1

enum
{
  MARCEL_SCOPE_SYSTEM,
#define MARCEL_SCOPE_SYSTEM	MARCEL_SCOPE_SYSTEM
  MARCEL_SCOPE_PROCESS
#define MARCEL_SCOPE_PROCESS	MARCEL_SCOPE_PROCESS
};

#define MARCEL_SCOPE_INVALID -1

/* Cancellation */

enum
{
  MARCEL_CANCEL_ENABLE,
#define MARCEL_CANCEL_ENABLE	MARCEL_CANCEL_ENABLE
  MARCEL_CANCEL_DISABLE
#define MARCEL_CANCEL_DISABLE	MARCEL_CANCEL_DISABLE
};
enum
{
  MARCEL_CANCEL_DEFERRED,
#define MARCEL_CANCEL_DEFERRED	MARCEL_CANCEL_DEFERRED
  MARCEL_CANCEL_ASYNCHRONOUS
#define MARCEL_CANCEL_ASYNCHRONOUS	MARCEL_CANCEL_ASYNCHRONOUS
};
#define MARCEL_CANCELED ((void *) -1)

#define MARCEL_IS_CANCELED 1
#define MARCEL_NOT_CANCELED 0

#section macros
#define MAX_ATEXIT_FUNCS	5

#define MARCEL_THREAD_ISALIVE(thread) \
   (((thread)->sched.state != MA_TASK_DEAD)||(!(thread)->detached))


/****************************************************************/

#section types
#depend "marcel_utils.h[types]"
#depend "tbx_compiler.h"
typedef any_t (*marcel_func_t)(any_t);
typedef void (*cleanup_func_t)(any_t);
typedef void (*marcel_atexit_func_t)(any_t);
typedef void (*marcel_postexit_func_t)(any_t);

#section marcel_functions
void marcel_create_init_marcel_thread(marcel_t __restrict t, 
				      __const marcel_attr_t * __restrict attr);
#section functions

DEC_MARCEL_POSIX(int,create,(marcel_t * __restrict pid,
		  __const marcel_attr_t * __restrict attr, 
		  marcel_func_t func, any_t __restrict arg) __THROW);

int marcel_create_dontsched(marcel_t * __restrict pid,
		  __const marcel_attr_t * __restrict attr, 
		  marcel_func_t func, any_t __restrict arg) __THROW;

#section marcel_functions
int marcel_create_special(marcel_t * __restrict pid,
			  __const marcel_attr_t * __restrict attr, 
			  marcel_func_t func, any_t __restrict arg) __THROW;
#section functions

DEC_MARCEL_POSIX(int, join, (marcel_t pid, any_t *status) __THROW);

DEC_MARCEL_POSIX(void TBX_NORETURN, exit, (any_t val) __THROW);
#section marcel_functions
void marcel_exit_special(any_t val) __THROW TBX_NORETURN;

void marcel_funerals(marcel_t t);

#section functions

DEC_MARCEL_POSIX(int, detach, (marcel_t pid) __THROW);

DEC_MARCEL_POSIX(int, cancel, (marcel_t pid) __THROW);

#section functions
static __tbx_inline__ int marcel_equal(marcel_t pid1, marcel_t pid2);
#section inline
static __tbx_inline__ int marcel_equal(marcel_t pid1, marcel_t pid2)
{
  return (pid1 == pid2);
}

#section macros
#define pmarcel_equal(t1,t2) ((t1) == (t2))

#section functions

/**********************set/getconcurrency**********************/
DEC_MARCEL_POSIX(int, setconcurrency, (int newlevel) __THROW);
DEC_MARCEL_POSIX(int, getconcurrency, (void) __THROW);
/******************set/testcancelstate/type/  *****************/
DEC_MARCEL_POSIX(int, setcancelstate,(int state, int *oldstate) __THROW);
DEC_MARCEL_POSIX(int, setcanceltype,(int type, int *oldtype) __THROW);
DEC_MARCEL_POSIX(void, testcancel,(void) __THROW);
#depend "asm/linux_linkage.h[marcel_macros]"
int fastcall __pmarcel_enable_asynccancel (void);
void fastcall __pmarcel_disable_asynccancel(int old);
#ifndef MA__IFACE_PMARCEL
#define __pmarcel_enable_asynccancel() 0
#define __pmarcel_disable_asynccancel(old) (void)(old)
#endif
/******************set/getschedparam/prio*****************/
DEC_MARCEL_POSIX(int, setschedprio,(marcel_t thread,int prio) __THROW);
int pthread_setschedprio(pthread_t thread, int prio) __THROW;
DEC_MARCEL(int, setschedparam,(marcel_t thread, int policy,
                                     __const struct marcel_sched_param *__restrict param) __THROW);
DEC_MARCEL(int, getschedparam,(marcel_t thread, int *__restrict policy,
                                     struct marcel_sched_param *__restrict param) __THROW);
/******************getcpuclockid*******************************/
DEC_POSIX(int,getcpuclockid,(pmarcel_t thread_id, clockid_t *clock_id) __THROW);

void marcel_freeze(marcel_t *pids, int nb);
void marcel_unfreeze(marcel_t *pids, int nb);

/* === stack user space === */

void marcel_getuserspace(marcel_t __restrict pid,
		void * __restrict * __restrict user_space);

void marcel_run(marcel_t __restrict pid, any_t __restrict arg);

/* ========== callbacks ============ */

void marcel_postexit(marcel_postexit_func_t, any_t);
void marcel_atexit(marcel_atexit_func_t, any_t);

#section functions
void marcel_thread_preemption_enable(void);
void marcel_thread_preemption_disable(void);
int marcel_thread_is_preemption_disabled(void);
static __tbx_inline__ void marcel_some_thread_preemption_enable(marcel_t t);
static __tbx_inline__ void marcel_some_thread_preemption_disable(marcel_t t);
static __tbx_inline__ int marcel_some_thread_is_preemption_disabled(marcel_t t);
#define	marcel_thread_preemption_enable() marcel_some_thread_preemption_enable(MARCEL_SELF)
#define	marcel_thread_preemption_disable() marcel_some_thread_preemption_disable(MARCEL_SELF)
#define	marcel_thread_is_preemption_disabled() marcel_some_thread_is_preemption_disabled(MARCEL_SELF)

#section inline
/* Pour ma_barrier */
#depend "marcel_compiler.h[marcel_macros]"
/* Pour les membres de marcel_t */
#depend "marcel_descr.h[structures]"
static __tbx_inline__ void __marcel_some_thread_preemption_enable(marcel_t t)
{
#ifdef MA__DEBUG
	MA_BUG_ON(!THREAD_GETMEM(t, not_preemptible));
#endif
        ma_barrier();
	THREAD_GETMEM(t, not_preemptible)--;
}
static __tbx_inline__ void marcel_some_thread_preemption_enable(marcel_t t) {
	__marcel_some_thread_preemption_enable(t);
}

static __tbx_inline__ void __marcel_some_thread_preemption_disable(marcel_t t) {
	THREAD_GETMEM(t, not_preemptible)++;
        ma_barrier();
}

static __tbx_inline__ void marcel_some_thread_preemption_disable(marcel_t t) {
	__marcel_some_thread_preemption_disable(t);
}

static __tbx_inline__ int __marcel_some_thread_is_preemption_disabled(marcel_t t) {
	return THREAD_GETMEM(t, not_preemptible) != 0;
}

static __tbx_inline__ int marcel_some_thread_is_preemption_disabled(marcel_t t) {
	return __marcel_some_thread_is_preemption_disabled(t);
}


#section marcel_macros
#define ma_thread_preemptible() (!SELF_GETMEM(not_preemptible))

#section structures
/* Cleanup buffers */
struct _marcel_cleanup_buffer
{
  void (*__routine) (void *);		  /* Function to call.  */
  void *__arg;				  /* Its argument.  */
  int __canceltype;			  /* Saved cancellation type. */
  struct _marcel_cleanup_buffer *__prev; /* Chaining of cleanup functions.  */
};
#section functions

#undef NAME_PREFIX
#define NAME_PREFIX _
DEC_MARCEL_POSIX(void, cleanup_push,(struct _marcel_cleanup_buffer * __restrict __buffer,
				     cleanup_func_t func, any_t __restrict arg) __THROW);
DEC_MARCEL_POSIX(void, cleanup_pop,(struct _marcel_cleanup_buffer *__buffer,
				    tbx_bool_t execute) __THROW);
#undef NAME_PREFIX
#define NAME_PREFIX


/* ===== suspending & resuming ===== */

void marcel_suspend(marcel_t pid);
void marcel_resume(marcel_t pid);

#section types
/* =========== migration =========== */
#depend "marcel_descr.h[]"

typedef void (*transfert_func_t)(marcel_t t, unsigned long depl, unsigned long blksize, void *arg);

typedef void (*post_migration_func_t)(void *arg);

#section functions
MARCEL_INLINE void marcel_disablemigration(marcel_t pid);
#section marcel_inline
MARCEL_INLINE void marcel_disablemigration(marcel_t pid)
{
  pid->not_migratable++;
}

#section functions
MARCEL_INLINE void marcel_enablemigration(marcel_t pid);
#section marcel_inline
MARCEL_INLINE void marcel_enablemigration(marcel_t pid)
{
  pid->not_migratable--;
}

#section functions
void marcel_begin_hibernation(marcel_t __restrict t, transfert_func_t transf, void * __restrict arg, tbx_bool_t fork);

void marcel_end_hibernation(marcel_t __restrict t, post_migration_func_t f, void * __restrict arg);

#section functions
int marcel_setname(marcel_t __restrict pid, const char * __restrict name);
int marcel_getname(marcel_t __restrict pid, char * __restrict name, size_t n);

/* Install a cleanup handler: ROUTINE will be called with arguments ARG
   when the thread is cancelled or calls marcel_exit.  ROUTINE will also
   be called with arguments ARG when the matching marcel_cleanup_pop
   is executed with non-zero EXECUTE argument.
   marcel_cleanup_push and marcel_cleanup_pop are macros and must always
   be used in matching pairs at the same nesting level of braces. */

#define marcel_cleanup_push(routine,arg) \
{       struct _marcel_cleanup_buffer _buffer; \
    _marcel_cleanup_push (&_buffer, (routine), (arg));

extern void _marcel_cleanup_push (struct _marcel_cleanup_buffer *__buffer,
				   void (*__routine) (void *),
				   void *__arg) __THROW;

/* Remove a cleanup handler installed by the matching marcel_cleanup_push.
   If EXECUTE is non-zero, the handler function is called. */

#define marcel_cleanup_pop(execute) \
    _marcel_cleanup_pop (&_buffer, (execute)); }

extern void _marcel_cleanup_pop (struct _marcel_cleanup_buffer *__buffer,
		tbx_bool_t __execute) __THROW;

/* Install a cleanup handler as marcel_cleanup_push does, but also
   saves the current cancellation type and set it to deferred cancellation.  */

# define marcel_cleanup_push_defer_np(routine,arg) \
{       struct _marcel_cleanup_buffer _buffer; \
    _marcel_cleanup_push_defer (&_buffer, (routine), (arg));

extern void _marcel_cleanup_push_defer (struct _marcel_cleanup_buffer *__buffer,
					 void (*__routine) (void *),
					 void *__arg) __THROW;

/* Remove a cleanup handler as marcel_cleanup_pop does, but also
   restores the cancellation type that was in effect when the matching
   marcel_cleanup_push_defer was called.  */

# define marcel_cleanup_pop_restore_np(execute) \
  _marcel_cleanup_pop_restore (&_buffer, (execute)); }

extern void _marcel_cleanup_pop_restore (struct _marcel_cleanup_buffer *__buffer,
					  int __execute) __THROW;



/* Guarantee that the initialization function INIT_ROUTINE will be called
   only once, even if marcel_once is executed several times with the
   same ONCE_CONTROL argument. ONCE_CONTROL must point to a static or
   extern variable initialized to MARCEL_ONCE_INIT.  */
/* extern int marcel_once (marcel_once_t *__once_control, */
/* 			 void (*__init_routine) (void)) __THROW; */


#section macros
#depend "marcel_descr.h[types]"
#ifdef STANDARD_MAIN
extern marcel_task_t __main_thread_struct;
#define __main_thread  (&__main_thread_struct)
#else
extern marcel_task_t *__main_thread;
#endif
#section marcel_structures
