
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
#include <pthread.h>
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


// Param�tre optionnel 'avoid_self' � la fonction 'next_runnable_task'
// dans le cas 'MARCEL_RT'.
#ifdef MARCEL_RT
#define __CALLED_FROM_YIELD__   , FALSE
#define __CALLED_FROM_UNCHAIN__ , TRUE
#else
#define __CALLED_FROM_YIELD__
#define __CALLED_FROM_UNCHAIN__
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

inline static unsigned get_nb_lwps()
{
#ifdef MA__LWPS
  return ma__nb_lwp;
#else
  return 1;
#endif
}

inline static void set_nb_lwps(unsigned value)
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
  if(next_schedpolicy_available <=
     __MARCEL_SCHED_AVAILABLE - MARCEL_MAX_USER_SCHED)
    RAISE(CONSTRAINT_ERROR);

  user_policy[__MARCEL_SCHED_AVAILABLE -
	      next_schedpolicy_available] = func;
  *policy = next_schedpolicy_available--;
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
      && (!marcel_vpmask_vp_ismember(&pid->vpmask, lwp->number))
#endif
      );
}

static __inline__ int CANNOT_RUN(__lwp_t *lwp, marcel_t pid)
{
  return !CAN_RUN(lwp, pid);
}

#ifndef MA__ONE_QUEUE

// Retourne le LWP de numero "vp modulo NB_LWPS" en parcourant
// betement la liste chainee. A ameliorer en utilisant un tableau pour
// indexer les LWPs...
static __inline__ __lwp_t *find_lwp(unsigned vp)
{
  register __lwp_t *lwp = &__main_lwp;

  while(vp--)
    lwp = next_lwp(lwp);

  return lwp;
}

// Le cas echeant, retourne un LWP fortement sous-charge (nb de
// threads running < THREAD_THREASHOLD_LOW), ou alors retourne le LWP
// "suggested".
static __inline__ __lwp_t *find_good_lwp(__lwp_t *suggested)
{
  __lwp_t *lwp = suggested;

  for(;;) {
    if(SCHED_DATA(lwp).running_tasks < THREAD_THRESHOLD_LOW)
      return lwp;
    lwp = next_lwp(lwp);
    if(lwp == suggested)
      return lwp; /* No underloaded LWP, so return the suggested one */
  }
}

// Retourne le LWP le moins charge (ce qui oblige a parcourir toute la
// liste)
static __inline__ __lwp_t *find_best_lwp(void)
{
  __lwp_t *lwp = &__main_lwp;
  __lwp_t *best = lwp;
  unsigned nb = SCHED_DATA(lwp).running_tasks;

  mdebug("find_best_lwp: there are %d running tasks on lwp %d\n",
	 SCHED_DATA(lwp).running_tasks,
	 lwp->number);

  for(;;) {
    lwp = next_lwp(lwp);
    if(lwp == &__main_lwp)
      return best;

    mdebug("find_best_lwp: there are %d running tasks on lwp %d\n",
	   SCHED_DATA(lwp).running_tasks,
	   lwp->number);

    if(SCHED_DATA(lwp).running_tasks < nb) {
      nb = SCHED_DATA(lwp).running_tasks;
      best = lwp;
    }
  }
}

inline static __lwp_t *choose_lwp(marcel_t t)
{
  switch(t->sched_policy) {
  case MARCEL_SCHED_OTHER :
    return GET_LWP(marcel_self());
  case MARCEL_SCHED_AFFINITY :
    return find_good_lwp(t->previous_lwp ? : GET_LWP(marcel_self()));
  case MARCEL_SCHED_BALANCE :
    return find_best_lwp();
  default:
    if(t->sched_policy >= 0) {
      /* MARCEL_SCHED_FIXED(vp) */
      mdebug("\t\t\t<Insertion of task %p requested on LWP %d>\n", 
	     t, t->sched_policy);
      return (t->previous_lwp ? : find_lwp(t->sched_policy));
    } else
      /* MARCEL_SCHED_USER_... */
      return (*user_policy[__MARCEL_SCHED_AVAILABLE - 
			  (t->sched_policy)])(t, GET_LWP(marcel_self()));
  }
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

// Pour le debug. ATTENTION : sched_lock/unlock doivent �tre appel�s
// avant et apr�s !
static __inline__ void display_sched_queue(__lwp_t *lwp)
{
  marcel_t t;

  t = SCHED_DATA(lwp).first;
  if(t) {
    mdebug("\t\t\t<Queue of regular tasks on LWP %d:\n", lwp->number);
    do {
      mdebug("\t\t\t\tThread %p (num %d, LWP %d)\n",
	     t, t->number, GET_LWP(t)->number);
      t = next_task(t);
    } while(t != SCHED_DATA(lwp).first);
    mdebug("\t\t\t>\n");
  }

#ifdef MARCEL_RT
  t = SCHED_DATA(lwp).rt_first;
  if(t) {
    mdebug("\t\t\t<Queue of real-time tasks on LWP %d:\n", lwp->number);
    do {
      mdebug("\t\t\t\tThread %p (num %d, LWP %d)\n",
	     t, t->number, GET_LWP(t)->number);
      t = next_task(t);
    } while(t != SCHED_DATA(lwp).rt_first);
    mdebug("\t\t\t>\n");
  }
#endif
}

// Insere la tache "idle" et retourne son identificateur. Cette
// fonction est uniquement appelee dans "radical_next_task" lorsqu'il
// n'y a pas d'autres taches "pr�tes" dans la file.
static marcel_t insert_and_choose_idle_task(marcel_t cur, __lwp_t *lwp)
{
  marcel_t idle;
#if defined(MA__ACTIVATION) || defined(MA__SMP)
  if (cur && IS_TASK_TYPE_IDLE(cur)) {
    /* c'est d�j� idle qui regarde s'il y a autre chose. Apparemment,
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

// Cherche effectivement la prochaine t�che � ex�cuter. En mode
// "MARCEL_RT", on regarde d'abord s'il existe une �ventuelle t�che
// 'real-time'. Si aucune t�che n'est trouv�e, alors on retourne la
// t�che courante (cur).
static __inline__ marcel_t next_runnable_task(marcel_t cur
					      , __lwp_t *lwp
#ifdef MARCEL_RT
					      , boolean avoid_self
#endif
					      )
{
  register marcel_t res;

#ifdef MARCEL_RT
  marcel_t first;

  // Si 'cur' est une t�che normale mais qu'il existe des t�ches dans
  // la file 'real-time', alors il faut d'abord chercher dans cette
  // file.
  if(!MA_TASK_REAL_TIME(cur) &&
     (first = SCHED_DATA(lwp).rt_first) != NULL) {
#ifdef MA__MULTIPLE_RUNNING
    res = first;
    do {
      if(CAN_RUN(lwp, res))
	return res;
      res = next_task(res);
    } while (res != first);
#else
    return first;
#endif
    // Si on arrive ici, alors res == first et c'est un echec. On va
    // donc passer au bloc d'instruction suivant...
  }
#endif // MARCEL_RT

  // On arrive ici si :
  //     - 'cur' est une t�che 'real-time'
  // ou
  //     - 'cur' est une t�che normale et la file 'real-time' est vide
  // ou
  //     - 'cur' est normale, la file 'real-time' n'est pas vide mais
  //       on n'a pas trouv� de t�ches "ex�cutables sur ce LWP" dedans
  // On cherche donc au sein de sa propre file...
#ifdef MA__MULTIPLE_RUNNING
  res = cur;
  do {
    res = next_task(res);
  } while ((CANNOT_RUN(lwp, res)) && (res != cur));
#else
  res = next_task(cur);
#endif

#ifdef MARCEL_RT
  // Si on est appel� depuis 'radical_next_task' (avoid_self) et que
  // la t�che courante "real-time" n'a pas daign� trouver quelqu'un
  // d'autre, alors il faut aller regarder du c�t� de la file des
  // t�ches normales.
  if(avoid_self && res == cur && MA_TASK_REAL_TIME(cur) &&
     (first = SCHED_DATA(lwp).first) != NULL) {
#ifdef MA__MULTIPLE_RUNNING
    res = first;
    do {
      if(CAN_RUN(lwp, res))
	return res;
      res = next_task(res);
    } while (res != first);
#else
    return first;
#endif
    // Si echec, on retourne cur en d�finitive...
    return cur;
  }
#endif

  return res;
}

// Cherche la prochaine t�che � ex�cuter et la marque 'RUNNING'.
// Appel� par "yield".
static __inline__ marcel_t next_task_to_run(marcel_t cur, __lwp_t *lwp)
{
  register marcel_t res;

  sched_lock(lwp);

  res = next_runnable_task(cur, lwp __CALLED_FROM_YIELD__);
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
  marcel_t first = cur;

  LOG_IN();

#ifdef MA__ACTIVATION
#error � corriger : devrait pouvoir �tre �vit�
  if (first == NULL)
    cur = SCHED_DATA(cur_lwp).first;
#endif

  next = next_runnable_task(cur, lwp __CALLED_FROM_UNCHAIN__);
  if(next == cur)
    next = insert_and_choose_idle_task(first, lwp);

  LOG_OUT();
  return next;
}

// R�alise effectivement l'insertion d'une t�che dans la file des
// t�ches pr�tes. En fait, en mode "MARCEL_RT", il y a deux files : en
// fonction du caract�re 'realtime' ou pas de la t�che, on la place
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

// Ins�re la t�che t (de mani�re inconditionnelle) dans une file de
// t�ches pr�tes. L'�tat est positionn� � RUNNING. Dans le cas de
// files multiples (!= MA__ONE_QUEUE), l'attribut sched_policy est
// consult� afin d'utiliser l'algorithme appropri� pour le choix de la
// file...
void marcel_insert_task(marcel_t t)
{
  DEFINE_CUR_LWP(,,);

  LOG_IN();

#ifdef MA__ONE_QUEUE
  // La variable cur_lwp pourrait en fait rester ind�finie ici (!) car
  // la macro SCHED_DATA() (utilis�e par sched_lock) ignore le
  // param�tre si MA__ONE_QUEUE est d�fini. Ceci dit, ce n'est pas
  // joli, c'est pourquoi cur_lwp est positionn� � sa juste valeur
  // par pr�caution...
  SET_CUR_LWP(GET_LWP(marcel_self()));
#else
  SET_CUR_LWP(choose_lwp(t));
#endif

  // De mani�re g�n�rale, il faut d'abord acqu�rir le "lock" sur la
  // file choisie, sauf si la t�che � ins�rer est la t�che "idle",
  // auquel cas le lock est d�j� acquis.
  if(!MA_TASK_NO_USE_SCHEDLOCK(t))
    sched_lock(cur_lwp);

  // Surtout, ne pas oublier de positionner ce champ !
  t->lwp = cur_lwp;

  mdebug("\t\t\t<Insertion of task %p on LWP %d>\n", t, cur_lwp->number);

  insert_running_queue(t, cur_lwp);

  SET_RUNNING(t);
  one_more_active_task(t, cur_lwp);

  // On rel�che le "lock" acquis pr�c�demment
  if(!MA_TASK_NO_USE_SCHEDLOCK(t))
    sched_unlock(cur_lwp);

#ifndef MA__ONE_QUEUE
  // Lorsque l'on ins�re un t�che dans une file, on positionne le
  // champ "has_new_tasks" � 1 afin de signaler l'�v�nement � la t�che
  // idle. Si on contraire on ins�re idle, alors on remet le champ �
  // 0. NOTE: Ce champ n'est pas vraiment n�cessaire car la t�che idle
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

// R�veille la t�che t. Selon son �tat (sleeping, blocked, frozen), la
// t�che est d'abord retir�e d'une �ventuelle autre file, ensuite
// "insert_task" est appel�e. Si la t�che �tait d�j� RUNNING (c'est le
// cas lorsque la t�che a �t� d�vi� alors qu'elle �tait bloqu�e par
// exemple), "insert_task" n'est pas appel�e et la fonction a juste
// pour effet de positionner *blocked � FALSE.
void marcel_wake_task(marcel_t t, boolean *blocked)
{
  LOG_IN();

  mdebug("\t\t\t<Waking thread %p (num %d)>\n",
	 t, t->number);

  if(IS_SLEEPING(t)) {

    // Le cas SLEEPING est un cas particulier : la fonction est
    // forc�ment appel�e depuis "check_sleeping" qui a d�j� acquis le
    // verrou "__delayed_lock", donc il ne faut pas tenter de
    // l'acqu�rir � nouveau...

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

  LOG_IN();

  sched_lock(cur_lwp);

  one_active_task_less(t, cur_lwp);

#if defined(MA__LWPS) && ! defined(MA__ONE_QUEUE)
  /* For affinity scheduling: */
  t->previous_lwp = cur_lwp;
#endif

  r = cur;
  if (t == cur) {
    /* on s'enl�ve nous m�me, on doit se trouver un rempla�ant */
    if (find_next == FIND_NEXT) {
      r = radical_next_task(cur, cur_lwp);
      if(r == NULL) {
	/* dans le cas o� on est la tache idle et que l'on ne trouve
         * personne d'autre */
	SET_RUNNING(cur);
	sched_unlock(cur_lwp);

	LOG_OUT();
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
      /* Insertion juste avant la position trouv�e */
      /* Si la liste est vide, pos=&__delayed_tasks donc �a marche */
      /* Si on va jusqu'au bout de la boucle pr�c�dente,
         pos=&__delayed_tasks donc �a marche aussi */
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

  LOG_OUT();

  return r;
}

static __inline__ void marcel_switch_to(marcel_t cur, marcel_t next)
{
  if (cur != next) {
    if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
      MA_THR_RESTARTED(cur, "Preemption");

      LOG_OUT();
      return;
    }
    can_goto_next_task(cur, next);
    MA_THR_RESTARTED(cur, "");
  }
}

// R�alise effectivement un changement de contexte vers une autre
// t�che, ou ne fait rien dans le cas o� aucune autre t�che n'est
// pr�te. Note : on suppose que les appels � lock_task/unlock_task
// sont effectu�s en dehors...
void ma__marcel_yield(void)
{
  register marcel_t cur = marcel_self();
  DEFINE_CUR_LWP(,= ,GET_LWP(cur));

  LOG_IN();

  marcel_switch_to(cur, next_task_to_run(cur, cur_lwp));
  
  LOG_OUT();
}

#ifdef MARCEL_RT
// Cette fonction est appel�e dans unlock_task() lorsque l'on devine
// qu'il y a probablement une t�che "real-time" � ex�cuter en
// priorit�. Il faut donc d'abord v�rifier qu'il en existe puis lui
// c�der la main.
void ma__marcel_find_and_yield_to_rt_task(void)
{
  marcel_t res, first, cur = marcel_self();
  DEFINE_CUR_LWP(,= ,GET_LWP(cur));

  LOG_IN();

  sched_lock(cur_lwp);

  first = SCHED_DATA(cur_lwp).rt_first;

  if(first != NULL) {
#ifdef MA__MULTIPLE_RUNNING
    res = first;
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

// Effectue un changement de contexte + �ventuellement ex�cute des
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

  if(locked() != 1)
    RAISE(LOCK_TASK_ERROR);
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

#if defined(MA__SMP) || defined(MA__ACTIVATION)
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

// Checks to see if some tasks should be waked. NOTE: The function
// assumes that "lock_task()" was called previously.
static int marcel_check_sleeping(void)
{
  unsigned long present = __milliseconds;
  int waked_some_task = 0;
  register struct list_head *pos;

  LOG_IN();

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

  LOG_OUT();

  if(!waked_some_task)
    mdebug("Failed to wake any task (present = %ld)\n",
	   present);

  return waked_some_task;
}

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
  if(lwp->has_to_stop) {
    stop_timer();
    if(lwp == &__main_lwp)
      RAISE(PROGRAM_ERROR);
    else {
      mdebug("\t\t\t<LWP %d exiting>\n", lwp->number);
      longjmp(lwp->home_jb, 1);
    }
  }
#endif
}

static __inline__ void idle_check_tasks_to_wake(__lwp_t *lwp)
{
  mdebug("check_delayed\n");

#ifdef MA__SMP
  marcel_check_delayed_tasks();
#endif

#ifdef MA__MONO
  /* If no polling jobs were registered, then we can sleep and only
     check delayed tasks when we are signaled (SIGALRM) */
  if(marcel_polling_is_required(MARCEL_POLL_AT_IDLE)) {
    if(!marcel_check_delayed_tasks())
      /* Busy polling ! */ ;
  } else {
    if(!marcel_check_delayed_tasks()) {
      pause();
    }
  }
#endif
}

static __inline__ marcel_t idle_find_runnable_task(__lwp_t *lwp)
{
  marcel_t next = NULL, cur = marcel_self();

#ifdef MA__MULTIPLE_RUNNING // SMP + ONE_QUEUE ou bien ACTIVATIONS

  // next sera positionn� � NULL si aucune autre t�che pr�te n'est
  // trouv�e (et dans ce cas SET_RUNNING(cur) est effectu� par
  // unchain_task).
  SET_FROZEN(cur);
  next = UNCHAIN_TASK_AND_FIND_NEXT(cur);

#else // Reste les cas "SMP avec plusieurs files" et "mono"

#ifdef MA__SMP // Mode "SMP avec plusieurs files"
  if(SCHED_DATA(lwp).has_new_tasks) {
    SET_FROZEN(cur);
    next = UNCHAIN_TASK_AND_FIND_NEXT(cur);
  } else {
    next = do_work_stealing();
  }
#else // Mode MONO
  // On tente sa chance avec next_task : � ce stade, il y a peut-�tre
  // d'autres t�ches dans la file.
  if(next_task(cur) != cur) {
    SET_FROZEN(cur);
    next = UNCHAIN_TASK_AND_FIND_NEXT(cur);
  } else {
    /* On va quand m�me tester s'il n'y a pas un petit deadlock dans
       l'air... */
    if(list_empty(&__delayed_tasks)
       && !marcel_polling_is_required(MARCEL_POLL_AT_IDLE)) {
      RAISE(DEADLOCK_ERROR);
    }
  }
#endif // MA__SMP
#endif // MA__MULTIPLE_RUNNING

  return next;
}

static marcel_t idle_body(void)
{
  marcel_t next;
  DEFINE_CUR_LWP(, =, GET_LWP(marcel_self()));

  for(;;) {

    /* Do I have to stop? */
    idle_check_end(cur_lwp);

    /* Check for delayed tasks and polling stuff to do... */
    idle_check_tasks_to_wake(cur_lwp);

    /* Can I find a ready task? */
    next = idle_find_runnable_task(cur_lwp);

    if(next != NULL)
      break;

    /* If nothing successful */
    PAUSE();
  }
  return next;
}

#ifndef MA__ACTIVATION
any_t idle_func(any_t arg) // Sans activation (mono et smp)
{
  marcel_t next, cur = marcel_self();
#ifdef MA__DEBUG
    DEFINE_CUR_LWP(, =, GET_LWP(cur));
#endif

  LOG_IN();

#ifdef MA__TIMER
  start_timer(); /* May be redundant for main LWP */
#endif

  /* Except at the beginning, lock_task() is supposed to be permanently
     handled */
  lock_task();

  for(;;) {

    mdebug("\t\t\t<Scheduler scheduled> (LWP = %d)\n", cur_lwp->number);

    next = idle_body();

    mdebug("\t\t\t<Scheduler unscheduled> (LWP = %d)\n", cur_lwp->number);

    if(MA_THR_SETJMP(cur) == FIRST_RETURN)
      goto_next_task(next);

    MA_THR_RESTARTED(cur, "Preemption");
  }
  LOG_OUT(); // Probably never executed
}

#else // MA__ACTIVATION

any_t idle_func(any_t arg) // Pour les activations
{
  volatile marcel_t next, cur = marcel_self();
  static int counter=0;
  int lc;
  volatile int init=0;
  DEFINE_CUR_LWP(, =, GET_LWP(cur));

  LOG_IN();

  lock_task();
  MA_THR_SETJMP(cur);
  if(!init) {
    init=1;
    MTRACE("Starting idle_func", cur);   
  } else {
    MA_THR_RESTARTED(cur, "Idle restart");
  }
  for(;;){

    mdebug("\t\t\t<Scheduler scheduled> (LWP = %d)\n", cur_lwp->number);

    //has_new_tasks=0;

    //act_cntl(ACT_CNTL_READY_TO_WAIT,0);
    SET_FROZEN(cur);
    marcel_check_delayed_tasks();
    next = UNCHAIN_TASK_AND_FIND_NEXT(cur);
    GET_LWP(cur)->prev_running=cur;
#ifdef MA__ACTSMP
#define ACT_DONT_USE_SYSCALL
#endif
#define myfprintf(arg...) mdebug(##arg)
    lc=counter++;
    while (!(next || act_nb_unblocked)) {
//	    int i;
	    MTRACE("active wait", cur);
	    mdebug("active wait *0* / %i\n", lc);
	      
#ifndef ACT_DONT_USE_SYSCALL
	    act_cntl(ACT_CNTL_READY_TO_WAIT,0);
	    if (act_nb_unblocked) break;
	    if(__delayed_tasks == NULL) {
	      mdebug("Idle waiting in kernel...\n");
	      act_cntl(ACT_CNTL_DO_WAIT,0);	
	    }
#else      
	    while(!(act_nb_unblocked || next)) {
		    volatile int i=0;
		    volatile int j=0;
		    while((i++<100000000) && !(act_nb_unblocked||next)) {
			    //SET_FROZEN(cur);
			    //next = UNCHAIN_TASK_AND_FIND_NEXT(cur);
			    j=0;
			    while((j++<10) && !(act_nb_unblocked||next))
				    ;
		    }
		    if (!act_nb_unblocked) {
			    myfprintf("act_nb_unblocked=%i (LWP = %d)\n",
				    act_nb_unblocked, cur_lwp->number);
		    }
	    }
	    mdebug("fin attente active *1* / %i\n", lc);
	    MTRACE("end active wait", cur);
	    mdebug("fin attente active *2* / %i\n", lc);
	    if (next) {
		    myfprintf("idle has job (LWP = %d)\n",
			    cur_lwp->number);
		    break;
	    }
#endif
	    marcel_check_delayed_tasks();
	    SET_FROZEN(cur);
	    next = UNCHAIN_TASK_AND_FIND_NEXT(cur);
    }
    mdebug("\t\t\t<Scheduler unscheduled> (LWP = %d) next=%p"
	   " act_nb_unblocked=%i / %i\n", 
	   cur_lwp->number, next, act_nb_unblocked, lc);
    
    if(MA_THR_SETJMP(cur) == FIRST_RETURN) {
	    if (next) {
		    myfprintf("idle has job (LWP = %d)\n",
			    cur_lwp->number);
		    goto_next_task(next);
	    } else {
		    myfprintf("idle can have job (LWP = %d)\n",
			    cur_lwp->number);
		    act_goto_next_task(NULL, ACT_RESTART_FROM_IDLE);
	    }
    }
    MA_THR_RESTARTED(cur, "Preemption");
  }

  LOG_OUT();

  // Pour la terminaison...
  for (;;) {
	  act_cntl(ACT_CNTL_READY_TO_WAIT,0);
	  act_cntl(ACT_CNTL_DO_WAIT,0);
  }
}
#endif // MA__ACTIVATION

#ifdef MA__ACTIVATION
static void void_func(void* param)
{
  RAISE("Quoi, on est l� ???\n");
}
#endif

static void init_lwp(__lwp_t *lwp, marcel_t initial_task)
{
  static unsigned __nb_lwp = 0;
  marcel_attr_t attr;

  LOG_IN();

  if (__nb_lwp >= MA__MAX_LWPS) 
	  RAISE("Too many lwp\n");

  SET_LWP_NB(__nb_lwp, lwp);
  lwp->number = __nb_lwp++;

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
      /* D�sormais, on s'ex�cute dans le lwp 0 */
    }
  }

  /*****************************************/
  /* Cr�ation de la t�che Idle (idle_task) */
  /*****************************************/
  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);
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

  lwp->idle_task->special_flags |= MA_SF_POLL | MA_SF_NOSCHEDLOCK;

  MTRACE("IdleTask", lwp->idle_task);

#if !defined(MA__SMP) || defined(MA__ONE_QUEUE)
  // En mode 'smp multi-files', on doit retirer _toutes_ les t�ches idle
  if(!initial_task)
    {
      SET_STATE_RUNNING(NULL, lwp->idle_task, lwp);
      MTRACE("RUNNING", lwp->idle_task);
    }
  else
#endif
    {
      // On retire la t�che idle
      mdebug("Extracting idle of LWP(%d)\n", lwp->number);
      SET_FROZEN(lwp->idle_task);
      UNCHAIN_TASK(lwp->idle_task);
    }

  lwp->idle_task->special_flags |= MA_SF_NORUN;

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

  // ATTENTION: les 'sched_policy' seront bient�t obsol�tes
  lwp->idle_task->sched_policy = MARCEL_SCHED_FIXED(lwp->number);
  // Il vaut mieux g�n�raliser l'utilisation des 'vpmask'
  lwp->idle_task->vpmask = MARCEL_VPMASK_ALL_BUT_VP(lwp->number);

  /***************************************************/

  unlock_task();

#ifdef MA__ACTIVATION
  /****************************************************/
  /* Cr�ation de la t�che pour les upcalls upcall_new */
  /****************************************************/
  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);
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
  /* la fonction ne sera jamais ex�cut�e, c'est juste pour avoir une
  * structure de thread marcel dans upcall_new
  * */
  sched_unlock(lwp);
  
  marcel_create(&lwp->upcall_new_task, &attr, (void*)void_func, NULL);

  MTRACE("Upcall_Task", lwp->upcall_new_task);
  SET_FROZEN(lwp->upcall_new_task);
  UNCHAIN_TASK(lwp->upcall_new_task);
  lwp->upcall_new_task->lwp = lwp;
  lwp->upcall_new_task->special_flags |= MA_SF_UPCALL_NEW | MA_SF_NORUN;
  _main_struct.nb_tasks--;
  MTRACE("U_T OK", lwp->upcall_new_task);
  /****************************************************/

  unlock_task();
  /* lib�r� dans marcel_init_sched */
  if (initial_task)
    sched_lock(lwp);
#else // MA__ACTIVATION
  /* lib�r� dans lwp_startup_func pour SMP ou marcel_init_sched pour
   * le pool de la main_task
   * */
#ifdef MA__ONE_QUEUE
  // Si une seule file, alors seul LWP(0) ex�cute sched_lock
  if(initial_task)
#endif
    sched_lock(lwp);

#endif /* MA__ACTIVATION */

  LOG_OUT();
}

#if defined(MA__LWPS)
static unsigned __nb_processors = 1;

static void create_new_lwp(void)
{
  __lwp_t *lwp = (__lwp_t *)__TBX_MALLOC(sizeof(__lwp_t), __FILE__, __LINE__),
          *cur_lwp = marcel_self()->lwp;

  LOG_IN();

  init_lwp(lwp, NULL);

  /* Add to global linked list */
  list_add(&lwp->lwp_list, &cur_lwp->lwp_list);

  LOG_OUT();
}
#endif

#ifdef MA__SMP

static void *lwp_startup_func(void *arg)
{
  __lwp_t *lwp = (__lwp_t *)arg;

  LOG_IN();

  if(setjmp(lwp->home_jb)) {
#ifdef MARCEL_DEBUG
    // Attention : pas de mdebug ici !
  if(marcel_mdebug.show)
    fprintf(stderr, "sched %d Exiting\n", lwp->number);
#endif
    LOG_OUT();
    pthread_exit(0);
  }

#ifdef MARCEL_DEBUG
  if(marcel_mdebug.show)
    fprintf(stderr, "\t\t\t<LWP %d started (pthread_self == %lx)>\n",
	    lwp->number, (unsigned long)pthread_self());
#endif

#if defined(BIND_LWP_ON_PROCESSORS) && defined(SOLARIS_SYS)
  if(processor_bind(P_LWPID, P_MYID,
		    (processorid_t)(lwp->number % __nb_processors),
		    NULL) != 0) {
    perror("processor_bind");
    exit(1);
  } else
  if(marcel_mdebug.show)
    fprintf(stderr, "LWP %d bound to processor %d\n",
	   lwp->number, (lwp->number % __nb_processors));
#endif

  /* Can't use lock_task() because marcel_self() is not yet usable ! */
  atomic_inc(&lwp->_locked);

#ifndef MA__ONE_QUEUE
  /* pris dans  init_sched() */
  sched_unlock(lwp);
#endif

  MA_THR_LONGJMP(lwp->idle_task, NORMAL_RETURN);
  
  return NULL;
}

static void start_lwp(__lwp_t *lwp)
{
  pthread_attr_t attr;

  LOG_IN();

  /* Start new Pthread */
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  pthread_create(&lwp->pid, &attr, lwp_startup_func, (void *)lwp);

  LOG_OUT();
}
#endif

void marcel_sched_init(unsigned nb_lwp)
{
  marcel_initialized = TRUE;

  LOG_IN();

  sigemptyset(&sigeptset);
  sigemptyset(&sigalrmset);
  sigaddset(&sigalrmset, MARCEL_TIMER_SIGNAL);

#ifdef MA__LWPS

#ifdef SOLARIS_SYS
  __nb_processors = sysconf(_SC_NPROCESSORS_CONF);

#if defined(BIND_LWP_ON_PROCESSORS)
  if(processor_bind(P_LWPID, P_MYID, (processorid_t)0, NULL) != 0) {
    perror("processor_bind");
    exit(1);
  } else
    mdebug("LWP %d bound to processor %d\n", 0, 0);
#endif

#elif defined(LINUX_SYS)

  __nb_processors = WEXITSTATUS(system("exit `grep rocessor /proc/cpuinfo"
				       " | wc -l`"));

#elif defined(IRIX_SYS)

  __nb_processors = sysconf(_SC_NPROC_CONF);

#else
  __nb_processors = 1;
#endif

  mdebug("%d processors available\n", __nb_processors);

#ifdef MA__ACTSMP
  set_nb_lwps(nb_lwp ? nb_lwp : ACT_NB_MAX_CPU);
#else
  set_nb_lwps(nb_lwp ? nb_lwp : __nb_processors);
#endif
  _main_struct.nb_tasks = -get_nb_lwps();

#else /* MA__LWPS */
  _main_struct.nb_tasks = -1;
#endif /* MA__LWPS */

  memset(__main_thread, 0, sizeof(task_desc));
  __main_thread->detached = FALSE;
  __main_thread->not_migratable = 1;
  /*
   * Some Pthread packages do not support that a
   * LWP != main_LWP execute on the main stack, so: 
   */
  __main_thread->sched_policy = MARCEL_SCHED_FIXED(0);
  __main_thread->vpmask = MARCEL_VPMASK_ALL_BUT_VP(0);

  /* 
   * Initialization of "main LWP" (required even when SMP not set). 
   * init_lwp calls init_sched 
   */
  init_lwp(&__main_lwp, __main_thread);

  mdebug("marcel_sched_init: init_lwp done\n");

  INIT_LIST_HEAD(&__main_lwp.lwp_list);

  SET_STATE_RUNNING(NULL, __main_thread, (&__main_lwp));
  one_more_active_task(__main_thread, &__main_lwp);
  MTRACE("MainTask", __main_thread);

  /* Pris dans  init_sched() */
  sched_unlock(&__main_lwp);
}

void marcel_sched_start(void)
{
#ifdef MA__LWPS

  /* Creation en deux phases en prevision du work stealing qui peut
     demarrer tres vite sur certains LWP... */
  {
    int i;
#ifdef MA__SMP
    struct list_head *lwp=NULL;
#endif

    for(i=1; i<get_nb_lwps(); i++)
      create_new_lwp();
 
    mdebug("marcel_sched_init  : %i lwps created\n",
	   get_nb_lwps());

#ifdef MA__SMP
    list_for_each(lwp, &__main_lwp.lwp_list) {
      start_lwp(list_entry(lwp, __lwp_t, lwp_list));
    }
#endif

  }
#endif /* MA__LWPS */

#ifdef MA__ACTIVATION
  init_upcalls(get_nb_lwps());
#endif

#ifdef PM2DEBUG
  pm2debug_marcel_launched=1;
#endif

#ifdef MA__TIMER
  /* D�marrage d'un timer Unix pour r�cup�rer p�riodiquement 
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
  int cc;

  LOG_IN();

  lwp->has_to_stop = TRUE;
  /* Join corresponding Pthread */
  do {
    cc = pthread_join(lwp->pid, NULL);
  } while(cc == -1 && errno == EINTR);

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
  // TODO : arr�ter les autres activations...

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

  if(!locked() && preemption_enabled()) {

    LOG_IN();

    MTRACE_TIMER("TimerSig", cur);

    lock_task();
    marcel_check_sleeping();
    marcel_check_polling(MARCEL_POLL_AT_TIMER_SIG);
    unlock_task();

#ifdef MA__SMP
    pthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
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
    pthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif

  if(marcel_initialized) {
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_usec = time_slice;
    value.it_value = value.it_interval;
    setitimer(MARCEL_ITIMER_TYPE, &value, (struct itimerval *)NULL);
  }

  LOG_OUT();
}

static void start_timer(void)
{
  static struct sigaction sa;

  LOG_IN();

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

inline static int want_to_see(marcel_t t, int which)
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

