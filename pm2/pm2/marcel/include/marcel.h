
#ifndef MARCEL_EST_DEF
#define MARCEL_EST_DEF


#define _PRIVATE_

#include <errno.h>
#include <stdio.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/time.h>

#ifdef AIX_SYS
#include <time.h>
#endif

#include "pm2debug.h"

#ifndef FALSE
	typedef enum { FALSE, TRUE } boolean;
#else
	typedef int boolean;
#endif


/* ========== customization =========== */

#define MAX_KEY_SPECIFIC	20

#define MAX_STACK_CACHE		1024

#define MAX_PRIO		100

#define MAX_CLEANUP_FUNCS	5

/* Only meaningful when the SMP or ACT flag is on */
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

#ifdef MA__ACTIVATION
#include <asm/act.h>
#undef MAX_LWP
#define MAX_LWP ACT_NB_MAX_CPUS
#endif

#include "sys/marcel_flags.h"
#include "sys/archdep.h"
#include "sys/archsetjmp.h"
#include "sys/marcel_debug.h"
#include "exception.h"
#include "marcel_lock.h"
#include "marcel_sem.h"
#include "marcel_mutex.h"
#include "marcel_io.h"
#include "sys/privatedefs.h"
#include "marcel_sched.h"
#include "marcel_polling.h"
#include "marcel_attr.h"
#include "mar_timing.h"
#include "marcel_stdio.h"

#include "tbx.h"
#include "profile.h"
#include "pm2debug.h"
#include "common.h"

/* = initialization & termination == */

#define marcel_init(argc, argv) common_init(argc, argv)

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

int marcel_create(marcel_t *pid, marcel_attr_t *attr, marcel_func_t func, any_t arg);

int marcel_join(marcel_t pid, any_t *status);

int marcel_exit(any_t val);

int marcel_detach(marcel_t pid);

int marcel_cancel(marcel_t pid);

static __inline__ int marcel_equal(marcel_t pid1, marcel_t pid2) __attribute__ ((unused));
static __inline__ int marcel_equal(marcel_t pid1, marcel_t pid2)
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

int marcel_key_create(marcel_key_t *key, marcel_key_destructor_t any_t);
int marcel_key_delete(marcel_key_t key);

_PRIVATE_ extern volatile unsigned _nb_keys;
static __inline__ int marcel_setspecific(marcel_key_t key, any_t value) __attribute__ ((unused));
static __inline__ int marcel_setspecific(marcel_key_t key, any_t value)
{
#ifdef MA__DEBUG
   if((key < 0) || (key >= marcel_nb_keys))
      RAISE(CONSTRAINT_ERROR);
#endif
   marcel_self()->key[key] = value;
   return 0;
}

extern int marcel_key_present[MAX_KEY_SPECIFIC];

static __inline__ any_t marcel_getspecific(marcel_key_t key) __attribute__ ((unused));
static __inline__ any_t marcel_getspecific(marcel_key_t key)
{
#ifdef MA__DEBUG
   if((key < 0) || (key>=MAX_KEY_SPECIFIC) || (!marcel_key_present[key]))
      RAISE(CONSTRAINT_ERROR);
#endif
   return marcel_self()->key[key];
}

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

/* ========== time slicing ========= */

void marcel_settimeslice(unsigned long microsecs);


/* ============= miscellaneous ============ */

unsigned long marcel_cachedthreads(void);


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
extern int marcel_main(int argc, char *argv[]);
#endif

#endif
