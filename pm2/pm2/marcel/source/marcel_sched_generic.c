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
	ma_schedule_timeout(millisecs*1000/marcel_gettimeslice());

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

MA_DEFINE_PER_LWP(marcel_task_t *, previous_thread)=NULL;

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

static LIST_HEAD(all_threads);

// Appele a chaque fois qu'une tache est creee (y compris par le biais
// de end_hibernation).
void marcel_one_more_task(marcel_t pid)
{
	ma_spin_lock(&__wait_lock);

	pid->number = task_number++;
	_main_struct.nb_tasks++;
	list_add(&pid->all_threads,&all_threads);

	ma_spin_unlock(&__wait_lock);
}

// Appele a chaque fois qu'une tache est terminee.
void marcel_one_task_less(marcel_t pid)
{
	ma_spin_lock(&__wait_lock);

	list_del(&pid->all_threads);
	if(((--(_main_struct.nb_tasks)) == 0) && (_main_struct.main_is_waiting)) {
		marcel_sem_V(&_main_struct.blocked);
	}

	ma_spin_unlock(&__wait_lock);
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

  if(MA_TASK_IS_BLOCKED(t)) {
    if(which & NOT_BLOCKED_ONLY)
      return 0;
  } else if(which & BLOCKED_ONLY)
    return 0;

  if(MA_TASK_IS_SLEEPING(t)) {
    if(which & NOT_SLEEPING_ONLY)
      return 0;
  } else if(which & SLEEPING_ONLY)
    return 0;

  return 1;
}

void marcel_threadslist(int max, marcel_t *pids, int *nb, int which)
{
	marcel_t t;
	int nb_pids = 0;
	DEFINE_CUR_LWP(, __attribute__((unused)) =, GET_LWP(marcel_self()));


	if( ((which & MIGRATABLE_ONLY) && (which & NOT_MIGRATABLE_ONLY)) ||
		((which & DETACHED_ONLY) && (which & NOT_DETACHED_ONLY)) ||
		((which & BLOCKED_ONLY) && (which & NOT_BLOCKED_ONLY)) ||
		((which & SLEEPING_ONLY) && (which & NOT_SLEEPING_ONLY)))
		RAISE(CONSTRAINT_ERROR);

	ma_spin_lock(&__wait_lock);
	list_for_each_entry(t, &all_threads, all_threads) {
		if (want_to_see(t, which)) {
			if (nb_pids < max)
				pids[nb_pids++] = t;
			else
				nb_pids++;
		}
	}
	*nb = nb_pids;
	ma_spin_unlock(&__wait_lock);
}

void marcel_snapshot(snapshot_func_t f)
{
	marcel_t t;
	DEFINE_CUR_LWP(, __attribute__((unused)) =, GET_LWP(marcel_self()));

	ma_spin_lock(&__wait_lock);
	list_for_each_entry(t, &all_threads, all_threads)
		(*f)(t);
	ma_spin_unlock(&__wait_lock);
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

void marcel_gensched_shutdown(void)
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
	__TBX_FREE(marcel_stackbase(ma_per_lwp(idle_task,&__main_lwp)), __FILE__, __LINE__);
#endif
#endif

	LOG_OUT();
}

static any_t TBX_NORETURN idle_func(any_t hlwp)
{
	if (hlwp == NULL) {
		/* upcall_new_task est venue ici ? */
		RAISE(PROGRAM_ERROR);
	}
	for(;;) {
//		marcel_yield();
	  marcel_check_polling(MARCEL_EV_POLL_AT_IDLE);
	  marcel_yield_intern();
	}
}

MA_DEFINE_PER_LWP(marcel_task_t *,idle_task)=NULL;

static void marcel_sched_lwp_init(marcel_lwp_t* lwp)
{
	marcel_attr_t attr;
	char name[MARCEL_MAXNAMESIZE];
	LOG_IN();

	if (IS_FIRST_LWP(lwp)) {
		marcel_sem_init(&_main_struct.blocked,0);
	} else {
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
	marcel_attr_setvpmask(&attr, MARCEL_VPMASK_ALL_BUT_VP(LWP_NUMBER(lwp)));
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
	marcel_attr_setprio(&attr, MA_IDLE_PRIO);
	marcel_attr_setinitrq(&attr, ma_dontsched_rq(lwp));
	marcel_create_special(&(ma_per_lwp(idle_task, lwp)),
			      &attr, idle_func, (void*)(ma_lwp_t)lwp);
	MTRACE("IdleTask", ma_per_lwp(idle_task, lwp));

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

MA_DEFINE_LWP_NOTIFIER_START_PRIO(generic_sched, 100, "Sched generic",
				  marcel_sched_lwp_init, "Création de idle",
				  marcel_sched_lwp_start, "Réveil de idle et démarrage de la préemtion");

MA_LWP_NOTIFIER_CALL_UP_PREPARE(generic_sched, MA_INIT_GENSCHED_IDLE);
MA_LWP_NOTIFIER_CALL_ONLINE_PRIO(generic_sched, MA_INIT_GENSCHED_PREEMPT,
				 MA_INIT_GENSCHED_PREEMPT_PRIO);

#ifdef MA__LWPS
void __marcel_init marcel_gensched_start_lwps(void)
{
	int i;
	LOG_IN();
	for(i=1; i<get_nb_lwps(); i++)
		marcel_lwp_add_vp();
	
	mdebug("marcel_sched_init  : %i lwps created\n", get_nb_lwps());
	LOG_OUT();
}

__ma_initfunc(marcel_gensched_start_lwps, MA_INIT_GENSCHED_START_LWPS,
	       "Création et démarrage des LWPs");
#endif /* MA__LWPS */
