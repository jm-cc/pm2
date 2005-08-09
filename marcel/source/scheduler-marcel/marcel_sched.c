

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

#ifdef PM2DEBUG
//#define DO_NOT_DISPLAY_IN_IDLE
#endif

#include "marcel.h"
#include "tbx_compiler.h"

#warning "[1;33m<<< [1;37mScheduler [1;32mmarcel[1;37m selected[1;33m >>>[0m"

marcel_sched_attr_t marcel_sched_attr_default = MARCEL_SCHED_ATTR_INITIALIZER;

int marcel_sched_attr_init(marcel_sched_attr_t *attr)
{
  *attr = marcel_sched_attr_default;
  return 0;
}

int marcel_sched_attr_setinitrq(marcel_sched_attr_t *attr, ma_runqueue_t *rq)
{
  attr->init_rq = rq;
  return 0;
}

int marcel_sched_attr_getinitrq(__const marcel_sched_attr_t *attr, ma_runqueue_t **rq)
{
  *rq = attr->init_rq;
  return 0;
}

int marcel_sched_attr_setprio(marcel_sched_attr_t *attr, int prio)
{
  attr->prio = prio;
  return 0;
}

int marcel_sched_attr_getprio(__const marcel_sched_attr_t *attr, int *prio)
{
  *prio = attr->prio;
  return 0;
}

int marcel_sched_attr_setstayinbubble(marcel_sched_attr_t *attr, int stay)
{
  attr->stayinbubble = stay;
  return 0;
}

int marcel_sched_attr_getstayinbubble(__const marcel_sched_attr_t *attr, int *stay)
{
  *stay = attr->stayinbubble;
  return 0;
}

#define work_pending(thread) (0)

// Restriction d'affichage de debug lorsque l'on est la tâche idle
#ifdef DO_NOT_DISPLAY_IN_IDLE

#define IDLE_LOG_IN() \
  do { \
    if (!IS_TASK_TYPE_IDLE(marcel_self())) \
      LOG_IN(); \
  } while(0)

#define IDLE_LOG_OUT() \
  do { \
    if (!IS_TASK_TYPE_IDLE(marcel_self())) \
      LOG_OUT(); \
  } while(0)

#else

#define IDLE_LOG_IN()    LOG_IN()
#define IDLE_LOG_OUT()   LOG_OUT()

#endif

static volatile unsigned __active_threads,
  __sleeping_threads,
  __blocked_threads,
  __frozen_threads;


#ifndef DO_NOT_CHAIN_BLOCKED_THREADS
static LIST_HEAD(__waiting_tasks);
#endif
static LIST_HEAD(__delayed_tasks);

/* These two locks must be acquired before accessing the corresponding
   global queue.  They should only encapsulate *non-blocking* code
   sections. */
//static marcel_lock_t __delayed_lock = MARCEL_LOCK_INIT;
//static marcel_lock_t __blocking_lock = MARCEL_LOCK_INIT;

#if 0 //TODO
void marcel_schedpolicy_create(int *policy,
			       marcel_schedpolicy_func_t func)
{
  if(next_schedpolicy_available <
     MARCEL_MAX_USER_SCHED + __MARCEL_SCHED_AVAILABLE)
    RAISE(CONSTRAINT_ERROR);

  user_policy[next_schedpolicy_available - __MARCEL_SCHED_AVAILABLE] = func;
  *policy = next_schedpolicy_available++;
}
#endif

#if 0
inline static marcel_t next_task(marcel_t task)
{
  return list_entry(task->sched.internal.run_list.next, marcel_task_t, sched.internal.run_list);
}

inline static marcel_t prev_task(marcel_t task)
{
  return list_entry(task->sched.internal.run_list.prev, marcel_task_t, sched.internal.run_list);
}
#endif

int marcel_sched_setparam(marcel_t t, const struct marcel_sched_param *p) {
	return ma_sched_change_prio(t,p->sched_priority);
}

int marcel_sched_getparam(marcel_t t, struct marcel_sched_param *p) {
	p->sched_priority=t->sched.internal.prio;
	return 0;
}

int marcel_sched_setscheduler(marcel_t t, int policy, const struct marcel_sched_param *p) {
	t->sched.internal.sched_policy=policy;
	return ma_sched_change_prio(t,p->sched_priority);
}

int marcel_sched_getscheduler(marcel_t t) {
	return t->sched.internal.sched_policy;
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Consultation de variables                                      */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

unsigned marcel_activethreads(void)
{
   return __active_threads;
}

unsigned marcel_sleepingthreads(void)
{
  return __sleeping_threads;
}

unsigned marcel_blockedthreads(void)
{
  return __blocked_threads;
}

unsigned marcel_frozenthreads(void)
{
  return __frozen_threads;
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*              Choix des lwp, choix des threads, etc.                    */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

#if 0
static __inline__ int CAN_RUN(marcel_lwp_t *lwp, marcel_t pid)
{
  return
    ( 1
#ifdef MA__MULTIPLE_RUNNING
      && (pid->sched.internal.ext_state != MARCEL_RUNNING)
#endif
#ifdef MA__LWPS
      && (!marcel_vpmask_vp_ismember(pid->sched.vpmask, lwp->number))
#endif
      );
}

static __inline__ int CANNOT_RUN(marcel_lwp_t *lwp, marcel_t pid)
{
  return !CAN_RUN(lwp, pid);
}
#endif

#if 0
#ifndef MA__ONE_QUEUE

// On cherche le premier LWP compatible avec le 'vpmask' de la tâche
// en commençant par 'first'.
static __inline__ marcel_lwp_t *__find_first_lwp(marcel_t pid, marcel_lwp_t *first)
{
  marcel_lwp_t *lwp = first;

  do {
    if(!marcel_vpmask_vp_ismember(pid->vpmask, lwp->number))
      return lwp;
    lwp = next_lwp(lwp);
  } while(lwp != first);

  // Si on arrive ici, c'est une erreur de l'application
  RAISE(CONSTRAINT_ERROR);
  return NULL;
}

static __inline__ marcel_lwp_t *find_first_lwp(marcel_t pid, marcel_lwp_t *first)
{
  marcel_lwp_t *lwp;

  lwp_list_lock();

  lwp = __find_first_lwp(pid, first);

  lwp_list_unlock();

  return lwp;
}

static __inline__ marcel_lwp_t *find_next_lwp(marcel_t pid, marcel_lwp_t *first)
{
  marcel_lwp_t *lwp;

  lwp_list_lock();

  lwp = __find_first_lwp(pid, next_lwp(first));

  lwp_list_unlock();

  return lwp;
}

// Le cas echeant, retourne un LWP fortement sous-charge (nb de
// threads running < THREAD_THRESHOLD_LOW), ou alors retourne le LWP
// "suggested".
static __inline__ marcel_lwp_t *find_good_lwp(marcel_t pid, marcel_lwp_t *suggested)
{
  marcel_lwp_t *first = find_first_lwp(pid, suggested);
  marcel_lwp_t *lwp = first;

  for(;;) {
    if(SCHED_DATA(lwp).running_tasks < THREAD_THRESHOLD_LOW)
      return lwp;

    // On avance au prochain possible
    lwp = find_next_lwp(pid, lwp);

    // Si on a fait le tour de la liste sans succès, on prend le
    // premier...
    if(lwp == first)
      return first;
  }
}

// Retourne le LWP le moins charge (ce qui oblige a parcourir toute la
// liste)
static __inline__ marcel_lwp_t *find_best_lwp(marcel_t pid)
{
  marcel_lwp_t *first = find_first_lwp(pid, &__main_lwp);
  unsigned nb = SCHED_DATA(first).running_tasks;
  marcel_lwp_t *best, *lwp;

  lwp = best = first;
  for(;;) {
    // On avance au prochain possible
    lwp = find_next_lwp(pid, lwp);

    if(lwp == first)
      return best;

    if(SCHED_DATA(lwp).running_tasks < nb) {
      nb = SCHED_DATA(lwp).running_tasks;
      best = lwp;
    }
  }
}

static __inline__ marcel_lwp_t *choose_lwp(marcel_t t)
{
  marcel_t cur = marcel_self();
  DEFINE_CUR_LWP(, =, GET_LWP(cur));

  switch(t->sched_policy)
    {
    case MARCEL_SCHED_OTHER :
      return find_first_lwp(t, cur_lwp);
    case MARCEL_SCHED_AFFINITY :
      return find_good_lwp(t, t->previous_lwp ? : cur_lwp);
    case MARCEL_SCHED_BALANCE :
      return find_best_lwp(t);
    default:
      /* MARCEL_SCHED_USER_... */
      return (*user_policy[t->sched_policy - __MARCEL_SCHED_AVAILABLE])
	(t, cur_lwp);
    }

  RAISE(PROGRAM_ERROR);
  return NULL;
}

#endif /* MA__ONE_QUEUE */
#endif

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*                          statistiques                                  */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

#if 0
/* TODO: Use atomic_inc and atomic_dec when accessing running_tasks! */
#define INC_STATS(t, lwp) \
  ((! MA_TASK_NOT_COUNTED_IN_RUNNING(t)) ? SCHED_DATA(lwp).running_tasks++ : 0)
#define DEC_STATS(t, lwp) \
  ((! MA_TASK_NOT_COUNTED_IN_RUNNING(t)) ? SCHED_DATA(lwp).running_tasks-- : 0)

#ifdef MA__ACTIVATION
#define ONE_MORE_ACTIVE_TASK_HOOK() \
  (nb_idle_sleeping ? act_cntl(ACT_CNTL_WAKE_UP,NULL) : 0)
#else
#define ONE_MORE_ACTIVE_TASK_HOOK() ((void)0)
#endif

#define one_more_active_task(t, lwp) \
  ((! MA_TASK_NOT_COUNTED_IN_RUNNING(t)) ? __active_threads++ : 0), \
  ONE_MORE_ACTIVE_TASK_HOOK(), \
  INC_STATS((t), (lwp)), \
  MTRACE("Activation", (t))


#define one_active_task_less(t, lwp) \
  ((! MA_TASK_NOT_COUNTED_IN_RUNNING(t)) ? __active_threads-- : 0), \
  DEC_STATS((t), (lwp))

#define one_more_sleeping_task(t) \
  __sleeping_threads++, MTRACE("Sleeping", (t))

#define one_sleeping_task_less(t) \
  __sleeping_threads--

#define one_more_blocked_task(t) \
  __blocked_threads++, MTRACE("Blocking", (t))

#define one_blocked_task_less(t) \
  __blocked_threads--

#define one_more_frozen_task(t) \
  __frozen_threads++, MTRACE("Freezing", (t))

#define one_frozen_task_less(t) \
  __frozen_threads--
#endif

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*                            SCHEDULER                                   */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

// Pour le debug. ATTENTION : sched_lock/unlock doivent être appelés
// avant et après !
#if 0
static __inline__ void display_sched_queue(marcel_lwp_t *lwp)
{
  marcel_t t;
  int max_iter=10;

  LOG_IN();
  t = SCHED_DATA(lwp).first;
  if(t) {
    mdebug_sched_q("\t\t\t<Queue of regular tasks on LWP %d:\n", LWP_NUMBER(lwp));
    do {
#ifdef MA__MULTIPLE_RUNNING
      mdebug_sched_q("\t\t\t\tThread %p (num %ld, LWP %d, ext_state %.8x, sf %.8lx)\n",
	     t, t->number, LWP_NUMBER(GET_LWP(t)), t->sched.internal.ext_state, t->flags);
#else
      mdebug_sched_q("\t\t\t\tThread %p (num %ld, LWP %d, fl %.8lx)\n" ,
             t, t->number, LWP_NUMBER(GET_LWP(t)), t->flags);
#endif
      t = next_task(t);
    } while(t != SCHED_DATA(lwp).first && (max_iter-- > 0));
    if (max_iter == -1) {
	    mdebug_sched_q("\t\t\t\tStopping list\n");
    }
    mdebug_sched_q("\t\t\t>\n");
  }

#ifdef MARCEL_RT
  t = SCHED_DATA(lwp).rt_first;
  if(t) {
    mdebug_sched_q("\t\t\t<Queue of real-time tasks on LWP %d:\n", LWP_NUMBER(lwp));
    do {
#ifdef MA__MULTIPLE_RUNNING
      mdebug_sched_q("\t\t\t\tThread %p (num %ld, LWP %d, ext_state %.8x, sf %.8lx)\n",
	     t, t->number, LWP_NUMBER(GET_LWP(t)), t->sched.internal.ext_state, t->flags);
#else
      mdebug_sched_q("\t\t\t\tThread %p (num %ld, LWP %d, sf %.8lx)\n" ,
             t, t->number, LWP_NUMBER(GET_LWP(t)), t->flags);
#endif
      t = next_task(t);
    } while(t != SCHED_DATA(lwp).rt_first);
    mdebug_sched_q("\t\t\t>\n");
  }
#endif
  LOG_OUT();
}
#endif


/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Changement de contexte                                         */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

#if 0
static int nr_running=0;

#define task_has_lwp(tsk) ((tsk)->sched.internal.cpus_runnable != ~0UL)

static inline void task_set_lwp(marcel_task_t *tsk, marcel_lwp_t *lwp)
{
        tsk->sched.lwp = lwp;
        tsk->sched.internal.lwps_runnable = 1UL << LWP_NUMBER(lwp);
}

static inline void task_release_cpu(marcel_task_t *tsk)
{
        tsk->sched.internal.lwps_runnable = ~0UL;
}

static inline void del_from_runqueue(marcel_task_t * p)
{
        nr_running--;
        //p->sleep_time = jiffies;
        list_del(&p->sched.internal.run_list);
        p->sched.internal.run_list.next = NULL;
}

static inline int task_on_runqueue(marcel_task_t *p)
{
        return (p->sched.internal.run_list.next != NULL);
}
#endif


#if 0 //TODO
// Checks to see if some tasks should be waked. NOTE: The function
// assumes that "lock_task()" was called previously.
int marcel_check_sleeping(void)
{
  unsigned long now = marcel_clock();
  int waked_some_task = 0;

  IDLE_LOG_IN();

  for(;;) {

    if(marcel_lock_tryacquire(&__delayed_lock)) {
      marcel_t t;

      if(list_empty(&__delayed_tasks)) {
	marcel_lock_release(&__delayed_lock);
	break;
      }

      t = list_entry(__delayed_tasks.next, marcel_task_t, sched.internal.task_list);

      if (t->time_to_wake > now) {
	marcel_lock_release(&__delayed_lock);
	break;
      }

      marcel_lock_release(&__delayed_lock);

      mdebug("\t\t\t<Check delayed: waking thread %p (%ld)>\n",
	     t, t->number);

      ma_wake_up_task(t, NULL);
      waked_some_task++;

    } else {
      mdebug("LWP(%d) failed to acquire __delayed_lock\n",
	     GET_LWP(marcel_self())->number);
      break;
    }

  } // for

  IDLE_LOG_OUT();

  return waked_some_task;
}
#endif

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Gestion des threads idle                                       */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

#if 0
// NOTE: This function assumes that "lock_task()" was called
// previously. It is currently _only_ called from within "idle_func",
// so MARCEL_POLL_AT_IDLE is used for polling.
static __inline__ int marcel_check_delayed_tasks(void)
{
  // Attention : C'est bien un "ou logique" ici car on veut executer
  // les deux fonctions...
  return marcel_check_sleeping() | marcel_check_polling(MARCEL_POLL_AT_IDLE);
}

#ifdef MA__SMP
static __inline__ marcel_t do_work_stealing(void)
{
  /* Currently not implemented ! */
  return NULL;
}
#endif

static __inline__ void idle_check_end(marcel_lwp_t *lwp)
{
#ifdef MA__SMP
/*
  IDLE_LOG_IN();
  if(lwp->sched.internal.has_to_stop) {
    marcel_sig_stop_timer();
    if(lwp == &__main_lwp) {
      IDLE_LOG_OUT();
      RAISE(PROGRAM_ERROR);
    } else {
      mdebug("\t\t\t<LWP %d exiting>\n", lwp->number);
      marcel_ctx_longjmp(lwp->home_ctx, 1);
    }
  }
  IDLE_LOG_OUT();
*/
#endif // MA__SMP
}

static __inline__ void idle_check_tasks_to_wake(marcel_lwp_t *lwp)
{
  IDLE_LOG_IN();

#ifndef MA__MONO
  marcel_check_delayed_tasks();
#else /* MA__MONO */
  /* If no polling jobs were registered, then we can sleep and only
     check delayed tasks when we are signaled (SIGALRM) */
  if(marcel_polling_is_required(MARCEL_POLL_AT_IDLE)) {
    marcel_check_delayed_tasks();
  } else {
    if(!marcel_check_delayed_tasks()) {
      marcel_sig_pause();
    }
  }
#endif /* MA__MONO */

  IDLE_LOG_OUT();
}

static __inline__ marcel_t idle_find_runnable_task(marcel_lwp_t *lwp)
{
  marcel_t next = NULL, cur = marcel_self();

  IDLE_LOG_IN();

#ifdef MA__ONE_QUEUE

  // next sera positionné à NULL si aucune autre tâche prête n'est
  // trouvée (et dans ce cas SET_RUNNING(cur) est effectué par
  // unchain_task).
  state_lock(cur);
  marcel_set_frozen(cur);
  next = UNCHAIN_TASK_AND_FIND_NEXT(cur);

#ifdef MA__MONO
  // Le but de ce code est de détecter une situation de deadlock.
  if(next == NULL) {
    if(list_empty(&__delayed_tasks)
       && !marcel_polling_is_required(MARCEL_POLL_AT_IDLE)) {
      IDLE_LOG_OUT();
      RAISE(DEADLOCK_ERROR);
    }
  }
#endif

#else // Reste le cas "SMP avec plusieurs files"

  if(SCHED_DATA(lwp).has_new_tasks) {
    state_lock(cur);
    marcel_set_frozen(cur);
    next = UNCHAIN_TASK_AND_FIND_NEXT(cur);
  } else {
    next = do_work_stealing();
  }

#endif // MA__ONE_QUEUE

  IDLE_LOG_OUT();
  return next;
}

static marcel_t idle_body(void)
{
  marcel_t next;
  DEFINE_CUR_LWP(, =, GET_LWP(marcel_self()));

  LOG_IN();
  for(;;) {

    /* Do I have to stop? */
    idle_check_end(cur_lwp);

    /* Check for delayed tasks and polling stuff to do... */
    idle_check_tasks_to_wake(cur_lwp);

    /* Can I find a ready task? */
    next = idle_find_runnable_task(cur_lwp);

    if(next != NULL) {
	    display_sched_queue(cur_lwp);
      break;
    }

    /* If nothing successful */
    SCHED_YIELD();

  }
  LOG_OUT();
  return next;
}

any_t idle_func(any_t arg)
{
  marcel_t next, cur = marcel_self();
  volatile int init_done=0;
#ifdef PM2DEBUG
    DEFINE_CUR_LWP(, =, GET_LWP(cur));
#endif

  LOG_IN();
  /* Code plus maintenu (cf marcel_sched_generic.c pour la vraie
   * fonction idle_func) */
  RAISE(PROGRAM_ERROR);

  marcel_sig_start_timer(); // May be redundant for main LWP

  // From now on, lock_task() is supposed to be permanently handled
  lock_task();

  // Nécessaire pour les activations, sans effet pour le reste
  MA_THR_SETJMP(cur);
  if(!init_done) {
    init_done=1;
    MTRACE("Starting idle_func", cur);
  } else {
    MA_THR_RESTARTED(cur, "Idle restart");
  }

  for(;;) {

    mdebug("\t\t\t<Scheduler scheduled> (LWP = %d)\n", cur_lwp->number);
    next = idle_body();
    mdebug("\t\t\t<Scheduler unscheduled> (LWP = %d)\n", cur_lwp->number);

    if (next)
      marcel_switch_to(cur, next);

  }
  //LOG_OUT(); // Probably never executed
  return NULL;
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Initialisation/Terminaison                                     */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

#ifdef MA__ACTIVATION
static void void_func(void* param)
{
  RAISE("Quoi, on est là ???\n");
}
#endif

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Parcours de la liste des threads                               */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

void marcel_snapshot(snapshot_func_t f)
{
  marcel_t t;
  struct list_head *pos;
  DEFINE_CUR_LWP(, TBX_UNUSED =, GET_LWP(marcel_self()));

  lock_task();

  t = SCHED_DATA(cur_lwp).first;
  do {
    (*f)(t);
    t = next_task(t);
  } while(t != SCHED_DATA(cur_lwp).first);

  list_for_each(pos, &__delayed_tasks) {
    t = list_entry(pos, marcel_task_t, sched.internal.task_list);
    (*f)(t);
  }

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS

  list_for_each(pos, &__waiting_tasks) {
    t = list_entry(pos, marcel_task_t, sched.internal.task_list);
    (*f)(t);
  }

#endif

  unlock_task();
}

static __inline__ int want_to_see(marcel_t t, int which)
{
  if(t->detached) {
    if(which & NOT_DETACHED_ONLY)
      return 0;
  } else if(which & DETACHED_ONLY)
    return 0;

  if(t->not_migratable) {
    if(which & MIGRATABLE_ONLY)
      return 0;
  } else if(which & NOT_MIGRATABLE_ONLY)
      return 0;

  if(IS_BLOCKED(t)) {
    if(which & NOT_BLOCKED_ONLY)
      return 0;
  } else if(which & BLOCKED_ONLY)
    return 0;

  if(IS_SLEEPING(t)) {
    if(which & NOT_SLEEPING_ONLY)
      return 0;
  } else if(which & SLEEPING_ONLY)
    return 0;

  return 1;
}

void marcel_threadslist(int max, marcel_t *pids, int *nb, int which)
{
  marcel_t t;
  struct list_head *pos;
  int nb_pids = 0;
  DEFINE_CUR_LWP(, TBX_UNUSED =, GET_LWP(marcel_self()));

  if( ((which & MIGRATABLE_ONLY) && (which & NOT_MIGRATABLE_ONLY)) ||
      ((which & DETACHED_ONLY) && (which & NOT_DETACHED_ONLY)) ||
      ((which & BLOCKED_ONLY) && (which & NOT_BLOCKED_ONLY)) ||
      ((which & SLEEPING_ONLY) && (which & NOT_SLEEPING_ONLY)))
    RAISE(CONSTRAINT_ERROR);

  lock_task();

  t = SCHED_DATA(cur_lwp).first;
  do {
    if (want_to_see(t, which)) {
      if(nb_pids < max)
	pids[nb_pids++] = t;
      else
	nb_pids++;
    }
  } while(t = next_task(t), t != SCHED_DATA(cur_lwp).first);

  list_for_each(pos, &__delayed_tasks) {
    t = list_entry(pos, marcel_task_t, sched.internal.task_list);
    if (want_to_see(t, which)) {
      if(nb_pids < max)
	pids[nb_pids++] = t;
      else
	nb_pids++;
    }
  }

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS

  list_for_each(pos, &__waiting_tasks) {
    t = list_entry(pos, marcel_task_t, sched.internal.task_list);
    if (want_to_see(t, which)) {
      if(nb_pids < max)
	pids[nb_pids++] = t;
      else
	nb_pids++;
    }
  }

#endif

  *nb = nb_pids;
  unlock_task();
}

extern int FASTCALL(marcel_wake_up_thread(marcel_task_t * tsk))
{
	return 0;

};
#endif

