
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

#include "marcel.h"
#include "marcel_timing.h"
#include "marcel_alloc.h"
#include "sys/marcel_work.h"

#include "pm2_common.h"
#include "tbx.h"
#include "pm2_profile.h"

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
#include "sys/marcel_upcalls.h"
#endif

#ifdef UNICOS_SYS
#include <sys/mman.h>
#endif


/*
 * Begin: added by O.A.
 * --------------------
 */
static tbx_flag_t marcel_activity = tbx_flag_clear;

_PRIVATE_
int
marcel_test_activity(void)
{
  tbx_bool_t result = tbx_false;

  LOG_IN();
  result = tbx_flag_test(&marcel_activity);
  LOG_OUT();

  return result;
}

_PRIVATE_
void
marcel_set_activity(void)
{
  LOG_IN();
  if (tbx_flag_test(&marcel_activity))
    FAILURE("marcel_activity flag already set");
  
  tbx_flag_set(&marcel_activity);
  LOG_OUT();
}

_PRIVATE_
void
marcel_clear_activity(void)
{
  LOG_IN();
  if (!tbx_flag_test(&marcel_activity))
    FAILURE("marcel_activity flag already cleared");

  tbx_flag_clear(&marcel_activity);
  LOG_OUT();
}

/*
 * End: added by O.A.
 * ------------------
 */


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
#warning set_sp() should not be directly used
  set_sp(SP_FIELD(buf)-256);
  longjmp(_buf, _val);
}
#define longjmp(buf, v)	LONGJMP(buf, v)
#endif

static long page_size;

#define PA(X) ((((long)X)+(page_size-1)) & ~(page_size-1))

volatile boolean always_false = FALSE;

marcel_exception_t
   TASKING_ERROR = "TASKING_ERROR: A non-handled exception occurred in a task",
   DEADLOCK_ERROR = "DEADLOCK_ERROR: Global Blocking Situation Detected",
   STORAGE_ERROR = "STORAGE_ERROR: No space left on the heap",
   CONSTRAINT_ERROR = "CONSTRAINT_ERROR",
   PROGRAM_ERROR = "PROGRAM_ERROR",
   STACK_ERROR = "STACK_ERROR: Stack Overflow",
   TIME_OUT = "TIME OUT while being blocked on a semaphor",
   NOT_IMPLEMENTED = "NOT_IMPLEMENTED (sorry)",
   USE_ERROR = "USE_ERROR: Marcel was not compiled to enable this functionality",
   LOCK_TASK_ERROR = "LOCK_TASK_ERROR: All tasks blocked after bad use of lock_task()";

volatile unsigned long threads_created_in_cache = 0;

/* C'EST ICI QU'IL EST PRATIQUE DE METTRE UN POINT D'ARRET
   LORSQUE L'ON VEUT EXECUTER PAS A PAS... */
void breakpoint()
{
  /* Lieu ideal ;-) pour un point d'arret */
}

/* =========== specifs =========== */
int marcel_cancel(marcel_t pid);

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

void print_thread(marcel_t pid)
{
  long sp;

   if(pid == marcel_self()) {
     sp = (long)get_sp();
   } else {
     sp = (long)marcel_ctx_get_sp(pid->ctx_yield);
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

#ifdef IA64_ARCH
#  define SHOW_SP ""
#else
#  define SHOW_SP (((char **)buf + i) == (char **)&SP_FIELD(buf) ? "(sp)":"")
#endif
  for(i=0; i<sizeof(jmp_buf)/sizeof(char *); i++) {
    mdebug("%s[%d] = %p %s\n",
	   name,
	   i,
	   ((char **)buf)[i],
	   SHOW_SP
	  );
  }
}

#endif

static __inline__ void init_task_desc(marcel_t t)
{
  t->cur_excep_blk = NULL;
  t->deviate_work = NULL;
  t->state_lock = MARCEL_LOCK_INIT;
#if defined(MA__LWPS) && ! defined(MA__ONE_QUEUE)
  t->previous_lwp = NULL;
#endif
  t->next_cleanup_func = 0;
#ifdef ENABLE_STACK_JUMPING
  *((marcel_t *)((char *)t + MAL(sizeof(task_desc)) - sizeof(void *))) = t;
#endif
  SET_STATE_READY(t);
  t->special_flags = 0;
  marcel_sem_init(&t->suspend_sem, 0);
  t->p_readlock_list=NULL;
  t->p_readlock_free=NULL;
  t->p_untracked_readlock_count=0;
  t->p_nextwaiting=NULL;
  marcel_sem_init(&t->pthread_sync, 0);  

}

int marcel_create(marcel_t *pid, __const marcel_attr_t *attr, marcel_func_t func, any_t arg)
{
  marcel_t cur = marcel_self(), new_task;
#ifdef MA__POSIX_BEHAVIOUR
  marcel_attr_t myattr;
#endif

  LOG_IN();

  TIMING_EVENT("marcel_create");

  if(!attr) {
#ifdef MA__POSIX_BEHAVIOUR
    marcel_attr_init(&myattr);
    attr=&myattr;
#else
    attr = &marcel_attr_default;
#endif
  }

  lock_task();

  if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
    MA_THR_RESTARTED(cur, "Father Preemption");
    if(cur->child) {
      // On ins�re le fils un peu tardivement
      marcel_insert_task(cur->child);
    }
    unlock_task();
    LOG_OUT();
    return 0;
  } else {
    if(attr->__stackaddr_set) {
      register unsigned long top = MAL_BOT((long)attr->__stackaddr +
					   attr->__stacksize);
#ifdef MA__DEBUG
      mdebug("top=%lx, stack_base=%p\n", top, attr->__stackaddr);
      if(top & (THREAD_SLOT_SIZE-1)) { /* Not slot-aligned */
	unlock_task();
	RAISE(CONSTRAINT_ERROR);
      }
#endif
      new_task = (marcel_t)(top - MAL(sizeof(task_desc)));
#ifdef STACK_CHECKING_ALLOWED
      memset(attr->__stackaddr, 0, attr->__stacksize);
#endif
      init_task_desc(new_task);
      new_task->stack_base = attr->__stackaddr;

      new_task->static_stack = TRUE;
    } else { /* (!attr->stack_base) */
      char *bottom;

#ifdef MA__DEBUG
      if(attr->__stacksize > THREAD_SLOT_SIZE)
	RAISE(NOT_IMPLEMENTED);
#endif
      bottom = marcel_slot_alloc();

      PROF_EVENT(thread_stack_allocated);

      new_task = (marcel_t)(MAL_BOT((long)bottom + THREAD_SLOT_SIZE) -
			    MAL(sizeof(task_desc)));
      init_task_desc(new_task);
      new_task->stack_base = bottom;
      new_task->static_stack = FALSE;
    } /* fin (attr->stack_base) */

    new_task->sched_policy = attr->__schedpolicy;
    new_task->not_migratable = attr->not_migratable;
    new_task->not_deviatable = attr->not_deviatable;
    new_task->detached = (attr->__detachstate == MARCEL_CREATE_DETACHED);
    new_task->vpmask = attr->vpmask;
    new_task->special_flags = attr->flags;

    if(attr->rt_thread)
      new_task->special_flags |= MA_SF_RT_THREAD;

    if(!attr->__detachstate) {
      marcel_sem_init(&new_task->client, 0);
      marcel_sem_init(&new_task->thread, 0);
    }

    if(attr->user_space)
      new_task->user_space_ptr = (char *)new_task - MAL(attr->user_space);
    else
      new_task->user_space_ptr = NULL;

    new_task->father = cur;
    SET_LWP(new_task, GET_LWP(cur)); /* In case timer_interrupt is
				 called before insert_task ! */
    new_task->f_to_call = func;
    new_task->arg = arg;
    new_task->initial_sp = (long)new_task - MAL(attr->user_space) -
      TOP_STACK_FREE_AREA;

    if(pid)
      *pid = new_task;

    marcel_one_more_task(new_task);
    MTRACE("Creation", new_task);

    PROF_THREAD_BIRTH(new_task->number);

    if(
          // On ne peut pas c�der la main maintenant
          (locked() > 1)
	  // Le thread ne peut pas d�marrer imm�diatement
       || (new_task->user_space_ptr && !attr->immediate_activation)
#ifdef MA__LWPS
	  // On ne peut pas placer ce thread sur le LWP courant
       || (marcel_vpmask_vp_ismember(new_task->vpmask,
				     GET_LWP(cur)->number))
#ifndef MA__ONE_QUEUE
	  // Si la politique est du type 'placer sur le LWP le moins
	  // charg�', alors on ne peut pas placer ce thread sur le LWP
	  // courant
       || (new_task->sched_policy != MARCEL_SCHED_OTHER)
#endif
#endif
       ) {
      // Ici, le p�re _ne_doit_pas_ donner la main au fils
      // imm�diatement : lock_task() a �t� appel�, ou alors le fils va
      // �tre ins�r� sur un autre LWP, ou alors "immediate_activation"
      // a �t� positionn� � FALSE. La cons�quence est que le thread
      // fils est cr�� et initialis�, mais pas "ins�r�" dans une
      // quelconque file pour l'instant.

      PROF_IN_EXT(newborn_thread);

      // Il y a un cas o� il ne faut m�me pas ins�rer le fils tout de
      // suite : c'est celui ou "userspace" n'est pas 0, auquel cas le
      // p�re appelera explicitement "marcel_run" plus tard...
      new_task->father->child = ((new_task->user_space_ptr &&
				  !attr->immediate_activation)
				 ? NULL /* do not insert now */
				 : new_task); /* insert asap */

      if(IS_TASK_TYPE_IDLE(new_task))
	// Si la t�che cr��e est une t�che 'idle', alors il est
	// prudent de la marquer 'RUNNING' si l'on est en mode
	// MA__MULTIPLE_RUNNING
	SET_STATE_RUNNING_ONLY(new_task);
      else
	SET_STATE_READY(new_task);

      marcel_ctx_set_new_stack(new_task, new_task->initial_sp);

      MTRACE("Preemption", marcel_self());

      PROF_OUT_EXT(newborn_thread);

      if(MA_THR_SETJMP(marcel_self()) == FIRST_RETURN) {
	// On rend la main au p�re
	call_ST_FLUSH_WINDOWS();
	marcel_ctx_longjmp(marcel_self()->father->ctx_yield, NORMAL_RETURN);
      }
      MA_THR_RESTARTED(marcel_self(), "Preemption");
      unlock_task();

    } else {
      // Cas le plus favorable (sur le plan de l'efficacit�) : le p�re
      // sauve son contexte et on d�marre le fils imm�diatement. Note
      // : si le thread est un 'real-time thread', cela ne change
      // rien ici...

      PROF_SWITCH_TO(cur->number, new_task->number);

      PROF_IN_EXT(newborn_thread);

      new_task->father->child = NULL;

      SET_STATE_RUNNING(NULL, new_task, GET_LWP(new_task));
      marcel_insert_task(new_task);

      marcel_ctx_set_new_stack(new_task, new_task->initial_sp);
      SET_STATE_READY(marcel_self()->father);

      MTRACE("Preemption", marcel_self());

      unlock_task();

      PROF_OUT_EXT(newborn_thread);
    }

    marcel_exit((*marcel_self()->f_to_call)(marcel_self()->arg));
  }

  return 0;
}

DEF_MARCEL_POSIX(int, join, (marcel_t pid, any_t *status))
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
DEF_PTHREAD(join)

DEF_MARCEL_POSIX(void, exit, (any_t val))
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
      mdebug("  max iteration in key destructor for thread %li\n",cur->number);
#endif
  }

  lock_task();
  SET_CUR_LWP(GET_LWP(cur));

#ifdef MA__LWPS
  /* Durant cette fonction, il ne faut pas que le thread soit
     "d�plac�" intempestivement (e.g. apr�s avoir acquis stack_mutex)
     sur un autre LWP. */
  cur->vpmask = MARCEL_VPMASK_ALL_BUT_VP(cur_lwp->number);
  /* Ne pas oublier de mettre "ancien LWP a NULL", sinon la tache sera
     replacee dessus dans insert_task(). Cette optimisation est source
     de confusion et il vaudrait peut-etre mieux la supprimer... */
  cur->previous_lwp = NULL;
#endif

  cur->ret_val = val;
  unlock_task();
  if(!cur->detached) {
    marcel_sem_V(&cur->client);
    marcel_sem_P(&cur->thread);
  }

  // Dans le cas o� la pile a �t� allou�e � l'ext�rieur de Marcel
  // (typiquement par PM2/isomalloc), il faut effectuer un traitement
  // d�licat pour pouvoir appeler "marcel_terminate" quelque soit le
  // code ex�cut� par cette derni�re fonction.
  if(cur->static_stack) {

    // Il faut commencer par acqu�rir le verrou d'obtention de la pile
    // de s�curit� sur le LWP courant (il y a une pile de s�curit� par
    // LWP).
    marcel_mutex_lock(&cur_lwp->stack_mutex);

    lock_task();

    // On fabrique un "faux" thread dans la pile de secours
    // (cur_lwp->sec_desc) et ce thread devient le thread courant.
    init_task_desc(cur_lwp->sec_desc);

    // Pour les informations de debug, on reprend le num�ro du thread
    // pr�c�dent
    cur_lwp->sec_desc->number = cur->number;

    // Attention : il est n�cessaire de s'assurer que le thread "de
    // secours" ne sera pas ins�r� sur un autre LWP...
    cur_lwp->sec_desc->vpmask = MARCEL_VPMASK_ALL_BUT_VP(cur_lwp->number);

    MTRACE("Mutation", cur);

    // On insere le nouveau thread _avant_ de retirer l'ancien (pour
    // eviter le deadlock)
    SET_STATE_RUNNING(NULL, cur_lwp->sec_desc, cur_lwp);
    // TODO : DANGEROUS : marcel_insert_task modifie cur_lwp->sec_desc->lwp !
    // A priori, pas de cons�quence : c'est la m�me valeur.
    marcel_insert_task(cur_lwp->sec_desc);

    // On retire l'ancien
    SET_GHOST(cur);
    UNCHAIN_MYSELF(cur, cur_lwp->sec_desc);

    // Avant d'ex�cuter unlock_task, on stocke une r�f�rence sur
    // l'ancien thread dans une variable statique propre au LWP
    // courant de mani�re � pouvoir le d�truire par la suite...
    cur_lwp->sec_desc->private_val = (any_t)cur;
    cur_lwp->sec_desc->stack_base = (void*)SECUR_STACK_BOTTOM(cur_lwp);

    // On bascule maintenant sur la pile de secours : marcel_self()
    // devient donc �gal � cur_lwp->sec_desc...
    marcel_ctx_set_new_stack(cur_lwp->sec_desc, SECUR_STACK_TOP(cur_lwp));
#ifdef MA__LWPS
    // On recalcule "cur_lwp" car c'est une variable locale.
    cur_lwp = GET_LWP(marcel_self());
#endif

    PROF_EVENT(on_security_stack);

    unlock_task();

    // Ici, le noyau marcel n'est plus v�rouill�, on peut donc appeler
    // marcel_terminate sans risque particulier en cas d'appel
    // bloquant.
    marcel_terminate(cur_lwp->sec_desc->private_val);

    // Ok, maintenant on r�-entre en "mode noyau" avec lock_task.
    lock_task();

    // On peut alors rel�cher le mutex : aucun thread ne pourra
    // l'acqu�rir avant que l'on ex�cute unlock_task, c'est � dire
    // lorsque le goto_next_task aboutira.
    marcel_mutex_unlock(&cur_lwp->stack_mutex);

    // On averti le main qu'une tache de plus va disparaitre, ce qui
    // va provoquer insert_task(main_task) si le thread courant est le
    // dernier de l'application...  ATTENTION : cela signifie qu'il
    // faut imp�rativement ex�cuter cette instruction _avant_
    // unchain_task, faute de quoi on risquerait d'ins�rer la t�che
    // idle � tort !!!
    marcel_one_task_less(cur_lwp->sec_desc);

    // Il est maintenant temps de retirer le "faux" thread par un
    // appel � unchain_task...
    SET_GHOST(cur_lwp->sec_desc);
    cur = UNCHAIN_TASK_AND_FIND_NEXT(cur_lwp->sec_desc);

    MTRACE("Exit", cur_lwp->sec_desc);

#ifdef MA__MULTIPLE_RUNNING
    cur_lwp->prev_running=NULL;
#endif

    PROF_THREAD_DEATH(cur_lwp->sec_desc->number);

    LOG_OUT();

    MA_THR_LONGJMP(cur_lwp->sec_desc->number, cur, NORMAL_RETURN);

  } else { // Ici, la pile a �t� allou�e par le noyau Marcel

    //#ifdef PM2
    //    RAISE(PROGRAM_ERROR);
    //#endif

    // On appelle marcel_terminate sur la pile courante puisque
    // celle-ci ne sera jamais d�truite par l'une des fonction "cleanup"...
    marcel_terminate(cur);

    // Il faut acqu�rir le verrou de la pile de secours avant
    // d'ex�cuter lock_task.
    marcel_mutex_lock(&cur_lwp->stack_mutex);

    lock_task();

    // Il peut para�tre stupide de rel�cher le verrou � cet endroit
    // (on vient de le prendre !). Premi�rement, ce n'est pas grave
    // car lock_task garanti qu'aucun autre thread (sur le LWP)
    // n'utilisera la pile. Deuxi�mement, il _faut_ rel�cher ce verrou
    // tr�s t�t (avant d'ex�cuter unchain_task) car sinon la t�che
    // idle risquerait d'�tre r�veill�e (par unchain_task) alors que
    // mutex_unlock r�veillerait par ailleurs une autre t�che du
    // programme !
    marcel_mutex_unlock(&cur_lwp->stack_mutex); /* idem ci-dessus */

    // M�me remarque que pr�c�demment : main_thread peut �tre r�veill�
    // � cet endroit, donc il ne faut appeler unchain_task qu'apr�s.
    marcel_one_task_less(cur);

    // On peut se retirer de la file des threads pr�ts. le champ
    // "child" de la pile de secours d�signe maintenant le thread �
    // qui on donnera la "main" avant de dispara�tre.
    SET_GHOST(cur);
    cur_lwp->sec_desc->child = UNCHAIN_TASK_AND_FIND_NEXT(cur);

    MTRACE("Exit", cur);

    // Le champ "father" d�signe la t�che courante : cela permettra de
    // la d�truire un peu plus loin...
    cur_lwp->sec_desc->father = cur;

    // Avant de changer de pile il faut, comme toujours, positionner
    // correctement le champ lwp...
    SET_LWP(cur_lwp->sec_desc, cur_lwp);

    // Pour les informations de debug, on reprend le num�ro du thread
    // pr�c�dent
    cur_lwp->sec_desc->number = cur->number;

    // Ca y est, on peut basculer sur la pile de secours.
    marcel_ctx_set_new_stack(cur_lwp->sec_desc, SECUR_STACK_TOP(cur_lwp));

#ifdef MA__LWPS
    // On recalcule "cur_lwp" car c'est une variable locale.
    cur_lwp = GET_LWP(marcel_self());
#endif

    // On d�truit l'ancien thread
    marcel_slot_free(marcel_stackbase(cur_lwp->sec_desc->father));

#ifdef MA__MULTIPLE_RUNNING
    cur_lwp->prev_running=NULL;
#endif

    PROF_THREAD_DEATH(cur_lwp->sec_desc->number);

    LOG_OUT();

    // Enfin, on effectue un changement de contexte vers le thread suivant.
    MA_THR_LONGJMP(cur_lwp->sec_desc->number,
		   cur_lwp->sec_desc->child, NORMAL_RETURN);
  }

  abort(); // For security
}
DEF_PTHREAD(exit)

DEF_MARCEL_POSIX(int, cancel, (marcel_t pid))
{
  if(pid == marcel_self()) {
    marcel_exit(NULL);
  } else {
    pid->ret_val = NULL;
    mdebug("marcel %li kill %li\n", marcel_self()->number, pid->number);
    marcel_deviate(pid, (handler_func_t)marcel_exit, NULL);
  }
  return 0;
}
DEF_PTHREAD(cancel)

DEF_MARCEL_POSIX(int, detach, (marcel_t pid))
{
   pid->detached = TRUE;
   return 0;
}
DEF_PTHREAD(detach)

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

static void suspend_handler(any_t arg)
{
  if((long int)arg) {
    // Suspend
    marcel_sem_P(&(marcel_self()->suspend_sem));
  } else {
    // Resume
    marcel_sem_V(&(marcel_self()->suspend_sem));
  }
}

void marcel_suspend(marcel_t pid)
{
  marcel_deviate(pid, suspend_handler, (any_t)1);
}

void marcel_resume(marcel_t pid)
{
  marcel_deviate(pid, suspend_handler, (any_t)0);
}

#undef NAME_PREFIX
#define NAME_PREFIX _
DEF_MARCEL_POSIX(void, cleanup_push,(struct _marcel_cleanup_buffer *__buffer,
				     cleanup_func_t func, any_t arg))
{
  marcel_t cur = marcel_self();

   if(cur->next_cleanup_func == MAX_CLEANUP_FUNCS)
      RAISE(CONSTRAINT_ERROR);

   cur->cleanup_args[cur->next_cleanup_func] = arg;
   cur->cleanup_funcs[cur->next_cleanup_func++] = func;
}
DEF_PTHREAD(cleanup_push)

DEF_MARCEL_POSIX(void, cleanup_pop,(struct _marcel_cleanup_buffer *__buffer,
				    boolean execute))
{
  int i;
  marcel_t cur = marcel_self();

   if(!cur->next_cleanup_func)
      RAISE(CONSTRAINT_ERROR);

   i = --cur->next_cleanup_func;
   if(execute)
      (*cur->cleanup_funcs[i])(cur->cleanup_args[i]);
}
DEF_PTHREAD(cleanup_pop)
#undef NAME_PREFIX
#define NAME_PREFIX

static void __inline__ freeze(marcel_t t)
{
#ifndef MA__MONO
  RAISE(NOT_IMPLEMENTED);
#endif
   if(t != marcel_self()) {
      lock_task();
      if(IS_BLOCKED(t) || IS_FROZEN(t)) {
         unlock_task();
         RAISE(PROGRAM_ERROR);
      } else if(IS_SLEEPING(t)) {
         marcel_wake_task(t, NULL);
      }
      marcel_set_frozen(t);
      UNCHAIN_TASK(t);
      unlock_task();
   }
}

static void __inline__ unfreeze(marcel_t t)
{
#ifndef MA__MONO
  RAISE(NOT_IMPLEMENTED);
#endif
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

// TODO : V�rifier le code avec les activations
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
    if(marcel_ctx_setjmp(cur->ctx_migr) == FIRST_RETURN) {

      call_ST_FLUSH_WINDOWS();
      top = (long)cur + ALIGNED_32(sizeof(task_desc));
      bottom = ALIGNED_32((long)marcel_ctx_get_sp(cur->ctx_migr)) - ALIGNED_32(1);
      blk = top - bottom;
      depl = bottom - (long)cur->stack_base;
      unlock_task();

      mdebug("hibernation of thread %p", cur);
      mdebug("sp = %lu\n", get_sp());
      mdebug("sp_field = %u\n", (int)marcel_ctx_get_sp(cur->ctx_migr));
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
    memcpy(t->ctx_migr, t->ctx_yield, sizeof(marcel_ctx_t));

    top = (long)t + ALIGNED_32(sizeof(task_desc));
    bottom = ALIGNED_32((long)marcel_ctx_get_sp(t->ctx_yield)) - ALIGNED_32(1);
    blk = top - bottom;
    depl = bottom - (long)t->stack_base;

    mdebug("hibernation of thread %p", t);
    mdebug("sp_field = %u\n", (int)marcel_ctx_get_sp(cur->ctx_migr));
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

#ifdef STACK_CHECKING_ALLOWED
  memset(t->stack_base, 0, size);
#endif

  unlock_task();

  return t;
}

// TODO : V�rifier le code avec les activations
void marcel_end_hibernation(marcel_t t, post_migration_func_t f, void *arg)
{
#ifdef MA__ACTIVATION
  RAISE("Not implemented");
#endif

  memcpy(t->ctx_yield, t->ctx_migr, sizeof(marcel_ctx_t));

  mdebug("end of hibernation for thread %p", t);

  lock_task();

  marcel_insert_task(t);
  marcel_one_more_task(t);

  if(f != NULL)
    marcel_deviate(t, f, arg);

  unlock_task();
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
    if(!strcmp(argv[i], "--marcel-nvp")) {
      if(i == *argc-1) {
	fprintf(stderr,
		"Fatal error: --marcel-nvp option must be followed "
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

  // Windows/Cygwin specific stuff
  marcel_win_sys_init(argc, argv);

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
  marcel_sched_init();

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
  marcel_sched_start(__nb_lwp);
  marcel_set_activity();
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
  return (long)get_sp() - (long)marcel_self()->stack_base;
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
    if((p = __TBX_MALLOC(size, file, line)) == NULL) {
      fprintf(stderr, "Storage error at %s:%d\n", file, line);
      RAISE(STORAGE_ERROR);
    } else {
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

DEF_MARCEL_POSIX(int, key_create, (marcel_key_t *key, 
				   marcel_key_destructor_t func))
{ /* pour l'instant, le destructeur n'est pas utilise */

   lock_task();
   marcel_lock_acquire(&marcel_key_lock);
   while ((++marcel_last_key < MAX_KEY_SPECIFIC) &&
	  (marcel_key_present[marcel_last_key])) {
   }
   if(marcel_last_key == MAX_KEY_SPECIFIC) {
     /* sinon, il faudrait remettre � 0 toutes les valeurs sp�cifiques
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
DEF_PTHREAD(key_create)
DEF___PTHREAD(key_create)

DEF_MARCEL_POSIX(int, key_delete, (marcel_key_t key))
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
DEF_PTHREAD(key_delete)
//DEF___PTHREAD(key_delete)

DEFINLINE_MARCEL_POSIX(int, setspecific, (marcel_key_t key,
					  __const void* value))
DEF_PTHREAD(setspecific)
DEF___PTHREAD(setspecific)

DEFINLINE_MARCEL_POSIX(any_t, getspecific, (marcel_key_t key))
DEF_PTHREAD(getspecific)
DEF___PTHREAD(getspecific)


/* ================== Gestion des exceptions : ================== */

int _marcel_raise(marcel_exception_t ex)
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
      marcel_ctx_longjmp(cur->cur_excep_blk->ctx, 1);
   }
   return 0;
}

#ifndef STANDARD_MAIN

extern int marcel_main(int argc, char *argv[]);

marcel_t __main_thread;
static volatile int __main_ret;

static marcel_ctx_t __initial_main_ctx;

#ifdef WIN_SYS
void win_stack_allocate(unsigned n)
{
  int tab[n];

  tab[0] = 0;
}
#endif // WIN_SYS

#ifdef MARCEL_MAIN_AS_FUNC
int go_marcel_main(int argc, char *argv[])
#warning go_marcel_main defined
#else
int main(int argc, char *argv[])
#endif // MARCEL_MAIN_AS_FUNC
{
  static int __argc;
  static char **__argv;
  static unsigned long valsp;

#ifdef MAD2
  marcel_debug_init(&argc, argv, PM2DEBUG_DO_OPT);
#else
  marcel_debug_init(&argc, argv, PM2DEBUG_DO_OPT|PM2DEBUG_CLEAROPT);
#endif
  if(!marcel_ctx_setjmp(__initial_main_ctx)) {

    valsp = (unsigned long) get_sp();
    __main_thread = (marcel_t)(((valsp - 128) &
				~(THREAD_SLOT_SIZE-1)) -
			       MAL(sizeof(task_desc)));

    mdebug("\t\t\t<main_thread is %p>\n", __main_thread);

    __argc = argc; __argv = argv;

#ifdef WIN_SYS
      win_stack_allocate(THREAD_SLOT_SIZE);
#endif

    /* On se contente de descendre la pile. Tout va bien, m�me sur Itanium */
    set_sp((unsigned long)__main_thread - TOP_STACK_FREE_AREA);

#ifdef ENABLE_STACK_JUMPING
    *((marcel_t *)((char *)__main_thread + MAL(sizeof(task_desc)) - sizeof(void *))) =
      __main_thread;
#endif

    __main_ret = marcel_main(__argc, __argv);

    marcel_ctx_longjmp(__initial_main_ctx, 1);
  }

  return __main_ret;
}


#endif // STANDARD_MAIN
