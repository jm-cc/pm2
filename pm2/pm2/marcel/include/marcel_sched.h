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

______________________________________________________________________________
$Log: marcel_sched.h,v $
Revision 1.4  2000/02/28 10:26:38  rnamyst
Changed #include <> into #include "".

Revision 1.3  2000/01/31 15:56:26  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef MARCEL_SCHED_EST_DEF
#define MARCEL_SCHED_EST_DEF


/* ==== Starting and Shuting down the scheduler ==== */

void marcel_sched_init(unsigned nb_lwp);
void marcel_sched_shutdown(void);


/* ==== get current thread or LWP id ==== */

static __inline__ marcel_t marcel_self()
{
  return __marcel_self();
}

static __inline__ unsigned marcel_current_vp()
{
#ifdef SMP
  __lwp_t *cur_lwp = marcel_self()->lwp;
#endif

  return cur_lwp->number;
}


/* ==== explicit preemtion ==== */

void marcel_yield(void);
void marcel_trueyield(void);
void marcel_explicityield(marcel_t pid);


/* ==== SMP scheduling policies ==== */

#define MARCEL_MAX_USER_SCHED    16

#define MARCEL_SCHED_FIXED(vp)   (vp)
#define MARCEL_SCHED_OTHER       (-1)
#define MARCEL_SCHED_AFFINITY    (-2)
#define MARCEL_SCHED_BALANCE     (-3)
#define __MARCEL_SCHED_AVAILABLE (-4)

typedef __lwp_t *(*marcel_schedpolicy_func_t)(marcel_t pid,
					      __lwp_t *current_lwp);
void marcel_schedpolicy_create(int *policy, marcel_schedpolicy_func_t func);


/* ==== priority management ==== */

int marcel_setprio(marcel_t pid, unsigned prio);
int marcel_getprio(marcel_t pid, unsigned *prio);


/* ==== `sleep' functions ==== */

void marcel_delay(unsigned long millisecs);


/* ==== scheduler status ==== */

unsigned marcel_nbvps(void);
unsigned marcel_nbthreads(void);
unsigned marcel_activethreads(void);
unsigned marcel_sleepingthreads(void);
unsigned marcel_blockedthreads(void);
unsigned marcel_frozenthreads(void);
unsigned long marcel_createdthreads(void);


/* ==== preemption directed to one special thread ==== */

void marcel_setspecialthread(marcel_t pid);

void marcel_givehandback(void);


/* ==== snapshot ==== */

typedef void (*snapshot_func_t)(marcel_t pid);

void marcel_snapshot(snapshot_func_t f);

#define ALL_THREADS		0
#define MIGRATABLE_ONLY		1
#define NOT_MIGRATABLE_ONLY	2
#define DETACHED_ONLY		4
#define NOT_DETACHED_ONLY	8
#define BLOCKED_ONLY		16
#define NOT_BLOCKED_ONLY	32
#define SLEEPING_ONLY           64
#define NOT_SLEEPING_ONLY      128

void marcel_threadslist(int max, marcel_t *pids, int *nb, int which);


/* ==== polling  ==== */

#define MAX_POLL_IDS    16

_PRIVATE_ struct _poll_struct;
_PRIVATE_ struct _poll_cell_struct;

typedef struct _poll_struct *marcel_pollid_t;
typedef struct _poll_cell_struct *marcel_pollinst_t;

typedef void (*marcel_pollgroup_func_t)(marcel_pollid_t id);

typedef void *(*marcel_poll_func_t)(marcel_pollid_t id,
				    unsigned active,
				    unsigned sleeping,
				    unsigned blocked);

marcel_pollid_t marcel_pollid_create(marcel_pollgroup_func_t g,
				     marcel_poll_func_t f,
				     int divisor);

#define MARCEL_POLL_FAILED                NULL
#define MARCEL_POLL_SUCCESS(id)           ((id)->cur_cell)
#define MARCEL_POLL_SUCCESS_FOR(pollinst) (pollinst)

void marcel_poll(marcel_pollid_t id, any_t arg);

#define FOREACH_POLL(id, _arg) \
  for((id)->cur_cell = (id)->first_cell; \
      (id)->cur_cell != NULL && (_arg = (typeof(_arg))((id)->cur_cell->arg)); \
      (id)->cur_cell = (id)->cur_cell->next)

#define GET_CURRENT_POLLINST(id) ((id)->cur_cell)

/* ==== Scheduler locking ==== */

/* Must be called each time a LWP is about to access its local task
   queue. */
static __inline__ void sched_lock(__lwp_t *lwp)
{
  marcel_lock_acquire(&lwp->sched_queue_lock);
}

/* Must be called when a LWP is not modifying the local queue any
   more. */
static __inline__ void sched_unlock(__lwp_t *lwp)
{
  marcel_lock_release(&lwp->sched_queue_lock);
}

#ifdef X86_ARCH

#ifdef __ACT__
#include "sys/upcalls.h"
#endif

static __inline__ void lock_task()
{
#ifdef SMP
  atomic_inc(&marcel_self()->lwp->_locked);
#elif defined(__ACT__)
  marcel_t self=marcel_self();
  if (! self->marcel_lock) {
    act_lock(self);
  }
  self->marcel_lock++;
#else
  atomic_inc(&cur_lwp->_locked);
#endif
}

static __inline__ void unlock_task()
{
#ifdef SMP
  atomic_dec(&marcel_self()->lwp->_locked);
#elif defined(__ACT__)
  marcel_t self=marcel_self();
  self->marcel_lock--;  
  if (! self->marcel_lock) {
    act_unlock(self);
  }
#else
  atomic_dec(&cur_lwp->_locked);
#endif
}

static __inline__ int locked()
{
#ifdef SMP
  return atomic_read(&marcel_self()->lwp->_locked);
#elif defined(__ACT__)
  return marcel_self()->marcel_lock;
#else
  return atomic_read(&cur_lwp->_locked);
#endif
}

#else

static __inline__ __volatile__ void lock_task(void)
{
#ifndef SMP
  cur_lwp->_locked++;
#else
  marcel_self()->lwp->_locked++;
#endif
}

static __inline__ __volatile__ void unlock_task(void)
{
#ifndef SMP
  cur_lwp->_locked--;
#else
  marcel_self()->lwp->_locked--;
#endif
}

static __inline__ __volatile__ unsigned locked(void)
{
#ifndef SMP
  return cur_lwp->_locked;
#else
  return marcel_self()->lwp->_locked;
#endif
}

#endif

/* ==== miscelaneous private defs ==== */

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS
_PRIVATE_ extern volatile marcel_t __waiting_tasks;
#endif
_PRIVATE_ extern volatile marcel_t __delayed_tasks;

_PRIVATE_ void marcel_one_task_less(marcel_t pid);
_PRIVATE_ void marcel_one_more_task(marcel_t pid);

_PRIVATE_ void marcel_give_hand(boolean *blocked, marcel_lock_t *lock);
_PRIVATE_ void marcel_tempo_give_hand(unsigned long timeout, boolean *blocked, marcel_sem_t *s);
_PRIVATE_ void marcel_wake_task(marcel_t t, boolean *blocked);
_PRIVATE_ marcel_t marcel_unchain_task(marcel_t t);
_PRIVATE_ void marcel_insert_task(marcel_t t);
_PRIVATE_ marcel_t marcel_radical_next_task(void);

_PRIVATE_ typedef struct _poll_struct {
  marcel_pollgroup_func_t gfunc;
  marcel_poll_func_t func;
  unsigned divisor, count;
  struct _poll_cell_struct *first_cell, *cur_cell;
  struct _poll_struct *prev, *next;
} poll_struct_t;

_PRIVATE_ typedef struct _poll_cell_struct {
  marcel_t task;
  boolean blocked;
  any_t arg;
  struct _poll_cell_struct *prev, *next;
} poll_cell_t;

#ifdef USE_VIRTUAL_TIMER

#define MARCEL_TIMER_SIGNAL   SIGVTALRM
#define MARCEL_ITIMER_TYPE    ITIMER_VIRTUAL

#else

#define MARCEL_TIMER_SIGNAL   SIGALRM
#define MARCEL_ITIMER_TYPE    ITIMER_REAL

#endif

#endif
