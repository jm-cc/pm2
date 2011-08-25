/* Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009
   Free Software Foundation, Inc.
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

#ifndef __PMARCEL_H__
#define __PMARCEL_H__


#ifdef MA__IFACE_PMARCEL


/* 
 * WARNING: don't edit this file. It is a mere usual pthread.h + semaphore.h + signals
 *  with pthread_ replaced by pmarcel_, so as to make sure we stick to the standard declarations.
 */

#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#ifdef __USE_XOPEN2K
#  define __need_timespec
#endif
#include <time.h>

#define __need_sigset_t
#include <signal.h>
#undef __need_sigset_t

#include "tbx_compiler.h"
#include "sys/marcel_types.h"

#undef __nonnull
#define __nonnull(list)


/************** pmarcel types ***********/
typedef marcel_vpset_t pmarcel_cpu_set_t;

typedef unsigned long int pmarcel_t;
typedef marcel_attr_t pmarcel_attr_t;

typedef marcel_spinlock_t pmarcel_spinlock_t;

typedef lpt_rwlock_t pmarcel_rwlock_t;
typedef marcel_rwlockattr_t pmarcel_rwlockattr_t;

typedef marcel_sem_t pmarcel_sem_t;

typedef marcel_sigset_t pmarcel_sigset_t;

typedef union __pmarcel_mutex_t pmarcel_mutex_t;
typedef union __pmarcel_mutexattr_t pmarcel_mutexattr_t;

typedef marcel_cond_t pmarcel_cond_t;
typedef marcel_condattr_t pmarcel_condattr_t;

typedef marcel_barrier_t pmarcel_barrier_t;
typedef marcel_barrierattr_t pmarcel_barrierattr_t;

typedef marcel_once_t pmarcel_once_t;

typedef marcel_key_t pmarcel_key_t;


/******** pmarcel function added *********/
// stdio
DEC_PMARCEL(int, printf, (const char *__restrict format, ...));
DEC_PMARCEL(int, sprintf, (char *__restrict s, const char *__restrict format, ...));
DEC_PMARCEL(int, fprintf, (FILE *__restrict stream, const char *__restrict format, ...));
DEC_PMARCEL(int, vfprintf, (FILE *__restrict stream, const char *__restrict format, va_list ap));
DEC_PMARCEL(int, vprintf, (const char *__restrict format, va_list ap));
DEC_PMARCEL(int, vsprintf, (char *__restrict s, const char *__restrict format, va_list ap));
DEC_PMARCEL(int, puts, (const char *s));
DEC_PMARCEL(int, putchar, (int c));
DEC_PMARCEL(int, putc,(int c, FILE *stream));
DEC_PMARCEL(int, fputc, (int c, FILE *stream));
DEC_PMARCEL(int, fputs, (const char *s, FILE *stream));
DEC_PMARCEL(char*, fgets, (char *__restrict s, int n, FILE *__restrict stream));
DEC_PMARCEL(size_t, fread, (void *ptr, size_t size, size_t nmemb, FILE *stream));
DEC_PMARCEL(size_t, fwrite, (const void *ptr, size_t size, size_t nmemb, FILE *stream));
DEC_PMARCEL(FILE*, fopen, (const char *__restrict filename, const char *__restrict mode));
DEC_PMARCEL(int, fclose, (FILE *stream));
DEC_PMARCEL(int, fflush, (FILE *stream));
DEC_PMARCEL(int, feof, (FILE *stream));
DEC_PMARCEL(int, fscanf, (FILE *__restrict stream, const char *__restrict format, ... ));
DEC_PMARCEL(int, vfscanf, (FILE *__restrict stream, const char *__restrict format, va_list arg));
DEC_PMARCEL(int, scanf, (const char *__restrict format, ... ));
DEC_PMARCEL(int, sscanf, (const char *__restrict s, const char *__restrict format, ... ));
DEC_PMARCEL(int, vscanf, (const char *__restrict format, va_list arg ));
DEC_PMARCEL(int, vsscanf, (const char *__restrict s, const char *__restrict format, va_list arg ));

// stdmem
DEC_PMARCEL(void*, calloc, (size_t nmemb, size_t size));
DEC_PMARCEL(void*, malloc, (size_t size));
DEC_PMARCEL(void, free, (void *ptr));
DEC_PMARCEL(void*, realloc, (void *ptr, size_t size));
DEC_PMARCEL(void*, valloc, (size_t size));
DEC_PMARCEL(int, brk, (void *addr));
DEC_PMARCEL(void*, sbrk, (intptr_t increment));
DEC_PMARCEL(void*, mmap, (void *addr, size_t length, int prot, int flags, int fd, off_t offset));
DEC_PMARCEL(int, munmap, (void *addr, size_t length));
DEC_PMARCEL(char*, strdup, (const char *s));
DEC_PMARCEL(char*, strndup, (const char *s, size_t n));
DEC_PMARCEL(void*, memalign, (size_t boundary, size_t size));
#if HAVE_DECL_POSIX_MEMALIGN
DEC_PMARCEL(int, posix_memalign, (void **memptr, size_t alignment, size_t size));
#endif

// exec
DEC_PMARCEL(pid_t, wait, (int *status));
DEC_PMARCEL(pid_t, waitid, (idtype_t idtype, id_t id, siginfo_t *infop, int options));
DEC_PMARCEL(pid_t, waitpid, (pid_t wpid, int *status, int options));
DEC_PMARCEL(pid_t, wait3, (int *status, int options, struct rusage *rusage));
DEC_PMARCEL(pid_t, wait4, (pid_t wpid, int *status, int options, struct rusage *rusage));
DEC_PMARCEL(int, execl, (const char *path, const char *arg, ...));
DEC_PMARCEL(int, execlp, (const char *file, const char *arg, ...));
DEC_PMARCEL(int, execle, (const char *path, const char *arg, ...));
DEC_PMARCEL(int, execve, (const char *name, char *const argv[], char *const envp[]));
DEC_PMARCEL(int, execv, (const char *name, char *const argv[]));
DEC_PMARCEL(int, execvp, (const char *name, char *const argv[]));
DEC_PMARCEL(int, system, (const char *line));
#if HAVE_DECL_POSIX_SPAWN
#include <spawn.h>
DEC_PMARCEL(int, posix_spawn, (pid_t * __restrict pid, const char *__restrict path,
			       const posix_spawn_file_actions_t * file_actions,
			       const posix_spawnattr_t * __restrict attrp, char *const argv[],
			       char *const envp[]));
DEC_PMARCEL(int, posix_spawnp, (pid_t * __restrict pid, const char *__restrict file,
				const posix_spawn_file_actions_t * file_actions,
				const posix_spawnattr_t * __restrict attrp, char *const argv[],
				char *const envp[]));
#endif

// process
DEC_PMARCEL(pid_t, fork, (void));
DEC_PMARCEL(void, kill_other_threads_np, (void));

// signal
DEC_PMARCEL(int, sigqueue, (pmarcel_t *thread, int sig, union sigval value));


/************** pthread.h ***************/


/* Detach state.  */
enum
{
  PMARCEL_CREATE_JOINABLE,
#define PMARCEL_CREATE_JOINABLE	PMARCEL_CREATE_JOINABLE
  PMARCEL_CREATE_DETACHED
#define PMARCEL_CREATE_DETACHED	PMARCEL_CREATE_DETACHED
};


/* Mutex types.  */

/** Marcel: use marcel_mutex.h definitions **/

/* enum */
/* { */
/*   PMARCEL_MUTEX_TIMED_NP, */
/*   PMARCEL_MUTEX_RECURSIVE_NP, */
/*   PMARCEL_MUTEX_ERRORCHECK_NP, */
/*   PMARCEL_MUTEX_ADAPTIVE_NP */
/* #ifdef __USE_UNIX98 */
/*   , */
/*   PMARCEL_MUTEX_NORMAL = PMARCEL_MUTEX_TIMED_NP, */
/*   PMARCEL_MUTEX_RECURSIVE = PMARCEL_MUTEX_RECURSIVE_NP, */
/*   PMARCEL_MUTEX_ERRORCHECK = PMARCEL_MUTEX_ERRORCHECK_NP, */
/*   PMARCEL_MUTEX_DEFAULT = PMARCEL_MUTEX_NORMAL */
/* #endif */
/* #ifdef __USE_GNU */
/*   /\* For compatibility.  *\/ */
/*   , PMARCEL_MUTEX_FAST_NP = PMARCEL_MUTEX_TIMED_NP */
/* #endif */
/* }; */


/* #ifdef __USE_XOPEN2K */
/* /\* Robust mutex or not flags.  *\/ */
/* enum */
/* { */
/*   PMARCEL_MUTEX_STALLED, */
/*   PMARCEL_MUTEX_STALLED_NP = PMARCEL_MUTEX_STALLED, */
/*   PMARCEL_MUTEX_ROBUST, */
/*   PMARCEL_MUTEX_ROBUST_NP = PMARCEL_MUTEX_ROBUST */
/* }; */
/* #endif */


/* #ifdef __USE_UNIX98 */
/* /\* Mutex protocols.  *\/ */
/* enum */
/* { */
/*   PMARCEL_PRIO_NONE, */
/*   PMARCEL_PRIO_INHERIT, */
/*   PMARCEL_PRIO_PROTECT */
/* }; */
/* #endif */


/* Mutex initializers.  */
/* #if __WORDSIZE == 64 */
/* # define PMARCEL_MUTEX_INITIALIZER \ */
/*   { { 0, 0, 0, 0, 0, 0, { 0, 0 } } } */
/* # ifdef __USE_GNU */
/* #  define PMARCEL_RECURSIVE_MUTEX_INITIALIZER_NP \ */
/*   { { 0, 0, 0, 0, PMARCEL_MUTEX_RECURSIVE_NP, 0, { 0, 0 } } } */
/* #  define PMARCEL_ERRORCHECK_MUTEX_INITIALIZER_NP \ */
/*   { { 0, 0, 0, 0, PMARCEL_MUTEX_ERRORCHECK_NP, 0, { 0, 0 } } } */
/* #  define PMARCEL_ADAPTIVE_MUTEX_INITIALIZER_NP \ */
/*   { { 0, 0, 0, 0, PMARCEL_MUTEX_ADAPTIVE_NP, 0, { 0, 0 } } } */
/* # endif */
/* #else */
/* # define PMARCEL_MUTEX_INITIALIZER \ */
/*   { { 0, 0, 0, 0, 0, { 0 } } } */
/* # ifdef __USE_GNU */
/* #  define PMARCEL_RECURSIVE_MUTEX_INITIALIZER_NP \ */
/*   { { 0, 0, 0, PMARCEL_MUTEX_RECURSIVE_NP, 0, { 0 } } } */
/* #  define PMARCEL_ERRORCHECK_MUTEX_INITIALIZER_NP \ */
/*   { { 0, 0, 0, PMARCEL_MUTEX_ERRORCHECK_NP, 0, { 0 } } } */
/* #  define PMARCEL_ADAPTIVE_MUTEX_INITIALIZER_NP \ */
/*   { { 0, 0, 0, PMARCEL_MUTEX_ADAPTIVE_NP, 0, { 0 } } } */
/* # endif */
/* #endif */


/* Read-write lock types.  */
//#if defined __USE_UNIX98 || defined __USE_XOPEN2K
enum
{
  PMARCEL_RWLOCK_PREFER_READER_NP,
  PMARCEL_RWLOCK_PREFER_WRITER_NP,
  PMARCEL_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP,
  PMARCEL_RWLOCK_DEFAULT_NP = PMARCEL_RWLOCK_PREFER_READER_NP
};

/* Read-write lock initializers.  */
# define PMARCEL_RWLOCK_INITIALIZER \
  { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }
# ifdef __USE_GNU
#  if __WORDSIZE == 64
#   define PMARCEL_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP \
  { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,					      \
	PMARCEL_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP } }
#  else
#   if __BYTE_ORDER == __LITTLE_ENDIAN
#    define PMARCEL_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP \
  { { 0, 0, 0, 0, 0, 0, PMARCEL_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP, \
      0, 0, 0, 0 } }
#   else
#    define PMARCEL_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP \
  { { 0, 0, 0, 0, 0, 0, 0, 0, 0, PMARCEL_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP,\
      0 } }
#   endif
#  endif
# endif
//#endif  /* Unix98 or XOpen2K */


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
/** Marcel: use marcel_cond.h definition **/
/* #define PMARCEL_COND_INITIALIZER { { 0, 0, 0, 0, 0, (void *) 0, 0, 0 } } */


/* Cleanup buffers */
/* struct _pmarcel_cleanup_buffer */
/* { */
/*   void (*__routine) (void *);             /\* Function to call.  *\/ */
/*   void *__arg;                            /\* Its argument.  *\/ */
/*   int __canceltype;                       /\* Saved cancellation type. *\/ */
/*   struct _pmarcel_cleanup_buffer *__prev; /\* Chaining of cleanup functions.  *\/ */
/* }; */

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


/** Marcel: use marcel_barrier.h definition **/
/* #ifdef __USE_XOPEN2K */
/* /\* Value returned by 'pmarcel_barrier_wait' for one of the threads after */
/*    the required number of threads have called this function. */
/*    -1 is distinct from 0 and all errno constants *\/ */
/* # define PMARCEL_BARRIER_SERIAL_THREAD -1 */
/* #endif */


__TBX_BEGIN_DECLS

/* Create a new thread, starting with execution of START-ROUTINE
   getting passed ARG.  Creation attributed come from ATTR.  The new
   handle is stored in *NEWTHREAD.  */
extern int pmarcel_create (pmarcel_t *__restrict __newthread,
			   __const pmarcel_attr_t *__restrict __attr,
			   void *(*__start_routine) (void *),
			   void *__restrict __arg) __THROW __nonnull ((1, 3));

/* Terminate calling thread.

   The registered cleanup handlers are called via exception handling
   so we cannot mark this function with __THROW.*/
extern void pmarcel_exit (void *__retval) __attribute__ ((__noreturn__));

/* Make calling thread wait for termination of the thread TH.  The
   exit status of the thread is stored in *THREAD_RETURN, if THREAD_RETURN
   is not NULL.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int pmarcel_join (pmarcel_t __th, void **__thread_return);

#ifdef __USE_GNU
/* Check whether thread TH has terminated.  If yes return the status of
   the thread in *THREAD_RETURN, if THREAD_RETURN is not NULL.  */
extern int pmarcel_tryjoin_np (pmarcel_t __th, void **__thread_return) __THROW;

/* Make calling thread wait for termination of the thread TH, but only
   until TIMEOUT.  The exit status of the thread is stored in
   *THREAD_RETURN, if THREAD_RETURN is not NULL.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int pmarcel_timedjoin_np (pmarcel_t __th, void **__thread_return,
				 __const struct timespec *__abstime);
#endif

/* Indicate that the thread TH is never to be joined with PMARCEL_JOIN.
   The resources of TH will therefore be freed immediately when it
   terminates, instead of waiting for another thread to perform PMARCEL_JOIN
   on it.  */
extern int pmarcel_detach (pmarcel_t __th) __THROW;


/* Obtain the identifier of the current thread.  */
extern pmarcel_t pmarcel_self (void) __THROW __attribute__ ((__const__));

/* Compare two thread identifiers.  */
/** Marcel: provides t1 == t2 **/
//extern int pmarcel_equal (pmarcel_t __thread1, pmarcel_t __thread2) __THROW;


/* Thread attribute handling.  */

/* Initialize thread attribute *ATTR with default attributes
   (detachstate is PMARCEL_JOINABLE, scheduling policy is SCHED_OTHER,
    no user-provided stack).  */
extern int pmarcel_attr_init (pmarcel_attr_t *__attr) __THROW __nonnull ((1));

/* Destroy thread attribute *ATTR.  */
extern int pmarcel_attr_destroy (pmarcel_attr_t *__attr)
     __THROW __nonnull ((1));

/* Get detach state attribute.  */
extern int pmarcel_attr_getdetachstate (__const pmarcel_attr_t *__attr,
					int *__detachstate)
     __THROW __nonnull ((1, 2));

/* Set detach state attribute.  */
extern int pmarcel_attr_setdetachstate (pmarcel_attr_t *__attr,
					int __detachstate)
     __THROW __nonnull ((1));


/* Get the size of the guard area created for stack overflow protection.  */
extern int pmarcel_attr_getguardsize (__const pmarcel_attr_t *__attr,
				      size_t *__guardsize)
     __THROW __nonnull ((1, 2));

/* Set the size of the guard area created for stack overflow protection.  */
extern int pmarcel_attr_setguardsize (pmarcel_attr_t *__attr,
				      size_t __guardsize)
     __THROW __nonnull ((1));


/* Return in *PARAM the scheduling parameters of *ATTR.  */
extern int pmarcel_attr_getschedparam (__const pmarcel_attr_t *__restrict
				       __attr,
				       struct marcel_sched_param *__restrict __param)
     __THROW __nonnull ((1, 2));

/* Set scheduling parameters (priority, etc) in *ATTR according to PARAM.  */
extern int pmarcel_attr_setschedparam (pmarcel_attr_t *__restrict __attr,
				       __const struct marcel_sched_param *__restrict
				       __param) __THROW __nonnull ((1, 2));

/* Return in *POLICY the scheduling policy of *ATTR.  */
extern int pmarcel_attr_getschedpolicy (__const pmarcel_attr_t *__restrict
					__attr, int *__restrict __policy)
     __THROW __nonnull ((1, 2));

/* Set scheduling policy in *ATTR according to POLICY.  */
extern int pmarcel_attr_setschedpolicy (pmarcel_attr_t *__attr, int __policy)
     __THROW __nonnull ((1));

/* Return in *INHERIT the scheduling inheritance mode of *ATTR.  */
extern int pmarcel_attr_getinheritsched (__const pmarcel_attr_t *__restrict
					 __attr, int *__restrict __inherit)
     __THROW __nonnull ((1, 2));

/* Set scheduling inheritance mode in *ATTR according to INHERIT.  */
extern int pmarcel_attr_setinheritsched (pmarcel_attr_t *__attr,
					 int __inherit)
     __THROW __nonnull ((1));


/* Return in *SCOPE the scheduling contention scope of *ATTR.  */
extern int pmarcel_attr_getscope (__const pmarcel_attr_t *__restrict __attr,
				  int *__restrict __scope)
     __THROW __nonnull ((1, 2));

/* Set scheduling contention scope in *ATTR according to SCOPE.  */
extern int pmarcel_attr_setscope (pmarcel_attr_t *__attr, int __scope)
     __THROW __nonnull ((1));

/* Return the previously set address for the stack.  */
extern int pmarcel_attr_getstackaddr (__const pmarcel_attr_t *__restrict
				      __attr, void **__restrict __stackaddr)
     __THROW __nonnull ((1, 2));

/* Set the starting address of the stack of the thread to be created.
   Depending on whether the stack grows up or down the value must either
   be higher or lower than all the address in the memory block.  The
   minimal size of the block must be PMARCEL_STACK_MIN.  */
extern int pmarcel_attr_setstackaddr (pmarcel_attr_t *__attr,
				      void *__stackaddr)
     __THROW __nonnull ((1));

/* Return the currently used minimal stack size.  */
extern int pmarcel_attr_getstacksize (__const pmarcel_attr_t *__restrict
				      __attr, size_t *__restrict __stacksize)
     __THROW __nonnull ((1, 2));

/* Add information about the minimum stack size needed for the thread
   to be started.  This size must never be less than PMARCEL_STACK_MIN
   and must also not exceed the system limits.  */
extern int pmarcel_attr_setstacksize (pmarcel_attr_t *__attr,
				      size_t __stacksize)
     __THROW __nonnull ((1));

//#ifdef __USE_XOPEN2K
/* Return the previously set address for the stack.  */
extern int pmarcel_attr_getstack (__const pmarcel_attr_t *__restrict __attr,
				  void **__restrict __stackaddr,
				  size_t *__restrict __stacksize)
     __THROW __nonnull ((1, 2, 3));

/* The following two interfaces are intended to replace the last two.  They
   require setting the address as well as the size since only setting the
   address will make the implementation on some architectures impossible.  */
extern int pmarcel_attr_setstack (pmarcel_attr_t *__attr, void *__stackaddr,
				  size_t __stacksize) __THROW __nonnull ((1));
//#endif

//#ifdef __USE_GNU
/* Thread created with attribute ATTR will be limited to run only on
   the processors represented in CPUSET.  */
extern int pmarcel_attr_setaffinity_np (pmarcel_attr_t *__attr,
					size_t __cpusetsize,
					__const pmarcel_cpu_set_t *__cpuset)
     __THROW __nonnull ((1, 3));

/* Get bit set in CPUSET representing the processors threads created with
   ATTR can run on.  */
extern int pmarcel_attr_getaffinity_np (__const pmarcel_attr_t *__attr,
					size_t __cpusetsize,
					pmarcel_cpu_set_t *__cpuset)
     __THROW __nonnull ((1, 3));


/* Initialize thread attribute *ATTR with attributes corresponding to the
   already running thread TH.  It shall be called on uninitialized ATTR
   and destroyed with pmarcel_attr_destroy when no longer needed.  */
extern int pmarcel_getattr_np (pmarcel_t __th, pmarcel_attr_t *__attr)
     __THROW __nonnull ((2));
//#endif


/* Functions for scheduling control.  */

/* Set the scheduling parameters for TARGET_THREAD according to POLICY
   and *PARAM.  */
extern int pmarcel_setschedparam (pmarcel_t __target_thread, int __policy,
				  __const struct marcel_sched_param *__param)
     __THROW __nonnull ((3));

/* Return in *POLICY and *PARAM the scheduling parameters for TARGET_THREAD. */
extern int pmarcel_getschedparam (pmarcel_t __target_thread,
				  int *__restrict __policy,
				  struct marcel_sched_param *__restrict __param)
     __THROW __nonnull ((2, 3));

/* Set the scheduling priority for TARGET_THREAD.  */
extern int pmarcel_setschedprio (pmarcel_t __target_thread, int __prio)
     __THROW;


//#ifdef __USE_UNIX98
/* Determine level of concurrency.  */
extern int pmarcel_getconcurrency (void) __THROW;

/* Set new concurrency level to LEVEL.  */
extern int pmarcel_setconcurrency (int __level) __THROW;
//#endif

//#ifdef __USE_GNU
/* Yield the processor to another thread or process.
   This function is similar to the POSIX `sched_yield' function but
   might be differently implemented in the case of a m-on-n thread
   implementation.  */
extern int pmarcel_yield (void) __THROW;


/* Limit specified thread TH to run only on the processors represented
   in CPUSET.  */
extern int pmarcel_setaffinity_np (pmarcel_t __th, size_t __cpusetsize,
				   __const pmarcel_cpu_set_t *__cpuset)
     __THROW __nonnull ((3));

/* Get bit set in CPUSET representing the processors TH can run on.  */
extern int pmarcel_getaffinity_np (pmarcel_t __th, size_t __cpusetsize,
				   pmarcel_cpu_set_t *__cpuset)
     __THROW __nonnull ((3));
//#endif


/* Functions for handling initialization.  */

/* Guarantee that the initialization function INIT_ROUTINE will be called
   only once, even if pmarcel_once is executed several times with the
   same ONCE_CONTROL argument. ONCE_CONTROL must point to a static or
   extern variable initialized to PMARCEL_ONCE_INIT.

   The initialization functions might throw exception which is why
   this function is not marked with __THROW.  */
extern int pmarcel_once (pmarcel_once_t *__once_control,
			 void (*__init_routine) (void)) __nonnull ((1, 2));


/* Functions for handling cancellation.

   Note that these functions are explicitly not marked to not throw an
   exception in C++ code.  If cancellation is implemented by unwinding
   this is necessary to have the compiler generate the unwind information.  */

/* Set cancelability state of current thread to STATE, returning old
   state in *OLDSTATE if OLDSTATE is not NULL.  */
extern int pmarcel_setcancelstate (int __state, int *__oldstate);

/* Set cancellation state of current thread to TYPE, returning the old
   type in *OLDTYPE if OLDTYPE is not NULL.  */
extern int pmarcel_setcanceltype (int __type, int *__oldtype);

/* Cancel THREAD immediately or at the next possibility.  */
extern int pmarcel_cancel (pmarcel_t __th);

/* Test for pending cancellation for the current thread and terminate
   the thread as per pmarcel_exit(PMARCEL_CANCELED) if it has been
   cancelled.  */
extern void pmarcel_testcancel (void);


/* Cancellation handling with integration into exception handling.  */
#ifdef MARCEL_CLEANUP_ENABLED
/** Install a clean-up handler with a private clean-up buffer.
    Note: marcel_cleanup_push and marcel_cleanup_pop are macros and must always
    be used in matching pairs at the same nesting level of braces.
*/
#define pmarcel_cleanup_push(routine,arg) \
{       struct _marcel_cleanup_buffer _buffer;	\
	_pmarcel_cleanup_push (&_buffer, (routine), (arg));

/** Remove a clean-up handler installed by the matching marcel_cleanup_push.
 */
#define pmarcel_cleanup_pop(execute) \
	_pmarcel_cleanup_pop (&_buffer, (execute)); }

/** Install a clean-up handler and save the current cancellation type
    with a private clean-up buffer.
 */
# define pmarcel_cleanup_push_defer_np(routine,arg) \
{       struct _marcel_cleanup_buffer _buffer; \
	_pmarcel_cleanup_push_defer (&_buffer, (routine), (arg));

/** Remove a clean-up handler and restore the cancellation type that was in effect when the matching
    marcel_cleanup_push_defer_np was called.
 */
# define pmarcel_cleanup_pop_restore_np(execute) \
	_pmarcel_cleanup_pop_restore (&_buffer, (execute)); }

void _pmarcel_cleanup_push(struct _marcel_cleanup_buffer * __restrict __buffer,
			   void (*__fct) (void *), any_t __restrict arg);
void _pmarcel_cleanup_pop(struct _marcel_cleanup_buffer *__buffer, int execute);
#endif


/****** CLEANUP *****/
/* typedef struct */
/* { */
/*   struct */
/*   { */
/*     __jmp_buf __cancel_jmp_buf; */
/*     int __mask_was_saved; */
/*   } __cancel_jmp_buf[1]; */
/*   void *__pad[4]; */
/* } __pmarcel_unwind_buf_t __attribute__ ((__aligned__)); */

/* /\* No special attributes by default.  *\/ */
/* #ifndef __cleanup_fct_attribute */
/* # define __cleanup_fct_attribute */
/* #endif */


/* /\* Structure to hold the cleanup handler information.  *\/ */
/* struct __pmarcel_cleanup_frame */
/* { */
/*   void (*__cancel_routine) (void *); */
/*   void *__cancel_arg; */
/*   int __do_it; */
/*   int __cancel_type; */
/* }; */

/* #if defined __GNUC__ && defined __EXCEPTIONS */
/* # ifdef __cplusplus */
/* /\* Class to handle cancellation handler invocation.  *\/ */
/* class __pmarcel_cleanup_class */
/* { */
/*   void (*__cancel_routine) (void *); */
/*   void *__cancel_arg; */
/*   int __do_it; */
/*   int __cancel_type; */

/*  public: */
/*   __pmarcel_cleanup_class (void (*__fct) (void *), void *__arg) */
/*     : __cancel_routine (__fct), __cancel_arg (__arg), __do_it (1) { } */
/*   ~__pmarcel_cleanup_class () { if (__do_it) __cancel_routine (__cancel_arg); } */
/*   void __setdoit (int __newval) { __do_it = __newval; } */
/*   void __defer () { pmarcel_setcanceltype (PMARCEL_CANCEL_DEFERRED, */
/* 					   &__cancel_type); } */
/*   void __restore () const { pmarcel_setcanceltype (__cancel_type, 0); } */
/* }; */

/* /\* Install a cleanup handler: ROUTINE will be called with arguments ARG */
/*    when the thread is canceled or calls pmarcel_exit.  ROUTINE will also */
/*    be called with arguments ARG when the matching pmarcel_cleanup_pop */
/*    is executed with non-zero EXECUTE argument. */

/*    pmarcel_cleanup_push and pmarcel_cleanup_pop are macros and must always */
/*    be used in matching pairs at the same nesting level of braces.  *\/ */
/* #  define pmarcel_cleanup_push(routine, arg) \ */
/*   do {									      \ */
/*     __pmarcel_cleanup_class __clframe (routine, arg) */

/* /\* Remove a cleanup handler installed by the matching pmarcel_cleanup_push. */
/*    If EXECUTE is non-zero, the handler function is called. *\/ */
/* #  define pmarcel_cleanup_pop(execute) \ */
/*     __clframe.__setdoit (execute);					      \ */
/*   } while (0) */

/* #  ifdef __USE_GNU */
/* /\* Install a cleanup handler as pmarcel_cleanup_push does, but also */
/*    saves the current cancellation type and sets it to deferred */
/*    cancellation.  *\/ */
/* #   define pmarcel_cleanup_push_defer_np(routine, arg) \ */
/*   do {									      \ */
/*     __pmarcel_cleanup_class __clframe (routine, arg);			      \ */
/*     __clframe.__defer () */

/* /\* Remove a cleanup handler as pmarcel_cleanup_pop does, but also */
/*    restores the cancellation type that was in effect when the matching */
/*    pmarcel_cleanup_push_defer was called.  *\/ */
/* #   define pmarcel_cleanup_pop_restore_np(execute) \ */
/*     __clframe.__restore ();						      \ */
/*     __clframe.__setdoit (execute);					      \ */
/*   } while (0) */
/* #  endif */
/* # else */
/* /\* Function called to call the cleanup handler.  As an extern inline */
/*    function the compiler is free to decide inlining the change when */
/*    needed or fall back on the copy which must exist somewhere */
/*    else.  *\/ */
/* __extern_inline void */
/* __pmarcel_cleanup_routine (struct __pmarcel_cleanup_frame *__frame) */
/* { */
/*   if (__frame->__do_it) */
/*     __frame->__cancel_routine (__frame->__cancel_arg); */
/* } */

/* /\* Install a cleanup handler: ROUTINE will be called with arguments ARG */
/*    when the thread is canceled or calls pmarcel_exit.  ROUTINE will also */
/*    be called with arguments ARG when the matching pmarcel_cleanup_pop */
/*    is executed with non-zero EXECUTE argument. */

/*    pmarcel_cleanup_push and pmarcel_cleanup_pop are macros and must always */
/*    be used in matching pairs at the same nesting level of braces.  *\/ */
/* #  define pmarcel_cleanup_push(routine, arg) \ */
/*   do {									      \ */
/*     struct __pmarcel_cleanup_frame __clframe				      \ */
/*       __attribute__ ((__cleanup__ (__pmarcel_cleanup_routine)))		      \ */
/*       = { .__cancel_routine = (routine), .__cancel_arg = (arg),	 	      \ */
/* 	  .__do_it = 1 }; */

/* /\* Remove a cleanup handler installed by the matching pmarcel_cleanup_push. */
/*    If EXECUTE is non-zero, the handler function is called. *\/ */
/* #  define pmarcel_cleanup_pop(execute) \ */
/*     __clframe.__do_it = (execute);					      \ */
/*   } while (0) */

/* #  ifdef __USE_GNU */
/* /\* Install a cleanup handler as pmarcel_cleanup_push does, but also */
/*    saves the current cancellation type and sets it to deferred */
/*    cancellation.  *\/ */
/* #   define pmarcel_cleanup_push_defer_np(routine, arg) \ */
/*   do {									      \ */
/*     struct __pmarcel_cleanup_frame __clframe				      \ */
/*       __attribute__ ((__cleanup__ (__pmarcel_cleanup_routine)))		      \ */
/*       = { .__cancel_routine = (routine), .__cancel_arg = (arg),		      \ */
/* 	  .__do_it = 1 };						      \ */
/*     (void) pmarcel_setcanceltype (PMARCEL_CANCEL_DEFERRED,		      \ */
/* 				  &__clframe.__cancel_type) */

/* /\* Remove a cleanup handler as pmarcel_cleanup_pop does, but also */
/*    restores the cancellation type that was in effect when the matching */
/*    pmarcel_cleanup_push_defer was called.  *\/ */
/* #   define pmarcel_cleanup_pop_restore_np(execute) \ */
/*     (void) pmarcel_setcanceltype (__clframe.__cancel_type, NULL);	      \ */
/*     __clframe.__do_it = (execute);					      \ */
/*   } while (0) */
/* #  endif */
/* # endif */
/* #else */
/* /\* Install a cleanup handler: ROUTINE will be called with arguments ARG */
/*    when the thread is canceled or calls pmarcel_exit.  ROUTINE will also */
/*    be called with arguments ARG when the matching pmarcel_cleanup_pop */
/*    is executed with non-zero EXECUTE argument. */

/*    pmarcel_cleanup_push and pmarcel_cleanup_pop are macros and must always */
/*    be used in matching pairs at the same nesting level of braces.  *\/ */
/* # define pmarcel_cleanup_push(routine, arg) \ */
/*   do {									      \ */
/*     __pmarcel_unwind_buf_t __cancel_buf;				      \ */
/*     void (*__cancel_routine) (void *) = (routine);			      \ */
/*     void *__cancel_arg = (arg);						      \ */
/*     int not_first_call = __sigsetjmp ((struct __jmp_buf_tag *) (void *)	      \ */
/* 				      __cancel_buf.__cancel_jmp_buf, 0);      \ */
/*     if (__builtin_expect (not_first_call, 0))				      \ */
/*       {									      \ */
/* 	__cancel_routine (__cancel_arg);				      \ */
/* 	__pmarcel_unwind_next (&__cancel_buf);				      \ */
/* 	/\* NOTREACHED *\/						      \ */
/*       }									      \ */
/* 									      \ */
/*     __pmarcel_register_cancel (&__cancel_buf);				      \ */
/*     do { */
/* extern void __pmarcel_register_cancel (__pmarcel_unwind_buf_t *__buf) */
/*      __cleanup_fct_attribute; */

/* /\* Remove a cleanup handler installed by the matching pmarcel_cleanup_push. */
/*    If EXECUTE is non-zero, the handler function is called. *\/ */
/* # define pmarcel_cleanup_pop(execute) \ */
/*       do { } while (0);/\* Empty to allow label before pmarcel_cleanup_pop.  *\/\ */
/*     } while (0);							      \ */
/*     __pmarcel_unregister_cancel (&__cancel_buf);			      \ */
/*     if (execute)							      \ */
/*       __cancel_routine (__cancel_arg);					      \ */
/*   } while (0) */
/* extern void __pmarcel_unregister_cancel (__pmarcel_unwind_buf_t *__buf) */
/*   __cleanup_fct_attribute; */

/* # ifdef __USE_GNU */
/* /\* Install a cleanup handler as pmarcel_cleanup_push does, but also */
/*    saves the current cancellation type and sets it to deferred */
/*    cancellation.  *\/ */
/* #  define pmarcel_cleanup_push_defer_np(routine, arg) \ */
/*   do {									      \ */
/*     __pmarcel_unwind_buf_t __cancel_buf;				      \ */
/*     void (*__cancel_routine) (void *) = (routine);			      \ */
/*     void *__cancel_arg = (arg);						      \ */
/*     int not_first_call = __sigsetjmp ((struct __jmp_buf_tag *) (void *)	      \ */
/* 				      __cancel_buf.__cancel_jmp_buf, 0);      \ */
/*     if (__builtin_expect (not_first_call, 0))				      \ */
/*       {									      \ */
/* 	__cancel_routine (__cancel_arg);				      \ */
/* 	__pmarcel_unwind_next (&__cancel_buf);				      \ */
/* 	/\* NOTREACHED *\/						      \ */
/*       }									      \ */
/* 									      \ */
/*     __pmarcel_register_cancel_defer (&__cancel_buf);			      \ */
/*     do { */
/* extern void __pmarcel_register_cancel_defer (__pmarcel_unwind_buf_t *__buf) */
/*      __cleanup_fct_attribute; */

/* /\* Remove a cleanup handler as pmarcel_cleanup_pop does, but also */
/*    restores the cancellation type that was in effect when the matching */
/*    pmarcel_cleanup_push_defer was called.  *\/ */
/* #  define pmarcel_cleanup_pop_restore_np(execute) \ */
/*       do { } while (0);/\* Empty to allow label before pmarcel_cleanup_pop.  *\/\ */
/*     } while (0);							      \ */
/*     __pmarcel_unregister_cancel_restore (&__cancel_buf);		      \ */
/*     if (execute)							      \ */
/*       __cancel_routine (__cancel_arg);					      \ */
/*   } while (0) */
/* extern void __pmarcel_unregister_cancel_restore (__pmarcel_unwind_buf_t *__buf) */
/*   __cleanup_fct_attribute; */
/* # endif */

/* /\* Internal interface to initiate cleanup.  *\/ */
/* extern void __pmarcel_unwind_next (__pmarcel_unwind_buf_t *__buf) */
/*      __cleanup_fct_attribute __attribute__ ((__noreturn__)) */
/* # ifndef SHARED */
/*      __attribute__ ((__weak__)) */
/* # endif */
/*      ; */
/* #endif */

/* /\* Function used in the macros.  *\/ */
/* struct __jmp_buf_tag; */
/* extern int __sigsetjmp (struct __jmp_buf_tag *__env, int __savemask) __THROW; */
/********** END CLEANUP GNU ******/

/* Mutex handling.  */

/* Initialize a mutex.  */
extern int pmarcel_mutex_init (pmarcel_mutex_t *__mutex,
			       __const pmarcel_mutexattr_t *__mutexattr)
     __THROW __nonnull ((1));

/* Destroy a mutex.  */
extern int pmarcel_mutex_destroy (pmarcel_mutex_t *__mutex)
     __THROW __nonnull ((1));

/* Try locking a mutex.  */
extern int pmarcel_mutex_trylock (pmarcel_mutex_t *__mutex)
     __THROW __nonnull ((1));

/* Lock a mutex.  */
extern int pmarcel_mutex_lock (pmarcel_mutex_t *__mutex)
     __THROW __nonnull ((1));

//#ifdef __USE_XOPEN2K
/* Wait until lock becomes available, or specified time passes. */
extern int pmarcel_mutex_timedlock (pmarcel_mutex_t *__restrict __mutex,
                                    __const struct timespec *__restrict
                                    __abstime) __THROW __nonnull ((1, 2));
//#endif

/* Unlock a mutex.  */
extern int pmarcel_mutex_unlock (pmarcel_mutex_t *__mutex)
     __THROW __nonnull ((1));


#ifdef __USE_UNIX98
/* Get the priority ceiling of MUTEX.  */
extern int pmarcel_mutex_getprioceiling (__const pmarcel_mutex_t *
					 __restrict __mutex,
					 int *__restrict __prioceiling)
     __THROW __nonnull ((1, 2));

/* Set the priority ceiling of MUTEX to PRIOCEILING, return old
   priority ceiling value in *OLD_CEILING.  */
extern int pmarcel_mutex_setprioceiling (pmarcel_mutex_t *__restrict __mutex,
					 int __prioceiling,
					 int *__restrict __old_ceiling)
     __THROW __nonnull ((1, 3));
#endif


#ifdef __USE_XOPEN2K8
/* Declare the state protected by MUTEX as consistent.  */
extern int pmarcel_mutex_consistent_np (pmarcel_mutex_t *__mutex)
     __THROW __nonnull ((1));
# ifdef __USE_GNU
extern int pmarcel_mutex_consistent_np (pmarcel_mutex_t *__mutex)
     __THROW __nonnull ((1));
# endif
#endif


/* Functions for handling mutex attributes.  */

/* Initialize mutex attribute object ATTR with default attributes
   (kind is PMARCEL_MUTEX_TIMED_NP).  */
extern int pmarcel_mutexattr_init (pmarcel_mutexattr_t *__attr)
     __THROW __nonnull ((1));

/* Destroy mutex attribute object ATTR.  */
extern int pmarcel_mutexattr_destroy (pmarcel_mutexattr_t *__attr)
     __THROW __nonnull ((1));

/* Get the process-shared flag of the mutex attribute ATTR.  */
extern int pmarcel_mutexattr_getpshared (__const pmarcel_mutexattr_t *
					 __restrict __attr,
					 int *__restrict __pshared)
     __THROW __nonnull ((1, 2));

/* Set the process-shared flag of the mutex attribute ATTR.  */
extern int pmarcel_mutexattr_setpshared (pmarcel_mutexattr_t *__attr,
					 int __pshared)
     __THROW __nonnull ((1));

//#ifdef __USE_UNIX98
/* Return in *KIND the mutex kind attribute in *ATTR.  */
extern int pmarcel_mutexattr_gettype (__const pmarcel_mutexattr_t *__restrict
				      __attr, int *__restrict __kind)
     __THROW __nonnull ((1, 2));

/* Set the mutex kind attribute in *ATTR to KIND (either PMARCEL_MUTEX_NORMAL,
   PMARCEL_MUTEX_RECURSIVE, PMARCEL_MUTEX_ERRORCHECK, or
   PMARCEL_MUTEX_DEFAULT).  */
extern int pmarcel_mutexattr_settype (pmarcel_mutexattr_t *__attr, int __kind)
     __THROW __nonnull ((1));

/* Return in *PROTOCOL the mutex protocol attribute in *ATTR.  */
extern int pmarcel_mutexattr_getprotocol (__const pmarcel_mutexattr_t *
					  __restrict __attr,
					  int *__restrict __protocol)
     __THROW __nonnull ((1, 2));

/* Set the mutex protocol attribute in *ATTR to PROTOCOL (either
   PMARCEL_PRIO_NONE, PMARCEL_PRIO_INHERIT, or PMARCEL_PRIO_PROTECT).  */
extern int pmarcel_mutexattr_setprotocol (pmarcel_mutexattr_t *__attr,
					  int __protocol)
     __THROW __nonnull ((1));

/* Return in *PRIOCEILING the mutex prioceiling attribute in *ATTR.  */
extern int pmarcel_mutexattr_getprioceiling (__const pmarcel_mutexattr_t *
					     __restrict __attr,
					     int *__restrict __prioceiling)
     __THROW __nonnull ((1, 2));

/* Set the mutex prioceiling attribute in *ATTR to PRIOCEILING.  */
extern int pmarcel_mutexattr_setprioceiling (pmarcel_mutexattr_t *__attr,
					     int __prioceiling)
     __THROW __nonnull ((1));
//#endif

#ifdef __USE_XOPEN2K
/* Get the robustness flag of the mutex attribute ATTR.  */
extern int pmarcel_mutexattr_getrobust (__const pmarcel_mutexattr_t *__attr,
					int *__robustness)
     __THROW __nonnull ((1, 2));
# ifdef __USE_GNU
extern int pmarcel_mutexattr_getrobust_np (__const pmarcel_mutexattr_t *__attr,
					   int *__robustness)
     __THROW __nonnull ((1, 2));
# endif

/* Set the robustness flag of the mutex attribute ATTR.  */
extern int pmarcel_mutexattr_setrobust (pmarcel_mutexattr_t *__attr,
					int __robustness)
     __THROW __nonnull ((1));
# ifdef __USE_GNU
extern int pmarcel_mutexattr_setrobust_np (pmarcel_mutexattr_t *__attr,
					   int __robustness)
     __THROW __nonnull ((1));
# endif
#endif


//#if defined __USE_UNIX98 || defined __USE_XOPEN2K
/* Functions for handling read-write locks.  */

/* Initialize read-write lock RWLOCK using attributes ATTR, or use
   the default values if later is NULL.  */
extern int pmarcel_rwlock_init (pmarcel_rwlock_t *__restrict __rwlock,
				__const pmarcel_rwlockattr_t *__restrict
				__attr) __THROW __nonnull ((1));

/* Destroy read-write lock RWLOCK.  */
extern int pmarcel_rwlock_destroy (pmarcel_rwlock_t *__rwlock)
     __THROW __nonnull ((1));

/* Acquire read lock for RWLOCK.  */
extern int pmarcel_rwlock_rdlock (pmarcel_rwlock_t *__rwlock)
     __THROW __nonnull ((1));

/* Try to acquire read lock for RWLOCK.  */
extern int pmarcel_rwlock_tryrdlock (pmarcel_rwlock_t *__rwlock)
  __THROW __nonnull ((1));

//# ifdef __USE_XOPEN2K
/* Try to acquire read lock for RWLOCK or return after specfied time.  */
extern int pmarcel_rwlock_timedrdlock (pmarcel_rwlock_t *__restrict __rwlock,
				       __const struct timespec *__restrict
				       __abstime) __THROW __nonnull ((1, 2));
//# endif

/* Acquire write lock for RWLOCK.  */
extern int pmarcel_rwlock_wrlock (pmarcel_rwlock_t *__rwlock)
     __THROW __nonnull ((1));

/* Try to acquire write lock for RWLOCK.  */
extern int pmarcel_rwlock_trywrlock (pmarcel_rwlock_t *__rwlock)
     __THROW __nonnull ((1));

//# ifdef __USE_XOPEN2K
/* Try to acquire write lock for RWLOCK or return after specfied time.  */
extern int pmarcel_rwlock_timedwrlock (pmarcel_rwlock_t *__restrict __rwlock,
				       __const struct timespec *__restrict
				       __abstime) __THROW __nonnull ((1, 2));
//# endif

/* Unlock RWLOCK.  */
extern int pmarcel_rwlock_unlock (pmarcel_rwlock_t *__rwlock)
     __THROW __nonnull ((1));


/* Functions for handling read-write lock attributes.  */

/* Initialize attribute object ATTR with default values.  */
extern int pmarcel_rwlockattr_init (pmarcel_rwlockattr_t *__attr)
     __THROW __nonnull ((1));

/* Destroy attribute object ATTR.  */
extern int pmarcel_rwlockattr_destroy (pmarcel_rwlockattr_t *__attr)
     __THROW __nonnull ((1));

/* Return current setting of process-shared attribute of ATTR in PSHARED.  */
extern int pmarcel_rwlockattr_getpshared (__const pmarcel_rwlockattr_t *
					  __restrict __attr,
					  int *__restrict __pshared)
     __THROW __nonnull ((1, 2));

/* Set process-shared attribute of ATTR to PSHARED.  */
extern int pmarcel_rwlockattr_setpshared (pmarcel_rwlockattr_t *__attr,
					  int __pshared)
     __THROW __nonnull ((1));

/* Return current setting of reader/writer preference.  */
extern int pmarcel_rwlockattr_getkind_np (__const pmarcel_rwlockattr_t *
					  __restrict __attr,
					  int *__restrict __pref)
     __THROW __nonnull ((1, 2));

/* Set reader/write preference.  */
extern int pmarcel_rwlockattr_setkind_np (pmarcel_rwlockattr_t *__attr,
					  int __pref) __THROW __nonnull ((1));
//#endif


/* Functions for handling conditional variables.  */

/* Initialize condition variable COND using attributes ATTR, or use
   the default values if later is NULL.  */
extern int pmarcel_cond_init (pmarcel_cond_t *__restrict __cond,
			      __const pmarcel_condattr_t *__restrict
			      __cond_attr) __THROW __nonnull ((1));

/* Destroy condition variable COND.  */
extern int pmarcel_cond_destroy (pmarcel_cond_t *__cond)
     __THROW __nonnull ((1));

/* Wake up one thread waiting for condition variable COND.  */
extern int pmarcel_cond_signal (pmarcel_cond_t *__cond)
     __THROW __nonnull ((1));

/* Wake up all threads waiting for condition variables COND.  */
extern int pmarcel_cond_broadcast (pmarcel_cond_t *__cond)
     __THROW __nonnull ((1));

/* Wait for condition variable COND to be signaled or broadcast.
   MUTEX is assumed to be locked before.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int pmarcel_cond_wait (pmarcel_cond_t *__restrict __cond,
			      pmarcel_mutex_t *__restrict __mutex)
     __nonnull ((1, 2));

/* Wait for condition variable COND to be signaled or broadcast until
   ABSTIME.  MUTEX is assumed to be locked before.  ABSTIME is an
   absolute time specification; zero is the beginning of the epoch
   (00:00:00 GMT, January 1, 1970).

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int pmarcel_cond_timedwait (pmarcel_cond_t *__restrict __cond,
				   pmarcel_mutex_t *__restrict __mutex,
				   __const struct timespec *__restrict
				   __abstime) __nonnull ((1, 2, 3));

/* Functions for handling condition variable attributes.  */

/* Initialize condition variable attribute ATTR.  */
extern int pmarcel_condattr_init (pmarcel_condattr_t *__attr)
     __THROW __nonnull ((1));

/* Destroy condition variable attribute ATTR.  */
extern int pmarcel_condattr_destroy (pmarcel_condattr_t *__attr)
     __THROW __nonnull ((1));

/* Get the process-shared flag of the condition variable attribute ATTR.  */
extern int pmarcel_condattr_getpshared (__const pmarcel_condattr_t *
                                        __restrict __attr,
                                        int *__restrict __pshared)
     __THROW __nonnull ((1, 2));

/* Set the process-shared flag of the condition variable attribute ATTR.  */
extern int pmarcel_condattr_setpshared (pmarcel_condattr_t *__attr,
                                        int __pshared) __THROW __nonnull ((1));

#ifdef __USE_XOPEN2K
/* Get the clock selected for the conditon variable attribute ATTR.  */
extern int pmarcel_condattr_getclock (__const pmarcel_condattr_t *
				      __restrict __attr,
				      __clockid_t *__restrict __clock_id)
     __THROW __nonnull ((1, 2));

/* Set the clock selected for the conditon variable attribute ATTR.  */
extern int pmarcel_condattr_setclock (pmarcel_condattr_t *__attr,
				      __clockid_t __clock_id)
     __THROW __nonnull ((1));
#endif


//#ifdef __USE_XOPEN2K
/* Functions to handle spinlocks.  */

/* Initialize the spinlock LOCK.  If PSHARED is nonzero the spinlock can
   be shared between different processes.  */
extern int pmarcel_spin_init (pmarcel_spinlock_t *__lock, int __pshared)
     __THROW __nonnull ((1));

/* Destroy the spinlock LOCK.  */
extern int pmarcel_spin_destroy (pmarcel_spinlock_t *__lock)
     __THROW __nonnull ((1));

/* Wait until spinlock LOCK is retrieved.  */
extern int pmarcel_spin_lock (pmarcel_spinlock_t *__lock)
     __THROW __nonnull ((1));

/* Try to lock spinlock LOCK.  */
extern int pmarcel_spin_trylock (pmarcel_spinlock_t *__lock)
     __THROW __nonnull ((1));

/* Release spinlock LOCK.  */
extern int pmarcel_spin_unlock (pmarcel_spinlock_t *__lock)
     __THROW __nonnull ((1));


/* Functions to handle barriers.  */

/* Initialize BARRIER with the attributes in ATTR.  The barrier is
   opened when COUNT waiters arrived.  */
extern int pmarcel_barrier_init (pmarcel_barrier_t *__restrict __barrier,
				 __const pmarcel_barrierattr_t *__restrict
				 __attr, unsigned int __count)
     __THROW __nonnull ((1));

/* Destroy a previously dynamically initialized barrier BARRIER.  */
extern int pmarcel_barrier_destroy (pmarcel_barrier_t *__barrier)
     __THROW __nonnull ((1));

/* Wait on barrier BARRIER.  */
extern int pmarcel_barrier_wait (pmarcel_barrier_t *__barrier)
     __THROW __nonnull ((1));


/* Initialize barrier attribute ATTR.  */
extern int pmarcel_barrierattr_init (pmarcel_barrierattr_t *__attr)
     __THROW __nonnull ((1));

/* Destroy previously dynamically initialized barrier attribute ATTR.  */
extern int pmarcel_barrierattr_destroy (pmarcel_barrierattr_t *__attr)
     __THROW __nonnull ((1));

/* Get the process-shared flag of the barrier attribute ATTR.  */
extern int pmarcel_barrierattr_getpshared (__const pmarcel_barrierattr_t *
					   __restrict __attr,
					   int *__restrict __pshared)
     __THROW __nonnull ((1, 2));

/* Set the process-shared flag of the barrier attribute ATTR.  */
extern int pmarcel_barrierattr_setpshared (pmarcel_barrierattr_t *__attr,
                                           int __pshared)
     __THROW __nonnull ((1));
//#endif


/* Functions for handling thread-specific data.  */

/* Create a key value identifying a location in the thread-specific
   data area.  Each thread maintains a distinct thread-specific data
   area.  DESTR_FUNCTION, if non-NULL, is called with the value
   associated to that key when the key is destroyed.
   DESTR_FUNCTION is not called if the value associated is NULL when
   the key is destroyed.  */
extern int pmarcel_key_create (pmarcel_key_t *__key,
			       void (*__destr_function) (void *))
     __THROW __nonnull ((1));

/* Destroy KEY.  */
extern int pmarcel_key_delete (pmarcel_key_t __key) __THROW;

/* Return current value of the thread-specific data slot identified by KEY.  */
extern void *pmarcel_getspecific (pmarcel_key_t __key) __THROW;

/* Store POINTER in the thread-specific data slot identified by KEY. */
extern int pmarcel_setspecific (pmarcel_key_t __key,
				__const void *__pointer) __THROW ;


#ifdef __USE_XOPEN2K
/* Get ID of CPU-time clock for thread THREAD_ID.  */
extern int pmarcel_getcpuclockid (pmarcel_t __thread_id,
				  __clockid_t *__clock_id)
     __THROW __nonnull ((2));
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


/* #ifdef __USE_EXTERN_INLINES */
/* /\* Optimizations.  *\/ */
/* __extern_inline int */
/* __NTH (pmarcel_equal (pmarcel_t __thread1, pmarcel_t __thread2)) */
/* { */
/*   return __thread1 == __thread2; */
/* } */
/* #endif */

#define pmarcel_equal(t1, t2) ((t1) == (t2))


/************** semaphore.h ***************/


/* Initialize semaphore object SEM to VALUE.  If PSHARED then share it
   with other processes.  */
extern int pmarcel_sem_init (pmarcel_sem_t *__sem, int __pshared, unsigned int __value)
     __THROW;
/* Free resources associated with semaphore object SEM.  */
extern int pmarcel_sem_destroy (pmarcel_sem_t *__sem) __THROW;

/* Open a named semaphore NAME with open flags OFLAG.  */
extern pmarcel_sem_t *pmarcel_sem_open (__const char *__name, int __oflag, ...) __THROW;

/* Close descriptor for named semaphore SEM.  */
extern int pmarcel_sem_close (pmarcel_sem_t *__sem) __THROW;

/* Remove named semaphore NAME.  */
extern int pmarcel_sem_unlink (__const char *__name) __THROW;

/* Wait for SEM being posted.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int pmarcel_sem_wait (pmarcel_sem_t *__sem);

//#ifdef __USE_XOPEN2K
/* Similar to `sem_wait' but wait only until ABSTIME.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int pmarcel_sem_timedwait (pmarcel_sem_t *__restrict __sem,
				  __const struct timespec *__restrict __abstime);
//#endif

/* Test whether SEM is posted.  */
extern int pmarcel_sem_trywait (pmarcel_sem_t *__sem) __THROW;

/* Post SEM.  */
extern int pmarcel_sem_post (pmarcel_sem_t *__sem) __THROW;

/* Get current value of SEM and store it in *SVAL.  */
extern int pmarcel_sem_getvalue (pmarcel_sem_t *__restrict __sem, int *__restrict __sval)
	__THROW;


/************** signals ***************/

#define pmarcel_sigemptyset     marcel_sigemptyset
#define pmarcel_sigfillset      marcel_sigfillset
#define pmarcel_sigisemptyset   marcel_sigisemptyset
#define pmarcel_sigisfullset    marcel_sigisfullset
#define pmarcel_sigismember     marcel_sigismember
#define pmarcel_sigaddset       marcel_sigaddset
#define pmarcel_sigdelset       marcel_sigdelset
#define pmarcel_sigorset        marcel_sigorset
#define pmarcel_sigorismember   marcel_sigorismember
#define pmarcel_signandset      marcel_signandset
#define pmarcel_signandisempty  marcel_signandisempty
#define pmarcel_signandismember marcel_signandismember
#define pmarcel_sigequalset     marcel_sigequalset

int pmarcel_getitimer(int which, struct itimerval *value);
int pmarcel_setitimer(int which, const struct itimerval *value, struct itimerval *ovalue);

int pmarcel_raise(int sig);
int pmarcel_pause(void);
unsigned int pmarcel_alarm(unsigned int nb_sec);
void *pmarcel_signal(int sig, void *handler);
int pmarcel_kill(pmarcel_t thread, int sig);
int pmarcel_sigmask(int how, const marcel_sigset_t *set, marcel_sigset_t *oldset);

int pmarcel_sigpending(marcel_sigset_t * set);
int pmarcel_sigtimedwait(const marcel_sigset_t * __restrict set, siginfo_t * __restrict info,
			 const struct timespec *__restrict timeout);
int pmarcel_sigwait(const marcel_sigset_t * __restrict set, int *__restrict sig);
int pmarcel_sigwaitinfo(const marcel_sigset_t * __restrict set, siginfo_t * __restrict info);
int pmarcel_sigsuspend(const marcel_sigset_t * sigmask);
int pmarcel_sigaction(int sig, const struct marcel_sigaction *act, struct marcel_sigaction *oact);
int pmarcel_sighold(int sig);
int pmarcel_sigrelse(int sig);
int pmarcel_sigignore(int sig);
int pmarcel_sigpause(int mask);
int pmarcel_xpg_sigpause(int sig);

int TBX_RETURNS_TWICE pmarcel_sigsetjmp(sigjmp_buf env, int savemask);
void TBX_NORETURN pmarcel_siglongjmp(sigjmp_buf env, int val);


__TBX_END_DECLS


#endif /** MA__IFACE_PMARCEL **/


#endif /** __PMARCEL_H__ **/
