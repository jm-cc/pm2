
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

#define _GNU_SOURCE /* pour sched_setaffinity sous linux */
#include "marcel.h"
#ifdef MA__SMP
#ifdef LINUX_SYS
#include <sched.h>
#ifndef CPU_SET
	/* libc doesn't have support for sched_setaffinity,
	 * build system call ourselves: */
#include <linux/unistd.h>
#ifndef __NR_sched_setaffinity
	/* not even syscall number... */
#if defined(X86_ARCH)
#define __NR_sched_setaffinity 241
#elif defined(IA64_ARCH)
#define __NR_sched_setaffinity 1231
#else
#error "don't know the syscall number for sched_setaffinity on this architecture"
#endif
	/* declare system call */
_syscall3(int, sched_setaffinity, pid_t, pid, unsigned int, lg,
		unsigned long *, mask);
#endif
#endif
#endif
#endif

#ifdef MA__LWPS
MA_DEFINE_PER_LWP(unsigned, number)=0;
#endif
MA_DEFINE_PER_LWP(int, online)=0;

#ifdef MA__LWPS
static struct ma_notifier_block *lwp_chain = NULL;

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

#if defined(MA__LWPS)

unsigned  ma__nb_lwp;
static unsigned __nb_processors = 1;

void marcel_lwp_fix_nb_vps(unsigned nb_lwp)
{
	// Détermination du nombre de processeurs disponibles
#ifdef SOLARIS_SYS
	__nb_processors = sysconf(_SC_NPROCESSORS_CONF);
#elif defined(LINUX_SYS)
	__nb_processors = sysconf(_SC_NPROCESSORS_CONF);
  //__nb_processors = 2;
#elif defined(IRIX_SYS)
	__nb_processors = sysconf(_SC_NPROC_CONF);
#elif defined(OSF_SYS)
	__nb_processors = sysconf(_SC_NPROCESSORS_CONF);
#else
#warning __nb_processors set to 1 for this system
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
		RAISE(PROGRAM_ERROR);
        }

	ma_per_lwp(online,lwp)=1;

	MA_BUG_ON(!preemption_enabled());
}

/* Fonction du thread run_task pour le démarrage d'un LWP
 *
 * Dans ce cas des activations, on appelle directement
 * marcel_lwp_start depuis un upcall
 */
static void* lwp_start_func(void* arg)
{
	marcel_lwp_start(GET_LWP(marcel_self()));
	marcel_exit_special(0);
	return NULL; /* For gcc */
}

MA_DEFINE_PER_LWP(marcel_task_t *, run_task)=NULL;

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
	unlock_task();

	mdebug("\t\t\t<LWP %d started (self == %lx)>\n",
	       ma_per_lwp(number,lwp), (unsigned long)marcel_kthread_self());

	marcel_lwp_start(GET_LWP(marcel_self()));

	marcel_sem_P(&lwp->kthread_stop);

	mdebug("\t\t\t<LWP %d exiting>\n", ma_per_lwp(number,lwp));

	lock_task();
	lwp_list_lock_write();
	list_del(&lwp->lwp_list);	
	lwp_list_unlock_write();
	unlock_task();

	marcel_kthread_exit(NULL);
	LOG_OUT();

	return NULL;
}
#endif /* MA__SMP */

static int lwp_notify(struct ma_notifier_block *self, unsigned long action, void *hlwp);
static struct ma_notifier_block lwp_nb;

unsigned marcel_lwp_add_vp(void)
{
  marcel_lwp_t *lwp = (marcel_lwp_t *)__TBX_MALLOC(sizeof(marcel_lwp_t) + __ma_per_lwp_size, __FILE__, __LINE__),
          *cur_lwp = GET_LWP(marcel_self());

  LOG_IN();

  // Initialisation de la structure marcel_lwp_t
  ma_call_lwp_notifier(MA_LWP_UP_PREPARE, lwp);

  lwp_list_lock_write();
  {
    // Ajout dans la liste globale des LWP
    list_add_tail(&lwp->lwp_list, &cur_lwp->lwp_list);
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

  marcel_kthread_join(lwp->pid);

  {
	  /* La structure devrait être libérée ainsi que
	   * les piles des threads résidents... 
	   */
		  __TBX_FREE(lwp, __FILE__, __LINE__);
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
	static unsigned __nb_lwp = 0;
	marcel_attr_t attr;
	char name[MARCEL_MAXNAMESIZE];

	LOG_IN();

	memset(lwp,0,sizeof(marcel_lwp_t) + __ma_per_lwp_size);

#ifdef MA__SMP
	marcel_sem_init(&lwp->kthread_stop, 0);
#endif

	// ATTENTION: la tentation est forte d'initialiser _locked à 2
	// pour éviter le 'lock_task' juste après. Erreur: cela ne
	// serait valable que pour la création du _premier_ lwp car
	// ensuite, cur_lwp != lwp...

	// Le lwp est créé dans l'état locked : tant que le scheduler
	// ne le remet pas dans l'état not locked, il n'y aura pas de
	// changement de contexte non voulu.
	//atomic_set(&lwp->_locked, 1);

	// A cet endroit, on peut appeler 'lock_task'. Pour le premier
	// LWP, on passera à 2, pour les autres, on passera à 1 sur le
	// LWP courant.
	//lock_task();

	lwp_list_lock_write();
	{
		if(__nb_lwp >= MA_NR_LWPS)
			RAISE("Too many lwp\n");
		
		SET_LWP_NB(__nb_lwp, lwp);
		// Attribution du numéro protégé par le lwp_list_lock_write()
		__nb_lwp++;
	}
	lwp_list_unlock_write();

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
	marcel_attr_setdetachstate(&attr, TRUE);
	marcel_attr_setvpmask(&attr, MARCEL_VPMASK_ALL_BUT_VP(LWP_NUMBER(lwp)));
	marcel_attr_setflags(&attr, MA_SF_NORUN | MA_SF_RUNTASK);
#ifdef PM2
	{
		char *stack = __TBX_MALLOC(2*THREAD_SLOT_SIZE, __FILE__, __LINE__);
		
		unsigned long stsize = (((unsigned long)(stack + 2*THREAD_SLOT_SIZE) & 
					 ~(THREAD_SLOT_SIZE-1)) - (unsigned long)stack);
		
		marcel_attr_setstackaddr(&attr, stack);
		marcel_attr_setstacksize(&attr, stsize);
	}
#endif
	marcel_create_special(&(ma_per_lwp(run_task, lwp)), &attr, lwp_start_func, NULL);
	SET_LWP(ma_per_lwp(run_task, lwp), lwp);
	ma_barrier();
	MTRACE("RunTask", ma_per_lwp(run_task, lwp));

	/***************************************************/

	LOG_OUT();
}

#ifdef MA__SMP

inline static void bind_on_processor(marcel_lwp_t *lwp)
{
#if defined(MA__BIND_LWP_ON_PROCESSORS)
	unsigned long target = LWP_NUMBER(lwp) % __nb_processors;
#if defined(SOLARIS_SYS)
	if(processor_bind(P_LWPID, P_MYID,
			  (processorid_t)(target),
			  NULL) != 0) {
		perror("processor_bind");
		exit(1);
	}
#elif defined(LINUX_SYS)

#ifndef CPU_SET
	/* no libc support, use direct system call */
	unsigned long mask = 1UL<<target;

	if (sched_setaffinity(0,sizeof(target),&target)<0) {
		perror("sched_setaffinity");
		exit(1);
	}
#else
	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(target, &mask);
	if(sched_setaffinity(0,&mask)<0) {
		perror("sched_setaffinity");
		exit(1);
	}
#endif
#else
#error "don't know how to bind on processors on this system"
#endif
	mdebug("LWP %u bound to processor %lu\n",
			LWP_NUMBER(lwp), target);
#endif
}

inline static void marcel_lwp_bind_on_processor(marcel_lwp_t *lwp) {
	return bind_on_processor(lwp);
}

#endif /* MA__SMP */

static int lwp_start(ma_lwp_t lwp)
{
	LOG_IN();
	PROF_NEW_LWP(LWP_NUMBER(lwp), ma_per_lwp(run_task,lwp)->number);

#ifdef MA__SMP
	bind_on_processor(lwp);
#endif
	LOG_OUT();
	return 0;
}

static int lwp_notify(struct ma_notifier_block *self, 
		      unsigned long action, void *hlwp)
{
	ma_lwp_t lwp = (ma_lwp_t)hlwp;
	switch(action) {
	case MA_LWP_UP_PREPARE:
		lwp_init(lwp);
		break;
	case MA_LWP_ONLINE:
		lwp_start(lwp);
		break;
	default:
		break;
	}
	return 0;
}

static struct ma_notifier_block lwp_nb = {
	.notifier_call	= lwp_notify,
	.next		= NULL,
	.priority       = 300, /* Pour prendre la main au tout début 
				  A la déclaration aussi, on se dépêche
				  de remplir le champ 'number'
				*/
};

void __init marcel_lwp_finished(void)
{
	ma_per_lwp(online,(ma_lwp_t)LWP_SELF)=1;
}

__ma_initfunc_prio(marcel_lwp_finished, MA_INIT_LWP_FINISHED,
		   MA_INIT_LWP_FINISHED_PRIO, "Tell __main_lwp is online");

void __init marcel_lwp_decl_bind(void)
{
	lwp_notify(&lwp_nb, (unsigned long)MA_LWP_ONLINE,
		   (void *)(ma_lwp_t)LWP_SELF);
	ma_register_lwp_notifier(&lwp_nb);
}

__ma_initfunc(marcel_lwp_decl_bind, MA_INIT_LWP,
	       "Declare (and bind) lwp");

void __init marcel_lwp_init(void)
{
	lwp_notify(&lwp_nb, (unsigned long)MA_LWP_UP_PREPARE,
		   (void *)(ma_lwp_t)LWP_SELF);
}

__ma_initfunc_prio(marcel_lwp_init, MA_INIT_LWP_MAIN_STRUCT,
		   MA_INIT_LWP_MAIN_STRUCT_PRIO, "Initialise __main_lwp");

