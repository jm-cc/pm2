
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

#ifndef MARCEL_EST_DEF
#define MARCEL_EST_DEF

#define _GNU_SOURCE 1
#include "sys/marcel_flags.h"
#ifdef LINUX_SYS // AD:
#include <features.h>
#endif

#ifdef MARCEL_COMPILE_INLINE_FUNCTIONS
#  define MARCEL_INLINE
#else
#  define MARCEL_INLINE inline
#endif

#include "marcel_pthread.h"
#ifndef __attribute_deprecated__
#  if (__GNUC__ >= 3)
#    define __attribute_deprecated__ __attribute__((deprecated))
#  else
#    define __attribute_deprecated__
#  endif
#endif
#if (__GNUC__ < 3)
#  define __builtin_expect(x, y) (x)
#endif
#ifdef MA__POSIX_FUNCTIONS_NAMES
#  include "marcel_pmarcel.h"
#endif 
#include "marcel_alias.h"

#define _PRIVATE_

#include <errno.h>
#include <stdio.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/time.h>

#ifdef AIX_SYS
#include <time.h>
#endif

#include "tbx_debug.h"

#if 0
typedef enum { FALSE, TRUE } boolean;
#else
#if !(defined(FALSE) || defined(TRUE)) // AD: TRUE/FALSE are sometimes already defined in libc
enum { FALSE, TRUE };
#endif
typedef int boolean;
#endif

#ifdef MA__POSIX_FUNCTIONS_NAMES
#  include "pthread_libc-symbols.h"
#endif

/* ========== customization =========== */

#define MAX_KEY_SPECIFIC	20

#define MAX_STACK_CACHE		1024

#define MAX_CLEANUP_FUNCS	5

/* Only meaningful when the SMP or ACT flag is on */
#define MAX_LWP                 16

#define THREAD_THRESHOLD_LOW    1

/* ============ constants ============= */

#define NO_TIME_OUT		-1

#define NO_TIME_SLICE		0

#define DEFAULT_STACK		0


/* ================= included files ================= */

typedef void *any_t;

typedef any_t (*marcel_func_t)(any_t);
typedef void (*cleanup_func_t)(any_t);
typedef void (*handler_func_t)(any_t);

#ifdef MA__ACTIVATION
#include <asm/act.h>
#undef MAX_LWP
#define MAX_LWP ACT_NB_MAX_CPU
#endif

#include "sys/marcel_archdep.h"
#include "sys/marcel_sig.h"
#include "sys/marcel_archsetjmp.h"
#include "sys/marcel_debug.h"
#include "marcel_exception.h"
#include "marcel_lock.h"
#include "marcel_sem.h"
#include "marcel_mutex.h"
#include "marcel_cond.h"
#include "marcel_rwlock.h"
#include "marcel_io.h"
#include "sys/marcel_privatedefs.h"
#include "marcel_sched.h"
#include "marcel_polling.h"
#include "marcel_attr.h"
#include "marcel_timing.h"
#include "marcel_stdio.h"

#include "tbx.h"
#include "pm2_profile.h"
#include "tbx_debug.h"

#include "pm2_common.h"

/* = initialization & termination == */

#define marcel_init(argc, argv) common_init(argc, argv, NULL)

// When completed, calls to marcel_self() are ok, etc.
// So do calls to the Unix Fork primitive.
void marcel_init_data(int *argc, char *argv[]);

// May start some internal threads or activations.
// When completed, fork calls are prohibited.
void marcel_start_sched(int *argc, char *argv[]);

void marcel_purge_cmdline(int *argc, char *argv[]);

void marcel_end(void);

void marcel_strip_cmdline(int *argc, char *argv[]);

/* ============ threads ============ */

int marcel_create(marcel_t *pid, __const marcel_attr_t *attr, marcel_func_t func, any_t arg) __THROW;

DEC_MARCEL_POSIX(int, join, (marcel_t pid, any_t *status) __THROW);

DEC_MARCEL_POSIX(void, exit, (any_t val) __THROW);

DEC_MARCEL_POSIX(int, detach, (marcel_t pid) __THROW);

DEC_MARCEL_POSIX(int, cancel, (marcel_t pid) __THROW);

extern MARCEL_INLINE int marcel_equal(marcel_t pid1, marcel_t pid2) __THROW
{
  return (pid1 == pid2);
}

void marcel_freeze(marcel_t *pids, int nb);
void marcel_unfreeze(marcel_t *pids, int nb);


/* === stack user space === */

void marcel_getuserspace(marcel_t pid, void **user_space);

void marcel_run(marcel_t pid, any_t arg);


/* ====== specific data ====== */

/*typedef int marcel_key_t;*/

DEC_MARCEL_POSIX(int, key_create, (marcel_key_t *key, 
				   marcel_key_destructor_t any_t) __THROW);
DEC_MARCEL_POSIX(int, key_delete, (marcel_key_t key) __THROW);

_PRIVATE_ extern volatile unsigned _nb_keys;
#ifdef MA__DEBUG
DECINLINE_MARCEL_POSIX(int, setspecific, (marcel_key_t key,
					  __const void* value),
{
   if((key < 0) || (key >= marcel_nb_keys))
      RAISE(CONSTRAINT_ERROR);
   marcel_self()->key[key] = (any_t)value;
   return 0;
})
DECINLINE_MARCEL_POSIX(any_t, getspecific, (marcel_key_t key),
{
   if((key < 0) || (key>=MAX_KEY_SPECIFIC) || (!marcel_key_present[key]))
      RAISE(CONSTRAINT_ERROR);
   return marcel_self()->key[key];
})
#else
DECINLINE_MARCEL_POSIX(int, setspecific, (marcel_key_t key,
					  __const void* value) __THROW,
{
   marcel_self()->key[key] = (any_t)value;
   return 0;
})
DECINLINE_MARCEL_POSIX(any_t, getspecific, (marcel_key_t key) __THROW,
{
   return marcel_self()->key[key];
})
#endif

extern int marcel_key_present[MAX_KEY_SPECIFIC];


static __inline__ any_t* marcel_specificdatalocation(marcel_t pid, marcel_key_t key) __attribute__ ((unused));
static __inline__ any_t* marcel_specificdatalocation(marcel_t pid, marcel_key_t key)
{
#ifdef MA__DEBUG
   if((key < 0) || (key>=MAX_KEY_SPECIFIC) || (!marcel_key_present[key]))
      RAISE(CONSTRAINT_ERROR);
#endif
   return &pid->key[key];
}

/* ========== once objects ============ */

#define marcel_once_init { 0 }

/* ========== callbacks ============ */

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

/* =========== migration =========== */

typedef void (*transfert_func_t)(marcel_t t, unsigned long depl, unsigned long blksize, void *arg);

typedef void (*post_migration_func_t)(void *arg);

static __inline__ void marcel_disablemigration(marcel_t pid) __attribute__ ((unused));
static __inline__ void marcel_disablemigration(marcel_t pid)
{
  pid->not_migratable++;
}

static __inline__ void marcel_enablemigration(marcel_t pid) __attribute__ ((unused));
static __inline__ void marcel_enablemigration(marcel_t pid)
{
  pid->not_migratable--;
}

void marcel_begin_hibernation(marcel_t t, transfert_func_t transf, void *arg, boolean fork);

void marcel_end_hibernation(marcel_t t, post_migration_func_t f, void *arg);

_PRIVATE_ int marcel_testiflocal(marcel_t t, long *adr);


/* ============== stack ============= */

marcel_t marcel_alloc_stack(unsigned size);

unsigned long marcel_usablestack(void);

unsigned long marcel_unusedstack(void);

static __inline__ char *marcel_stackbase(marcel_t pid) __attribute__ ((unused));
static __inline__ char *marcel_stackbase(marcel_t pid)
{
  return (char *)pid->stack_base;
}


/* ============= miscellaneous ============ */

unsigned long marcel_cachedthreads(void);
unsigned marcel_get_nb_lwps_np(void);

_PRIVATE_ int  marcel_test_activity(void);
_PRIVATE_ void marcel_set_activity(void);
_PRIVATE_ void marcel_clear_activity(void);


/* ======= MT-Safe functions from standard library ======= */

#define tmalloc(size)          marcel_malloc(size, __FILE__, __LINE__)
#define trealloc(ptr, size)    marcel_realloc(ptr, size, __FILE__, __LINE__)
#define tcalloc(nelem, elsize) marcel_calloc(nelem, elsize, __FILE__, __LINE__)
#define tfree(ptr)             marcel_free(ptr, __FILE__, __LINE__)

void *marcel_malloc(unsigned size, char *file, unsigned line);
void *marcel_realloc(void *ptr, unsigned size, char *file, unsigned line);
void *marcel_calloc(unsigned nelem, unsigned elsize, char *file, unsigned line);
void marcel_free(void *ptr, char *file, unsigned line);

#ifdef STANDARD_MAIN
#define marcel_main main
#else
#ifdef MARCEL_MAIN_AS_FUNC
int go_marcel_main(int argc, char *argv[]);
#endif
#ifdef MARCEL_KERNEL
extern int marcel_main(int argc, char *argv[]);
#endif // MARCEL_KERNEL
#endif

#endif // MARCEL_EST_DEF
