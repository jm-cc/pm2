
#ifndef MARCEL_MACROS_IS_DEF
#define MARCEL_MACROS_IS_DEF

#include "profile.h"

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

#ifdef MA__MULTIPLE_RUNNING

#ifdef MA__LWPS

#define SET_STATE_RUNNING(previous, next, next_lwp) \
  (SET_STATE_RUNNING_HOOK(next), \
   (next)->ext_state=MARCEL_RUNNING, \
   MTRACE("RUNNING", (next)), \
   (next)->lwp=next_lwp, \
   next_lwp->prev_running=previous)

#else

#define SET_STATE_RUNNING(previous, next, lwp) \
  (SET_STATE_RUNNING_HOOK(next), \
   (next)->ext_state=MARCEL_RUNNING, \
   MTRACE("RUNNING", (next)), \
   lwp->prev_running=previous)

#endif

#define SET_STATE_READY(current) \
  (SET_STATE_READY_HOOK(current), \
   MTRACE("READY", (current)), \
   (current)->ext_state=MARCEL_READY)

#define CANNOT_RUN(current) \
  ((current)->ext_state == MARCEL_RUNNING)

#else

#define SET_STATE_RUNNING(previous, next, lwp) SET_STATE_RUNNING_HOOK(next)

#define SET_STATE_READY(current)               SET_STATE_READY_HOOK(current)

#endif



#ifdef MA__LWPS

#define GET_LWP_BY_NUM(proc)    (addr_lwp[proc])
#define GET_LWP_NUMBER(current)        ((current)->lwp->number)
#define GET_LWP(current)        ((current)->lwp)
#define SET_LWP(current, value) ((current)->lwp=(value))
#define GET_CUR_LWP()           (cur_lwp)
#define SET_CUR_LWP(value)      (cur_lwp=(value))
#define SET_LWP_NB(proc, value) (addr_lwp[proc]=(value))
#define DEFINE_CUR_LWP(OPTIONS, signe, lwp) \
   OPTIONS __lwp_t *cur_lwp signe lwp

#else

#define cur_lwp                 (&__main_lwp)
#define GET_LWP_NUMBER(current)        0
#define GET_LWP_BY_NUM(nb)      (cur_lwp)
#define GET_LWP(current)        (cur_lwp)
#define SET_LWP(current, value)
#define GET_CUR_LWP()           (cur_lwp)
#define SET_CUR_LWP(value)
#define SET_LWP_NB(proc, value)
#define DEFINE_CUR_LWP(OPTIONS, signe, current)

#endif



#ifdef MA__DEBUG
#ifdef MA__MULTIPLE_RUNNING
#define MA_THR_DEBUG__MULTIPLE_RUNNING(current) \
  ((current->ext_state != MARCEL_RUNNING) ? \
   RAISE("Thread not running") : \
   (void)0)
#else
#define MA_THR_DEBUG__MULTIPLE_RUNNING(current)  (void)0
#endif
#define MA_THR_DEBUG(current) \
  (breakpoint(), MA_THR_DEBUG__MULTIPLE_RUNNING(current))
#else
#define MA_THR_DEBUG__MULTIPLE_RUNNING(current)  (void)0
#define MA_THR_DEBUG(current)                    (void)0
#endif

#ifdef MA__MULTIPLE_RUNNING
#define MA_THR_UPDATE_LAST_THR(current) \
  { \
    marcel_t prev = GET_LWP(current)->prev_running; \
    if (prev && (current != prev)) \
      SET_STATE_READY(prev); \
    GET_LWP(current)->prev_running=NULL; \
  }
#else
#define MA_THR_UPDATE_LAST_THR(current) (void)0
#endif


/* effectue un setjmp. On doit être RUNNING avant et après
 * */
#define MA_THR_SETJMP(current) \
  setjmp(current->jbuf)

/* On vient de reprendre la main. On doit déjà être RUNNING. On enlève
 * le flags RUNNING au thread qui tournait avant.
 * */
#define MA_THR_RESTARTED(current, info) \
   { MA_THR_DEBUG(current); \
   MTRACE(info, current); \
   MA_THR_UPDATE_LAST_THR(current);}

/* on effectue un longjmp. Le thread courrant ET le suivant doivent
 * être RUNNING. La variable previous_task doit être correctement
 * positionnée pour pouvoir annuler la propriété RUNNING du thread
 * courant.
 * */

#if defined(DSM_SHARED_STACK)
#define MA_THR_LONGJMP(next, ret) \
  (MA_THR_DEBUG__MULTIPLE_RUNNING(next), \
   __next_thread = next,  \
   call_ST_FLUSH_WINDOWS(), \
   PROF_SWITCH_TO(next), \
   longjmp(next->jbuf, ret))
#else
#define MA_THR_LONGJMP(next, ret) \
  (MA_THR_DEBUG__MULTIPLE_RUNNING(next), \
   call_ST_FLUSH_WINDOWS(), \
   PROF_SWITCH_TO(next), \
   longjmp(next->jbuf, ret))
#endif

// TODO: C'est dans cette fonction qu'il faut tester si une activation
// est debloquee...  NOTE: Le parametre "pid" peut etre NULL dans le
// cas ou l'on sait deja qu'une activation est debloquee.
#ifdef MA__ACTIVATION
#define goto_next_task(pid) \
  (act_nb_unblocked) ? (act_goto_next_task(pid, ACT_RESTART_FROM_SCHED)): \
                       (MA_THR_LONGJMP((pid), NORMAL_RETURN))
#define can_goto_next_task(current, pid) \
  (act_nb_unblocked) ? (act_goto_next_task(pid, ACT_RESTART_FROM_SCHED)): \
                       (((pid)==(current)) ? (void)0 : \
			            MA_THR_LONGJMP((pid), NORMAL_RETURN))
#else
#define goto_next_task(pid) \
   MA_THR_LONGJMP((pid), NORMAL_RETURN)
#define can_goto_next_task(current, pid) \
   (((pid)==(current)) ? (void)0 : MA_THR_LONGJMP((pid), NORMAL_RETURN))
#endif

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
#define MA_SF_NORMAL 0
// UPCALL_NEW : no comment
#define MA_SF_UPCALL_NEW 1
// POLL : le thread "sched_task" qui fait plein de choses
#define MA_SF_POLL 2
// IDLE : le thread "wait_and_yield" ne consomme pas de CPU...
#define MA_SF_IDLE 4

#define MA_TASK_TYPE_NORMAL     MA_SF_NORMAL
#define MA_TASK_TYPE_UPCALL_NEW MA_SF_UPCALL_NEW
#define MA_TASK_TYPE_POLL       MA_SF_POLL
#define MA_TASK_TYPE_IDLE       MA_SF_IDLE
#define MA_GET_TASK_TYPE(task)  (((task)->special_flags) & 0x7)
#define IS_TASK_TYPE_UPCALL_NEW(task) \
   (((task)->special_flags) & MA_SF_UPCALL_NEW)
#define IS_TASK_TYPE_IDLE(task) \
   (((task)->special_flags) & (MA_SF_POLL|MA_SF_IDLE))
// NORUN : ne pas prendre en compte ce thread dans le calcul des
// taches actives
#define MA_SF_NORUN 0x8
#define MA_TASK_NOT_COUNTED_IN_RUNNING(task) \
   (((task)->special_flags) & MA_SF_NORUN)
// NOSCHEDLOCK : ne pas appeler "sched_lock" dans insert_task...
#define MA_SF_NOSCHEDLOCK 0x10
#define MA_TASK_NO_USE_SCHEDLOCK(task) \
   (((task)->special_flags) & MA_SF_NOSCHEDLOCK)

#endif
