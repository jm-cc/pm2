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

void marcel_delay(unsigned long millisecs)
{
#ifdef MA__ACTIVATION
	usleep(millisecs*1000);
#else
	LOG_IN();

	ma_set_current_state(MA_TASK_INTERRUPTIBLE);
#warning Need better convertion milliseconde/ma_jiffies
	ma_schedule_timeout(millisecs/10);

	LOG_OUT();
#endif
}

__attribute__((__section__(".ma.main.lwp"))) marcel_lwp_t __main_lwp;

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Gestion des threads                                            */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

static struct {
	unsigned nb_tasks;
	boolean main_is_waiting;
	marcel_sem_t blocked;
} _main_struct = { 0, FALSE, };

unsigned marcel_nbthreads(void)
{
   return _main_struct.nb_tasks + 1;   /* + 1 pour le main */
}

static volatile unsigned long task_number = 1;

unsigned long marcel_createdthreads(void)
{
   return task_number -1;    /* -1 pour le main */
}

// Utilise par les fonctions one_more_task, wait_all_tasks, etc.
static ma_spinlock_t __wait_lock = MA_SPIN_LOCK_UNLOCKED;

// Appele a chaque fois qu'une tache est creee (y compris par le biais
// de end_hibernation).
void marcel_one_more_task(marcel_t pid)
{
	ma_spin_lock(&__wait_lock);

	pid->number = task_number++;
	_main_struct.nb_tasks++;

	ma_spin_unlock(&__wait_lock);
}

// Appele a chaque fois qu'une tache est terminee.
void marcel_one_task_less(marcel_t pid)
{
	ma_spin_lock(&__wait_lock);

	if(((--(_main_struct.nb_tasks)) == 0) && (_main_struct.main_is_waiting)) {
		marcel_sem_V(&_main_struct.blocked);
	}

	ma_spin_unlock(&__wait_lock);
}

void marcel_sched_start(unsigned nb_lwp)
{
#ifdef MA__LWPS
	int i;
#endif
	LOG_IN();


#ifdef MA__LWPS
	marcel_lwp_fix_nb_vps(nb_lwp);

	for(i=1; i<get_nb_lwps(); i++)
		marcel_lwp_add_vp();
	
	mdebug("marcel_sched_init  : %i lwps created\n",
	       get_nb_lwps());
#endif /* MA__LWPS */
	
	mdebug("marcel_sched_init done\n");
	
	LOG_OUT();
}

// Attend que toutes les taches soient terminees. Cette fonction
// _doit_ etre appelee par la tache "main".
static void wait_all_tasks_end(void)
{
	LOG_IN();

	lock_task();

#ifdef MA__DEBUG
	if (marcel_self() != __main_thread) {
		RAISE(PROGRAM_ERROR);
	}
#endif 

	ma_spin_lock(&__wait_lock);
	
	if(_main_struct.nb_tasks) {
		_main_struct.main_is_waiting = TRUE;
		ma_spin_unlock(&__wait_lock);
		unlock_task();
		marcel_sem_P(&_main_struct.blocked);
	} else {
		ma_spin_unlock(&__wait_lock);
		unlock_task();
	}

	LOG_OUT();
}

void marcel_sched_shutdown()
{
#ifdef MA__SMP
	marcel_lwp_t *lwp, *lwp_found;
#endif

	LOG_IN();
	
	wait_all_tasks_end();

	// Si nécessaire, on bascule sur le LWP(0)
	marcel_change_vpmask(MARCEL_VPMASK_ALL_BUT_VP(0));

#ifdef MA__TIMER
	marcel_sig_exit();
#endif
#ifdef MA__SMP


	if(GET_LWP(marcel_self()) != &__main_lwp)
		RAISE(PROGRAM_ERROR);

	//lwp = next_lwp(&__main_lwp);

	lock_task();
	for(;;) {
		lwp_found=NULL;
		lwp_list_lock_read();
		for_all_lwp(lwp) {
			if (lwp != GET_LWP(marcel_self())) {
				lwp_found=lwp;
				break;
			}
		}
		lwp_list_unlock_read();
		unlock_task();

		if (!lwp_found) {
			break;
		}
		marcel_lwp_stop_lwp(lwp_found);
		lock_task();
	}
#elif defined(MA__ACTIVATION)
	// TODO : arrêter les autres activations...

	//act_cntl(ACT_CNTL_UPCALLS, (void*)ACT_DISABLE_UPCALLS);
#else
	/* Destroy master-sched's stack */
	//marcel_cancel(__main_lwp.sched.idle_task);
#ifdef PM2
	/* __sched_task is detached, so we can free its stack now */
	__TBX_FREE(marcel_stackbase(__main_lwp.sched.idle_task), __FILE__, __LINE__);
#endif
#endif

	LOG_OUT();
}

static any_t __attribute__((noreturn)) idle_func(any_t hlwp)
{
	if (hlwp == NULL) {
		/* upcall_new_task est venue ici ? */
		RAISE(PROGRAM_ERROR);
	}
	for(;;) {
		marcel_yield();
	}
}

MA_DEFINE_PER_LWP(marcel_task_t *,idle_task)=NULL;
#ifdef MA__ACTIVATION
MA_DEFINE_PER_LWP(marcel_task_t *,upcall_new_task)=NULL;
#endif

static void marcel_sched_lwp_init(marcel_lwp_t* lwp)
{
	marcel_attr_t attr;
	char name[MARCEL_MAXNAMESIZE];
	LOG_IN();

	if (!IS_FIRST_LWP(lwp)) {
		/* run_task DOIT démarrer en contexte d'irq */
		ma_per_lwp(run_task, lwp)->preempt_count=MA_HARDIRQ_OFFSET+MA_PREEMPT_OFFSET;
	}

	/*****************************************/
	/* Création de la tâche Idle (idle_task) */
	/*****************************************/
	marcel_attr_init(&attr);
	snprintf(name,MARCEL_MAXNAMESIZE,"idle/%u",LWP_NUMBER(lwp));
	marcel_attr_setname(&attr,name);
	marcel_attr_setdetachstate(&attr, TRUE);
	// Il vaut mieux généraliser l'utilisation des 'vpmask'
	marcel_attr_setvpmask(&attr, 
		MARCEL_VPMASK_FULL);
	     //MARCEL_VPMASK_ALL_BUT_VP(LWP_NUMBER(lwp)));
	marcel_attr_setflags(&attr, MA_SF_POLL | /*MA_SF_NOSCHEDLOCK |*/
			     MA_SF_NORUN);
#ifdef PM2
	{
		char *stack = __TBX_MALLOC(2*THREAD_SLOT_SIZE, __FILE__, __LINE__);
		
		unsigned long stsize = (((unsigned long)(stack + 2*THREAD_SLOT_SIZE) & 
					 ~(THREAD_SLOT_SIZE-1)) - (unsigned long)stack);
		
		marcel_attr_setstackaddr(&attr, stack);
		marcel_attr_setstacksize(&attr, stsize);
	}
#endif
	marcel_create_special(&(ma_per_lwp(idle_task, lwp)),
			      &attr, idle_func, (void*)(ma_lwp_t)lwp);
	MTRACE("IdleTask", ma_per_lwp(idle_task, lwp));

#ifdef MA__ACTIVATION
  /****************************************************/
  /* Création de la tâche pour les upcalls upcall_new */
  /****************************************************/
	marcel_attr_init(&attr);
	snprintf(name,MARCEL_MAXNAMESIZE,"upcalld/%u",LWP_NUMBER(lwp));
	marcel_attr_setname(&attr,name);
	marcel_attr_setdetachstate(&attr, TRUE);
	marcel_attr_setvpmask(&attr, MARCEL_VPMASK_FULL
			/*MARCEL_VPMASK_ALL_BUT_VP(lwp->number)*/);
	marcel_attr_setflags(&attr, MA_SF_UPCALL_NEW | MA_SF_NORUN);
#ifdef PM2
	{
		char *stack = __TBX_MALLOC(2*THREAD_SLOT_SIZE, __FILE__, __LINE__);

		unsigned long stsize = (((unsigned long)(stack + 2*THREAD_SLOT_SIZE) & 
					 ~(THREAD_SLOT_SIZE-1)) - (unsigned long)stack);

		marcel_attr_setstackaddr(&attr, stack);
		marcel_attr_setstacksize(&attr, stsize);
	}
#endif

	// la fonction ne sera jamais exécutée, c'est juste pour avoir une
	// structure de thread marcel dans upcall_new
	marcel_create_special(&(ma_per_lwp(upcall_new_task, lwp)),
			      &attr, (void*)idle_func, NULL);
	
	MTRACE("Upcall_Task", ma_per_lwp(upcall_new_task, lwp));
	
	/****************************************************/
#endif
	LOG_OUT();
}


static void marcel_sched_lwp_start(ma_lwp_t lwp)
{
	LOG_IN();

	MA_BUG_ON(!ma_in_irq());

	ma_wake_up_created_thread(ma_per_lwp(idle_task,lwp));

	ma_irq_exit();
	MA_BUG_ON(ma_in_atomic());
	ma_preempt_count()=0;
	ma_barrier();

	LOG_OUT();
}

static int sched_lwp_notify(struct ma_notifier_block *self, 
				   unsigned long action, void *hlwp)
{
	ma_lwp_t lwp = (ma_lwp_t)hlwp;
	switch(action) {
	case MA_LWP_UP_PREPARE:
		marcel_sched_lwp_init(lwp);
		break;
	case MA_LWP_ONLINE:
		marcel_sched_lwp_start(lwp);
		break;
	default:
		break;
	}
	return 0;
}

static struct ma_notifier_block generic_sched_nb = {
	.notifier_call	= sched_lwp_notify,
	.next		= NULL,
	.priority       = 100, /* Pour authoriser la préemption */
};

void __init marcel_gensched_init_preempt(void)
{
	sched_lwp_notify(&generic_sched_nb, (unsigned long)MA_LWP_ONLINE,
			   (void *)(ma_lwp_t)LWP_SELF);
	ma_register_lwp_notifier(&generic_sched_nb);
}

void __init marcel_gensched_init_idle(void)
{
	marcel_sem_init(&_main_struct.blocked,0);
	
	sched_lwp_notify(&generic_sched_nb, (unsigned long)MA_LWP_UP_PREPARE,
			   (void *)(ma_lwp_t)LWP_SELF);
}

__ma_initfunc_prio(marcel_gensched_init_preempt, MA_INIT_GENSCHED_PREEMPT,
		   MA_INIT_GENSCHED_PREEMPT_PRIO,
		   "Démarre la préemtion pour les LWPs");

__ma_initfunc(marcel_gensched_init_idle, MA_INIT_GENSCHED_IDLE,
	       "Création Idle");

