
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

#ifdef MA__LWPS
static MA_DEFINE_NOTIFIER_CHAIN(lwp_chain, "LWP");

/* Need to know about LWPs going up/down? */
int ma_register_lwp_notifier(struct ma_notifier_block *nb)
{
        return ma_notifier_chain_register(&lwp_chain, nb);
}

void ma_unregister_lwp_notifier(struct ma_notifier_block *nb)
{
        ma_notifier_chain_unregister(&lwp_chain,nb);
}

static int ma_call_lwp_notifier(unsigned long val, ma_lwp_t lwp)
{
	return ma_notifier_call_chain(&lwp_chain, val, (void*)lwp);
}

#else

#define ma_call_lwp_notifier(val, lwp) (MA_NOTIFY_DONE)

#endif

LIST_HEAD(list_lwp_head);

// Verrou protégeant la liste chaînée des LWPs
ma_rwlock_t __lwp_list_lock = MA_RW_LOCK_UNLOCKED;

marcel_lwp_t* addr_lwp[MA_NR_LWPS]={&__main_lwp,};

//#endif

unsigned marcel_nbvps(void)
{
  return get_nb_lwps();
}

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Gestion des LWPs                                               */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

unsigned  ma__nb_lwp = 1;

#if defined(MA__LWPS)

void marcel_lwp_fix_nb_vps(unsigned nb_lwp)
{
	ma_set_nbprocessors();

	// Choix du nombre de LWP
#ifdef MA__ACTSMP
	set_nb_lwps(nb_lwp ? nb_lwp : ACT_NB_MAX_CPU);
#else
	set_nb_lwps(nb_lwp ? nb_lwp : marcel_nbprocessors);
#endif

	ma_set_processors();
}

#endif /* MA__LWPS */

/* Fonction exécutée dans le thread run_task au démarrage d'un LWP
 * quelque soit ce LWP (kthread ou activation), sauf le premier.
 */
void marcel_lwp_start(marcel_lwp_t *lwp)
{
	int ret;
	LOG_IN();

	MA_BUG_ON(!ma_in_irq());

	ret = ma_call_lwp_notifier(MA_LWP_ONLINE, lwp);
        if (ret == MA_NOTIFY_BAD) {
                pm2debug("%s: attempt to bring up LWP %p failed\n",
                                __FUNCTION__, lwp);
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
        }

	__ma_get_lwp_var(online)=1;

	MA_BUG_ON(!preemption_enabled());
}

/* Fonction du thread run_task pour le démarrage d'un LWP
 * 
 * Dans ce cas des activations, on appelle directement
 * marcel_lwp_start depuis un upcall
 */
static void* lwp_start_func(void* arg)
{
	marcel_lwp_start(LWP_SELF);
	marcel_exit_special(0);
	return NULL; /* For gcc */
}

#ifdef MA__LWPS
#ifdef MA__SMP
/* Fonction exécutée par les kthreads lancées
 *
 * La pile de run_task est utilisée pour l'exécution de ce code
 */
static void *lwp_kthread_start_func(void *arg)
{
	marcel_lwp_t *lwp = (marcel_lwp_t *)arg;

	LOG_IN();

	/* Le unlock du changement de contexte */
	ma_preempt_enable();

	mdebug("\t\t\t<LWP %d started (self == %lx)>\n",
	       ma_per_lwp(number,lwp), (unsigned long)marcel_kthread_self());

	marcel_lwp_start(lwp);

	marcel_sem_P(&lwp->kthread_stop);

	mdebug("\t\t\t<LWP %d exiting>\n", ma_per_lwp(number,lwp));

	lwp_list_lock_write();
	list_del(&lwp->lwp_list);	
	lwp_list_unlock_write();

	marcel_kthread_exit(NULL);
	LOG_OUT();

	return NULL;
}
#endif /* MA__SMP */

//static int lwp_notify(struct ma_notifier_block *self, unsigned long action, void *hlwp);
//static struct ma_notifier_block lwp_nb;

unsigned marcel_lwp_add_vp(void)
{
  marcel_lwp_t *lwp;
  static ma_atomic_t nb_lwp = MA_ATOMIC_INIT(0);
  unsigned num;

  LOG_IN();

  if ((num = ma_atomic_inc_return(&nb_lwp)) >= MA_NR_LWPS)
    MARCEL_EXCEPTION_RAISE("Too many lwp\n");

  lwp = marcel_malloc_node(sizeof(*lwp), ma_lwp_node[num]);
  /* initialiser le lwp *avant* de l'enregistrer */
  *lwp = MA_LWP_INITIALIZER(lwp);

  SET_LWP_NB(num, lwp);

  // Initialisation de la structure marcel_lwp_t
  ma_call_lwp_notifier(MA_LWP_UP_PREPARE, lwp);

  lwp_list_lock_write();
  {
    // Ajout dans la liste globale des LWP
    list_add_tail(&lwp->lwp_list,&list_lwp_head);
  }
  lwp_list_unlock_write();

#ifdef MA__SMP
  // Lancement du thread noyau "propulseur". Il faut désactiver les
  // signaux 'SIGALRM' pour que le kthread 'fils' hérite d'un masque
  // correct.
  marcel_sig_disable_interrupts();
  marcel_kthread_create(&lwp->pid, 
			ma_per_lwp(run_task,lwp)?(void*)THREAD_GETMEM(ma_per_lwp(run_task,lwp), initial_sp):NULL,
			ma_per_lwp(run_task,lwp)?THREAD_GETMEM(ma_per_lwp(run_task,lwp), stack_base):0,
			lwp_kthread_start_func, (void *)lwp);
  marcel_sig_enable_interrupts();
#endif
  return ma_per_lwp(number,lwp);

  LOG_OUT();
}

#endif // MA__LWPS

#ifdef MA__SMP
/* TODO: the stack of the lwp->sched_task is currently *NOT FREED* as
   run_task, and other system threads.

   This should be fixed! 
*/
void marcel_lwp_stop_lwp(marcel_lwp_t *lwp)
{
  LOG_IN();

  if (IS_FIRST_LWP(lwp)) {
	  pm2debug("Arghh, trying to kill main_lwp\n");
	  return;
  }
  marcel_sem_V(&lwp->kthread_stop);

  marcel_kthread_join(&lwp->pid);

  {
	  /* La structure devrait être libérée ainsi que
	   * les piles des threads résidents... 
	   */
	  marcel_free_node(lwp, sizeof(marcel_lwp_t),
			  ma_lwp_node[LWP_NUMBER(lwp)]);
  }

  LOG_OUT();
}
#endif // MA__SMP

/* Initialisation d'une structure LWP */
/* On crée deux threads : run_task et idle_task
  
   - run_task sera la tâche ordonnancée en premier sur ce LWP. Il
     créera d'autres threads si nécessaire (postexit, ...) puis
     disparaîtra.
 
   - idle est la tâche idle classique. Elle est créée ici (ie pas sur
     le bon LWP). Peut-être faudrait-t-il déléguer sa création à
     run_task pour allouer de la mémoire propre au LWP... Pb: malloc
     est utilisé => verrou avec contention possible (dès qu'il y a
     deux LWPs). Donc bof dans run_task. Autre solution, faire un
     malloc respectant le placement du prochain LWP. (Ces pb
     n'interviennent que pour les NUMA où la mémoire n'est pas
     identique sur tous les noeuds)

 */
static void lwp_init(ma_lwp_t lwp)
{
	marcel_attr_t attr;
	char name[MARCEL_MAXNAMESIZE];

	LOG_IN();

#ifdef MA__SMP
	marcel_sem_init(&lwp->kthread_stop, 0);
#endif
 
	lwp->polling_list = NULL;

	if (IS_FIRST_LWP(lwp)) {
		ma_per_lwp(run_task, lwp)=MARCEL_SELF;

		lwp_list_lock_write();
		list_add_tail(&lwp->lwp_list,&list_lwp_head);
		lwp_list_unlock_write();

		LOG_OUT();
		return;
	}
	/************************************************/
	/* Création de la tâche de démarrage (run_task) */
	/************************************************/
	/* Cette tâche est lancée aussi tôt que possible
	 * dès que le lwp est créé.
	 * Elle exécute la fonction marcel_lwp_start
	 * - soit grâce au code fournit ici
	 *   (le thread noyau fait un marcel_longjmp sur cette tâche)
	 * - soit par un appel direct à cette fonction (ACTIVATIONS)
	 */
	marcel_attr_init(&attr);
	snprintf(name,MARCEL_MAXNAMESIZE,"run_task/%u",LWP_NUMBER(lwp));
	marcel_attr_setname(&attr,name);
	marcel_attr_setdetachstate(&attr, tbx_true);
	marcel_attr_setvpmask(&attr, MARCEL_VPMASK_ALL_BUT_VP(LWP_NUMBER(lwp)));
	marcel_attr_setflags(&attr, MA_SF_NORUN | MA_SF_RUNTASK);
	/* Elle doit prendre la main sur toute autre tâche de ce LWP */
	marcel_attr_setprio(&attr, MA_SYS_RT_PRIO);
#ifdef PM2
	{
		char *stack = __TBX_MALLOC(2*THREAD_SLOT_SIZE, __FILE__, __LINE__);
		
		marcel_attr_setstackaddr(&attr, (void*)((unsigned long)(stack + THREAD_SLOT_SIZE) & ~(THREAD_SLOT_SIZE-1)));
	}
#endif
	/* On passe lwp_start_func() comme fonction, mais seul le LWP
	 * 0 l'exécutera. Les autres LWP en SMP utiliseront ce thread
	 * pour exécuter lwp_kthread_start_func().
	 * En mode act-smp, ce thread sera aussi exécuté normalement.
	 */
	marcel_create_special(&(ma_per_lwp(run_task, lwp)), &attr, lwp_start_func, NULL);
	ma_set_task_state((ma_per_lwp(run_task, lwp)), MA_TASK_RUNNING);
	SET_LWP(ma_per_lwp(run_task, lwp), lwp);
	ma_barrier();
	MTRACE("RunTask", ma_per_lwp(run_task, lwp));

	/***************************************************/

	LOG_OUT();
}

static int lwp_start(ma_lwp_t lwp)
{
	LOG_IN();
	PROF_NEW_LWP(LWP_NUMBER(lwp), ma_per_lwp(run_task,lwp));

#if defined(MA__SMP) && defined(MA__BIND_LWP_ON_PROCESSORS)
	if (LWP_NUMBER(lwp)<get_nb_lwps()) {
		unsigned long target = ma_cpu_of_lwp_num(LWP_NUMBER(lwp));
		ma_bind_on_processor(target);
		mdebug("LWP %u bound to processor %lu\n",
				LWP_NUMBER(lwp), target);
	}
#endif
	LOG_OUT();
	return 0;
}

/* Pour prendre la main au tout début : à la déclaration aussi, on se
   dépêche de remplir le champ 'number'
*/
MA_DEFINE_LWP_NOTIFIER_START_PRIO(lwp, 300, "Initialisation",
				  lwp_init, "Création de RunTask",
				  lwp_start, "PROF_NEW_LWP et bind_on_proc");

MA_LWP_NOTIFIER_CALL_UP_PREPARE_PRIO(lwp, MA_INIT_LWP_MAIN_STRUCT,
				     MA_INIT_LWP_MAIN_STRUCT_PRIO);
MA_LWP_NOTIFIER_CALL_ONLINE(lwp, MA_INIT_LWP);

void __marcel_init marcel_lwp_finished(void)
{
#ifdef MA__LWPS
	LWP_SELF->pid=marcel_kthread_self();
#endif
	__ma_get_lwp_var(online)=1;
}

__ma_initfunc_prio(marcel_lwp_finished, MA_INIT_LWP_FINISHED, MA_INIT_LWP_FINISHED_PRIO, "Tell __main_lwp is online");

