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
$Log: marcel_sched.c,v $
Revision 1.25  2000/04/25 14:39:38  vdanjean
remove debug stuff

Revision 1.24  2000/04/21 11:19:28  vdanjean
fixes for actsmp

Revision 1.23  2000/04/17 16:09:40  vdanjean
clean up : remove __ACT__ flags and use of MA__ACTIVATION instead of MA__ACT when needed

Revision 1.22  2000/04/17 08:31:54  rnamyst
Changed DEBUG into MA__DEBUG.

Revision 1.21  2000/04/14 14:01:48  rnamyst
Minor modifs.

Revision 1.20  2000/04/14 11:41:46  vdanjean
move SCHED_DATA(lwp).sched_task to lwp->idle_task

Revision 1.19  2000/04/14 09:06:16  vdanjean
new kernel activation version : adaptation of marcel

Revision 1.18  2000/04/11 09:07:35  rnamyst
Merged the "reorganisation" development branch.

Revision 1.17.2.32  2000/04/11 08:54:26  vdanjean
attente active en ACT

Revision 1.17.2.31  2000/04/11 08:48:52  vdanjean
added a few MTRACE calls

Revision 1.17.2.30  2000/04/11 08:17:31  rnamyst
Comments are back !

Revision 1.17.2.29  2000/04/10 09:56:19  rnamyst
Moved the polling stuff into marcel_polling....

Revision 1.17.2.28  2000/04/08 15:09:14  vdanjean
few bugs fixed

Revision 1.17.2.27  2000/04/06 15:31:00  rnamyst
Bug fixed in marcel_wake_task concerning "deviated tasks".

Revision 1.17.2.26  2000/04/06 13:55:20  rnamyst
Fixed a minor bug in marcel_insert_task and in marcel_exit...

Revision 1.17.2.25  2000/04/06 07:38:03  vdanjean
Activations mono OK :-)

Revision 1.17.2.22  2000/03/31 18:38:39  vdanjean
Activation mono OK

Revision 1.17.2.21  2000/03/31 11:20:36  vdanjean
debug ACT

Revision 1.17.2.20  2000/03/31 09:39:53  vdanjean
debug ACT

Revision 1.17.2.19  2000/03/31 08:08:11  rnamyst
Added disable_preemption() and enable_preemption().

Revision 1.17.2.18  2000/03/30 16:57:39  rnamyst
Introduced TOP_STACK_FREE_AREA...

Revision 1.17.2.17  2000/03/30 14:44:39  rnamyst
Minor modif.

Revision 1.17.2.16  2000/03/30 09:52:59  rnamyst
Bug fixed in init_sched.

Revision 1.17.2.15  2000/03/29 17:21:57  rnamyst
Minor cleaning of the code.

Revision 1.17.2.14  2000/03/29 17:01:11  vdanjean
Minor modifs

Revision 1.17.2.13  2000/03/29 16:49:41  vdanjean
ajout de du champs special_flags dans marcel_t

Revision 1.17.2.12  2000/03/29 16:25:57  rnamyst
Bug fixed in sched_task (successful story...)

Revision 1.17.2.11  2000/03/29 14:24:54  rnamyst
Added the marcel_stdio.c that provides the marcel_printf functions.

Revision 1.17.2.10  2000/03/29 11:29:17  vdanjean
move lwp.sched_task to SCHED_DATA(lwp).sched_task

Revision 1.17.2.9  2000/03/29 09:46:19  vdanjean
*** empty log message ***

Revision 1.17.2.7  2000/03/24 17:55:25  vdanjean
fixes

Revision 1.17.2.6  2000/03/24 14:16:46  vdanjean
few bugs fixed

Revision 1.17.2.5  2000/03/22 16:34:19  vdanjean
*** empty log message ***

Revision 1.17.2.4  2000/03/22 10:32:57  vdanjean
*** empty log message ***

Revision 1.17.2.3  2000/03/17 20:09:56  vdanjean
*** empty log message ***

Revision 1.17.2.2  2000/03/15 15:55:12  vdanjean
réorganisation de marcel : commit pour CVS

Revision 1.17.2.1  2000/03/15 15:41:22  vdanjean
réorganisation de marcel. branche de développement

Revision 1.17  2000/03/09 11:07:53  rnamyst
Modified to use the sched_data() macro.

Revision 1.16  2000/03/06 12:57:36  vdanjean
*** empty log message ***

Revision 1.15  2000/03/06 09:24:03  vdanjean
Nouvelle version des activations

Revision 1.14  2000/03/01 16:48:14  oaumage
- suppression des warnings CVS
- ajout d'une compilation conditionnelle des  `find_lwp_*'  par rapport a SMP

Revision 1.13  2000/02/28 10:25:09  rnamyst
Changed #include <> into #include "".

Revision 1.12  2000/01/31 15:57:21  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

//#define MA__DEBUG
//#define TICK
#define BIND_LWP_ON_PROCESSORS
//#define DO_PAUSE_INSTEAD_OF_SCHED_YIELD

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "marcel.h"
#include "safe_malloc.h"

#ifdef SMP
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
#include "sys/upcalls.h"
int nb_idle_sleeping = 0;
#endif


static int next_schedpolicy_available = __MARCEL_SCHED_AVAILABLE;
static marcel_schedpolicy_func_t user_policy[MARCEL_MAX_USER_SCHED];

static boolean marcel_initialized = FALSE;

static volatile unsigned __active_threads = 0,
  __sleeping_threads = 0,
  __blocked_threads = 0,
  __frozen_threads = 0;

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

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS
static marcel_t __waiting_tasks = NULL;
#endif
static marcel_t __delayed_tasks = NULL;
static volatile unsigned long task_number = 1;

/* These two locks must be acquired before accessing the corresponding
   global queue.  They should only encapsulate *non-blocking* code
   sections. */
static marcel_lock_t __delayed_lock = MARCEL_LOCK_INIT;
static marcel_lock_t __blocking_lock = MARCEL_LOCK_INIT;

#ifdef MA__LWPS
static unsigned  ma__nb_lwp;
__lwp_t* addr_lwp[MA__MAX_LWPS];
#define GET_NB_LWPS (ma__nb_lwp)
#define SET_NB_LWPS(value) (ma__nb_lwp=(value))
#else
#define GET_NB_LWPS (1)
#define SET_NB_LWPS(value)
#endif

#define MIN_TIME_SLICE		10000
#define DEFAULT_TIME_SLICE	20000

#ifdef X86_ARCH
volatile atomic_t __preemption_disabled = ATOMIC_INIT(0);
#else
volatile unsigned int __preemption_disabled = 0;
#endif

static volatile unsigned long time_slice = DEFAULT_TIME_SLICE;
static volatile unsigned long __milliseconds = 0;

static struct {
       unsigned nb_tasks;
       boolean main_is_waiting, blocked;
} _main_struct = { 0, FALSE, FALSE };

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

unsigned marcel_nbvps(void)
{
  return GET_NB_LWPS;
}

static __inline__ void display_sched_queue(marcel_t pid)
{
#ifdef MA__DEBUG
  marcel_t t = pid;

  mdebug("\t\t\t<Task queue on LWP %d:\n", pid->lwp->number);
  do {
    mdebug("\t\t\t\tThread %p (num %d, LWP %d)\n", t, t->number, t->lwp->number);
    t = t->next;
  } while(t != pid);
  mdebug("\t\t\t>\n");
#endif
}

#ifndef MA__ONE_QUEUE
// Retourne le LWP de numero "vp modulo NB_LWPS" en parcourant
// betement la liste chainee. A ameliorer en utilisant un tableau pour
// indexer les LWPs...
static __inline__ __lwp_t *find_lwp(unsigned vp)
{
  register __lwp_t *lwp = &__main_lwp;

  while(vp--)
    lwp = lwp->next;

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
    lwp = lwp->next;
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

  for(;;) {
    lwp = lwp->next;
    if(lwp == &__main_lwp)
      return best;
    if(SCHED_DATA(lwp).running_tasks < nb) {
      nb = SCHED_DATA(lwp).running_tasks;
      best = lwp;
    }
  }
}
#endif /* MA__ONE_QUEUE */

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

// Attend que toutes les taches soient terminees. Cette fonction
// _doit_ etre appelee par la tache "main".
static void wait_all_tasks_end(void)
{
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
}

// Insere la tache "idle" et retourne son identificateur. Cette
// fonction est uniquement appelee dans "radical_next_task" lorsqu'il
// n'y a pas d'autres taches "prêtes" dans la file.
static marcel_t insert_and_choose_idle_task(marcel_t cur, __lwp_t *lwp)
{
  marcel_t idle;
#ifdef MA__ACTIVATION
  if (cur && IS_TASK_TYPE_IDLE(cur)) {
    /* c'est déjà idle qui regarde s'il y a autre chose. Apparemment,
     * ce n'est pas le cas */
    return NULL;
  }
#endif

  idle = lwp->idle_task;
  mdebug("marcel_unchain_task chooses idle %p, lwp=%p (%i), sched_data : %p\n",
	 idle, lwp, lwp->number, &SCHED_DATA(lwp));
  marcel_wake_task(idle, NULL);

  return idle;
  
}

// Retourne une tache prete differente de "cur". Le cas echeant,
// insere la tache "idle" (ou "poll"). Attention: on suppose ici que
// la tache renvoyee _sera_ ordonnancee au retour. On suppose
// egalement que "sched_lock" a ete appele.
static marcel_t radical_next_task(marcel_t cur, __lwp_t *lwp)
{
  marcel_t next;
#ifdef MA__ACTIVATION
  marcel_t first = cur;

  if (first == NULL) {
    cur = SCHED_DATA(cur_lwp).__first[0];
  }
#endif
#ifdef MA__MULTIPLE_RUNNING
  next = cur;
  do {
    next = next->next;
#ifdef MA__DEBUG
    if (!next)
      mdebug("NULL pointer !!\n");
#endif
  } while ((CANNOT_RUN(next)) && (next != cur));
  mdebug("marcel_unchain_task choose %p on lwp %p\n", next, lwp);
#else
  /* plus simple dans ce cas : on prend juste la tâche suivante */
  next = cur->next;
#endif
  if (next == cur) {
#ifdef MA__ACTIVATION
	next = insert_and_choose_idle_task(first, lwp);
#else
	next = insert_and_choose_idle_task(cur, lwp);
#endif
  }
  return next;
}

marcel_t marcel_radical_next_task(void)
{
  marcel_t cur = marcel_self(), t;

  DEFINE_CUR_LWP( , ,);

  SET_CUR_LWP(GET_LWP(cur));

  sched_lock(cur_lwp);

#ifdef MA__ACTIVATION
  if (IS_TASK_TYPE_UPCALL_NEW(cur)) {
    mdebug("upcall...%p on %p\n", cur, cur_lwp);
    t = radical_next_task(NULL, cur_lwp);
    SET_STATE_RUNNING(NULL, t, cur_lwp);
    sched_unlock(cur_lwp);
    return t;
  }
#endif

#ifdef MA__MULTIPLE_RUNNING
#ifdef USE_PRIORITY
  RAISE(NOT_IMPLEMENTED);
#endif
  
  t = radical_next_task(cur, cur_lwp);
  
#else
  if(cur->quantums-- == 1) {
    cur->quantums = cur->prio;
    t = cur->next;
  } else {
    if(cur->next->quantums <= cur->quantums) {
      if(SCHED_DATA(cur_lwp).__first[0] == cur)
	t = cur->next;
      else
	t = SCHED_DATA(cur_lwp).__first[0];
    } else
      t = cur->next;
  }
#endif
  SET_STATE_RUNNING(cur, t, cur_lwp);

  sched_unlock(cur_lwp);

  return t;
}

// Insère la tâche t (de manière inconditionnelle) dans une file de
// tâches prêtes. L'état est positionné à RUNNING. Dans le cas de
// files multiples (!= MA__ONE_QUEUE), l'attribut sched_policy est
// consulté afin d'utiliser l'algorithme approprié pour le choix de la
// file...
void marcel_insert_task(marcel_t t)
{
#ifdef USE_PRIORITIES
  unsigned i;
#endif
  register marcel_t p;
#if defined(MA__LWPS) || defined(MA__MULTIPLE_RUNNING)
#ifndef MA__ONE_QUEUE
  __lwp_t *self_lwp = GET_LWP(marcel_self());
#endif
  DEFINE_CUR_LWP(,,);
  SET_CUR_LWP(GET_LWP(marcel_self()));
#endif

  MTRACE("INSERT", t);
#ifndef MA__ONE_QUEUE
  switch(t->sched_policy) {
  case MARCEL_SCHED_OTHER : {
    cur_lwp = self_lwp;
    break;
  }
  case MARCEL_SCHED_AFFINITY : {
    cur_lwp = find_good_lwp(t->previous_lwp ? : self_lwp);
    break;
  }
  case MARCEL_SCHED_BALANCE : {
    cur_lwp = find_best_lwp();
    break;
  }
  default: {
    if(t->sched_policy >= 0) {
      /* MARCEL_SCHED_FIXED(vp) */
      cur_lwp = (t->previous_lwp ? : find_lwp(t->sched_policy));
      mdebug("\t\t\t<Insertion of task %p requested on LWP %d>\n", 
	     t, t->sched_policy);
    } else
      /* MARCEL_SCHED_USER_... */
      cur_lwp = (*user_policy[__MARCEL_SCHED_AVAILABLE - 
			      (t->sched_policy)])(t, self_lwp);
  }
  }
#endif

#if defined(MA__LWPS) || defined(MA__MULTIPLE_RUNNING)
  // De manière générale, il faut d'abord acquérir le "lock" sur la
  // file choisie, sauf si la tâche à insérer est la tâche "idle",
  // auquel cas le lock est déjà acquis.
  mdebug("\t\t\t<Trying to acquire lock on LWP %d>\n", cur_lwp->number);
  if(!MA_TASK_NO_USE_SCHEDLOCK(t))
    sched_lock(cur_lwp);

  mdebug("\t\t\t<Insertion of task %p on LWP %d>\n", t, cur_lwp->number);
#endif

  // On cherche une tâche "p" qui deviendra le "successeur" de t dans
  // la file (i.e. on va insérer t avant p)...
#ifdef USE_PRIORITIES
  for(i=t->prio; SCHED_DATA(cur_lwp).__first[i] == NULL; i--) ;
  p = SCHED_DATA(cur_lwp).__first[i];
#else
  p = SCHED_DATA(cur_lwp).__first[0];

#endif

  t->prev = p->prev;
  t->next = p;
  p->prev = t;
  t->prev->next = t;
#ifdef USE_PRIORITIES
  SCHED_DATA(cur_lwp).__first[t->prio] = t;
  if(i != 0 && p == SCHED_DATA(cur_lwp).__first[0])
    SCHED_DATA(cur_lwp).__first[0] = t;
#else
  SCHED_DATA(cur_lwp).__first[0] = t;
#endif

  // Surtout, le pas oublier de positionner ce champ !
  t->lwp = cur_lwp;

  SET_RUNNING(t);
  one_more_active_task(t, cur_lwp);

#if defined(MA__LWPS) || defined(MA__MULTIPLE_RUNNING)
  // On relâche le "lock" acquis précédemment
  if(!MA_TASK_NO_USE_SCHEDLOCK(t))
    sched_unlock(cur_lwp);
#endif

#ifndef MA__ONE_QUEUE
  // Lorsque l'on insére un tâche dans une file, on positionne le
  // champ "has_new_task" à 1 afin de signaler l'évènement à la tâche
  // idle. Si on contraire on insère idle, alors on remet le champ à
  // 0. NOTE: Ce champ n'est pas vraiment nécessaire car la tâche idle
  // pourrait simplement tester son successeur dans la file...
  if(MA_GET_TASK_TYPE(t) == MA_TASK_TYPE_NORMAL)
    cur_lwp->has_new_tasks = TRUE;
  else
    self_lwp->has_new_tasks = FALSE;
#endif
}

// Réveille la tâche t. Selon son état (sleeping, blocked, frozen), la
// tâche est d'abord retirée d'une éventuelle autre file, ensuite
// "insert_task" est appelée. Si la tâche était déjà RUNNING (c'est le
// cas lorsque la tâche a été dévié alors qu'elle était bloquée par
// exemple), "insert_task" n'est pas appelée et la fonction a juste
// pour effet de positionner *blocked à FALSE.
void marcel_wake_task(marcel_t t, boolean *blocked)
{
  register marcel_t p;

  mdebug("\t\t\t<Waking thread %p (num %d)>\n",
	 t, t->number);

  if(IS_SLEEPING(t)) {

    // Le cas SLEEPING est un cas particulier : la fonction est
    // forcément appelée depuis "check_sleeping" qui a déjà acquis le
    // verrou "__delayed_lock", donc il ne faut pas tenter de
    // l'acquérir à nouveau...

    if(t == __delayed_tasks)
      __delayed_tasks = t->next;
    if((p = t->next) != NULL)
      p->prev = t->prev;
    if((p = t->prev) != NULL)
      p->next = t->next;

    one_sleeping_task_less(t);

  } else if(IS_BLOCKED(t)) {

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS

    marcel_lock_acquire(&__blocking_lock);

    if((p = t->prev) == t)
      __waiting_tasks = NULL;
    else {
      if(t == __waiting_tasks)
	__waiting_tasks = p;
      p->next = t->next;
      p->next->prev = p;
    }

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
}

/* Remove from ready queue and insert into waiting queue
   (if it is BLOCKED) or delayed queue (if it is WAITING). */
marcel_t marcel_unchain_task_and_find_next(marcel_t t, marcel_t find_next)
{
  marcel_t p, r, cur=marcel_self();
  DEFINE_CUR_LWP( , , );
  SET_CUR_LWP(GET_LWP(cur));

  sched_lock(cur_lwp);

  one_active_task_less(t, cur_lwp);
#if defined(MA__LWPS) && ! defined(MA__ONE_QUEUE)
  /* For affinity scheduling: */
  t->previous_lwp = cur_lwp;
#endif

  r=cur;
  if (t==cur) {
    /* on s'enlève nous même, on doit se trouver un remplaçant */
    if (find_next == FIND_NEXT) {
      r=radical_next_task(cur, cur_lwp);
      if(r == NULL) {
	/* dans le cas où on est la tache de poll et que l'on ne
         * trouve personne d'autre */
	sched_unlock(cur_lwp);
	return r;
      }
      SET_STATE_RUNNING(t, r, cur_lwp);
#ifdef MA__DEBUG
    } else if (find_next == DO_NOT_REMOVE_MYSELF){
      RAISE("Removing ourself without new task\n");
#endif
    } else {
      mdebug("We remove ourself, but we know it\n");
      r=find_next;
    }

  }
  MTRACE("UNCHAIN", t);

#ifdef USE_PRIORITIES
  if(SCHED_DATA(cur_lwp).__first[t->prio] == t) {
    SCHED_DATA(cur_lwp).__first[t->prio] = ((r->prio == t->prio) ? r : NULL);
    if(SCHED_DATA(cur_lwp).__first[0] == t)
      SCHED_DATA(cur_lwp).__first[0] = r;
  }
#else
  SCHED_DATA(cur_lwp).__first[0] = r;
#endif
  /* la liste est non vide (il y a le suivant) : on se retire */
  t->next->prev = t->prev;
  t->prev->next = t->next;

  sched_unlock(cur_lwp);

  if(IS_SLEEPING(t)) {

    one_more_sleeping_task(t);

    marcel_lock_acquire(&__delayed_lock);

    /* insertion dans "__delayed_tasks" */
    {
      marcel_t cur;

      p = NULL;
      cur = __delayed_tasks;
      while(cur != NULL && cur->time_to_wake < t->time_to_wake) {
	p = cur;
	cur = cur->next;
      }
      t->next = cur;
      t->prev = p;
      if(p == NULL)
	__delayed_tasks = t;
      else
	p->next = t;
      if(cur != NULL)
	cur->prev = t;
    }

    marcel_lock_release(&__delayed_lock);

  } else if(IS_BLOCKED(t)) {

    one_more_blocked_task(t);

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS

    marcel_lock_acquire(&__blocking_lock);

    if(__waiting_tasks == NULL) {
      __waiting_tasks = t;
      t->next = t->prev = t;
    } else {
      t->next = __waiting_tasks;
      t->prev = __waiting_tasks->prev;
      t->prev->next = __waiting_tasks->prev = t;
    }

    marcel_lock_release(&__blocking_lock);

#endif

  } else if(IS_FROZEN(t)) {

    one_more_frozen_task(t);
    t->next = NULL;
    t->prev = NULL;

  }

  return r;
}

static __inline__ marcel_t next_task_to_run(marcel_t t, __lwp_t *lwp)
{
  register marcel_t res;

  sched_lock(lwp);

#ifdef USE_PRIORITIES

#ifdef MA__ACTIVATION
#error not yet implemented.
#endif

  if(t->quantums-- == 1) {
    t->quantums = t->prio;
    res = t->next;
  } else {
    if(t->next->quantums <= t->quantums)
      res = SCHED_DATA(lwp).__first[0];
    else
      res = t->next;
  }
#else

#ifdef MA__MULTIPLE_RUNNIG
  res=t;
  do {
    res = res->next;
  } while ((CANNOT_RUN(res)) && (res != t));
#else
  res = t->next;
#endif

#endif

  SET_STATE_RUNNING(t, res, lwp);
  sched_unlock(lwp);

  return res;
}

void marcel_yield(void)
{
  register marcel_t cur = marcel_self();
  DEFINE_CUR_LWP(,,);
  SET_CUR_LWP(GET_LWP(cur));

  lock_task();

  if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
    MA_THR_RESTARTED(cur, "Preemption");
    unlock_task();
    return;
  }
  goto_next_task(next_task_to_run(cur, cur_lwp));
}

//TODO : is it correct for marcel-smp ?
int marcel_explicityield(marcel_t t)
{
  register marcel_t cur = marcel_self();

  lock_task();

#ifdef MA__MULTIPLE_RUNNING
  sched_lock(GET_LWP(cur));
  if (CANNOT_RUN(t)) {
    sched_unlock(GET_LWP(cur));
    return 0;
  }  
  SET_STATE_RUNNING(cur, t, GET_LWP(cur));

  sched_unlock(GET_LWP(cur));
#endif

  if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
    MA_THR_RESTARTED(cur, "Preemption");
    unlock_task();
    return 1;
  }
  goto_next_task(t);
  return 1; /* for gcc */
}

void marcel_trueyield(void)
{
  marcel_t next;
  register marcel_t cur = marcel_self();

  lock_task();

  if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
    MA_THR_RESTARTED(cur, "Preemption");
    unlock_task();
    return;
  }

  next = marcel_radical_next_task(); // est-ce ok avec les activations ?
  goto_next_task(next);
}

void marcel_give_hand(boolean *blocked, marcel_lock_t *lock)
{
  marcel_t next;
  register marcel_t cur = marcel_self();
#ifdef MA__LWPS
  volatile boolean first_time = TRUE;
#endif

  if(locked() != 1)
    RAISE(LOCK_TASK_ERROR);
  do {
    if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
      MA_THR_RESTARTED(cur, "Preemption");
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
}

void marcel_tempo_give_hand(unsigned long timeout,
			    boolean *blocked, marcel_sem_t *s)
{
  marcel_t next, cur = marcel_self();
  unsigned long ttw = __milliseconds + timeout;

#if defined(MA__SMP) || defined(MA__ACTIVATION)
  RAISE(NOT_IMPLEMENTED);
#endif

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

      cur->time_to_wake = ttw;
      SET_SLEEPING(cur);
      next = UNCHAIN_TASK_AND_FIND_NEXT(cur);

      goto_next_task(next);
    }
  } while(*blocked);
  marcel_enablemigration(cur);
  unlock_task();
}

void marcel_setspecialthread(marcel_t pid)
{
#ifdef MINIMAL_PREEMPTION
  special_thread = pid;
#else
  RAISE(USE_ERROR);
#endif
}

void marcel_givehandback(void)
{
#ifdef MINIMAL_PREEMPTION
  marcel_t p = yielding_thread;

  if(p != NULL) {
    yielding_thread = NULL;
    marcel_explicityield(p);
  } else
    marcel_trueyield();
#else
  marcel_trueyield();
#endif
}

void marcel_delay(unsigned long millisecs)
{
#ifdef MA__ACTIVATION
  usleep(millisecs*1000);
#else
  marcel_t p, cur = marcel_self();
  unsigned long ttw = __milliseconds + millisecs;

  lock_task();

  mdebug("\t\t\t<Thread %p goes sleeping>\n", cur);

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
#endif
}

// Checks to see if some tasks should be waked. NOTE: The function
// assumes that "lock_task()" was called previously.
static int marcel_check_sleeping(void)
{
  unsigned long present = __milliseconds;
  int waked_some_task = 0;
  register marcel_t next;

  if(marcel_lock_tryacquire(&__delayed_lock)) {

    next = __delayed_tasks;
    while(next != NULL && next->time_to_wake <= present) {
      register marcel_t t = next;
      /* wake the "next" task */
      mdebug("\t\t\t<Check delayed: waking thread %p (%ld)>\n",
	     next, next->number);

      next = next->next;
      marcel_wake_task(t, NULL);
      waked_some_task = 1;
    }

    marcel_lock_release(&__delayed_lock);

  } else
    mdebug("LWP(%d) failed to acquire __delayed_lock\n",
	   marcel_self()->lwp->number);

  return waked_some_task;
}

// NOTE: This function assumes that "lock_task()" was called
// previously.
static __inline__ int marcel_check_delayed_tasks(void)
{
  // Attention : C'est bien un "ou logique" ici car on veut executer
  // les deux fonctions...
  return marcel_check_sleeping() | marcel_check_polling();
}

int marcel_setprio(marcel_t pid, unsigned prio)
{
  unsigned old = pid->prio;
#ifdef SMP
  __lwp_t *cur_lwp = marcel_self()->lwp;
#endif

   if(prio == 0 || prio > MAX_PRIO)
      RAISE(CONSTRAINT_ERROR);
   lock_task();
   if(!IS_RUNNING(pid))
      pid->prio = pid->quantums = prio;
   else if(pid->next == pid) { /* pid == seule tache active */
      SCHED_DATA(cur_lwp).__first[old] = NULL;
      SCHED_DATA(cur_lwp).__first[prio] = pid;
      pid->prio = pid->quantums = prio;
   } else {
     SET_FROZEN(pid);
     UNCHAIN_TASK(pid);
     pid->prio = pid->quantums = prio;
     marcel_wake_task(pid, NULL);
   }
   unlock_task();
   return 0;
}

int marcel_getprio(marcel_t pid, unsigned *prio)
{
   *prio = pid->prio;
   return 0;
}

#ifdef SMP
static int do_work_stealing(void)
{
  /* Currently not implemented ! */
  return 0;
}
#endif

static sigset_t sigalrmset, sigeptset;
static void start_timer(void);
static void set_timer(void);
void stop_timer(void);

#ifdef DO_PAUSE_INSTEAD_OF_SCHED_YIELD
#define PAUSE()	  sigsuspend(&sigeptset)
#else
#define PAUSE()	  SCHED_YIELD()
#endif

#ifdef MA__ACTIVATION
/* Except at the beginning, lock_task() is supposed to be permanently
   handled */
any_t idle_func(any_t arg) // Pour les activations
{
  marcel_t next, cur = marcel_self();
  DEFINE_CUR_LWP(,,);
  SET_CUR_LWP(GET_LWP(cur));

  lock_task();

  for(;;){

    mdebug("\t\t\t<Scheduler scheduled> (LWP = %d)\n", cur_lwp->number);

    //has_new_tasks=0;
	  
    //act_cntl(ACT_CNTL_READY_TO_WAIT,0);
    SET_FROZEN(cur);
    marcel_check_delayed_tasks();
    next = UNCHAIN_TASK_AND_FIND_NEXT(cur);
    GET_LWP(cur)->prev_running=cur;

    while (!(next || act_nb_unblocked)) {
//	    int i;
	    MTRACE("active wait", cur);
	      
#ifndef ACT_DONT_USE_SYSCALL
	    act_cntl(ACT_CNTL_READY_TO_WAIT,0);
	    if (act_nb_unblocked) break;
	    act_cntl(ACT_CNTL_DO_WAIT,0);	
#else      
	    while(!act_nb_unblocked) {
		    i=0;
		    while((i++<100000000) && !act_nb_unblocked)
			    ;
		    if (!act_nb_unblocked)
			    fprintf(stderr, "act_nb_unblocked=%i\n",
				    act_nb_unblocked);
	    }
	    MTRACE("end active wait", cur);
	    mdebug("fin attente active\n");
#endif
	    marcel_check_delayed_tasks();
	    SET_FROZEN(cur);
	    next = UNCHAIN_TASK_AND_FIND_NEXT(cur);
    }
    mdebug("\t\t\t<Scheduler unscheduled> (LWP = %d) next=%p\n",
	   cur_lwp->number, next);
    
    if(MA_THR_SETJMP(cur) == FIRST_RETURN) {
	    if (next) {
		    goto_next_task(next);
	    } else {
		    act_goto_next_task(NULL, ACT_RESTART_FROM_IDLE);
	    }
    }
    MA_THR_RESTARTED(cur, "Preemption");
  }
  // Pour la terminaison...
  for (;;) {
	  act_cntl(ACT_CNTL_READY_TO_WAIT,0);
	  act_cntl(ACT_CNTL_DO_WAIT,0);
  }
}
#else
/* Except at the beginning, lock_task() is supposed to be permanently
   handled */
any_t idle_func(any_t arg) // Sans activation (mono et smp)
{
  marcel_t next, cur = marcel_self();
  DEFINE_CUR_LWP(,,);
  SET_CUR_LWP(GET_LWP(cur));

#ifdef MA__TIMER
  start_timer(); /* May be redundant for main LWP */
#endif

  lock_task();

  for(;;) {

    mdebug("\t\t\t<Scheduler scheduled> (LWP = %d)\n", cur_lwp->number);

#ifdef MA__SMP
    /* Note that the current implementation does not detect DEADLOCKS
       any more when SMP mode is on. This will possibly get fixed in
       future versions. */
    {
      int successful;

      do {

	/* Do I have to stop? */
	if(cur_lwp->has_to_stop) {
	  stop_timer();
	  if(cur_lwp == &__main_lwp)
	    RAISE(PROGRAM_ERROR);
	  else {
	    mdebug("\t\t\t<LWP %d exiting>\n", cur_lwp->number);
	    longjmp(cur_lwp->home_jb, 1);
	  }
	}

	/* Do I have new jobs? */
	if(cur_lwp->has_new_tasks) {
#ifdef MA__DEBUG
	  mdebug("\tLWP %d has new tasks\n", cur_lwp->number);
	  sched_lock(cur_lwp);
	  display_sched_queue(marcel_self());
	  sched_unlock(cur_lwp);
#endif
	  break; /* Exit loop */
	}

	/* Check for delayed tasks and polling stuff to do... */
	successful = marcel_check_delayed_tasks() &&
	  marcel_self()->next != marcel_self();

	/* If previous step unsuccessful, then try to steal threads to
           other LWPs */
	if(!successful)
	  successful = do_work_stealing();

	/* If nothing successful */
	if(!successful)
	  PAUSE();

      } while(!successful);
    }
#else /* MA__SMP */
    /* Look if some delayed tasks can be waked */
    if(__delayed_tasks == NULL && !marcel_polling_is_required())
      RAISE(DEADLOCK_ERROR);

    if(marcel_polling_is_required()) {
      while(!marcel_check_delayed_tasks())
	/* True polling ! */ ;
    } else {
      while(!marcel_check_delayed_tasks()) {
	pause();
      }
    }
#endif /* MA__SMP */

    SET_FROZEN(cur);
    next = UNCHAIN_TASK_AND_FIND_NEXT(cur);

    mdebug("\t\t\t<Scheduler unscheduled> (LWP = %d)\n", cur_lwp->number);

    if(MA_THR_SETJMP(cur) == FIRST_RETURN)
      goto_next_task(next);

    MA_THR_RESTARTED(cur, "Preemption");
  };
}
#endif

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
  int i;

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

#ifdef X86_ARCH
  atomic_set(&lwp->_locked, 0);
#else
  lwp->_locked = 0;
#endif

#ifndef MA__ONE_QUEUE
  lwp->has_to_stop = FALSE;
  lwp->has_new_tasks = FALSE;
#endif
  marcel_mutex_init(&lwp->stack_mutex, NULL);
  lwp->sec_desc = SECUR_TASK_DESC(lwp);

#ifdef MA__ONE_QUEUE
  /* on initialise une seule fois la structure. */
  if (initial_task) 
#endif
  {
	  SCHED_DATA(lwp).sched_queue_lock = MARCEL_LOCK_INIT;
	  SCHED_DATA(lwp).running_tasks = 0;

	  for(i = 1; i <= MAX_PRIO; i++)
		  SCHED_DATA(lwp).__first[i] = NULL;

	  if(initial_task) {
		  SCHED_DATA(lwp).__first[0] = 
			  SCHED_DATA(lwp).__first[initial_task->prio] = 
			  initial_task;
		  initial_task->lwp = lwp;
		  initial_task->prev = initial_task->next = initial_task;
		  SET_STATE_RUNNING(NULL, initial_task, GET_LWP(initial_task));
		  /* Désormais, on s'exécute dans le lwp 0 */
	  }
  }

  /***************************************************/
  /* Création de la tâche __sched_task (i.e. "Idle") */
  /***************************************************/
  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);
#ifdef PM2
  {
    char *stack = MALLOC(2*SLOT_SIZE);

    unsigned long stsize = (((unsigned long)(stack + 2*SLOT_SIZE) & 
			     ~(SLOT_SIZE-1)) - (unsigned long)stack);

    marcel_attr_setstackaddr(&attr, stack);
    marcel_attr_setstacksize(&attr, stsize);
  }
#endif
  lock_task();
  //if(lwp->number == 0)
  //  sched_lock(lwp);
  marcel_create(&(lwp->idle_task), &attr, idle_func, NULL);
  //if(lwp->number == 0)
  //  sched_unlock(lwp);

  lwp->idle_task->special_flags |= \
	  MA_SF_POLL | MA_SF_NORUN | MA_SF_NOSCHEDLOCK;

  MTRACE("IdleTask", lwp->idle_task);
  SET_FROZEN(lwp->idle_task);
  UNCHAIN_TASK(lwp->idle_task);

#ifndef MA__ONE_QUEUE
  if (!initial_task) {
    SCHED_DATA(lwp).__first[0] = 
      SCHED_DATA(lwp).__first[lwp->idle_task->prio] = 
      lwp->idle_task;
    lwp->idle_task->prev = 
      lwp->idle_task->next = 
      lwp->idle_task;
  }
#endif

  lwp->idle_task->lwp = lwp;
#if defined(MA__LWPS) && ! defined(MA__ONE_QUEUE) 
  lwp->idle_task->previous_lwp = NULL;
#endif
  lwp->idle_task->sched_policy = MARCEL_SCHED_FIXED(lwp->number);
  /***************************************************/

  unlock_task();


#ifdef MA__ACTIVATION
  /****************************************************/
  /* Création de la tâche pour les upcalls upcall_new */
  /****************************************************/
  marcel_attr_init(&attr);
  marcel_attr_setdetachstate(&attr, TRUE);
#ifdef PM2
  {
    char *stack = MALLOC(2*SLOT_SIZE);

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
  lwp->upcall_new_task->special_flags |= MA_SF_UPCALL_NEW | MA_SF_NORUN;
  _main_struct.nb_tasks--;
  MTRACE("U_T OK", lwp->upcall_new_task);
  /****************************************************/

  unlock_task();
  /* libéré dans marcel_init_sched */
  if (initial_task)
    sched_lock(lwp);
#else /* MA__ACTIVATION */
  /* libéré dans lwp_startup_func pour SMP ou marcel_init_sched pour
   * le pool de la main_task
   * */
  sched_lock(lwp);
#endif /* MA__ACTIVATION */
}

#if defined(MA__LWPS)
static unsigned __nb_processors = 1;

static void create_new_lwp(void)
{
  __lwp_t *lwp = (__lwp_t *)MALLOC(sizeof(__lwp_t)),
          *cur_lwp = marcel_self()->lwp;

  init_lwp(lwp, NULL);

  /* Add to global linked list */
  lwp->next = cur_lwp;
  lwp->prev = cur_lwp->prev;
  cur_lwp->prev->next = lwp;
  cur_lwp->prev = lwp;
}
#endif

#ifdef SMP

static void *lwp_startup_func(void *arg)
{
  __lwp_t *lwp = (__lwp_t *)arg;

  if(setjmp(lwp->home_jb)) {
    mdebug("sched %d Exiting\n", lwp->number);
    pthread_exit(0);
  }

  mdebug("\t\t\t<LWP %d started (pthread_self == %lx)>\n",
	 lwp->number, (unsigned long)pthread_self());

#if defined(BIND_LWP_ON_PROCESSORS) && defined(SOLARIS_SYS)
  if(processor_bind(P_LWPID, P_MYID,
		    (processorid_t)(lwp->number % __nb_processors),
		    NULL) != 0) {
    perror("processor_bind");
    exit(1);
  } else
    mdebug("LWP %d bound to processor %d\n",
	   lwp->number, (lwp->number % __nb_processors));
#endif

  /* Can't use lock_task() because marcel_self() is not yet usable ! */
#ifdef X86_ARCH
  atomic_inc(&lwp->_locked);
#else
  lwp->_locked++;
#endif

  /* pris dans  init_sched() */
  sched_unlock(lwp);

  MA_THR_LONGJMP(lwp->idle_task, NORMAL_RETURN);
  
  return NULL;
}

static void start_lwp(__lwp_t *lwp)
{
  pthread_attr_t attr;

  /* Start new Pthread */
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
  pthread_create(&lwp->pid, &attr, lwp_startup_func, (void *)lwp);
}
#endif

void marcel_sched_init(unsigned nb_lwp)
{
  marcel_initialized = TRUE;

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
  SET_NB_LWPS(nb_lwp ? nb_lwp : ACT_NB_MAX_CPU);
#else
  SET_NB_LWPS(nb_lwp ? nb_lwp : __nb_processors);
#endif
  _main_struct.nb_tasks = -GET_NB_LWPS;

#else /* MA__LWPS */
  _main_struct.nb_tasks = -1;
#endif /* MA__LWPS */

  memset(__main_thread, 0, sizeof(task_desc));
  __main_thread->detached = FALSE;
  __main_thread->not_migratable = 1;
  __main_thread->prio = __main_thread->quantums = 1;
  /* Some Pthread packages do not support that a
     LWP != main_LWP execute on the main stack, so: */
  __main_thread->sched_policy = MARCEL_SCHED_FIXED(0);

  /* Initialization of "main LWP" (required even when SMP not set). 
  *
  * init_lwp calls init_sched */
  init_lwp(&__main_lwp, __main_thread);

  mdebug("marcel_sched_init: init_lwp done\n");

  __main_lwp.next = __main_lwp.prev = &__main_lwp;
// Already done in init_lwp...
//  SCHED_DATA(& __main_lwp).sched_task->sched_policy = MARCEL_SCHED_FIXED(0);
//#if defined(MA__LWPS) && ! defined(MA__ONE_QUEUE) 
//  SCHED_DATA(& __main_lwp).sched_task->previous_lwp = NULL;
//#endif

  SET_STATE_RUNNING(NULL, __main_thread, (&__main_lwp));
  one_more_active_task(__main_thread, &__main_lwp);
  MTRACE("MainTask", __main_thread);

  /* pris dans  init_sched() */
  sched_unlock(&__main_lwp);

#ifdef MA__LWPS

  /* Creation en deux phases en prevision du work stealing qui peut
     demarrer tres vite sur certains LWP... */
  {
    int i;
#ifdef MA__SMP
    __lwp_t *lwp;
#endif

    for(i=1; i<GET_NB_LWPS; i++)
      create_new_lwp();
 
    mdebug("marcel_sched_init  : %i lwps created\n",
	   GET_NB_LWPS);

#ifdef MA__SMP
    for(lwp = __main_lwp.next; lwp != &__main_lwp; lwp = lwp->next)
      start_lwp(lwp);
#endif

  }
#endif /* MA__LWPS */
#ifdef MA__ACTIVATION
  init_upcalls(GET_NB_LWPS);
#endif

#ifdef MA__TIMER
  /* Démarrage d'un timer Unix pour récupérer périodiquement 
     un signal (par ex. SIGALRM). */
  start_timer();
#endif
  mdebug("marcel_sched_init done : idle task= %p\n",
	 GET_LWP_BY_NUM(0)->idle_task);
}

#ifdef MA__SMP
static void stop_lwp(__lwp_t *lwp)
{
  int cc;

  lwp->has_to_stop = TRUE;
  /* Join corresponding Pthread */
  do {
    cc = pthread_join(lwp->pid, NULL);
  } while(cc == -1 && errno == EINTR);

  /* TODO: the stack of the lwp->sched_task is currently *NOT FREED*.
     This should be fixed! */
}
#endif

void marcel_sched_shutdown()
{
#ifdef MA__SMP
  __lwp_t *lwp;
#endif

  wait_all_tasks_end();

#ifdef MA__TIMER
  stop_timer();
#endif

#ifdef MA__SMP
  if(marcel_self()->lwp != &__main_lwp)
    RAISE(PROGRAM_ERROR);
  lwp = __main_lwp.next;
  while(lwp != &__main_lwp) {
    stop_lwp(lwp);
    lwp = lwp->next;
    FREE(lwp->prev);
  }
#elif defined(MA__ACTIVATION)
  // TODO : arrêter les autres activations...

  act_cntl(ACT_CNTL_UPCALLS, (void*)ACT_DISABLE_UPCALLS);
#else
  /* Destroy master-sched's stack */
  marcel_cancel(__main_lwp.idle_task);
#ifdef PM2
  /* __sched_task is detached, so we can free its stack now */
  FREE(marcel_stackbase(__main_lwp.idle_task));
#endif
#endif  
}


#ifndef TICK_RATE
#define TICK_RATE 1
#endif

/* TODO: Call lock_task() before re-enabling the signals to avoid stack
   overflow by recursive interrupts handlers. This needs a modified version
   of marcel_yield() that do not lock_task()... */
static void timer_interrupt(int sig)
{
  marcel_t cur = marcel_self();
#ifdef MA__DEBUG
  static unsigned long tick = 0;
#endif

#ifdef MA__DEBUG
  if (++tick == TICK_RATE) {
    try_mdebug("\t\t\t<<tick>>\n");
    tick = 0;
  }
#endif

  if(cur->lwp == &__main_lwp)
    __milliseconds += time_slice/1000;

  if(!locked() && preemption_enabled()) {

    MTRACE("TimerSig", cur);

    lock_task();
    marcel_check_delayed_tasks();
    unlock_task();

#ifdef SMP
    pthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#else
#if defined(SOLARIS_SYS) || defined(UNICOS_SYS)
    sigrelse(MARCEL_TIMER_SIGNAL);
#else
    sigprocmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif
#endif

#ifdef MINIMAL_PREEMPTION
    if(special_thread != NULL && special_thread != cur) {
      yielding_thread = cur;
      marcel_explicityield(special_thread);
    }
#else
    marcel_yield();
#endif
  }
}

static void set_timer(void)
{
  struct itimerval value;

#ifdef SMP
    pthread_sigmask(SIG_UNBLOCK, &sigalrmset, NULL);
#endif

  if(marcel_initialized) {
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_usec = time_slice;
    value.it_value = value.it_interval;
    setitimer(MARCEL_ITIMER_TYPE, &value, (struct itimerval *)NULL);
  }
}

static void start_timer(void)
{
  static struct sigaction sa;

  sigemptyset(&sa.sa_mask);
  sa.sa_handler = timer_interrupt;
  sa.sa_flags = SA_RESTART;

  sigaction(MARCEL_TIMER_SIGNAL, &sa, (struct sigaction *)NULL);

  set_timer();
}

void stop_timer(void)
{
  time_slice = 0;
  set_timer();
}

void marcel_settimeslice(unsigned long microsecs)
{
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
}

unsigned long marcel_clock(void)
{
   return __milliseconds;
}

void marcel_snapshot(snapshot_func_t f)
{
  marcel_t t;
#ifdef SMP
  __lwp_t *cur_lwp = marcel_self()->lwp;
#endif

  lock_task();

  t = SCHED_DATA(cur_lwp).__first[0];
  do {
    (*f)(t);
    t = t->next;
  } while(t != SCHED_DATA(cur_lwp).__first[0]);

  if(__delayed_tasks != NULL) {
    t = __delayed_tasks;
    do {
      (*f)(t);
      t = t->next;
    } while(t != NULL);
  }

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS

  if(__waiting_tasks != NULL) {
    t = __waiting_tasks;
    do {
      (*f)(t);
      t = t->next;
    } while(t != __waiting_tasks);
  }

#endif

  unlock_task();
}

void marcel_threadslist(int max, marcel_t *pids, int *nb, int which)
{
  marcel_t t;
  int nb_pids = 0;
#ifdef SMP
  __lwp_t *cur_lwp = marcel_self()->lwp;
#endif

  if( ((which & MIGRATABLE_ONLY) && (which & NOT_MIGRATABLE_ONLY)) ||
      ((which & DETACHED_ONLY) && (which & NOT_DETACHED_ONLY)) ||
      ((which & BLOCKED_ONLY) && (which & NOT_BLOCKED_ONLY)) ||
      ((which & SLEEPING_ONLY) && (which & NOT_SLEEPING_ONLY)))
    RAISE(CONSTRAINT_ERROR);

  lock_task();

  t = SCHED_DATA(cur_lwp).__first[0];
  do {

    if(t->detached) {
      if(which & NOT_DETACHED_ONLY)
	continue;
    } else if(which & DETACHED_ONLY)
      continue;

    if(t->not_migratable) {
      if(which & MIGRATABLE_ONLY)
	continue;
    } else if(which & NOT_MIGRATABLE_ONLY)
      continue;

    if(IS_BLOCKED(t)) {
      if(which & NOT_BLOCKED_ONLY)
	continue;
    } else if(which & BLOCKED_ONLY)
      continue;

    if(IS_SLEEPING(t)) {
      if(which & NOT_SLEEPING_ONLY)
	continue;
    } else if(which & SLEEPING_ONLY)
      continue;

    if(nb_pids < max)
      pids[nb_pids++] = t;
    else
      nb_pids++;

  } while(t = t->next, t != SCHED_DATA(cur_lwp).__first[0]);

  if(__delayed_tasks != NULL) {
    t = __delayed_tasks;
    do {

    if(t->detached) {
      if(which & NOT_DETACHED_ONLY)
	continue;
    } else if(which & DETACHED_ONLY)
      continue;

    if(t->not_migratable) {
      if(which & MIGRATABLE_ONLY)
	continue;
    } else if(which & NOT_MIGRATABLE_ONLY)
      continue;

    if(IS_BLOCKED(t)) {
      if(which & NOT_BLOCKED_ONLY)
	continue;
    } else if(which & BLOCKED_ONLY)
      continue;

    if(IS_SLEEPING(t)) {
      if(which & NOT_SLEEPING_ONLY)
	continue;
    } else if(which & SLEEPING_ONLY)
      continue;

    if(nb_pids < max)
      pids[nb_pids++] = t;
    else
      nb_pids++;

    } while(t = t->next, t != NULL);
  }

#ifndef DO_NOT_CHAIN_BLOCKED_THREADS

  if(__waiting_tasks != NULL) {
    t = __waiting_tasks;
    do {
      if(t->detached) {
	if(which & NOT_DETACHED_ONLY)
	  continue;
      } else if(which & DETACHED_ONLY)
	continue;

      if(t->not_migratable) {
	if(which & MIGRATABLE_ONLY)
	  continue;
      } else if(which & NOT_MIGRATABLE_ONLY)
	continue;

      if(IS_BLOCKED(t)) {
	if(which & NOT_BLOCKED_ONLY)
	  continue;
      } else if(which & BLOCKED_ONLY)
	continue;

      if(IS_SLEEPING(t)) {
	if(which & NOT_SLEEPING_ONLY)
	  continue;
      } else if(which & SLEEPING_ONLY)
	continue;

      if(nb_pids < max)
	pids[nb_pids++] = t;
      else
	nb_pids++;

    } while(t = t->next, t != __waiting_tasks);
  }

#endif

  *nb = nb_pids;
  unlock_task();
}

