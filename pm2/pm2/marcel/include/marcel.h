/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#ifndef MARCEL_EST_DEF
#define MARCEL_EST_DEF

/*
 * The definition of USE_MACROS will improve performances of some
 * commonly used functions. However, some debugging facilities
 * (contraints checking) will be lost (use at your own risk !).
 */

#define _PRIVATE_

#include <stdio.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/time.h>
#ifdef AIX_SYS
#include <sys/select.h>
#endif

#ifndef FALSE
	typedef enum { FALSE, TRUE } boolean;
#else
	typedef int boolean;
#endif


/* ========== customization =========== */

#define MAX_KEY_SPECIFIC	10

#define MAX_STACK_CACHE		1024

#define MAX_PRIO		100

#define MAX_CLEANUP_FUNCS	5

/* Only meaningful when the SMP flag is on */
#define MAX_LWP                 16

#define THREAD_THRESHOLD_LOW    1

/* ============ constants ============= */

#define NO_TIME_OUT		-1

#define NO_TIME_SLICE		0

#define DEFAULT_STACK		0

#define MIN_PRIO		1

#define STD_PRIO		MIN_PRIO


/* ================= included files ================= */

typedef void *any_t;

typedef any_t (*marcel_func_t)(any_t);
typedef void (*cleanup_func_t)(any_t);
typedef void (*handler_func_t)(any_t);

_PRIVATE_ struct task_desc_struct;
_PRIVATE_ typedef struct task_desc_struct *marcel_t;

#include <sys/archdep.h>
#include <sys/archsetjmp.h>
#include <sys/marcel_trace.h>
#include <sys/debug.h>
#include <exception.h>
#include <marcel_lock.h>
#include <marcel_sem.h>
#include <marcel_mutex.h>
#include <marcel_io.h>
#include <sys/privatedefs.h>
#include <marcel_sched.h>
#include <marcel_attr.h>
#include <mar_timing.h>


/* = initialization & termination == */

void marcel_init(int *argc, char *argv[]);

void marcel_end(void);

void marcel_strip_cmdline(int *argc, char *argv[]);

/* ============ threads ============ */

int marcel_create(marcel_t *pid, marcel_attr_t *attr, marcel_func_t func, any_t arg);

int marcel_join(marcel_t pid, any_t *status);

int marcel_exit(any_t val);

int marcel_detach(marcel_t pid);

int marcel_cancel(marcel_t pid);

static __inline__ int marcel_equal(pid1, pid2)
{
  return (pid1 == pid2);
}

void marcel_freeze(marcel_t *pids, int nb);
void marcel_unfreeze(marcel_t *pids, int nb);


/* === stack user space === */

void marcel_getuserspace(marcel_t pid, void **user_space);

void marcel_run(marcel_t pid, any_t arg);


/* ====== specific data ====== */

typedef int marcel_key_t;

int marcel_key_create(marcel_key_t *key, void (*func)(any_t));

_PRIVATE_ extern volatile unsigned _nb_keys;
static __inline__ int marcel_setspecific(marcel_key_t key, any_t value)
{
#ifdef DEBUG
   if((key < 0) || (key >= _nb_keys))
      RAISE(CONSTRAINT_ERROR);
#endif
   marcel_self()->key[key] = value;
   return 0;
}

static __inline__ any_t marcel_getspecific(marcel_key_t key)
{
#ifdef DEBUG
   if((key < 0) || (key >= _nb_keys))
      RAISE(CONSTRAINT_ERROR);
#endif
   return marcel_self()->key[key];
}

static __inline__ any_t* marcel_specificdatalocation(marcel_t pid, marcel_key_t key)
{
#ifdef DEBUG
   if((key < 0) || (key >= _nb_keys))
      RAISE(CONSTRAINT_ERROR);
#endif
   return &pid->key[key];
}

/* ========== once objects ============ */

#define marcel_once_init { 0, { 1, NULL, NULL } }

int marcel_once(marcel_once_t *once, void (*f)(void));


/* ========== callbacks ============ */

int marcel_cleanup_push(cleanup_func_t func, any_t arg);
int marcel_cleanup_pop(boolean execute);


/* ========= deviations ========== */

void marcel_enabledeviation(void);
void marcel_disabledeviation(void);

void marcel_deviate(marcel_t pid, handler_func_t h, any_t arg);


/* =========== migration =========== */

typedef void (*transfert_func_t)(marcel_t t, unsigned long depl, unsigned long blksize, void *arg);

typedef void (*post_migration_func_t)(void *arg);

static __inline__ void marcel_disablemigration(marcel_t pid)
{
  pid->not_migratable++;
}

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

static __inline__ char *marcel_stackbase(marcel_t pid)
{
  return pid->stack_base;
}

/* ========== time slicing ========= */

void marcel_settimeslice(unsigned long microsecs);


/* ============= miscellaneous ============ */

unsigned long marcel_cachedthreads(void);


/* ======= MT-Safe functions from standard library ======= */

void *tmalloc(unsigned size);
void *trealloc(void *ptr, unsigned size);
void *tcalloc(unsigned nelem, unsigned elsize);
void tfree(void *ptr);

#define tprintf  marcel_printf
#define tfprintf marcel_fprintf

int tprintf(char *format, ...);
int tfprintf(FILE *stream, char *format, ...);

int tselect(int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds);

#ifdef STANDARD_MAIN
#define marcel_main main
#endif

#ifndef ACTDEBUG
#define ACTDEBUG(todo)
#endif

#endif
