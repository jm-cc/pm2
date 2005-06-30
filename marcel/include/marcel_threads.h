
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

#section macros

#define MARCEL_ONCE_INIT 0
#ifdef __USE_XOPEN2K
/* -1 is distinct from 0 and all errno constants */
# define MARCEL_BARRIER_SERIAL_THREAD -1
#endif


#section types
enum
{
  MARCEL_CREATE_JOINABLE,
#define MARCEL_CREATE_JOINABLE	MARCEL_CREATE_JOINABLE
  MARCEL_CREATE_DETACHED
#define MARCEL_CREATE_DETACHED	MARCEL_CREATE_DETACHED
};

enum
{
  MARCEL_INHERIT_SCHED,
#define MARCEL_INHERIT_SCHED	MARCEL_INHERIT_SCHED
  MARCEL_EXPLICIT_SCHED
#define MARCEL_EXPLICIT_SCHED	MARCEL_EXPLICIT_SCHED
};

enum
{
  MARCEL_SCOPE_SYSTEM,
#define MARCEL_SCOPE_SYSTEM	MARCEL_SCOPE_SYSTEM
  MARCEL_SCOPE_PROCESS
#define MARCEL_SCOPE_PROCESS	MARCEL_SCOPE_PROCESS
};

enum
{
  MARCEL_PROCESS_PRIVATE,
#define MARCEL_PROCESS_PRIVATE	MARCEL_PROCESS_PRIVATE
  MARCEL_PROCESS_SHARED
#define MARCEL_PROCESS_SHARED	MARCEL_PROCESS_SHARED
};

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


#section macros
#define MAX_ATEXIT_FUNCS	5


/****************************************************************/
/* 
 */
#section types
#depend "marcel_utils.h[types]"
#depend "tbx_compiler.h"
typedef any_t (*marcel_func_t)(any_t);
typedef void (*cleanup_func_t)(any_t);
typedef void (*marcel_atexit_func_t)(any_t);
typedef void (*marcel_postexit_func_t)(any_t);

#section marcel_functions
void marcel_create_init_marcel_thread(marcel_t t, 
				      __const marcel_attr_t *attr);
#section functions

int marcel_create(marcel_t *pid, __const marcel_attr_t *attr, 
		  marcel_func_t func, any_t arg) __THROW;

int marcel_create_dontsched(marcel_t *pid, __const marcel_attr_t *attr, 
		  marcel_func_t func, any_t arg) __THROW;

#section marcel_functions
int marcel_create_special(marcel_t *pid, __const marcel_attr_t *attr, 
			  marcel_func_t func, any_t arg) __THROW;
#section functions

DEC_MARCEL_POSIX(int, join, (marcel_t pid, any_t *status) __THROW);

DEC_MARCEL_POSIX(void TBX_NORETURN, exit, (any_t val) __THROW);
#section marcel_functions
void marcel_exit_special(any_t val) __THROW TBX_NORETURN;

#section functions

DEC_MARCEL_POSIX(int, detach, (marcel_t pid) __THROW);

DEC_MARCEL_POSIX(int, cancel, (marcel_t pid) __THROW);

#section functions
static __tbx_inline__ int marcel_equal(marcel_t pid1, marcel_t pid2) __THROW;
#section inline
static __tbx_inline__ int marcel_equal(marcel_t pid1, marcel_t pid2)
{
  return (pid1 == pid2);
}

#section functions
void marcel_freeze(marcel_t *pids, int nb);
void marcel_unfreeze(marcel_t *pids, int nb);

/* === stack user space === */

void marcel_getuserspace(marcel_t pid, void **user_space);

void marcel_run(marcel_t pid, any_t arg);

/* ========== callbacks ============ */

void marcel_postexit(marcel_postexit_func_t, any_t);
void marcel_atexit(marcel_atexit_func_t, any_t);

__tbx_inline__ static void marcel_thread_preemption_enable(void);
__tbx_inline__ static void marcel_thread_preemption_disable(void);
#section inline
/* Pour ma_barrier */
#depend "marcel_compiler.h[marcel_macros]"
__tbx_inline__ static void marcel_thread_preemption_enable(void)
{
#ifdef MA__DEBUG
	MA_BUG_ON(!SELF_GETMEM(not_preemptible));
#endif
        ma_barrier();
	MARCEL_SELF->not_preemptible--;
}

__tbx_inline__ static void marcel_thread_preemption_disable(void)
{
	MARCEL_SELF->not_preemptible++;
        ma_barrier();
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
DEC_MARCEL_POSIX(void, cleanup_push,(struct _marcel_cleanup_buffer *__buffer,
				     cleanup_func_t func, any_t arg) __THROW)
DEC_MARCEL_POSIX(void, cleanup_pop,(struct _marcel_cleanup_buffer *__buffer,
				    boolean execute) __THROW)
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
static __tbx_inline__ void marcel_disablemigration(marcel_t pid);
#section inline
static __tbx_inline__ void marcel_disablemigration(marcel_t pid)
{
  pid->not_migratable++;
}

#section functions
static __tbx_inline__ void marcel_enablemigration(marcel_t pid);
#section inline
static __tbx_inline__ void marcel_enablemigration(marcel_t pid)
{
  pid->not_migratable--;
}

#section functions
void marcel_begin_hibernation(marcel_t t, transfert_func_t transf, void *arg, boolean fork);

void marcel_end_hibernation(marcel_t t, post_migration_func_t f, void *arg);

/* Install a cleanup handler: ROUTINE will be called with arguments ARG
   when the thread is cancelled or calls marcel_exit.  ROUTINE will also
   be called with arguments ARG when the matching marcel_cleanup_pop
   is executed with non-zero EXECUTE argument.
   marcel_cleanup_push and marcel_cleanup_pop are macros and must always
   be used in matching pairs at the same nesting level of braces. */

#define marcel_cleanup_push(routine,arg) \
  { struct _marcel_cleanup_buffer _buffer;                                   \
    _marcel_cleanup_push (&_buffer, (routine), (arg));

extern void _marcel_cleanup_push (struct _marcel_cleanup_buffer *__buffer,
				   void (*__routine) (void *),
				   void *__arg) __THROW;

/* Remove a cleanup handler installed by the matching marcel_cleanup_push.
   If EXECUTE is non-zero, the handler function is called. */

#define marcel_cleanup_pop(execute) \
    _marcel_cleanup_pop (&_buffer, (execute)); }

extern void _marcel_cleanup_pop (struct _marcel_cleanup_buffer *__buffer,
				  int __execute) __THROW;

/* Install a cleanup handler as marcel_cleanup_push does, but also
   saves the current cancellation type and set it to deferred cancellation.  */

//#ifdef __USE_GNU
# define marcel_cleanup_push_defer_np(routine,arg) \
  { struct _marcel_cleanup_buffer _buffer;				      \
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
//#endif

/* Guarantee that the initialization function INIT_ROUTINE will be called
   only once, even if marcel_once is executed several times with the
   same ONCE_CONTROL argument. ONCE_CONTROL must point to a static or
   extern variable initialized to MARCEL_ONCE_INIT.  */
extern int marcel_once (marcel_once_t *__once_control,
			 void (*__init_routine) (void)) __THROW;


#section marcel_macros
#ifdef STANDARD_MAIN
extern marcel_task_t __main_thread_struct;
#define __main_thread  (&__main_thread_struct)
#else
extern marcel_task_t *__main_thread;
#endif
