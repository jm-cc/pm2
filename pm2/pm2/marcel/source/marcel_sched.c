
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

#define BIND_LWP_ON_PROCESSORS
//#define DO_PAUSE_INSTEAD_OF_SCHED_YIELD
#ifdef PM2DEBUG
#define DO_NOT_DISPLAY_IN_IDLE
#endif

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "marcel.h"
#include "sys/marcel_debug.h"
#include "pm2_list.h"

#ifdef MA__SMP
#include <errno.h>
#include <sched.h>
#ifdef SOLARIS_SYS
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>
#endif
#endif

#ifdef MA__ACTIVATION
#include "sys/marcel_upcalls.h"
int nb_idle_sleeping = 0;
#endif


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

static int next_schedpolicy_available = __MARCEL_SCHED_AVAILABLE;
static marcel_schedpolicy_func_t user_policy[MARCEL_MAX_USER_SCHED];

static boolean marcel_initialized = FALSE;

static volatile unsigned __active_threads = 0,
  __sleeping_threads = 0,
  __blocked_threads = 0,
  __frozen_threads = 0;


#ifdef DSM_SHARED_STACK
volatile marcel_t __next_thread;
#endif

#ifdef MA__ONE_QUEUE
__sched_t __sched_data;
#endif
__lwp_t __main_lwp;

#ifdef STANDARD_MAIN
task_desc __main_thread_struct;
#define __main_thread  (&__main_thread_struct)
#else
extern marcel_t __main_thread;
#endif

#ifdef MARCEL_RT
unsigned __rt_task_exist = 0;
#endif

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS
static LIST_HEAD(__waiting_tasks);
#endif
static LIST_HEAD(__delayed_tasks);
static volatile unsigned long task_number = 1;

/* These two locks must be acquired before accessing the corresponding
   global queue.  They should only encapsulate *non-blocking* code
   sections. */
static marcel_lock_t __delayed_lock = MARCEL_LOCK_INIT;
static marcel_lock_t __blocking_lock = MARCEL_LOCK_INIT;

#ifdef MA__LWPS

// Verrou protégeant la liste chaînée des LWPs
static marcel_lock_t __lwp_list_lock = MARCEL_LOCK_INIT;

static unsigned  ma__nb_lwp;
__lwp_t* addr_lwp[MA__MAX_LWPS];

#endif

#ifdef MA__LWPS
#ifdef DO_PAUSE_INSTEAD_OF_SCHED_YIELD
#define PAUSE()	  sigsuspend(&sigeptset)
#else
#define PAUSE()	  SCHED_YIELD()
#endif
#else // MA__LWPS
#define PAUSE()
#endif

static __inline__ void lwp_list_lock(void)
{
#ifdef MA__LWPS
  marcel_lock_acquire(&__lwp_list_lock);
#endif
}

static __inline__ void lwp_list_unlock(void)
{
#ifdef MA__LWPS
  marcel_lock_release(&__lwp_list_lock);
#endif
}

static __inline__ unsigned get_nb_lwps()
{
#ifdef MA__LWPS
  return ma__nb_lwp;
#else
  return 1;
#endif
}

static __inline__ void set_nb_lwps(unsigned value)
{
#ifdef MA__LWPS
  ma__nb_lwp=value;
#endif
}

#define MIN_TIME_SLICE		10000
#define DEFAULT_TIME_SLICE	20000

volatile atomic_t __preemption_disabled = ATOMIC_INIT(0);

static volatile unsigned long time_slice = DEFAULT_TIME_SLICE;
static volatile unsigned long __milliseconds = 0;

static struct {
       unsigned nb_tasks;
       boolean main_is_waiting, blocked;
} _main_struct = { 0, FALSE, FALSE };


void marcel_schedpolicy_create(int *policy,
			       marcel_schedpolicy_func_t func)
{
  if(next_schedpolicy_available <
     MARCEL_MAX_USER_SCHED + __MARCEL_SCHED_AVAILABLE)
    RAISE(CONSTRAINT_ERROR);

  user_policy[next_schedpolicy_available - __MARCEL_SCHED_AVAILABLE] = func;
  *policy = next_schedpolicy_available++;
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Consultation de variables                                      */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

unsigned long marcel_createdthreads(void)
{
   return task_number -1;    /* -1 pour le main */
}

unsigned marcel_nbthreads(void)
{
   return _main_struct.nb_tasks + 1;   /* + 1 pour le main */
}

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

inline unsigned marcel_nbvps(void)
{
  return get_nb_lwps();
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*              Choix des lwp, choix des threads, etc.                    */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

static __inline__ int CAN_RUN(__lwp_t *lwp, marcel_t pid)
{
  return
    ( 1
#ifdef MA__MULTIPLE_RUNNING
      && (pid->ext_state != MARCEL_RUNNING)
#endif
#ifdef MA__LWPS
      && (!marcel_vpmask_vp_ismember(pid->vpmask, lwp->number))
#endif
      );
}

static __inline__ int CANNOT_RUN(__lwp_t *lwp, marcel_t pid)
{
  return !CAN_RUN(lwp, pid);
}

#ifndef MA__ONE_QUEUE

// On cherche le premier LWP compatible avec le 'vpmask' de la tâche
// en commençant par 'first'.
static __inline__ __lwp_t *__find_first_lwp(marcel_t pid, __lwp_t *first)
{
  __lwp_t *lwp = first;

  do {
    if(!marcel_vpmask_vp_ismember(pid->vpmask, lwp->number))
      return lwp;
    lwp = next_lwp(lwp);
  } while(lwp != first);

  // Si on arrive ici, c'est une erreur de l'application
  RAISE(CONSTRAINT_ERROR);
  return NULL;
}

static __inline__ __lwp_t *find_first_lwp(marcel_t pid, __lwp_t *first)
{
  __lwp_t *lwp;

  lwp_list_lock();

  lwp = __find_first_lwp(pid, first);

  lwp_list_unlock();

  return lwp;
}

static __inline__ __lwp_t *find_next_lwp(marcel_t pid, __lwp_t *first)
{
  __lwp_t *lwp;

  lwp_list_lock();

  lwp = __find_first_lwp(pid, next_lwp(first));

  lwp_list_unlock();

  return lwp;
}

// Le cas echeant, retourne un LWP fortement sous-charge (nb de
// threads running < THREAD_THRESHOLD_LOW), ou alors retourne le LWP
// "suggested".
static __inline__ __lwp_t *find_good_lwp(marcel_t pid, __lwp_t *suggested)
{
  __lwp_t *first = find_first_lwp(pid, suggested);
  __lwp_t *lwp = first;

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
static __inline__ __lwp_t *find_best_lwp(marcel_t pid)
{
  __lwp_t *first = find_first_lwp(pid, &__main_lwp);
  unsigned nb = SCHED_DATA(first).running_tasks;
  __lwp_t *best, *lwp;

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

static __inline__ __lwp_t *choose_lwp(marcel_t t)
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

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*                          statistiques                                  */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

// Utilise par les fonctions one_more_task, wait_all_tasks, etc.
static marcel_lock_t __wait_lock = MARCEL_LOCK_INIT;

// Appele a chaque fois qu'une tache est cree (y compris par le biais
// de end_hibernation).
void marcel_one_more_task(marcel_t pid)
{
  marcel_lock_acquire(&__wait_lock);

  pid->number = task_number++;
  _main_struct.nb_tasks++;

  marcel_lock_release(&__wait_lock);
}

// Appele a chaque fois qu'une tache est terminee.
void marcel_one_task_less(marcel_t pid)
{
  marcel_lock_acquire(&__wait_lock);

  if(--_main_struct.nb_tasks == 0 && _main_struct.main_is_waiting) {
    marcel_wake_task(__main_thread, &_main_struct.blocked);
  }

  marcel_lock_release(&__wait_lock);
}

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

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*                            SCHEDULER                                   */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

// Pour le debug. ATTENTION : sched_lock/unlock doivent être appelés
// avant et après !
static __inline__ void display_sched_queue(__lwp_t *lwp)
{
  marcel_t t;

  LOG_IN();
  t = SCHED_DATA(lwp).first;
  if(t) {
    mdebug_sched_q("\t\t\t<Queue of regular tasks on LWP %d:\n", lwp->number);
    do {
#ifdef MA__MULTIPLE_RUNNING
      mdebug_sched_q("\t\t\t\tThread %p (num %d, LWP %d, ext_state %p, sf %p)\n",
	     t, t->number, GET_LWP(t)->number, t->ext_state, t->special_flags);
#else
      mdebug_sched_q("\t\t\t\tThread %p (num %d, LWP %d, sf %p)\n" ,
             t, t->number, GET_LWP(t)->number, t->special_flags);
#endif
      t = next_task(t);
    } while(t != SCHED_DATA(lwp).first);
    mdebug_sched_q("\t\t\t>\n");
  }

#ifdef MARCEL_RT
  t = SCHED_DATA(lwp).rt_first;
  if(t) {
    mdebug_sched_q("\t\t\t<Queue of real-time tasks on LWP %d:\n", lwp->number);
    do {
      mdebug_sched_q("\t\t\t\tThread %p (num %d, LWP %d, ext_state %p, sf %p)\n",
	     t, t->number, GET_LWP(t)->number, t->ext_state, t->special_flags);
      t = next_task(t);
    } while(t != SCHED_DATA(lwp).rt_first);
    mdebug_sched_q("\t\t\t>\n");
  }
#endif
  LOG_OUT();
}

// Insere la tache "idle" et retourne son identificateur. Cette
// fonction est uniquement appelee dans "radical_next_task" lorsqu'il
// n'y a pas d'autres taches "prêtes" dans la file.
static marcel_t insert_and_choose_idle_task(marcel_t cur, __lwp_t *lwp)
{
  marcel_t idle;
#if defined(MA__ACTIVATION) || defined(MA__SMP)
  if (cur && IS_TASK_TYPE_IDLE(cur)) {
    /* c'est déjà idle qui regarde s'il y a autre chose. Apparemment,
     * ce n'est pas le cas */
    return NULL;
  }
#endif

  idle = lwp->idle_task;
  mdebug("\t\t\t<Idle rescheduled: %d on LWP(%d)\n",
	 idle->number, lwp->number);
  marcel_wake_task(idle, NULL);

  return idle;
}

// Cherche effectivement la prochaine tâche à exécuter. En mode
// "MARCEL_RT", on regarde d'abord s'il existe une éventuelle tâche
// 'real-time'. Si aucune tâche n'est trouvée, alors on retourne la
// tâche courante (cur).
static __inline__ marcel_t next_runnable_task(marcel_t cur, __lwp_t *lwp,
					      boolean avoid_self,
					      boolean cur_is_valid
					      )
     // * avoid_self : on doit retourner une autre tâche que nous
     // même. Utile uniquement en mode RT pour aller voir dans l'autre
     // file si cur est RT et qu'on ne trouve pas d'autres tâche RT
     // convenables.  
     // * cur_is_valid : TRUE sauf si cur n'est pas dans une des deux
     // files. Ceci ne survient qu'avec les pseudo threads des upcalls
     // new.
{
  register marcel_t res=NULL;
  register marcel_t first;

  IDLE_LOG_IN();

#ifdef MARCEL_RT
  // S'il existe des tâches RT, on cherche d'abord parmi elles.
  do {
    if (cur_is_valid && MA_TASK_REAL_TIME(cur)) {
      // Si on est dans la file et on est RT, on commence par nous
      first = cur;
      mdebug_sched_q("commençons pour nous (rt): %p\n", first);
    } else if ((first = SCHED_DATA(lwp).rt_first) == NULL) {
      // S'il n'y a pas de tâche RT, on part.
      break;
    } else {
      mdebug_sched_q("commençons au début rt: %p\n", first);
      if (CAN_RUN(lwp, first)) {
	// Le premier RT convient.
	IDLE_LOG_OUT();
	return first;
      }
    }
    res = next_task(first);    
#ifdef MA__MULTIPLE_RUNNING
    while (res != first) {
      if (CAN_RUN(lwp, res)) {
	IDLE_LOG_OUT();
	return res;
      }
      mdebug_sched_q("non, pas rt %p\n", res);
      res = next_task(res);      
    }
#else
    if (res != cur) {
      // Une autre tâche que nous même est là. On la prend.
      IDLE_LOG_OUT();
      return res;
    }
#endif
  } while (0);

  // On n'a pas trouvé de nouvelle tâche RT. Si res==cur, c'est que
  // cur est RT. Si avoid_self est faux, ça convient.
  if ((!avoid_self) && (res==cur)) {
    IDLE_LOG_OUT();
    return res;
  }
#endif

  // On arrive ici si aucune tâche RT ne convient. Il y a plusieurs
  // raisons à ceci :
  //    - aucune tâche RT ne convient (mauvais LWP ou absence de tâche RT)
  //    - une seule tâche RT disponible : nous-même. Mais avoid_self==TRUE.

  // S'il existe des tâches normales, on cherche parmi elles.
  do {
    if (cur_is_valid && !MA_TASK_REAL_TIME(cur)) {
      // Si on est dans la file, on commence par nous
      first = cur;
      mdebug_sched_q("commençons par nous: %p\n", first);
    } else if ((first = SCHED_DATA(lwp).first) == NULL) {
      // S'il n'y a pas de tâche, on part.
      IDLE_LOG_OUT();
      return cur;
    } else {
      mdebug_sched_q("commençons au début: %p\n", first);
      if (CAN_RUN(lwp, first)) {
	// Le premier thread convient.
	IDLE_LOG_OUT();
	return first;
      }
    }
    // Rem: sans MA__MULTIPLE_RUNNING, un des cas précédents a
    // forcément réussi : CAN_RUN(thread) est défini à TRUE...

    res = next_task(first);
#ifdef MA__MULTIPLE_RUNNING
    while (res != first) {
      if (CAN_RUN(lwp, res)) {
	LOG_OUT();
	return res;
      }
      mdebug_sched_q("non, pas %p\n", res);
      res = next_task(res);
    }
#else
    IDLE_LOG_OUT();
    return res;
#endif
  } while (0);

  // On n'a pas trouvé de tâche. On renvoie cur.
  
  IDLE_LOG_OUT();
  return cur;
}

// Cherche la prochaine tâche à exécuter et la marque 'RUNNING'.
// Appelé par "yield".
static __inline__ marcel_t next_task_to_run(marcel_t cur, __lwp_t *lwp)
{
  register marcel_t res;

  sched_lock(lwp);

  res = next_runnable_task(cur, lwp, FALSE, TRUE);
  SET_STATE_RUNNING(cur, res, lwp);

  sched_unlock(lwp);

  return res;
}

// Retourne une tache prete differente de "cur". Le cas echeant,
// insere la tache "idle" (ou "poll"). Attention: on suppose ici que
// la tache renvoyee _sera_ ordonnancee au retour. On suppose
// egalement que "sched_lock" a ete appele.
static marcel_t radical_next_task(marcel_t cur, __lwp_t *lwp)
{
  marcel_t next;

  IDLE_LOG_IN();

  next = next_runnable_task(cur, lwp, TRUE, TRUE);
  if(next == cur)
    next = insert_and_choose_idle_task(cur, lwp);

  IDLE_LOG_OUT();
  return next;
}

// Comme la précédente, mais appelée depuis l'upcall NEW. La pseudo
// tâche courante n'est donc dans aucune file.
marcel_t marcel_give_hand_from_upcall_new(marcel_t cur, __lwp_t *lwp)
{
#ifndef MA__ACTIVATION
  RAISE(PROGRAM_ERROR);
  return NULL;
#else
  marcel_t next;

  DEFINE_CUR_LWP( , ,);

  LOG_IN();

  SET_CUR_LWP(lwp);
  mdebug("upcall...%p on %p\n", cur, cur_lwp);

  sched_lock(cur_lwp);
  next = next_runnable_task(cur, lwp, TRUE, FALSE);
  if(next == cur)
    next = insert_and_choose_idle_task(cur, lwp);
  SET_STATE_RUNNING(NULL, next, cur_lwp);
  sched_unlock(cur_lwp);

  mdebug("\tupcall_new next=%p (state %i)\n", next, next->ext_state);

  /* On ne veut pas être mis en non_running */
  GET_LWP(marcel_self())->prev_running=NULL;
  MA_THR_LONGJMP(next, NORMAL_RETURN);

  LOG_OUT();
  return next;
#endif
}

// Réalise effectivement l'insertion d'une tâche dans la file des
// tâches prêtes. En fait, en mode "MARCEL_RT", il y a deux files : en
// fonction du caractère 'realtime' ou pas de la tâche, on la place
// respectivement dans la file 'rt_first' ou dans la file 'first'.
inline static void insert_running_queue(marcel_t t, __lwp_t *lwp)
{
  volatile head_running_list_t *queue;

#ifdef MARCEL_RT
  if(MA_TASK_REAL_TIME(t)) {
    queue = &SCHED_DATA(lwp).rt_first;
    __rt_task_exist = 1;
  } else
#endif
    queue = &SCHED_DATA(lwp).first;

  MTRACE("INSERT", t);

  if(*queue == NULL)
    INIT_LIST_HEAD(&t->task_list);
  else
    list_add_tail(&t->task_list, &((*queue)->task_list));

  *queue = t;
}

// Insère la tâche t (de manière inconditionnelle) dans une file de
// tâches prêtes. L'état est positionné à RUNNING. Dans le cas de
// files multiples (!= MA__ONE_QUEUE), l'attribut sched_policy est
// consulté afin d'utiliser l'algorithme approprié pour le choix de la
// file...
void marcel_insert_task(marcel_t t)
{
  DEFINE_CUR_LWP(,,);

  LOG_IN();

#ifdef MA__ONE_QUEUE
  // La variable cur_lwp pourrait en fait rester indéfinie ici (!) car
  // la macro SCHED_DATA() (utilisée par sched_lock) ignore le
  // paramètre si MA__ONE_QUEUE est défini. Ceci dit, ce n'est pas
  // joli, c'est pourquoi cur_lwp est positionné à sa juste valeur
  // par précaution...
  SET_CUR_LWP(GET_LWP(marcel_self()));
#else
  SET_CUR_LWP(choose_lwp(t));
#endif

  // De manière générale, il faut d'abord acquérir le "lock" sur la
  // file choisie, sauf si la tâche à insérer est la tâche "idle",
  // auquel cas le lock est déjà acquis.
  if(!MA_TASK_NO_USE_SCHEDLOCK(t))
    sched_lock(cur_lwp);

#if defined(MA__SMP) && !defined(MA__ONE_QUEUE)
  // Attention : modif récente. Peut-être source de bugs.
  // Surtout, ne pas oublier de positionner ce champ !
  t->lwp = cur_lwp;
#endif

  mdebug("\t\t\t<Insertion of task %p on LWP %d>\n", t, cur_lwp->number);

  insert_running_queue(t, cur_lwp);

  SET_RUNNING(t);
  one_more_active_task(t, cur_lwp);

  // On relâche le "lock" acquis précédemment
  if(!MA_TASK_NO_USE_SCHEDLOCK(t))
    sched_unlock(cur_lwp);

#ifndef MA__ONE_QUEUE
  // Lorsque l'on insére un tâche dans une file, on positionne le
  // champ "has_new_tasks" à 1 afin de signaler l'évènement à la tâche
  // idle. Si on contraire on insère idle, alors on remet le champ à
  // 0. NOTE: Ce champ n'est pas vraiment nécessaire car la tâche idle
  // pourrait simplement tester son successeur dans la file...
  if(MA_GET_TASK_TYPE(t) == MA_TASK_TYPE_NORMAL)
    SCHED_DATA(cur_lwp).has_new_tasks = TRUE;
  else {
    if (cur_lwp != GET_LWP(marcel_self()))
      RAISE("INTERNAL ERROR");
    SCHED_DATA(cur_lwp).has_new_tasks = FALSE;
  }
#endif

  LOG_OUT();
}

// Réveille la tâche t. Selon son état (sleeping, blocked, frozen), la
// tâche est d'abord retirée d'une éventuelle autre file, ensuite
// "insert_task" est appelée. Si la tâche était déjà RUNNING (c'est le
// cas lorsque la tâche a été dévié alors qu'elle était bloquée par
// exemple), "insert_task" n'est pas appelée et la fonction a juste
// pour effet de positionner *blocked à FALSE.
void marcel_wake_task(marcel_t t, boolean *blocked)
{
  LOG_IN();

  mdebug("\t\t\t<Waking thread %p (num %d)>\n",
	 t, t->number);

  if(IS_SLEEPING(t)) {

    // Le cas SLEEPING est un cas particulier : la fonction est
    // forcément appelée depuis "check_sleeping" qui a déjà acquis le
    // verrou "__delayed_lock", donc il ne faut pas tenter de
    // l'acquérir à nouveau...

    list_del(&t->task_list);

    one_sleeping_task_less(t);

  } else if(IS_BLOCKED(t)) {

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS

    marcel_lock_acquire(&__blocking_lock);

    list_del(&t->task_list);

    marcel_lock_release(&__blocking_lock);

#endif

    one_blocked_task_less(t);

  } else if(IS_FROZEN(t)) {

    one_frozen_task_less(t);

  }

  mdebug("%p on LWP(%d) is waking %p. locked = %d\n",
	 marcel_self(), GET_LWP(marcel_self())->number, t, locked());

  if(!IS_RUNNING(t))
    marcel_insert_task(t);

  if(blocked != NULL)
    *blocked = FALSE;

  LOG_OUT();
}

static __inline__ void do_unchain_from_queue(marcel_t pid, __lwp_t *lwp)
{
  volatile head_running_list_t *queue;

#ifdef MARCEL_RT
  if(MA_TASK_REAL_TIME(pid))
    queue = &SCHED_DATA(lwp).rt_first;
  else
#endif
    queue = &SCHED_DATA(lwp).first;

  MTRACE("UNCHAIN", pid);

  if(list_empty(&pid->task_list)) {
    *queue = NULL;
#ifdef MARCEL_RT
  if(MA_TASK_REAL_TIME(pid))
    __rt_task_exist = 0;
#endif
  } else {
    *queue = next_task(pid); // par exemple
    list_del(&pid->task_list);
  }
}

/* Remove from ready queue and insert into waiting queue
   (if it is BLOCKED) or delayed queue (if it is WAITING). */
marcel_t marcel_unchain_task_and_find_next(marcel_t t, marcel_t find_next)
{
  marcel_t r, cur=marcel_self();
  DEFINE_CUR_LWP(,= , GET_LWP(cur));

  IDLE_LOG_IN();

  sched_lock(cur_lwp);

  one_active_task_less(t, cur_lwp);

#if defined(MA__LWPS) && ! defined(MA__ONE_QUEUE)
  /* For affinity scheduling: */
  t->previous_lwp = cur_lwp;
#endif

  r = cur;
  if (t == cur) {
    /* on s'enlève nous même, on doit se trouver un remplaçant */
    if (find_next == FIND_NEXT) {
      r = radical_next_task(cur, cur_lwp);
      if(r == NULL) {
	/* dans le cas où on est la tache idle et que l'on ne trouve
         * personne d'autre */
	SET_RUNNING(cur);
	sched_unlock(cur_lwp);

	IDLE_LOG_OUT();
	return r;
      }
      SET_STATE_RUNNING(t, r, cur_lwp);
#ifdef MA__DEBUG
    } else if (find_next == DO_NOT_REMOVE_MYSELF){
      RAISE("Removing ourself without new task\n");
#endif
    } else {
      mdebug("We remove ourself, but we know it\n");
      r = find_next;
    }
  }

  do_unchain_from_queue(t, cur_lwp);

#ifdef MARCEL_DEBUG
  display_sched_queue(cur_lwp);
#endif

  sched_unlock(cur_lwp);

  if(IS_SLEEPING(t)) {

    one_more_sleeping_task(t);

    marcel_lock_acquire(&__delayed_lock);

    /* insertion dans "__delayed_tasks" */
    {
      register struct list_head *pos;

      list_for_each(pos, &__delayed_tasks) {
	if ( list_entry(pos, task_desc, task_list)->time_to_wake 
	     >= t->time_to_wake ) 
	  break;
      }
      /* Insertion juste avant la position trouvée */
      /* Si la liste est vide, pos=&__delayed_tasks donc ça marche */
      /* Si on va jusqu'au bout de la boucle précédente,
         pos=&__delayed_tasks donc ça marche aussi */
      list_add_tail(&t->task_list, pos);
    }

    marcel_lock_release(&__delayed_lock);

  } else if(IS_BLOCKED(t)) {

    one_more_blocked_task(t);

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS

    marcel_lock_acquire(&__blocking_lock);
    list_add_tail(&t->task_list, &__waiting_tasks);
    marcel_lock_release(&__blocking_lock);

#endif

  } else if(IS_FROZEN(t)) {

    one_more_frozen_task(t);
    INIT_LIST_HEAD(&t->task_list);

  }

  IDLE_LOG_OUT();

  return r;
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Changement de contexte                                         */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

#ifdef MA__ACTIVATION
marcel_t marcel_next[ACT_NB_MAX_CPU];

inline static void act_goto_next_task(marcel_t pid, int from)
{
	marcel_next[GET_LWP_NUMBER(marcel_self())]=pid;
	
	mdebug("\t\tcall to ACT_CNTL_RESTART_UNBLOCKED\n");
	act_cntl(ACT_CNTL_RESTART_UNBLOCKED, (void*)from);
	mdebug("\t\tcall to ACT_CNTL_RESTART_UNBLOCKED aborted\n");

	if (pid) {
		MA_THR_LONGJMP((pid), NORMAL_RETURN);
	}
}
#else
#define act_goto_next_task(pid,from) ((void)0)
#endif

inline static int act_to_restart()
{
#ifdef MA__ACTIVATION
  return act_nb_unblocked;
#else
  return 0;
#endif
}

// TODO: C'est dans ces fonctions qu'il faut tester si une activation
// est debloquee...  NOTE: Le parametre "pid" peut etre NULL dans le
// cas ou l'on sait deja qu'une activation est debloquee.
inline static int goto_next_task(marcel_t pid)
{
  if (act_to_restart()) {
    act_goto_next_task(pid, ACT_RESTART_FROM_SCHED);
  } else {
    MA_THR_LONGJMP((pid), NORMAL_RETURN);
  }
  return 0;
}

inline static int can_goto_next_task(marcel_t current, marcel_t pid)
{
  if (act_to_restart()) {
    act_goto_next_task(pid, ACT_RESTART_FROM_SCHED);
    return 0;
  }
    
  if (pid != current) {
    MA_THR_LONGJMP((pid), NORMAL_RETURN);
  }
  return 0;
}

static __inline__ void marcel_switch_to(marcel_t cur, marcel_t next)
{
  if (cur != next) {
    if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
      MA_THR_RESTARTED(cur, "Preemption");

      LOG_OUT();
      return;
    }
    mdebug("switchto(%p, %p) on LWP(%d)\n",
	   cur, next, GET_LWP(cur)->number);
    goto_next_task(next);
  }
}

// Réalise effectivement un changement de contexte vers une autre
// tâche, ou ne fait rien dans le cas où aucune autre tâche n'est
// prête. Note : on suppose que les appels à lock_task/unlock_task
// sont effectués en dehors...
void ma__marcel_yield(void)
{
  register marcel_t cur = marcel_self();
  DEFINE_CUR_LWP(, =, GET_LWP(cur));

  LOG_IN();

  marcel_switch_to(cur, next_task_to_run(cur, cur_lwp));
  
  LOG_OUT();
}

#ifdef MARCEL_RT
// Cette fonction est appelée dans unlock_task() lorsque l'on devine
// qu'il y a probablement une tâche "real-time" à exécuter en
// priorité. Il faut donc d'abord vérifier qu'il en existe puis lui
// céder la main.
void ma__marcel_find_and_yield_to_rt_task(void)
{
  marcel_t first, cur = marcel_self();
  DEFINE_CUR_LWP(,= ,GET_LWP(cur));

  LOG_IN();

  sched_lock(cur_lwp);

  first = SCHED_DATA(cur_lwp).rt_first;

  if(first != NULL) {
#ifdef MA__MULTIPLE_RUNNING
    marcel_t res = first;
    do {
      if(CAN_RUN(cur_lwp, res)) {
	SET_STATE_RUNNING(cur, res, cur_lwp);
	sched_unlock(cur_lwp);
	mdebug("\t\t\t<Must yield to real-time task %d>\n", res->number);
	marcel_switch_to(cur, res); // on choisit 'res'
	return;
      }
      res = next_task(res);
    } while (res != first);
#else
    SET_STATE_RUNNING(cur, first, cur_lwp);
    sched_unlock(cur_lwp);
    mdebug("\t\t\t<Must yield to real-time task %d>\n", first->number);
    marcel_switch_to(cur, first); // On choisit 'first'
    return;
#endif
  }

  sched_unlock(cur_lwp);

  LOG_OUT();
}
#endif

// Effectue un changement de contexte + éventuellement exécute des
// fonctions de scrutation...
void marcel_yield(void)
{
  LOG_IN();

  lock_task();
  marcel_check_polling(MARCEL_POLL_AT_YIELD);
  ma__marcel_yield();
  unlock_task();

  LOG_OUT();
}

void marcel_give_hand(boolean *blocked, marcel_lock_t *lock)
{
  marcel_t next;
  register marcel_t cur = marcel_self();
#ifdef MA__LWPS
  volatile boolean first_time = TRUE;
#endif

  LOG_IN();

  if(locked() != 1) {
    RAISE(LOCK_TASK_ERROR);
  }
  do {
    if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
      MA_THR_RESTARTED(cur, "Preemption");
#ifdef MA__WORK
      if (*blocked) {
	unlock_task();
	lock_task();
      }
#endif
    } else {
      SET_BLOCKED(cur);
      next = UNCHAIN_TASK_AND_FIND_NEXT(cur);

#ifdef MA__LWPS
      if(first_time) {
	first_time = FALSE;
	
	marcel_lock_release(lock);
      }
#endif
      goto_next_task(next);
    }
  } while(*blocked);
  unlock_task();

  LOG_OUT();
}

void marcel_tempo_give_hand(unsigned long timeout,
			    boolean *blocked, marcel_sem_t *s)
{
  marcel_t next, cur = marcel_self();
  unsigned long ttw = __milliseconds + timeout;
  volatile boolean first_time = TRUE;

#if defined(MA__ACTIVATION)
  RAISE(NOT_IMPLEMENTED);
#endif

  LOG_IN();

  if(locked() != 1)
    RAISE(LOCK_TASK_ERROR);
  marcel_disablemigration(cur);
  do {
    if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
      MA_THR_RESTARTED(cur, "Preemption");

      if((*blocked) && __milliseconds >= ttw) {
	/* Expiration timer ou retour d'une deviation : */
	/* le thread n'a pas ete reveille par un sem_V ! */
	cell *pc = NULL,
	     *cc;

	marcel_lock_acquire(&s->lock);

	cc = s->first;
	while(cc->task != cur) {
	  pc = cc;
	  cc = cc->next;
	}
	(s->value)++;
	if(pc == NULL)
            s->first = cc->next;
         else if((pc->next = cc->next) == NULL)
            s->last = pc;

	marcel_lock_release(&s->lock);

	marcel_enablemigration(cur);
	unlock_task();
	RAISE(TIME_OUT);
      }
    } else {

      if(first_time) {
	first_time = FALSE;
	
	marcel_lock_release(&s->lock);
      }

      cur->time_to_wake = ttw;
      SET_SLEEPING(cur);
      next = UNCHAIN_TASK_AND_FIND_NEXT(cur);

      goto_next_task(next);
    }
  } while(*blocked);
  marcel_enablemigration(cur);
  unlock_task();

  LOG_OUT();
}

void marcel_delay(unsigned long millisecs)
{
#ifdef MA__ACTIVATION
  usleep(millisecs*1000);
#else
  marcel_t p, cur = marcel_self();
  unsigned long ttw = __milliseconds + millisecs;

  LOG_IN();

  mdebug("\t\t\t<Thread %p goes sleeping>\n", cur);

  lock_task();

  do {
    if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
      MA_THR_RESTARTED(cur, "Preemption");
    } else {

      cur->time_to_wake = ttw;
      SET_SLEEPING(cur);
      p = UNCHAIN_TASK_AND_FIND_NEXT(cur);

      goto_next_task(p);
    }

  } while(__milliseconds < ttw);

  unlock_task();

  LOG_OUT();
#endif
}

// Modifie le 'vpmask' du thread courant. Le cas échéant, il faut donc
// retirer le thread de la file et le replacer dans la file
// adéquate...
void marcel_change_vpmask(marcel_vpmask_t mask)
{
  marcel_t cur = marcel_self();
  DEFINE_CUR_LWP(, =, GET_LWP(cur));

  LOG_IN();

  lock_task();

  cur->vpmask = mask;

  if(marcel_vpmask_vp_ismember(mask, cur_lwp->number)) {
#ifdef MA__ONE_QUEUE
    // Le LWP courant n'est plus autorisé : il faut s'extraire de la
    // file pour s'y ré-insérer !
    marcel_t next;

    // Il suffit d'effectuer un 'radical_next_task' en fait
    sched_lock(cur_lwp);

    next = radical_next_task(cur, cur_lwp);
    SET_STATE_RUNNING(cur, next, cur_lwp);

    sched_unlock(cur_lwp);

    marcel_switch_to(cur, next);
#else
    // Files multiples : c'est plus délicat. Il faut s'insérer dans
    // une file distante mais ATTENTION : dès que l'on aura relâché le
    // verrou de la file, un LWP pourra nous exécuter, donc il ne faut
    // pas rester sur cette pile...
    RAISE(NOT_IMPLEMENTED);
#endif
  }

  unlock_task();

  LOG_OUT();
}

// Checks to see if some tasks should be waked. NOTE: The function
// assumes that "lock_task()" was called previously.
static int marcel_check_sleeping(void)
{
  unsigned long present = __milliseconds;
  int waked_some_task = 0;
  register struct list_head *pos;

  IDLE_LOG_IN();

  if(marcel_lock_tryacquire(&__delayed_lock)) {

    pos=__delayed_tasks.next;
    while(pos != &__delayed_tasks) {
      register marcel_t t = list_entry(pos, task_desc, task_list);
      if (t->time_to_wake > present)
	break;
      
      mdebug("\t\t\t<Check delayed: waking thread %p (%ld)>\n",
	     t, t->number);
      pos=pos->next;
      marcel_wake_task(t, NULL);
      waked_some_task = 1;
    }

    marcel_lock_release(&__delayed_lock);

  } else
    mdebug("LWP(%d) failed to acquire __delayed_lock\n",
	   marcel_self()->lwp->number);

  IDLE_LOG_OUT();

  return waked_some_task;
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Gestion des threads idle                                       */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/


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

static sigset_t sigalrmset, sigeptset;
static void start_timer(void);
static void set_timer(void);
void stop_timer(void);

static __inline__ void idle_check_end(__lwp_t *lwp)
{
#ifdef MA__SMP
  IDLE_LOG_IN();
  if(lwp->has_to_stop) {
#ifdef MA__TIMER
    stop_timer();
#endif
    if(lwp == &__main_lwp) {
      IDLE_LOG_OUT();
      RAISE(PROGRAM_ERROR);
    } else {
      mdebug("\t\t\t<LWP %d exiting>\n", lwp->number);
      longjmp(lwp->home_jb, 1);
    }
  }
  IDLE_LOG_OUT();
#endif // MA__SMP
}

static __inline__ void idle_check_tasks_to_wake(__lwp_t *lwp)
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
      pause();
    }
  }
#endif /* MA__MONO */

  IDLE_LOG_OUT();
}

static __inline__ marcel_t idle_find_runnable_task(__lwp_t *lwp)
{
  marcel_t next = NULL, cur = marcel_self();

  IDLE_LOG_IN();

#ifdef MA__ONE_QUEUE

  // next sera positionné à NULL si aucune autre tâche prête n'est
  // trouvée (et dans ce cas SET_RUNNING(cur) est effectué par
  // unchain_task).
  SET_FROZEN(cur);
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
    SET_FROZEN(cur);
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

    if((next != NULL) || (act_to_restart())) {
      break;
    }

    /* If nothing successful */
    PAUSE();
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

#ifdef MA__TIMER
  start_timer(); /* May be redundant for main LWP */
#endif

  /* Except at the beginning, lock_task() is supposed to be permanently
     handled */
  lock_task();

  /* Nécessaire pour les activations, sans effet pour le reste */
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

    // next peut être NULL avec les activations (une serait prête à
    // être réveillée)
    if (next)
      marcel_switch_to(cur, next);
#ifdef MA__ACTIVATION
    else {
      mdebug("idle can have job (LWP = %d)\n",
	     cur_lwp->number);
      act_goto_next_task(NULL, ACT_RESTART_FROM_IDLE);
    }
#endif

  }
  LOG_OUT(); // Probably never executed
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

static void init_lwp(__lwp_t *lwp, marcel_t initial_task)
{
  static unsigned __nb_lwp = 0;
  marcel_attr_t attr;

  LOG_IN();

  lwp_list_lock();
  {
    if (__nb_lwp >= MA__MAX_LWPS) 
      RAISE("Too many lwp\n");

    SET_LWP_NB(__nb_lwp, lwp);
    lwp->number = __nb_lwp++;
  }
  lwp_list_unlock();

#ifdef MA__MULTIPLE_RUNNING
  lwp->prev_running=NULL;
#endif
#ifdef MA__ACTIVATION
  lwp->upcall_new_task=NULL;
#endif

  atomic_set(&lwp->_locked, 0);

  lwp->has_to_stop = FALSE;

  marcel_mutex_init(&lwp->stack_mutex, NULL);
  lwp->sec_desc = SECUR_TASK_DESC(lwp);

#ifdef MA__ONE_QUEUE
  /* on initialise une seule fois la structure. */
  if (initial_task) 
#endif
  {
    SCHED_DATA(lwp).sched_queue_lock = MARCEL_LOCK_INIT;
    SCHED_DATA(lwp).running_tasks = 0;
    SCHED_DATA(lwp).has_new_tasks = FALSE;
    SCHED_DATA(lwp).rt_first = NULL;

    if(initial_task) {
      SCHED_DATA(lwp).first = initial_task;
      initial_task->lwp = lwp;
      mdebug("marcel_self : %p, lwp : %i\n", marcel_self(),
	     marcel_self()->lwp->number);
      INIT_LIST_HEAD(&initial_task->task_list);
      SET_STATE_RUNNING(NULL, initial_task, GET_LWP(initial_task));
      /* Désormais, on s'exécute dans le lwp 0 */
    }
  }

  /*****************************************/
  /* Création de la tâche Idle (idle_task) */
  /*****************************************/
  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);
  marcel_attr_setflags(&attr, MA_SF_POLL | MA_SF_NOSCHEDLOCK | MA_SF_NORUN);
#ifdef PM2
  {
    char *stack = __TBX_MALLOC(2*SLOT_SIZE, __FILE__, __LINE__);

    unsigned long stsize = (((unsigned long)(stack + 2*SLOT_SIZE) & 
			     ~(SLOT_SIZE-1)) - (unsigned long)stack);

    marcel_attr_setstackaddr(&attr, stack);
    marcel_attr_setstacksize(&attr, stsize);
  }
#endif
  lock_task();

  marcel_create(&(lwp->idle_task), &attr, idle_func, NULL);

  marcel_one_task_less(lwp->idle_task);

  MTRACE("IdleTask", lwp->idle_task);

#if !defined(MA__SMP) || defined(MA__ONE_QUEUE)
  if(initial_task)
#endif
    {
      // En mode 'smp multi-files', on doit retirer _toutes_ les tâches idle
      mdebug("Extracting idle of LWP(%d)\n", lwp->number);
      SET_FROZEN(lwp->idle_task);
      UNCHAIN_TASK(lwp->idle_task);
    }

#ifndef MA__ONE_QUEUE
  if (!initial_task) {
    SCHED_DATA(lwp).first = lwp->idle_task;
    SCHED_DATA(lwp).rt_first = NULL;
    INIT_LIST_HEAD(&lwp->idle_task->task_list);
  }
#endif

  lwp->idle_task->lwp = lwp;
#if defined(MA__LWPS) && !defined(MA__ONE_QUEUE) 
  lwp->idle_task->previous_lwp = NULL;
#endif

  // Il vaut mieux généraliser l'utilisation des 'vpmask'
  lwp->idle_task->vpmask = MARCEL_VPMASK_ALL_BUT_VP(lwp->number);

  /***************************************************/

  unlock_task();

#ifdef MA__ACTIVATION
  /****************************************************/
  /* Création de la tâche pour les upcalls upcall_new */
  /****************************************************/
  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);
  marcel_attr_setflags(&attr, MA_SF_UPCALL_NEW | MA_SF_NORUN);
#ifdef PM2
  {
    char *stack = __TBX_MALLOC(2*SLOT_SIZE, __FILE__, __LINE__);

    unsigned long stsize = (((unsigned long)(stack + 2*SLOT_SIZE) & 
			     ~(SLOT_SIZE-1)) - (unsigned long)stack);

    marcel_attr_setstackaddr(&attr, stack);
    marcel_attr_setstacksize(&attr, stsize);
  }
#endif
  lock_task();
  /* la fonction ne sera jamais exécutée, c'est juste pour avoir une
  * structure de thread marcel dans upcall_new
  * */
  sched_unlock(lwp);
  
  marcel_create(&lwp->upcall_new_task, &attr, (void*)void_func, NULL);

  MTRACE("Upcall_Task", lwp->upcall_new_task);
  SET_FROZEN(lwp->upcall_new_task);
  UNCHAIN_TASK(lwp->upcall_new_task);
  lwp->upcall_new_task->lwp = lwp;

  marcel_one_task_less(lwp->upcall_new_task);

  MTRACE("U_T OK", lwp->upcall_new_task);
  /****************************************************/

  unlock_task();
  /* libéré dans marcel_init_sched */
  if (initial_task)
    sched_lock(lwp);
#else // MA__ACTIVATION
  /* libéré dans lwp_startup_func pour SMP ou marcel_init_sched pour
   * le pool de la main_task
   * */
#ifdef MA__ONE_QUEUE
  // Si une seule file, alors seul LWP(0) exécute sched_lock
  if(initial_task)
#endif
    sched_lock(lwp);

#endif /* MA__ACTIVATION */

  LOG_OUT();
}

#if defined(MA__LWPS)

static unsigned __nb_processors = 1;

static void marcel_fix_nb_vps(unsigned nb_lwp)
{
  // Détermination du nombre de processeurs disponibles
#ifdef SOLARIS_SYS
  __nb_processors = sysconf(_SC_NPROCESSORS_CONF);
#elif defined(LINUX_SYS)
  __nb_processors = WEXITSTATUS(system("exit `grep rocessor /proc/cpuinfo"
				       " | wc -l`"));
#elif defined(IRIX_SYS)
  __nb_processors = sysconf(_SC_NPROC_CONF);
#else
  __nb_processors = 1;
#endif

  mdebug("%d processors available\n", __nb_processors);

  // Choix du nombre de LWP
#ifdef MA__ACTSMP
  set_nb_lwps(nb_lwp ? nb_lwp : ACT_NB_MAX_CPU);
#else
  set_nb_lwps(nb_lwp ? nb_lwp : __nb_processors);
#endif
}

#ifdef MA__SMP

static void marcel_bind_on_processor(__lwp_t *lwp)
{
#if defined(BIND_LWP_ON_PROCESSORS) && defined(SOLARIS_SYS)
  if(processor_bind(P_LWPID, P_MYID,
		    (processorid_t)(lwp->number % __nb_processors),
		    NULL) != 0) {
    perror("processor_bind");
    exit(1);
  } 
#ifdef MARCEL_DEBUG
  if(marcel_mdebug.show)
    fprintf(stderr, "LWP %d bound to processor %d\n",
	    lwp->number, (lwp->number % __nb_processors));
#endif
#endif
}

// Attention : pas de mdebug dans cette fonction ! Il faut se
// contenter de fprintf...
static void *lwp_startup_func(void *arg)
{
  __lwp_t *lwp = (__lwp_t *)arg;

  LOG_IN();

  if(setjmp(lwp->home_jb)) {
#ifdef MARCEL_DEBUG
  if(marcel_mdebug.show)
    fprintf(stderr, "sched %d Exiting\n", lwp->number);
#endif
    LOG_OUT();
    marcel_kthread_exit(NULL);
  }

#ifdef MARCEL_DEBUG
  if(marcel_mdebug.show)
    fprintf(stderr, "\t\t\t<LWP %d started (self == %lx)>\n",
	    lwp->number, (unsigned long)marcel_kthread_self());
#endif

  marcel_bind_on_processor(lwp);

  /* Can't use lock_task() because marcel_self() is not yet usable ! */
  atomic_inc(&lwp->_locked);

#ifndef MA__ONE_QUEUE
  /* pris dans  init_lwp() */
  sched_unlock(lwp);
#endif

  MA_THR_LONGJMP(lwp->idle_task, NORMAL_RETURN);
  
  return NULL;
}

#endif // MA__SMP

unsigned marcel_sched_add_vp(void)
{
  __lwp_t *lwp = (__lwp_t *)__TBX_MALLOC(sizeof(__lwp_t), __FILE__, __LINE__),
          *cur_lwp = marcel_self()->lwp;

  LOG_IN();

  // Initialisation de la structure __lwp_t
  init_lwp(lwp, NULL);

  lwp_list_lock();
  {
    // Ajout dans la liste globale des LWP
    list_add(&lwp->lwp_list, &cur_lwp->lwp_list);
  }
  lwp_list_unlock();

#ifdef MA__SMP
  // Lancement du thread noyau "propulseur"
  marcel_kthread_sigmask(SIG_BLOCK, &sigalrmset, NULL);
  marcel_kthread_create(&lwp->pid, lwp_startup_func, (void *)lwp);
  marcel_kthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif
  return lwp->number;

  LOG_OUT();
}

#endif // MA__LWPS

void marcel_sched_init(void)
{
  marcel_initialized = TRUE;

  LOG_IN();

  sigemptyset(&sigeptset);
  sigemptyset(&sigalrmset);
  sigaddset(&sigalrmset, MARCEL_TIMER_SIGNAL);

  _main_struct.nb_tasks = 0;

  memset(__main_thread, 0, sizeof(task_desc));
  __main_thread->detached = FALSE;
  __main_thread->not_migratable = 1;

  __main_thread->sched_policy = MARCEL_SCHED_OTHER;
#ifdef MA__ONE_QUEUE
  __main_thread->vpmask = MARCEL_VPMASK_EMPTY;
#else
  // Limitation (actuelle) du mode 'SMP multi-files' : le thread
  // 'main' doit toujours rester sur le LWP 0
  __main_thread->vpmask = MARCEL_VPMASK_ALL_BUT_VP(0);
#endif

  // Initialization of "main LWP" is _required_ even when SMP not set.
  // 'init_sched' is called indirectly
  init_lwp(&__main_lwp, __main_thread);

  INIT_LIST_HEAD(&__main_lwp.lwp_list);

  SET_STATE_RUNNING(NULL, __main_thread, (&__main_lwp));
  one_more_active_task(__main_thread, &__main_lwp);
  MTRACE("MainTask", __main_thread);

  /* Pris dans  init_sched() */
  sched_unlock(&__main_lwp);
}

void marcel_sched_start(unsigned nb_lwp)
{
#ifdef MA__LWPS
  int i;

#ifdef MA__SMP
    marcel_bind_on_processor(&__main_lwp);
#endif

  marcel_fix_nb_vps(nb_lwp);

  for(i=1; i<get_nb_lwps(); i++)
    marcel_sched_add_vp();
 
  mdebug("marcel_sched_init  : %i lwps created\n",
	 get_nb_lwps());
#endif /* MA__LWPS */

#ifdef MA__ACTIVATION
  init_upcalls(get_nb_lwps());
#endif

#ifdef PM2DEBUG
  pm2debug_marcel_launched=1;
#endif

#ifdef MA__TIMER
  /* Démarrage d'un timer Unix pour récupérer périodiquement 
     un signal (par ex. SIGALRM). */
  start_timer();
#endif

  mdebug("marcel_sched_init done\n");

  LOG_OUT();
}

#ifdef MA__SMP
/* TODO: the stack of the lwp->sched_task is currently *NOT FREED*.
   This should be fixed! */
static void stop_lwp(__lwp_t *lwp)
{
  LOG_IN();

  lwp->has_to_stop = TRUE;

  marcel_kthread_join(lwp->pid);

  LOG_OUT();
}
#endif

// Attend que toutes les taches soient terminees. Cette fonction
// _doit_ etre appelee par la tache "main".
static void wait_all_tasks_end(void)
{
  LOG_IN();

  lock_task();

  marcel_lock_acquire(&__wait_lock);

  if(_main_struct.nb_tasks) {
    _main_struct.main_is_waiting = TRUE;
    _main_struct.blocked = TRUE;
    marcel_give_hand(&_main_struct.blocked, &__wait_lock);
  } else {
    marcel_lock_release(&__wait_lock);
    unlock_task();
  }

  LOG_OUT();
}

void marcel_sched_shutdown()
{
#ifdef MA__SMP
  __lwp_t *lwp;
#endif

  LOG_IN();

  wait_all_tasks_end();

#ifdef MA__TIMER
  stop_timer();
#endif

#if defined(MA__ONE_QUEUE) && defined(MA__MULTIPLE_RUNNING)
  // Si nécessaire, on bascule sur le LWP(0)
  marcel_change_vpmask(MARCEL_VPMASK_ALL_BUT_VP(0));
#endif

#ifdef MA__SMP
  if(marcel_self()->lwp != &__main_lwp)
    RAISE(PROGRAM_ERROR);

  lwp = next_lwp(&__main_lwp);
  while(lwp != &__main_lwp) {
    register __lwp_t *previous;
    stop_lwp(lwp);
    previous=lwp;
    lwp = next_lwp(lwp);
    __TBX_FREE(previous, __FILE__, __LINE__);
  }
#elif defined(MA__ACTIVATION)
  // TODO : arrêter les autres activations...

  //act_cntl(ACT_CNTL_UPCALLS, (void*)ACT_DISABLE_UPCALLS);
#else
  /* Destroy master-sched's stack */
  marcel_cancel(__main_lwp.idle_task);
#ifdef PM2
  /* __sched_task is detached, so we can free its stack now */
  __TBX_FREE(marcel_stackbase(__main_lwp.idle_task), __FILE__, __LINE__);
#endif
#endif

  LOG_OUT();
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Gestion du timer                                               */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

#ifndef TICK_RATE
#define TICK_RATE 1
#endif

inline void marcel_update_time(marcel_t cur)
{
  if(cur->lwp == &__main_lwp)
    __milliseconds += time_slice/1000;
}

/* TODO: Call lock_task() before re-enabling the signals to avoid stack
   overflow by recursive interrupts handlers. This needs a modified version
   of marcel_yield() that do not lock_task()... */
static void timer_interrupt(int sig)
{
  marcel_t cur = marcel_self();
#ifdef MARCEL_DEBUG
  static unsigned long tick = 0;
#endif

#ifdef MARCEL_DEBUG
  if (++tick == TICK_RATE) {
    try_mdebug("\t\t\t<<tick>>\n");
    tick = 0;
  }
#endif

  marcel_update_time(cur);

  if(cur->lwp == NULL) {
    fprintf(stderr, "WARNING!!! %p->lwp == NULL!\n", cur);
  }

  if(!locked() && preemption_enabled()) {

    LOG_IN();

    MTRACE_TIMER("TimerSig", cur);

    lock_task();
    marcel_check_sleeping();
    marcel_check_polling(MARCEL_POLL_AT_TIMER_SIG);
    unlock_task();

#ifdef MA__SMP
    marcel_kthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#else
#if defined(SOLARIS_SYS) || defined(UNICOS_SYS)
    sigrelse(MARCEL_TIMER_SIGNAL);
#else
    sigprocmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif
#endif

    lock_task();
    ma__marcel_yield();
    unlock_task();

    LOG_OUT();
  }
}

static void set_timer(void)
{
  struct itimerval value;

  LOG_IN();

#ifdef MA__SMP
  marcel_kthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif

  if(marcel_initialized) {
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_usec = time_slice;
    value.it_value = value.it_interval;
    setitimer(MARCEL_ITIMER_TYPE, &value, (struct itimerval *)NULL);
  }

  LOG_OUT();
}

static void fault_catcher(int sig)
{
  marcel_t cur = marcel_self();

  fprintf(stderr, "OOPS!!! Signal %d catched on thread %p\n",
	  sig, cur);
  if(cur->lwp != NULL)
    fprintf(stderr, "OOPS!!! current lwp is %d\n", cur->lwp->number);

#ifdef LINUX_SYS
  fprintf(stderr, "OOPS!!! Entering endless loop (pid = %d)\n",
	  getpid());
  for(;;) ;
#endif

  abort();
}

static void start_timer(void)
{
  static struct sigaction sa;

  LOG_IN();

  // On va essayer de rattraper SIGSEGV, etc.
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = fault_catcher;
  sa.sa_flags = 0;
  sigaction(SIGSEGV, &sa, (struct sigaction *)NULL);

  sigemptyset(&sa.sa_mask);
  sa.sa_handler = timer_interrupt;
#ifndef WIN_SYS
  sa.sa_flags = SA_RESTART;
#else
  sa.sa_flags = 0;
#endif

  sigaction(MARCEL_TIMER_SIGNAL, &sa, (struct sigaction *)NULL);

  set_timer();

  LOG_OUT();
}

void stop_timer(void)
{
  LOG_IN();

  time_slice = 0;
  set_timer();

#ifdef MA__SMP
  marcel_kthread_sigmask(SIG_BLOCK, &sigalrmset, NULL);
#else
  sigprocmask(SIG_BLOCK, &sigalrmset, NULL);
#endif

  LOG_OUT();
}

void marcel_settimeslice(unsigned long microsecs)
{
  LOG_IN();

  lock_task();
  if(microsecs == 0) {
    disable_preemption();
    if(time_slice != DEFAULT_TIME_SLICE) {
      time_slice = DEFAULT_TIME_SLICE;
      set_timer();
    }
  } else {
    enable_preemption();
    if(microsecs < MIN_TIME_SLICE) {
      time_slice = MIN_TIME_SLICE;
      set_timer();
    } else if(microsecs != time_slice) {
      time_slice = microsecs;
      set_timer();
    }
  }
  unlock_task();

  LOG_OUT();
}

unsigned long marcel_clock(void)
{
   return __milliseconds;
}

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
  DEFINE_CUR_LWP(, __attribute__((unused)) =, GET_LWP(marcel_self()));

  lock_task();

  t = SCHED_DATA(cur_lwp).first;
  do {
    (*f)(t);
    t = next_task(t);
  } while(t != SCHED_DATA(cur_lwp).first);

  list_for_each(pos, &__delayed_tasks) {
    t = list_entry(pos, task_desc, task_list);
    (*f)(t);
  }

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS

  list_for_each(pos, &__waiting_tasks) {
    t = list_entry(pos, task_desc, task_list);
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
  DEFINE_CUR_LWP(, __attribute__((unused)) =, GET_LWP(marcel_self()));

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
    t = list_entry(pos, task_desc, task_list);
    if (want_to_see(t, which)) {    
      if(nb_pids < max)
	pids[nb_pids++] = t;
      else
	nb_pids++;
    }
  }

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS

  list_for_each(pos, &__waiting_tasks) {
    t = list_entry(pos, task_desc, task_list);
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

