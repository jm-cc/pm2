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
Revision 1.13  2000/05/29 08:59:23  vdanjean
work added (mainly for SMP and ACT), minor modif in polling

Revision 1.12  2000/05/25 00:23:50  vdanjean
marcel_poll with sisci and few bugs fixes

Revision 1.11  2000/05/09 10:52:42  vdanjean
pm2debug module

Revision 1.10  2000/04/21 11:19:26  vdanjean
fixes for actsmp

Revision 1.9  2000/04/17 08:31:05  rnamyst
Changed DEBUG into MA__DEBUG.

Revision 1.8  2000/04/11 09:07:12  rnamyst
Merged the "reorganisation" development branch.

Revision 1.7.2.12  2000/04/11 08:17:10  rnamyst
Comments are back !

Revision 1.7.2.11  2000/04/10 09:56:06  rnamyst
Moved the polling stuff into marcel_polling....

Revision 1.7.2.10  2000/04/08 15:09:11  vdanjean
few bugs fixed

Revision 1.7.2.9  2000/03/31 18:38:37  vdanjean
Activation mono OK

Revision 1.7.2.8  2000/03/31 08:06:59  rnamyst
Added disable_preemption() and enable_preemption().

Revision 1.7.2.7  2000/03/29 09:46:17  vdanjean
*** empty log message ***

Revision 1.7.2.5  2000/03/24 17:55:08  vdanjean
fixes

Revision 1.7.2.4  2000/03/22 16:34:01  vdanjean
*** empty log message ***

Revision 1.7.2.2  2000/03/15 15:54:48  vdanjean
réorganisation de marcel : commit pour CVS

Revision 1.7.2.1  2000/03/15 15:41:14  vdanjean
réorganisation de marcel. branche de développement

Revision 1.7  2000/03/09 11:07:34  rnamyst
Modified to use the sched_data() macro.

Revision 1.6  2000/03/06 14:55:44  rnamyst
Modified to include "marcel_flags.h".

Revision 1.5  2000/03/01 16:45:23  oaumage
- suppression des warnings en compilation  -g

Revision 1.4  2000/02/28 10:26:38  rnamyst
Changed #include <> into #include "".

Revision 1.3  2000/01/31 15:56:26  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#ifndef MARCEL_SCHED_EST_DEF
#define MARCEL_SCHED_EST_DEF

#include "sys/marcel_flags.h"
#include "sys/marcel_macros.h"
#include "sys/marcel_work.h"

/* ==== Starting and Shuting down the scheduler ==== */

void marcel_sched_init(unsigned nb_lwp);
void marcel_sched_shutdown(void);


/* ==== get current thread or LWP id ==== */

static __inline__ marcel_t marcel_self()
{
  return __marcel_self();
}

static __inline__ unsigned marcel_current_vp() __attribute__ ((unused));
static __inline__ unsigned marcel_current_vp()
{
#ifdef MA__LWPS
  __lwp_t *cur_lwp = marcel_self()->lwp;
#endif

  return cur_lwp->number;
}


/* ==== explicit preemtion ==== */

void marcel_yield(void);
void marcel_trueyield(void);
int marcel_explicityield(marcel_t pid);


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


/* ==== Scheduler locking ==== */

/* Must be called each time a LWP is about to access its local task
   queue. */
static __inline__ void ma_sched_lock(__lwp_t *lwp) __attribute__ ((unused));
static __inline__ void ma_sched_lock(__lwp_t *lwp)
{
  marcel_lock_acquire(&(SCHED_DATA(lwp).sched_queue_lock));
}

/* Must be called when a LWP is not modifying the local queue any
   more. */
static __inline__ void ma_sched_unlock(__lwp_t *lwp) __attribute__ ((unused));
static __inline__ void ma_sched_unlock(__lwp_t *lwp)
{
  marcel_lock_release(&(SCHED_DATA(lwp).sched_queue_lock));
}

#ifdef X86_ARCH

static __inline__ void ma_lock_task(void) __attribute__ ((unused));
static __inline__ void ma_lock_task(void)
{
  atomic_inc(&GET_LWP(marcel_self())->_locked);
}

static __inline__ void ma_unlock_task(void) __attribute__ ((unused));
static __inline__ void ma_unlock_task(void)
{
  volatile atomic_t *locked=&GET_LWP(marcel_self())->_locked;
#ifdef MA__WORK
  if ((atomic_read(locked)==1) 
      && (marcel_self()->has_work || marcel_global_work)) {
    do_work(marcel_self());
  }
#endif
  atomic_dec(locked);
#if defined(PM2DEBUG) && defined(MA__ACTIVATION)
  pm2debug_flush();
#endif
}

static __inline__ void unlock_task_for_debug(void) __attribute__ ((unused));
static __inline__ void unlock_task_for_debug(void)
{
  atomic_dec(&GET_LWP(marcel_self())->_locked);
}

static __inline__ int locked(void) __attribute__ ((unused));
static __inline__ int locked(void)
{
  return atomic_read(&GET_LWP(marcel_self())->_locked);
}

extern volatile atomic_t __preemption_disabled;

static __inline__ void disable_preemption(void) __attribute__ ((unused));
static __inline__ void disable_preemption(void)
{
  atomic_inc(&__preemption_disabled);
}

static __inline__ void enable_preemption(void) __attribute__ ((unused));
static __inline__ void enable_preemption(void)
{
  atomic_dec(&__preemption_disabled);
}

static __inline__ unsigned int preemption_enabled(void) __attribute__ ((unused));
static __inline__ unsigned int preemption_enabled(void)
{
  return atomic_read(&__preemption_disabled) == 0;
}

#else

static __inline__ __volatile__ void ma_lock_task(void)
{
  GET_LWP(marcel_self())->_locked++;
}

static __inline__ __volatile__ void ma_unlock_task(void)
{
  GET_LWP(marcel_self())->_locked--;
}

static __inline__ __volatile__ unsigned locked(void)
{
  return GET_LWP(marcel_self())->_locked;
}

extern volatile unsigned int __preemption_disabled;

static __inline__ void disable_preemption(void) __attribute__ ((unused));
static __inline__ void disable_preemption(void)
{
  __preemption_disabled++;
}

static __inline__ void enable_preemption(void) __attribute__ ((unused));
static __inline__ void enable_preemption(void)
{
  __preemption_disabled--;
}

static __inline__ unsigned int preemption_enabled(void) __attribute__ ((unused));
static __inline__ unsigned int preemption_enabled(void)
{
  return __preemption_disabled == 0;
}

#endif

#ifdef DEBUG_LOCK_TASK
#define unlock_task() \
   (ma_unlock_task(), \
   lock_task_debug("\t=> unlock --%i\n", locked()))
#define lock_task() \
   (lock_task_debug("\t=> lock %i++\n", locked()), \
   ma_lock_task())
#else
#define unlock_task() ma_unlock_task()
#define lock_task() ma_lock_task()
#endif

#ifdef DEBUG_SCHED_LOCK
#define sched_lock(lwp) \
   (sched_lock_debug("sched_lock\n"), \
   ma_sched_lock(lwp))
#define sched_unlock(lwp) \
   (sched_lock_debug("sched_unlock\n"), \
   ma_sched_unlock(lwp))
#else
#define sched_lock(lwp) ma_sched_lock(lwp)
#define sched_unlock(lwp) ma_sched_unlock(lwp)
#endif

/* ==== miscelaneous private defs ==== */

#ifdef MA__DEBUG
extern void breakpoint();
#endif

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS
_PRIVATE_ extern volatile marcel_t __waiting_tasks;
#endif
_PRIVATE_ extern volatile marcel_t __delayed_tasks;

_PRIVATE_ void marcel_one_task_less(marcel_t pid);
_PRIVATE_ void marcel_one_more_task(marcel_t pid);

_PRIVATE_ void marcel_give_hand(boolean *blocked, marcel_lock_t *lock);
_PRIVATE_ void marcel_tempo_give_hand(unsigned long timeout, boolean *blocked, marcel_sem_t *s);
_PRIVATE_ void marcel_wake_task(marcel_t t, boolean *blocked);
_PRIVATE_ marcel_t marcel_unchain_task_and_find_next(marcel_t t, 
						     marcel_t find_next);
_PRIVATE_ void marcel_insert_task(marcel_t t);
_PRIVATE_ marcel_t marcel_radical_next_task(void);

#ifdef USE_VIRTUAL_TIMER

#define MARCEL_TIMER_SIGNAL   SIGVTALRM
#define MARCEL_ITIMER_TYPE    ITIMER_VIRTUAL

#else

#define MARCEL_TIMER_SIGNAL   SIGALRM
#define MARCEL_ITIMER_TYPE    ITIMER_REAL

#endif

void marcel_update_time(marcel_t cur);

#endif
