
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

#ifndef MARCEL_MACROS_IS_DEF
#define MARCEL_MACROS_IS_DEF

#include "pm2_profile.h"

#ifdef MA__ONE_QUEUE
#define SCHED_DATA(lwp) (__sched_data)
#else
#define SCHED_DATA(lwp) ((lwp)->__sched_data)
#endif

#ifdef MA__DEBUG
#define SET_STATE_RUNNING_HOOK(next) \
  mdebug_state("\t\t\t<State set to running %p>\n", next)
#define SET_STATE_READY_HOOK(next) \
  mdebug_state("\t\t\t<State set to ready %p>\n", next)
#else
#define SET_STATE_RUNNING_HOOK(next)  (void)0
#define SET_STATE_READY_HOOK(next)    (void)0
#endif

#ifdef MA__LWPS

#define GET_LWP_BY_NUM(proc)                (addr_lwp[proc])
#define GET_LWP_NUMBER(current)             ((current)->lwp->number)
#define GET_LWP(current)                    ((current)->lwp)
#define SET_LWP(current, value)             ((current)->lwp=(value))
#define GET_CUR_LWP()                       (cur_lwp)
#define SET_CUR_LWP(value)                  (cur_lwp=(value))
#define SET_LWP_NB(proc, value)             (addr_lwp[proc]=(value))
#define DEFINE_CUR_LWP(OPTIONS, signe, lwp) \
   OPTIONS __lwp_t *cur_lwp signe lwp

#else

#define cur_lwp                             (&__main_lwp)
#define GET_LWP_NUMBER(current)             0
#define GET_LWP_BY_NUM(nb)                  (cur_lwp)
#define GET_LWP(current)                    (cur_lwp)
#define SET_LWP(current, value)             ((void)0)
#define GET_CUR_LWP()                       (cur_lwp)
#define SET_CUR_LWP(value)                  ((void)0)
#define SET_LWP_NB(proc, value)             ((void)0)
#define DEFINE_CUR_LWP(OPTIONS, signe, current) \
   int __cur_lwp_unused__ __attribute__ ((unused))

#endif


#ifdef MA__MULTIPLE_RUNNING

#define SET_STATE_RUNNING(previous, next, next_lwp) \
  do {                                              \
    SET_STATE_RUNNING_HOOK(next);                   \
    (next)->ext_state = MARCEL_RUNNING;             \
    MTRACE("RUNNING", (next));                      \
    SET_LWP(next, next_lwp);                        \
    next_lwp->prev_running = previous;              \
  } while(0)

#define SET_STATE_RUNNING_ONLY(next)    \
  do {                                  \
    SET_STATE_RUNNING_HOOK(next),       \
    (next)->ext_state = MARCEL_RUNNING; \
    MTRACE("RUNNING", (next));          \
  } while(0)

#define SET_STATE_READY(current)         \
  do {                                   \
    SET_STATE_READY_HOOK(current);       \
    MTRACE("READY", (current));          \
    (current)->ext_state = MARCEL_READY; \
  } while(0)

#else // MA__MULTIPLE_RUNNING

#define SET_STATE_RUNNING_ONLY(next)            SET_STATE_RUNNING_HOOK(next)

#define SET_STATE_RUNNING(previous, next, lwp)  SET_STATE_RUNNING_HOOK(next)

#define SET_STATE_READY(current)                SET_STATE_READY_HOOK(current)

#endif

#ifdef MA__DEBUG

#ifdef MA__MULTIPLE_RUNNING

#define MA_THR_DEBUG__MULTIPLE_RUNNING(current) \
  do {                                          \
    if(current->ext_state != MARCEL_RUNNING)    \
      RAISE("Thread not running");              \
  } while(0)

#else

#define MA_THR_DEBUG__MULTIPLE_RUNNING(current)  (void)0

#endif

#define MA_THR_DEBUG(current)                \
  do {                                       \
    breakpoint();                            \
    MA_THR_DEBUG__MULTIPLE_RUNNING(current); \
  } while(0)

#else

#define MA_THR_DEBUG__MULTIPLE_RUNNING(current)  (void)0
#define MA_THR_DEBUG(current)                    (void)0

#endif

#ifdef MA__MULTIPLE_RUNNING

#define MA_THR_UPDATE_LAST_THR(current)             \
  do {                                              \
    marcel_t prev = GET_LWP(current)->prev_running; \
    if (prev && (current != prev))                  \
      SET_STATE_READY(prev);                        \
    GET_LWP(current)->prev_running=NULL;            \
  } while(0)

#else

#define MA_THR_UPDATE_LAST_THR(current) (void)0

#endif


/* effectue un setjmp. On doit être RUNNING avant et après
 * */
#define MA_THR_SETJMP(current) \
  marcel_ctx_setjmp(current->ctx_yield)

/* On vient de reprendre la main. On doit déjà être RUNNING. On enlève
 * le flags RUNNING au thread qui tournait avant.
 * */
#define MA_THR_RESTARTED(current, info) \
  do {                                 \
    MA_THR_DEBUG(current);             \
    MTRACE(info, current);             \
    MA_THR_UPDATE_LAST_THR(current);   \
  } while(0)

/* on effectue un longjmp. Le thread courrant ET le suivant doivent
 * être RUNNING. La variable previous_task doit être correctement
 * positionnée pour pouvoir annuler la propriété RUNNING du thread
 * courant.
 * */

#if defined(DSM_SHARED_STACK)
#error Ça se marche pas du tout en SMP !!!
#error Pourquoi pas marcel_self() ?
#define DSM_SET_NEXT_THREAD __next_thread = next
#else
#define DSM_SET_NEXT_THREAD
#endif

#define MA_THR_LONGJMP(cur_num, next, ret) \
  do {                                     \
    MA_THR_DEBUG__MULTIPLE_RUNNING(next);  \
    DSM_SET_NEXT_THREAD;                   \
    PROF_SWITCH_TO(cur_num, next->number); \
    call_ST_FLUSH_WINDOWS();               \
    marcel_ctx_longjmp(next->ctx_yield, ret);              \
  } while(0)

#define FIND_NEXT            (marcel_t)0
#define DO_NOT_REMOVE_MYSELF (marcel_t)1

#define FIND_NEXT_TASK_TO_RUN(current, lwp) \
  next_task_to_run(current, lwp)
#define UNCHAIN_MYSELF(task, next) \
  marcel_unchain_task_and_find_next(task, next)
#define UNCHAIN_TASK(task) \
  marcel_unchain_task_and_find_next(task, DO_NOT_REMOVE_MYSELF)
#define UNCHAIN_TASK_AND_FIND_NEXT(task) \
  marcel_unchain_task_and_find_next(task, FIND_NEXT)

/* Manipulation des champs de task->special_flags */

// NORMAL : Thread marcel "tout bete"
#define MA_SF_NORMAL       0
// UPCALL_NEW : no comment
#define MA_SF_UPCALL_NEW   1
// POLL : le thread "idle_task" qui fait plein de choses
#define MA_SF_POLL         2
// IDLE : le thread "wait_and_yield" ne consomme pas de CPU...
#define MA_SF_IDLE         4
// NORUN : ne pas prendre en compte ce thread dans le calcul des
// taches actives
#define MA_SF_NORUN        8
// NOSCHEDLOCK : ne pas appeler "sched_lock" dans insert_task...
#define MA_SF_NOSCHEDLOCK  16
// RT_THREAD : ce thread doit être ordonnancé en tant que thread "Real Time"
#define MA_SF_RT_THREAD    32

#define MA_TASK_TYPE_NORMAL     MA_SF_NORMAL
#define MA_TASK_TYPE_UPCALL_NEW MA_SF_UPCALL_NEW
#define MA_TASK_TYPE_POLL       MA_SF_POLL
#define MA_TASK_TYPE_IDLE       MA_SF_IDLE

#define MA_GET_TASK_TYPE(task)  (((task)->special_flags) & 0x7)

#define IS_TASK_TYPE_UPCALL_NEW(task) \
   (((task)->special_flags) & MA_SF_UPCALL_NEW)

#define IS_TASK_TYPE_IDLE(task) \
   (((task)->special_flags) & (MA_SF_POLL|MA_SF_IDLE))

#define MA_TASK_NOT_COUNTED_IN_RUNNING(task) \
   (((task)->special_flags) & MA_SF_NORUN)

#define MA_TASK_NO_USE_SCHEDLOCK(task) \
   (((task)->special_flags) & MA_SF_NOSCHEDLOCK)

#define MA_TASK_REAL_TIME(task) \
   (((task)->special_flags) & MA_SF_RT_THREAD)

#endif
