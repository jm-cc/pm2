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
$Log: marcel.c,v $
Revision 1.30  2000/11/15 21:32:20  rnamyst
Removed 'timing' and 'safe_malloc' : all modules now use the toolbox for timing & safe malloc

Revision 1.29  2000/11/13 20:41:35  rnamyst
common_init now performs calls to all libraries

Revision 1.28  2000/11/08 08:16:14  oaumage
*** empty log message ***

Revision 1.27  2000/10/18 12:41:19  rnamyst
Euh... Je ne sais plus ce que j'ai modifie, mais c'est par mesure d'hygiene..

Revision 1.26  2000/09/15 02:04:52  rnamyst
fut_print is now built by the makefile, rather than by pm2-build-fut-entries

Revision 1.25  2000/09/13 00:07:19  rnamyst
Support for profiling + minor bug fixes

Revision 1.24  2000/06/15 08:54:04  rnamyst
Minor modifs

Revision 1.23  2000/06/09 09:09:09  vdanjean
Correction de l'initialisation de la toolbox avec Marcel

Revision 1.22  2000/05/25 00:23:52  vdanjean
marcel_poll with sisci and few bugs fixes

Revision 1.21  2000/05/09 10:52:45  vdanjean
pm2debug module

Revision 1.20  2000/05/03 18:34:47  vdanjean
few bugs fixes in key managment

Revision 1.19  2000/04/28 18:33:37  vdanjean
debug actsmp + marcel_key

Revision 1.18  2000/04/28 13:11:30  vdanjean
argc correct if NULL

Revision 1.17  2000/04/21 11:19:28  vdanjean
fixes for actsmp

Revision 1.16  2000/04/17 16:09:39  vdanjean
clean up : remove __ACT__ flags and use of MA__ACTIVATION instead of MA__ACT when needed

Revision 1.15  2000/04/17 08:31:51  rnamyst
Changed DEBUG into MA__DEBUG.

Revision 1.14  2000/04/11 09:07:20  rnamyst
Merged the "reorganisation" development branch.

Revision 1.13.2.17  2000/04/11 08:17:30  rnamyst
Comments are back !

Revision 1.13.2.16  2000/04/08 15:09:13  vdanjean
few bugs fixed

Revision 1.13.2.15  2000/04/06 13:55:20  rnamyst
Fixed a minor bug in marcel_insert_task and in marcel_exit...

Revision 1.13.2.14  2000/04/06 07:38:01  vdanjean
Activations mono OK :-)

Revision 1.13.2.13  2000/04/04 08:01:42  rnamyst
Modified to introduce "goto_next_task" instead of THR_LONGJMP.

Revision 1.13.2.12  2000/03/31 18:38:39  vdanjean
Activation mono OK

Revision 1.13.2.11  2000/03/31 08:08:08  rnamyst
Added disable_preemption() and enable_preemption().

Revision 1.13.2.10  2000/03/30 16:57:38  rnamyst
Introduced TOP_STACK_FREE_AREA...

Revision 1.13.2.9  2000/03/30 09:52:58  rnamyst
Bug fixed in init_sched.

Revision 1.13.2.8  2000/03/29 16:49:40  vdanjean
ajout de du champs special_flags dans marcel_t

Revision 1.13.2.7  2000/03/29 14:24:54  rnamyst
Added the marcel_stdio.c that provides the marcel_printf functions.

Revision 1.13.2.6  2000/03/24 17:55:23  vdanjean
fixes

Revision 1.13.2.5  2000/03/24 14:16:44  vdanjean
few bugs fixed

Revision 1.13.2.4  2000/03/22 16:34:13  vdanjean
*** empty log message ***

Revision 1.13.2.3  2000/03/17 20:09:54  vdanjean
*** empty log message ***

Revision 1.13.2.2  2000/03/15 15:55:03  vdanjean
réorganisation de marcel : commit pour CVS

Revision 1.13.2.1  2000/03/15 15:41:20  vdanjean
réorganisation de marcel. branche de développement

Revision 1.13  2000/03/09 11:07:51  rnamyst
Modified to use the sched_data() macro.

Revision 1.12  2000/03/06 12:57:33  vdanjean
*** empty log message ***

Revision 1.11  2000/03/06 09:24:01  vdanjean
Nouvelle version des activations

Revision 1.10  2000/02/28 10:24:58  rnamyst
Changed #include <> into #include "".

Revision 1.9  2000/01/31 15:57:09  oaumage
- ajout du Log CVS


______________________________________________________________________________
*/

#include "marcel.h"
#include "mar_timing.h"
#include "marcel_alloc.h"
#include "sys/marcel_work.h"

#include "common.h"
#include "tbx.h"
#include "profile.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <signal.h>

#ifdef MA__ACTIVATION
#include "sys/upcalls.h"
#endif

#ifdef UNICOS_SYS
#include <sys/mman.h>
#endif

#ifdef SOLARIS_SYS
#include <sys/stack.h>
#endif

#ifdef RS6K_ARCH
int _jmg(int r)
{
  if(r!=0) 
    unlock_task();
  return r;
}

#undef longjmp
void LONGJMP(jmp_buf buf, int val)
{
  static jmp_buf _buf;
  static int _val;

  lock_task();
  memcpy(_buf, buf, sizeof(jmp_buf));
  _val = val;
  set_sp(SP_FIELD(buf)-256);
  longjmp(_buf, _val);
}
#define longjmp(buf, v)	LONGJMP(buf, v)
#endif


#ifdef STACK_OVERFLOW_DETECT
#error "STACK_OVERFLOW_DETECT IS CURRENTLY NOT IMPLEMENTED"
#endif

static long page_size;

#define PA(X) ((((long)X)+(page_size-1)) & ~(page_size-1))

volatile boolean always_false = FALSE;

exception
   TASKING_ERROR = "TASKING_ERROR : A non-handled exception occurred in a task",
   DEADLOCK_ERROR = "DEADLOCK_ERROR : Global Blocking Situation Detected",
   STORAGE_ERROR = "STORAGE_ERROR : No space left on the heap",
   CONSTRAINT_ERROR = "CONSTRAINT_ERROR",
   PROGRAM_ERROR = "PROGRAM_ERROR",
   STACK_ERROR = "STACK_ERROR : Stack Overflow",
   TIME_OUT = "TIME OUT while being blocked on a semaphor",
   NOT_IMPLEMENTED = "NOT_IMPLEMENTED (sorry)",
   USE_ERROR = "USE_ERROR : Marcel was not compiled to enable this functionality",
   LOCK_TASK_ERROR = "LOCK_TASK_ERROR : All tasks blocked after bad use of lock_task()";

volatile unsigned long threads_created_in_cache = 0;

#ifdef MINIMAL_PREEMPTION

static marcel_t special_thread = NULL,
                yielding_thread = NULL;

#endif

/* C'EST ICI QU'IL EST PRATIQUE DE METTRE UN POINT D'ARRET
   LORSQUE L'ON VEUT EXECUTER PAS A PAS... */
void breakpoint()
{
  /* Lieu ideal ;-) pour un point d'arret */
}

/* =========== specifs =========== */
int marcel_cancel(marcel_t pid);

#ifdef STACK_OVERFLOW_DETECT

void stack_protect(marcel_t t)
{
  int r;

   r = mprotect((char *)PA(t->stack_base), page_size, PROT_NONE);
   if(r == -1)
      RAISE(CONSTRAINT_ERROR);
}

void stack_unprotect(marcel_t t)
{
  int r;

   r = mprotect((char *)PA(t->stack_base), page_size, PROT_READ | PROT_WRITE);
   if(r == -1)
      RAISE(CONSTRAINT_ERROR);
}

#endif

#ifdef MA__DEBUG
void _showsigmask(char *msg)
{
  sigset_t set;

  sigprocmask(0, NULL, &set);
  mdebug("%s : SIG_ALRM %s\n",
	 msg, (sigismember(&set, SIGALRM) ? "masqued" : "unmasqued"));
}
#endif

static __inline__ void marcel_terminate(marcel_t t)
{
  int i;

  for(i=((marcel_t)t)->next_cleanup_func-1; i>=0; i--)
    (*((marcel_t)t)->cleanup_funcs[i])(((marcel_t)t)->cleanup_args[i]);
}

#ifdef MA__DEBUG

static void print_thread(marcel_t pid)
{
  long sp;

   if(pid == marcel_self()) {
#if defined(ALPHA_ARCH)
      get_sp(&sp);
#else
      sp = (long)get_sp();
#endif
   } else {
      sp = (long)SP_FIELD(pid->jbuf);
   }

  mdebug("thread %p :\n"
	 "\tlower bound : %p\n"
	 "\tupper bound : %lx\n"
	 "\tuser space : %p\n"
	 "\tinitial sp : %lx\n"
	 "\tcurrent sp : %lx\n"
	 ,
	 pid,
	 pid->stack_base,
	 (long)pid + MAL(sizeof(task_desc)),
	 pid->user_space_ptr,
	 pid->initial_sp,
	 sp
	 );
}

void print_jmp_buf(char *name, jmp_buf buf)
{
  int i;

  for(i=0; i<sizeof(jmp_buf)/sizeof(char *); i++) {
    mdebug("%s[%d] = %p %s\n",
	   name,
	   i,
	   ((char **)buf)[i],
	   (((char **)buf + i) == (char **)&SP_FIELD(buf) ? "(sp)" : "")
	   );
  }
}

#endif

static __inline__ void init_task_desc(marcel_t t)
{
  t->cur_excep_blk = NULL;
  t->deviation_func = NULL;
#if defined(MA__LWPS) && ! defined(MA__ONE_QUEUE)
  t->previous_lwp = NULL;
#endif
  t->next_cleanup_func = 0;
#ifdef ENABLE_STACK_JUMPING
  *((marcel_t *)((char *)t + MAL(sizeof(task_desc)) - sizeof(void *))) = t;
#endif
  SET_STATE_READY(t);
  t->special_flags = 0;
}

int marcel_create(marcel_t *pid, marcel_attr_t *attr, marcel_func_t func, any_t arg)
{
  marcel_t cur = marcel_self(), new_task;

  LOG_IN();

  TIMING_EVENT("marcel_create");

  if(!attr)
    attr = &marcel_attr_default;

  lock_task();

  if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
    MA_THR_RESTARTED(cur, "Preemption");
    if(cur->child) {
      marcel_insert_task(cur->child);
    }
    unlock_task();
    LOG_OUT();
    return 0;
  } else {
    if(attr->stack_base) {
      register unsigned long top = MAL_BOT((long)attr->stack_base +
					   attr->stack_size);
#ifdef MA__DEBUG
      mdebug("top=%p, stack_base=%p\n", top, attr->stack_base);
      if(top & (SLOT_SIZE-1)) { /* Not slot-aligned */
	unlock_task();
	RAISE(CONSTRAINT_ERROR);
      }
#endif
      new_task = (marcel_t)(top - MAL(sizeof(task_desc)));
#ifdef STACK_CHECKING_ALLOWED
      memset(attr->stack_base, 0, attr->stack_size);
#endif
      init_task_desc(new_task);
      new_task->stack_base = attr->stack_base;

#ifdef STACK_OVERFLOW_DETECT
      stack_protect(new_task);
#endif
      new_task->static_stack = TRUE;
    } else {
      char *bottom;

#ifdef MA__DEBUG
      if(attr->stack_size > SLOT_SIZE)
	RAISE(NOT_IMPLEMENTED);
#endif
      bottom = marcel_slot_alloc();
      new_task = (marcel_t)(MAL_BOT((long)bottom + SLOT_SIZE) - MAL(sizeof(task_desc)));
      init_task_desc(new_task);
      new_task->stack_base = bottom;
      new_task->static_stack = FALSE;
    }

#ifdef USE_PRIORITIES
    new_task->prio = new_task->quantums = attr->priority;
#endif
    new_task->sched_policy = attr->sched_policy;
    new_task->not_migratable = attr->not_migratable;
    new_task->not_deviatable = attr->not_deviatable;
    new_task->detached = attr->detached;

    if(!attr->detached) {
      marcel_sem_init(&new_task->client, 0);
      marcel_sem_init(&new_task->thread, 0);
    }

    /* memcpy(new_task->key, cur->key, sizeof(new_task->key)); */

    if(attr->user_space)
      new_task->user_space_ptr = (char *)new_task - MAL(attr->user_space);
    else
      new_task->user_space_ptr = NULL;

    new_task->father = cur;
    new_task->lwp = cur->lwp; /* In case timer_interrupt is called before
				 insert_task ! */
    new_task->f_to_call = func;
    new_task->arg = arg;
    new_task->initial_sp = (long)new_task - MAL(attr->user_space) -
      TOP_STACK_FREE_AREA;

    if(pid)
      *pid = new_task;

    marcel_one_more_task(new_task);
    MTRACE("Creation", new_task);

    PROF_SWITCH_TO(new_task);

    PROF_IN_EXT(newborn_thread);

#ifndef MINIMAL_PREEMPTION
    if((locked() > 1) /* lock_task has been called */
       || (new_task->user_space_ptr && !attr->immediate_activation)
#ifdef SMP
       || (new_task->sched_policy != MARCEL_SCHED_OTHER)
#endif
       ) {
#endif

      new_task->father->child = ((new_task->user_space_ptr && !attr->immediate_activation)
				 ? NULL /* do not insert now */
				 : new_task); /* insert asap */
      SET_STATE_RUNNING(NULL, new_task, GET_LWP(new_task));

      call_ST_FLUSH_WINDOWS();
      set_sp(new_task->initial_sp);

      MTRACE("Preemption", marcel_self());

      PROF_OUT_EXT(newborn_thread);

      if(MA_THR_SETJMP(marcel_self()) == FIRST_RETURN) {
	MA_THR_LONGJMP(marcel_self()->father, NORMAL_RETURN);
      }
      MA_THR_RESTARTED(marcel_self(), "Preemption");
      unlock_task();

#ifndef MINIMAL_PREEMPTION
    } else {

      new_task->father->child = NULL;

      SET_STATE_RUNNING(NULL, new_task, GET_LWP(new_task));
      marcel_insert_task(new_task);

      call_ST_FLUSH_WINDOWS();
      set_sp(new_task->initial_sp);
      SET_STATE_READY(marcel_self()->father);

      MTRACE("Preemption", marcel_self());

      unlock_task();

      PROF_OUT_EXT(newborn_thread);
    }
#endif

    marcel_exit((*marcel_self()->f_to_call)(marcel_self()->arg));
  }

  return 0;
}

int marcel_join(marcel_t pid, any_t *status)
{
  LOG_IN();

#ifdef MA__DEBUG
  if(pid->detached)
    RAISE(PROGRAM_ERROR);
#endif

  marcel_sem_P(&pid->client);
  if(status)
    *status = pid->ret_val;
  marcel_sem_V(&pid->thread);

  LOG_OUT();
  return 0;
}

int marcel_exit(any_t val)
{
  marcel_t cur = marcel_self();
  DEFINE_CUR_LWP(register, , );

  LOG_IN();

  // gestion des thread_keys
  {
    int nb_keys=marcel_nb_keys;
    int key;
    int nb_bcl=0;
#define NB_MAX_BCL 1
    while (nb_keys && (nb_bcl++<NB_MAX_BCL)) {
      nb_keys=0;
      for(key=1; key<MAX_KEY_SPECIFIC; key++) {
	if (marcel_key_destructor[key] && cur->key[key]) {
	   (*(marcel_key_destructor[key]))(cur->key[key]);
	   nb_keys++;
	}
      }
    }
#ifdef MA__DEBUG
   if((NB_MAX_BCL>1) && (nb_bcl==NB_MAX_BCL))
      mdebug("  max iteration in key destructor for thread %i\n",cur->number);
#endif
  }

  SET_CUR_LWP(GET_LWP(cur));

#ifdef SMP
  /* Durant cette fonction, il ne faut pas que le thread soit
     "déplacé" intempestivement (e.g. après avoir acquis stack_mutex)
     sur un autre LWP. */
  cur->sched_policy = MARCEL_SCHED_FIXED(cur_lwp->number);
  /* Ne pas oublier de mettre "ancien LWP a NULL", sinon la tache sera
     replacee dessus dans insert_task(). Cette optimisation est source
     de confusion et il vaudrait peut-etre mieux la supprimer... */
  cur->previous_lwp = NULL;
#endif

  cur->ret_val = val;
  if(!cur->detached) {
    marcel_sem_V(&cur->client);
    marcel_sem_P(&cur->thread);
  }

  // Dans le cas où la pile a été allouée à l'extérieur de Marcel
  // (typiquement par PM2/isomalloc), il faut effectuer un traitement
  // délicat pour pouvoir appeler "marcel_terminate" quelque soit le
  // code exécuté par cette dernière fonction.
  if(cur->static_stack) {

    // Il faut commencer par acquérir le verrou d'obtention de la pile
    // de sécurité sur le LWP courant (il y a une pile de sécurité par
    // LWP).
    marcel_mutex_lock(&cur_lwp->stack_mutex);

    lock_task();

    // On fabrique un "faux" thread dans la pile de secours
    // (cur_lwp->sec_desc) et ce thread devient le thread courant.
    init_task_desc(cur_lwp->sec_desc);

    // Pour les informations de debug, on reprend le numéro du thread
    // précédent
    cur_lwp->sec_desc->number = cur->number;

    // Attention : il est nécessaire de s'assurer que le thread "de
    // secours" ne sera pas inséré sur un autre LWP...
    cur_lwp->sec_desc->sched_policy = MARCEL_SCHED_FIXED(cur_lwp->number);

    MTRACE("Mutation", cur);

    // On insere le nouveau thread _avant_ de retirer l'ancien (pour
    // eviter le deadlock)
    SET_STATE_RUNNING(NULL, cur_lwp->sec_desc, cur_lwp);
    marcel_insert_task(cur_lwp->sec_desc);

    // On retire l'ancien
    SET_GHOST(cur);
    UNCHAIN_MYSELF(cur, cur_lwp->sec_desc);

    // Avant d'exécuter unlock_task, on stocke une référence sur
    // l'ancien thread dans une variable statique propre au LWP
    // courant de manière à pouvoir le détruire par la suite...
    cur_lwp->sec_desc->private_val = (any_t)cur;

#ifdef STACK_OVERFLOW_DETECT
    stack_unprotect(cur);
#endif

    // On bascule maintenant sur la pile de secours :marcel_self()
    // devient donc égal à cur_lwp->sec_desc...
    call_ST_FLUSH_WINDOWS();
    set_sp(SECUR_STACK_TOP(cur_lwp));
#ifdef MA__LWPS
    // On recalcule "cur_lwp" car c'est une variable locale.
    cur_lwp = marcel_self()->lwp;
#endif

    unlock_task();

    // Ici, le noyau marcel n'est plus vérouillé, on peut donc appeler
    // marcel_terminate sans risque particulier en cas d'appel
    // bloquant.
    marcel_terminate(cur_lwp->sec_desc->private_val);

    // Ok, maintenant on ré-entre en "mode noyau" avec lock_task.
    lock_task();

    // On peut alors relâcher le mutex : aucun thread ne pourra
    // l'acquérir avant que l'on exécute unlock_task, c'est à dire
    // lorsque le goto_next_task aboutira.
    marcel_mutex_unlock(&cur_lwp->stack_mutex);

    // On averti le main qu'une tache de plus va disparaitre, ce qui
    // va provoquer insert_task(main_task) si le thread courant est le
    // dernier de l'application...  ATTENTION : cela signifie qu'il
    // faut impérativement exécuter cette instruction _avant_
    // unchain_task, faute de quoi on risquerait d'insérer la tâche
    // idle à tort !!!
    marcel_one_task_less(cur_lwp->sec_desc);

    // Il est maintenant temps de retirer le "faux" thread par un
    // appel à unchain_task...
    SET_GHOST(cur_lwp->sec_desc);
    cur = UNCHAIN_TASK_AND_FIND_NEXT(cur_lwp->sec_desc);

    MTRACE("Exit", cur_lwp->sec_desc);

#ifdef MA__MULTIPLE_RUNNING
    cur_lwp->prev_running=NULL;
#endif

    LOG_OUT();

    goto_next_task(cur);

  } else { // Ici, la pile a été allouée par le noyau Marcel

#ifdef PM2
    RAISE(PROGRAM_ERROR);
#endif

    // On appelle marcel_terminate sur la pile courante puisque
    // celle-ci ne sera jamais détruite par l'une des fonction "cleanup"...
    marcel_terminate(cur);

    // Il faut acquérir le verrou de la pile de secours avant
    // d'exécuter lock_task.
    marcel_mutex_lock(&cur_lwp->stack_mutex);

    lock_task();

    // Il peut paraître stupide de relâcher le verrou à cet endroit
    // (on vient de le prendre !). Premièrement, ce n'est pas grave
    // car lock_task garanti qu'aucun autre thread (sur le LWP)
    // n'utilisera la pile. Deuxièmement, il _faut_ relâcher ce verrou
    // très tôt (avant d'exécuter unchain_task) car sinon la tâche
    // idle risquerait d'être réveillée (par unchain_task) alors que
    // mutex_unlock réveillerait par ailleurs une autre tâche du
    // programme !
    marcel_mutex_unlock(&cur_lwp->stack_mutex); /* idem ci-dessus */

    // Même remarque que précédemment : main_thread peut être réveillé
    // à cet endroit, donc il ne faut appeler unchain_task qu'après.
    marcel_one_task_less(cur);

    // On peut se retirer de la file des threads prêts. le champ
    // "child" de la pile de secours désigne maintenant le thread à
    // qui on donnera la "main" avant de disparaître.
    SET_GHOST(cur);
    cur_lwp->sec_desc->child = UNCHAIN_TASK_AND_FIND_NEXT(cur);

    MTRACE("Exit", cur);

    // Le champ "father" désigne la tâche courante : cela permettra de
    // la détruire un peu plus loin...
    cur_lwp->sec_desc->father = cur;

    // Avant de changer de pile il faut, comme toujours, positionner
    // correctement le champ lwp...
    cur_lwp->sec_desc->lwp = cur_lwp;

    // Ca y est, on peut basculer sur la pile de secours.
    call_ST_FLUSH_WINDOWS();
    set_sp(SECUR_STACK_TOP(cur_lwp));

#ifdef MA__LWPS
    // On recalcule "cur_lwp" car c'est une variable locale.
    cur_lwp = marcel_self()->lwp;
#endif

#ifdef STACK_OVERFLOW_DETECT
    stack_unprotect(cur_lwp->sec_desc->father);
#endif

    // On détruit l'ancien thread
    marcel_slot_free(marcel_stackbase(cur_lwp->sec_desc->father));

#ifdef MA__MULTIPLE_RUNNING
    cur_lwp->prev_running=NULL;
#endif

    LOG_OUT();

    // Enfin, on effectue un changement de contexte vers le thread suivant.
    goto_next_task(cur_lwp->sec_desc->child);
  }

  return 0; /* Silly, isn't it ? */
}

int marcel_cancel(marcel_t pid)
{
  if(pid == marcel_self()) {
    marcel_exit(NULL);
  } else {
    pid->ret_val = NULL;
    mdebug("marcel %i kill %i\n", marcel_self()->number, pid->number);
    marcel_deviate(pid, (handler_func_t)marcel_exit, NULL);
  }
  return 0;
}

int marcel_detach(marcel_t pid)
{
   pid->detached = TRUE;
   return 0;
}

void marcel_getuserspace(marcel_t pid, void **user_space)
{
   *user_space = pid->user_space_ptr;
}

void marcel_run(marcel_t pid, any_t arg)
{
  lock_task();
  pid->arg = arg;
  marcel_insert_task(pid);
  unlock_task();
}

int marcel_cleanup_push(cleanup_func_t func, any_t arg)
{
  marcel_t cur = marcel_self();

   if(cur->next_cleanup_func == MAX_CLEANUP_FUNCS)
      RAISE(CONSTRAINT_ERROR);

   cur->cleanup_args[cur->next_cleanup_func] = arg;
   cur->cleanup_funcs[cur->next_cleanup_func++] = func;
   return 0;
}

int marcel_cleanup_pop(boolean execute)
{
  int i;
  marcel_t cur = marcel_self();

   if(!cur->next_cleanup_func)
      RAISE(CONSTRAINT_ERROR);

   i = --cur->next_cleanup_func;
   if(execute)
      (*cur->cleanup_funcs[i])(cur->cleanup_args[i]);
   return 0;
}

int marcel_once(marcel_once_t *once, void (*f)(void))
{
   marcel_mutex_lock(&once->mutex);
   if(!once->executed) {
      once->executed = TRUE;
      marcel_mutex_unlock(&once->mutex);
      (*f)();
   } else
      marcel_mutex_unlock(&once->mutex);
   return 0;
}

static void __inline__ freeze(marcel_t t)
{
   if(t != marcel_self()) {
      lock_task();
      if(IS_BLOCKED(t) || IS_FROZEN(t)) {
         unlock_task();
         RAISE(PROGRAM_ERROR);
      } else if(IS_SLEEPING(t)) {
         marcel_wake_task(t, NULL);
      }
      SET_FROZEN(t);
      UNCHAIN_TASK(t);
      unlock_task();
   }
}

static void __inline__ unfreeze(marcel_t t)
{
   if(IS_FROZEN(t)) {
      lock_task();
      marcel_wake_task(t, NULL);
      unlock_task();
   }
}

void marcel_freeze(marcel_t *pids, int nb)
{
  int i;

   for(i=0; i<nb; i++)
      freeze(pids[i]);
}

void marcel_unfreeze(marcel_t *pids, int nb)
{
  int i;

   for(i=0; i<nb; i++)
      unfreeze(pids[i]);
}

/* WARNING!!! MUST BE LESS CONSTRAINED THAN MARCEL_ALIGN (64) */
#define ALIGNED_32(addr)(((unsigned long)(addr) + 31) & ~(31L))

// TODO : Vérifier le code avec les activations
void marcel_begin_hibernation(marcel_t t, transfert_func_t transf, 
			      void *arg, boolean fork)
{
  unsigned long depl, blk;
  unsigned long bottom, top;
  marcel_t cur = marcel_self();

#ifdef MA__ACTIVATION
  RAISE("Not implemented");
#endif

  if(t == cur) {
    lock_task();
    if(setjmp(cur->migr_jb) == FIRST_RETURN) {

      call_ST_FLUSH_WINDOWS();
      top = (long)cur + ALIGNED_32(sizeof(task_desc));
      bottom = ALIGNED_32((long)SP_FIELD(cur->migr_jb)) - ALIGNED_32(1);
      blk = top - bottom;
      depl = bottom - (long)cur->stack_base;
      unlock_task();

      mdebug("hibernation of thread %p", cur);
      mdebug("sp = %lu\n", get_sp());
      mdebug("sp_field = %u\n", SP_FIELD(cur->migr_jb));
      mdebug("bottom = %lu\n", bottom);
      mdebug("top = %lu\n", top);
      mdebug("blk = %lu\n", blk);

      (*transf)(cur, depl, blk, arg);

      if(!fork)
	marcel_exit(NULL);
    } else {
#ifdef MA__DEBUG
      breakpoint();
#endif
      unlock_task();
    }
  } else {
    memcpy(t->migr_jb, t->jbuf, sizeof(jmp_buf));

    top = (long)t + ALIGNED_32(sizeof(task_desc));
    bottom = ALIGNED_32((long)SP_FIELD(t->jbuf)) - ALIGNED_32(1);
    blk = top - bottom;
    depl = bottom - (long)t->stack_base;

    mdebug("hibernation of thread %p", t);
    mdebug("sp_field = %u\n", SP_FIELD(t->migr_jb));
    mdebug("bottom = %lu\n", bottom);
    mdebug("top = %lu\n", top);
    mdebug("blk = %lu\n", blk);

    (*transf)(t, depl, blk, arg);

    if(!fork) {
      marcel_cancel(t);
    }
  }
}

marcel_t marcel_alloc_stack(unsigned size)
{
  marcel_t t;
  char *st;

  lock_task();

#ifdef PM2
  RAISE(PROGRAM_ERROR);
#endif

  st = marcel_slot_alloc();

  t = (marcel_t)(MAL_BOT((long)st + size) - MAL(sizeof(task_desc)));

  init_task_desc(t);
  t->stack_base = st;

#ifdef STACK_OVERFLOW_DETECT
  stack_protect(t);
#endif
#ifdef STACK_CHECKING_ALLOWED
  memset(t->stack_base, 0, size);
#endif

  unlock_task();

  return t;
}

// TODO : Vérifier le code avec les activations
void marcel_end_hibernation(marcel_t t, post_migration_func_t f, void *arg)
{
#ifdef MA__ACTIVATION
  RAISE("Not implemented");
#endif

  memcpy(t->jbuf, t->migr_jb, sizeof(jmp_buf));

  mdebug("end of hibernation for thread %p", t);

  lock_task();

  marcel_insert_task(t);
  marcel_one_more_task(t);

  if(f != NULL)
    marcel_deviate(t, f, arg);

  unlock_task();
}

void marcel_enabledeviation(void)
{
  volatile handler_func_t func;
  any_t arg;
  marcel_t cur = marcel_self();

  lock_task();
  if(--cur->not_deviatable == 0 && cur->deviation_func != NULL) {
    func = cur->deviation_func;
    arg = cur->deviation_arg;
    cur->deviation_func = NULL;
    unlock_task();
    (*func)(arg);
  } else
    unlock_task();
}

void marcel_disabledeviation(void)
{
  ++marcel_self()->not_deviatable;
}

#ifndef MA__WORK
static void insertion_relai(handler_func_t f, void *arg)
{ 
  jmp_buf back;
  marcel_t cur = marcel_self();

#ifdef MA__ACTIVATION
  RAISE(NOT_IMPLEMENTED);
#endif

  memcpy(back, cur->jbuf, sizeof(jmp_buf));
  marcel_disablemigration(cur);
  // TODO: Si locked() != 1, on peut optimiser en autorisant le fils a
  // s'executer immediatement...
  if(MA_THR_SETJMP(cur) == FIRST_RETURN) {
    goto_next_task(cur->father);
  } else {
#ifdef MA__DEBUG
    breakpoint();
#endif
    unlock_task();
    (*f)(arg);
    lock_task();
    marcel_enablemigration(cur);
    // TODO: Ce memcpy est-il vraiment utile ???
    memcpy(cur->jbuf, back, sizeof(jmp_buf));
    MA_THR_LONGJMP(cur, NORMAL_RETURN);
  }
}

/* VERY INELEGANT: to avoid inlining of insertion_relai function... */
typedef void (*relai_func_t)(handler_func_t f, void *arg);
static relai_func_t relai_func = insertion_relai;
#endif

static int marcel_deviate_record(marcel_t pid, handler_func_t h, any_t arg){
#ifndef MA__WORK
  if (pid->deviation_func != NULL) {
    return 0;
  } else {
    pid->deviation_func = h;
    pid->deviation_arg = arg;
    return 1;
  }
#else
  if (pid->deviation_func != NULL) {
    return 0;
  } else {
    pid->deviation_func = h;
    pid->deviation_arg = arg;
    pid->has_work |= MARCEL_WORK_DEVIATE;
    return 1;
  }
#endif
}

void marcel_deviate(marcel_t pid, handler_func_t h, any_t arg)
{ 
#ifndef MA__WORK
  static volatile handler_func_t f_to_call;
  static void * volatile argument;
  static volatile long initial_sp;
#endif

  mdebug_deviate("%p deviate pid:%p, f:%p, arg:%p\n", marcel_self(),
		 pid,h,arg); 
  lock_task();
  LOCK_WORK(pid);
  if(pid == marcel_self()) {
    if(pid->not_deviatable) {
      if(! marcel_deviate_record(pid, h, arg)) {
	UNLOCK_WORK(pid);
	unlock_task();
	RAISE(NOT_IMPLEMENTED);
      }
      UNLOCK_WORK(pid);
      unlock_task();
    } else {
      UNLOCK_WORK(pid);
      unlock_task();
      (*h)(arg);
    }
  } else {
#ifndef MA__WORK
    if(pid->not_deviatable) {
      if(! marcel_deviate_record(pid, h, arg)) {
	UNLOCK_WORK(pid);
	unlock_task();
	RAISE(NOT_IMPLEMENTED);
      }
      unlock_task();
    } else {
      if(IS_BLOCKED(pid) || IS_SLEEPING(pid))
	marcel_wake_task(pid, NULL);
      else if(IS_FROZEN(pid))
	marcel_wake_task(pid, NULL);

#ifdef MA__ACTIVATION
      RAISE(NOT_IMPLEMENTED);
#endif
#ifdef SMP
      if(marcel_self()->lwp != pid->lwp)
	RAISE(NOT_IMPLEMENTED);
#endif

      UNLOCK_WORK(pid);
      if(MA_THR_SETJMP(marcel_self()) == NORMAL_RETURN) {
	unlock_task();
      } else {
	f_to_call = h;
	argument = arg;
     
	pid->father = marcel_self();
     
	initial_sp = MAL_BOT((long)SP_FIELD(pid->jbuf)) -
	  TOP_STACK_FREE_AREA - 256;

	call_ST_FLUSH_WINDOWS();
	set_sp(initial_sp);

	(*relai_func)(f_to_call, argument);
	
	RAISE(PROGRAM_ERROR); /* on ne doit jamais arriver ici ! */      
      }
    }
#else
    for(;;) {
      if(marcel_deviate_record(pid, h, arg)) {
	break;
      }
      if(pid->not_deviatable) {
	UNLOCK_WORK(pid);
	unlock_task();
	RAISE(NOT_IMPLEMENTED);
      }
      if(IS_BLOCKED(pid) || IS_SLEEPING(pid))
	marcel_wake_task(pid, NULL);
      else if(IS_FROZEN(pid))
	marcel_wake_task(pid, NULL);
      UNLOCK_WORK(pid);
      unlock_task();
      marcel_explicityield(pid);
      lock_task();
      LOCK_WORK(pid);
    }
    if(IS_BLOCKED(pid) || IS_SLEEPING(pid))
      marcel_wake_task(pid, NULL);
    else if(IS_FROZEN(pid))
      marcel_wake_task(pid, NULL);
    UNLOCK_WORK(pid);
    unlock_task();
#endif
  }   
}

static unsigned __nb_lwp = 0;

static void marcel_parse_cmdline(int *argc, char **argv, boolean do_not_strip)
{
  int i, j;

  if (!argc)
    return;

  i = j = 1;

  while(i < *argc) {
#ifdef MA__LWPS
    if(!strcmp(argv[i], "-nvp")) {
      if(i == *argc-1) {
	fprintf(stderr,
		"Fatal error: -nvp option must be followed "
		"by <nb_of_virtual_processors>.\n");
	exit(1);
      }
      if(do_not_strip) {
	__nb_lwp = atoi(argv[i+1]);
	if(__nb_lwp < 1 || __nb_lwp > MAX_LWP) {
	  fprintf(stderr,
		  "Error: nb of VP should be between 1 and %d\n",
		  MAX_LWP);
	  exit(1);
	}
	argv[j++] = argv[i++];
	argv[j++] = argv[i++];
      } else
	i += 2;
      continue;
    } else
#endif
      argv[j++] = argv[i++];
  }
  *argc = j;
  argv[j] = NULL;

  if(do_not_strip)
    mdebug("\t\t\t<Suggested nb of Virtual Processors : %d>\n", __nb_lwp);
}

void marcel_strip_cmdline(int *argc, char *argv[])
{
  marcel_parse_cmdline(argc, argv, FALSE);

  marcel_debug_init(argc, argv, PM2DEBUG_CLEAROPT);
}

// May start some internal threads or activations.
// When completed, fork calls are prohibited.
void marcel_init_data(int *argc, char *argv[])
{
  static volatile boolean already_called = FALSE;

  // Only execute this function once
  if(already_called)
    return;
  already_called = TRUE;

  // Initialize debug facilities
  marcel_debug_init(argc, argv, PM2DEBUG_DO_OPT);

  // Parse command line
  marcel_parse_cmdline(argc, argv, TRUE);

  // Get size of pages
#ifdef SOLARIS_SYS
  page_size = sysconf(_SC_PAGESIZE);
#else
#ifdef UNICOS_SYS
  page_size = 1024;
#else
  page_size = getpagesize();
#endif
#endif

  // Initialize the stack memory allocator
  // This is unuseful if PM2 is used, but it is not harmful anyway ;-)
  marcel_slot_init();

  // Initialize scheduler
  marcel_sched_init(__nb_lwp);

  // Initialize mechanism for handling Unix I/O through the scheduler
  marcel_io_init();
}

// When completed, some threads/activations may be started
// Fork calls are now prohibited
void marcel_start_sched(int *argc, char *argv[])
{
  static volatile boolean already_called = FALSE;

  // Only execute this function once
  if(already_called)
    return;
  already_called = TRUE;

  // Start scheduler (i.e. run LWP/activations, start timer)
  marcel_sched_start();
}

void marcel_purge_cmdline(int *argc, char *argv[])
{
  static volatile boolean already_called = FALSE;

  // Only execute this function once
  if(already_called)
    return;
  already_called = TRUE;

  // Remove marcel-specific arguments from command line
  marcel_strip_cmdline(argc, argv);
}

#if 0 // Just kept for documentation purposes (!?!)
void marcel_init_ext(int *argc, char *argv[], int debug_flags)
{
  static volatile boolean already_called = FALSE;

  if(!already_called) {

#ifdef PROFILE
    profile_init();
#endif

#ifdef TBX
    tbx_init(*argc, argv);
#endif
    marcel_debug_init(argc, argv, PM2DEBUG_DO_OPT);

    mdebug("\t\t\t<marcel_init>\n");

    /* Analyse but do not strip */
    marcel_parse_cmdline(argc, argv, TRUE);

#ifndef PM2
    /* Strip without analyse */
    marcel_strip_cmdline(argc, argv);
#endif

#ifdef SOLARIS_SYS
    page_size = sysconf(_SC_PAGESIZE);
#else
#ifdef UNICOS_SYS
    page_size = 1024;
#else
    page_size = getpagesize();
#endif
#endif

    marcel_slot_init();

    marcel_sched_init(__nb_lwp);
    mdebug("\t\t\t<marcel_init: sched_init done>\n");

    marcel_sched_start();
    mdebug("\t\t\t<marcel_init: sched_start done>\n");

    marcel_io_init();
    mdebug("\t\t\t<marcel_init: io_init done>\n");

    mdebug("\t\t\t<marcel_init completed>\n");

    already_called = TRUE;
  }
}
#endif // #if 0

void marcel_end(void)
{
  marcel_sched_shutdown();
  marcel_slot_exit();
  mdebug("threads created in cache : %ld\n", threads_created_in_cache);
}

unsigned long marcel_cachedthreads(void)
{
   return threads_created_in_cache;
}

unsigned long marcel_usablestack(void)
{
  long sp;

#ifdef ALPHA_ARCH
   get_sp(&sp);
#else
   sp = (long)get_sp();
#endif
   return sp - (long)marcel_self()->stack_base;
}

unsigned long marcel_unusedstack(void)
{
#ifdef STACK_CHECKING_ALLOWED
   char *repere = (char *)marcel_self()->stack_base;
   unsigned *ptr = (unsigned *)repere;

   while(!(*ptr++));
   return ((char *)ptr - repere);

#else
   RAISE(USE_ERROR);
   return 0;
#endif
}

void *marcel_malloc(unsigned size, char *file, unsigned line)
{
  void *p;

   if(size) {
      lock_task();
      if((p = __TBX_MALLOC(size, file, line)) == NULL)
         RAISE(STORAGE_ERROR);
      else {
         unlock_task();
         return p;
      }
   }
   return NULL;
}

void *marcel_realloc(void *ptr, unsigned size, char *file, unsigned line)
{
  void *p;

   lock_task();
   if((p = __TBX_REALLOC(ptr, size, file, line)) == NULL)
      RAISE(STORAGE_ERROR);
   unlock_task();
   return p;
}

void *marcel_calloc(unsigned nelem, unsigned elsize, char *file, unsigned line)
{
  void *p;

   if(nelem && elsize) {
      lock_task();
      if((p = __TBX_CALLOC(nelem, elsize, file, line)) == NULL)
         RAISE(STORAGE_ERROR);
      else {
         unlock_task();
         return p;
      }
   }
   return NULL;
}

void marcel_free(void *ptr, char *file, unsigned line)
{
   if(ptr) {
      lock_task();
      __TBX_FREE((char *)ptr, file, line);
      unlock_task();
   }
}

marcel_key_destructor_t marcel_key_destructor[MAX_KEY_SPECIFIC]={NULL};
int marcel_key_present[MAX_KEY_SPECIFIC]={0};
marcel_lock_t marcel_key_lock=MARCEL_LOCK_INIT_UNLOCKED;
unsigned marcel_nb_keys=1;
static unsigned marcel_last_key=0;
/* 
 * Hummm... Should be 0, but for obscure reasons,
 * 0 is a RESERVED value. DON'T CHANGE IT !!! 
*/

int marcel_key_create(marcel_key_t *key, marcel_key_destructor_t func)
{ /* pour l'instant, le destructeur n'est pas utilise */

   lock_task();
   marcel_lock_acquire(&marcel_key_lock);
   while ((++marcel_last_key < MAX_KEY_SPECIFIC) &&
	  (marcel_key_present[marcel_last_key])) {
   }
   if(marcel_last_key == MAX_KEY_SPECIFIC) {
     /* sinon, il faudrait remettre à 0 toutes les valeurs spécifiques
	des threads existants */
      marcel_lock_release(&marcel_key_lock);
      unlock_task();
      RAISE(CONSTRAINT_ERROR);
/*        marcel_last_key=0; */
/*        while ((++marcel_last_key < MAX_KEY_SPECIFIC) && */
/*  	     (marcel_key_present[marcel_last_key])) { */
/*        } */
/*        if(new_key == MAX_KEY_SPECIFIC) { */
/*  	 marcel_lock_release(&marcel_key_lock); */
/*  	 unlock_task(); */
/*  	 RAISE(CONSTRAINT_ERROR); */
/*        } */
   }
   *key = marcel_last_key;
   marcel_nb_keys++;
   marcel_key_present[marcel_last_key]=1;
   marcel_key_destructor[marcel_last_key]=func;
   marcel_lock_release(&marcel_key_lock);
   unlock_task();
   return 0;
}

int marcel_key_delete(marcel_key_t key)
{ /* pour l'instant, le destructeur n'est pas utilise */

   lock_task();
   marcel_lock_acquire(&marcel_key_lock);
   if (marcel_key_present[key]) {
      marcel_nb_keys--;
      marcel_key_present[key]=0;
      marcel_key_destructor[key]=NULL;
   }
   marcel_lock_release(&marcel_key_lock);
   unlock_task();
   return 0;
}

/* ================== Gestion des exceptions : ================== */

int _raise(exception ex)
{
  marcel_t cur = marcel_self();

   if(ex == NULL)
      ex = cur->cur_exception ;

   if(cur->cur_excep_blk == NULL) {
      lock_task();
      fprintf(stderr, "\nUnhandled exception %s in task %ld"
	      "\nFILE : %s, LINE : %d\n",
	      ex, cur->number, cur->exfile, cur->exline);
      abort(); /* To generate a core file */
      exit(1);
   } else {
      cur->cur_exception = ex ;
      call_ST_FLUSH_WINDOWS();
      longjmp(cur->cur_excep_blk->jb, 1);
   }
   return 0;
}

/* =============== Gestion des E/S non bloquantes =============== */

int tselect(int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds)
{
#ifdef MA__ACTIVATION
  return select(width, readfds, writefds, exceptfds, NULL);
#else
  int res = 0;
  struct timeval timeout;
  fd_set rfds, wfds, efds;

   do {
      FD_ZERO(&rfds); FD_ZERO(&wfds); FD_ZERO(&efds);
      if(readfds) rfds = *readfds;
      if(writefds) wfds = *writefds;
      if(exceptfds) efds = *exceptfds;

      timerclear(&timeout);
      res = select(width, &rfds, &wfds, &efds, &timeout);
      if(res <= 0) {
#ifdef MINIMAL_PREEMPTION
	marcel_givehandback();
#else
	marcel_trueyield();
#endif
      }
   } while(res <= 0);

   if(readfds) *readfds = rfds;
   if(writefds) *writefds = wfds;
   if(exceptfds) *exceptfds = efds;

   return res;
#endif /* MA__ACTIVATION */
}

#ifndef STANDARD_MAIN

extern int marcel_main(int argc, char *argv[]);

marcel_t __main_thread;

static jmp_buf __initial_main_jb;
static volatile int __main_ret;

int main(int argc, char *argv[])
{
  static int __argc;
  static char **__argv;

#ifdef MAD2
  marcel_debug_init(&argc, argv, PM2DEBUG_DO_OPT);
#else
  marcel_debug_init(&argc, argv, PM2DEBUG_DO_OPT|PM2DEBUG_CLEAROPT);
#endif
  if(!setjmp(__initial_main_jb)) {

    __main_thread = (marcel_t)((((unsigned long)get_sp() - 128) &
				~(SLOT_SIZE-1)) -
			       MAL(sizeof(task_desc)));

    mdebug("\t\t\t<main_thread is %p>\n", __main_thread);

    __argc = argc; __argv = argv;

    set_sp((unsigned long)__main_thread - TOP_STACK_FREE_AREA);

#ifdef ENABLE_STACK_JUMPING
    *((marcel_t *)((char *)__main_thread + MAL(sizeof(task_desc)) - sizeof(void *))) =
      __main_thread;
#endif

    __main_ret = marcel_main(__argc, __argv);

    longjmp(__initial_main_jb, 1);
  }

  return __main_ret;
}

#endif


#ifndef MA__ACTIVATION
/* *** Christian Perez Stuff ;-) *** */

void marcel_special_init(marcel_t *liste)
{
   *liste = NULL;
}

/* ATTENTION : lock_task doit avoir ete appele au prealable ! */
void marcel_special_P(marcel_t *liste)
{
  register marcel_t cur = marcel_self(), p;
  marcel_t next;
#ifdef SMP
  __lwp_t *cur_lwp = cur->lwp;
#endif
  
  next = marcel_radical_next_task();

  p = cur->prev;
  p->next = cur->next;
  p->next->prev = p;
  if(SCHED_DATA(cur_lwp).__first[cur->prio] == cur) {
    SCHED_DATA(cur_lwp).__first[cur->prio] = 
      ((cur->next->prio == cur->prio) ? cur->next : NULL);
    if(SCHED_DATA(cur_lwp).__first[0] == cur)
      SCHED_DATA(cur_lwp).__first[0] = cur->next;
  }

  if(*liste == NULL) {
    *liste = cur;
    cur->next = cur->prev = cur;
  } else {
    cur->next = *liste;
    cur->prev = (*liste)->prev;
    cur->prev->next = cur->next->prev = cur;
  }

  cur->quantums = cur->prio;
  SET_BLOCKED(cur);
  if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
    MA_THR_RESTARTED(cur, "Preemption");
    unlock_task();
  } else {
    goto_next_task(next);
  }
}

void marcel_special_V(marcel_t *pliste)
{
  register marcel_t liste = *pliste;
  register unsigned i;
  marcel_t p, queue = liste->prev;
#ifdef SMP
  __lwp_t *cur_lwp = marcel_self()->lwp;
#endif

   lock_task();

   /* on regarde la priorité du premier   */
   /* (tous doivent etre de meme priorite */
   if(liste->prio == MAX_PRIO)
      i = 0;
   else
      for(i=liste->prio; SCHED_DATA(cur_lwp).__first[i] == NULL; i--) ;
   p = SCHED_DATA(cur_lwp).__first[i];
   liste->prev = p->prev;
   queue->next = p;
   p->prev = queue;
   liste->prev->next = liste;
   SCHED_DATA(cur_lwp).__first[liste->prio] = liste;
   if(p == SCHED_DATA(cur_lwp).__first[0])
      SCHED_DATA(cur_lwp).__first[0] = liste;

   *pliste = 0;

   unlock_task();
}

void marcel_special_VP(marcel_sem_t *s, marcel_t *liste)
{
  cell *c;
  register marcel_t p, cur = marcel_self();
  marcel_t next;

  DEFINE_CUR_LWP( , ,);
  SET_CUR_LWP(GET_LWP(cur));

   lock_task();

   if(++(s->value) <= 0) {
     c = s->first;
     s->first = c->next;
     marcel_wake_task(c->task, &c->blocked);
   }

   next = marcel_radical_next_task();

      p = cur->prev;
      p->next = cur->next;
      p->next->prev = p;
      if(SCHED_DATA(cur_lwp).__first[cur->prio] == cur) {
         SCHED_DATA(cur_lwp).__first[cur->prio] = 
	   ((cur->next->prio == cur->prio) ? cur->next : NULL);
         if(SCHED_DATA(cur_lwp).__first[0] == cur)
	    SCHED_DATA(cur_lwp).__first[0] = cur->next;
      }

      if(*liste == NULL) {
         *liste = cur;
         cur->next = cur->prev = cur;
      } else {
         cur->next = *liste;
         cur->prev = (*liste)->prev;
         cur->prev->next = (*liste)->prev = cur;
      }

      cur->quantums = cur->prio;
      SET_BLOCKED(cur);
      if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
	MA_THR_RESTARTED(cur, "Preemption");
         unlock_task();
      } else {
	 goto_next_task(next);
      }
}

/* End of Christian Perez Stuff */

#endif /* MA__ACTIVATION */
