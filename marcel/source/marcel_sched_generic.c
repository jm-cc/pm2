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
#include "tbx_compiler.h"
#include <signal.h>

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

marcel_task_t *marcel_switch_to(marcel_task_t *cur, marcel_task_t *next)
{
	MA_BUG_ON(!ma_in_atomic());
	if (cur != next) {
		if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
			MA_THR_RESTARTED(cur, "Switch_to");
			MA_BUG_ON(!ma_in_atomic());
			return __ma_get_lwp_var(previous_thread);
		}
		debug_printf(&MA_DEBUG_VAR_NAME(default),
			     "switchto(%p, %p) on LWP(%d)\n",
		       cur, next, LWP_NUMBER(GET_LWP(cur)));
		__ma_get_lwp_var(previous_thread)=cur;
		MA_THR_LONGJMP(cur->number, (next), NORMAL_RETURN);
	}
	return cur;
}

marcel_lwp_t __main_lwp = MA_LWP_INITIALIZER(&__main_lwp);

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Gestion des threads                                            */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

static tbx_bool_t a_new_thread;

unsigned marcel_nbthreads(void)
{
   unsigned num = 0;
   ma_lwp_t lwp;
   for_each_lwp_begin(lwp)
      num += ma_per_lwp(nb_tasks,lwp)-1;
   for_each_lwp_end();
   return num + 1;   /* + 1 pour le main */
}

/* TODO: utiliser plutôt le numéro de slot ? (le profilage sait se débrouiller lors de la réutilisation des numéros) (problème avec les piles allouées statiquement) */
#define MA_MAX_LWP_THREADS 1000000

unsigned long marcel_createdthreads(void)
{
   unsigned long num = 0;
   ma_lwp_t lwp;
   for_each_lwp_begin(lwp)
      num += ma_per_lwp(task_number,lwp)-1;
   for_each_lwp_end();
   return num;
}

// Appele a chaque fois qu'une tache est creee (y compris par le biais
// de end_hibernation).
void marcel_one_more_task(marcel_t pid)
{
	unsigned oldnbtasks;

	/* record this thread on _this_ lwp */
	ma_local_bh_disable();
	ma_preempt_disable();
	_ma_raw_spin_lock(&__ma_get_lwp_var(threadlist_lock));

	pid->number = LWP_NUMBER(LWP_SELF)*MA_MAX_LWP_THREADS+__ma_get_lwp_var(task_number)++;
	MA_BUG_ON(__ma_get_lwp_var(task_number) == MA_MAX_LWP_THREADS);
	list_add(&pid->all_threads,&__ma_get_lwp_var(all_threads));
	oldnbtasks = __ma_get_lwp_var(nb_tasks)++;

	if (!oldnbtasks && &__ma_get_lwp_var(main_is_waiting))
		a_new_thread = tbx_true;

	_ma_raw_spin_unlock(&__ma_get_lwp_var(threadlist_lock));
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
}

// Appele a chaque fois qu'une tache est terminee.
void marcel_one_task_less(marcel_t pid)
{
	unsigned lwpnum = pid->number/MA_MAX_LWP_THREADS;
	ma_lwp_t lwp;

	MA_BUG_ON(lwpnum >= get_nb_lwps());

	lwp = GET_LWP_BY_NUMBER(lwpnum);

	ma_spin_lock_softirq(&ma_per_lwp(threadlist_lock, lwp));

	list_del(&pid->all_threads);
	if(((--(ma_per_lwp(nb_tasks,lwp))) == 0) && (ma_per_lwp(main_is_waiting, lwp)))
		ma_wake_up_thread(__main_thread);

	ma_spin_unlock_softirq(&ma_per_lwp(threadlist_lock, lwp));
}

static __tbx_inline__ int want_to_see(marcel_t t, int which)
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
	ma_lwp_t lwp;
	DEFINE_CUR_LWP(, TBX_UNUSED =, LWP_SELF);


	if( ((which & MIGRATABLE_ONLY) && (which & NOT_MIGRATABLE_ONLY)) ||
		((which & DETACHED_ONLY) && (which & NOT_DETACHED_ONLY)) ||
		((which & BLOCKED_ONLY) && (which & NOT_BLOCKED_ONLY)) ||
		((which & SLEEPING_ONLY) && (which & NOT_SLEEPING_ONLY)))
		MARCEL_EXCEPTION_RAISE(CONSTRAINT_ERROR);

	for_each_lwp_begin(lwp)
		ma_spin_lock_softirq(&ma_per_lwp(threadlist_lock, lwp));
		list_for_each_entry(t, &ma_per_lwp(all_threads,lwp), all_threads) {
			if (want_to_see(t, which)) {
				if (nb_pids < max)
					pids[nb_pids++] = t;
				else
					nb_pids++;
			}
		}
		ma_spin_unlock_softirq(&ma_per_lwp(threadlist_lock, lwp));
	for_each_lwp_end();
	*nb = nb_pids;
}

void marcel_snapshot(snapshot_func_t f)
{
	marcel_t t;
	ma_lwp_t lwp;
	DEFINE_CUR_LWP(, TBX_UNUSED =, LWP_SELF);

	for_each_lwp_begin(lwp)
		ma_spin_lock_softirq(&ma_per_lwp(threadlist_lock, lwp));
		list_for_each_entry(t, &ma_per_lwp(all_threads,lwp), all_threads)
			(*f)(t);
		ma_spin_unlock_softirq(&ma_per_lwp(threadlist_lock, lwp));
	for_each_lwp_end()
}

// Attend que toutes les taches soient terminees. Cette fonction
// _doit_ etre appelee par la tache "main".
static void wait_all_tasks_end(void)
{
	ma_lwp_t lwp_first_wait = &__main_lwp;
	ma_lwp_t lwp;
	LOG_IN();

#ifdef MA__DEBUG
	if (marcel_self() != __main_thread) {
		MARCEL_EXCEPTION_RAISE(PROGRAM_ERROR);
	}
#endif 

	for_each_lwp_begin(lwp)
		ma_per_lwp(main_is_waiting, lwp) = tbx_true;
	for_each_lwp_end();
retry:
	a_new_thread = tbx_false;
	for_each_lwp_begin(lwp)//, lwp_first_wait)
		ma_spin_lock_softirq(&ma_per_lwp(threadlist_lock, lwp));
		if (ma_per_lwp(nb_tasks, lwp)) {
			ma_set_current_state(MA_TASK_INTERRUPTIBLE);
			ma_spin_unlock_softirq(&ma_per_lwp(threadlist_lock, lwp));
			ma_schedule();
			lwp_first_wait = lwp;
			goto retry;
		}
		ma_spin_unlock_softirq(&ma_per_lwp(threadlist_lock, lwp));
	for_each_lwp_end();
	if (a_new_thread)
		goto retry;

	LOG_OUT();
}

void marcel_gensched_shutdown(void)
{
#ifdef MA__SMP
	marcel_lwp_t *lwp, *lwp_found;
#endif
	marcel_vpmask_t mask = MARCEL_VPMASK_ALL_BUT_VP(0);

	LOG_IN();

	wait_all_tasks_end();

	ma_write_lock(&ma_idle_scheduler_lock);
	ma_idle_scheduler = 0;
	ma_write_unlock(&ma_idle_scheduler_lock);

	// Si nécessaire, on bascule sur le LWP(0)
	marcel_change_vpmask(&mask);

#ifdef MA__SMP


	if(LWP_SELF != &__main_lwp)
		MARCEL_EXCEPTION_RAISE(PROGRAM_ERROR);

	//lwp = next_lwp(&__main_lwp);

	ma_preempt_disable();
	for(;;) {
		lwp_found=NULL;
		lwp_list_lock_read();
		for_all_lwp(lwp) {
			if (lwp != LWP_SELF) {
				lwp_found=lwp;
				break;
			}
		}
		lwp_list_unlock_read();
		ma_preempt_enable();

		if (!lwp_found) {
			break;
		}
		marcel_lwp_stop_lwp(lwp_found);
		ma_preempt_disable();
	}
#elif defined(MA__ACTIVATION)
	// TODO : arrêter les autres activations...

	//act_cntl(ACT_CNTL_UPCALLS, (void*)ACT_DISABLE_UPCALLS);
#else
	/* Destroy master-sched's stack */
	//marcel_cancel(__main_lwp.sched.idle_task);
#ifdef PM2
	/* __sched_task is detached, so we can free its stack now */
	//__TBX_FREE(marcel_stackbase(ma_per_lwp(idle_task,&__main_lwp)), __FILE__, __LINE__);
#endif
#endif
#ifdef MA__TIMER
	marcel_sig_exit();
#endif

	LOG_OUT();
}

#ifdef MA__LWPS
static any_t TBX_NORETURN idle_poll_func(any_t hlwp)
{
	int dopoll;
	if (hlwp == NULL) {
		/* upcall_new_task est venue ici ? */
		MARCEL_EXCEPTION_RAISE(PROGRAM_ERROR);
	}
	ma_set_thread_flag(TIF_POLLING_NRFLAG);
	for(;;) {
		dopoll = marcel_polling_is_required(MARCEL_EV_POLL_AT_IDLE);
		if (dopoll)
		        __marcel_check_polling(MARCEL_EV_POLL_AT_IDLE);
#ifdef MARCEL_IDLE_PAUSE
		/* let wakers now that we will shortly poll need_resched and
		 * thus they don't need to send a kill */
		ma_clear_thread_flag(TIF_POLLING_NRFLAG);
		ma_smp_mb__after_clear_bit();
#endif
		if (!marcel_yield_intern()) {
#ifdef MARCEL_IDLE_PAUSE
			if (dopoll)
				marcel_sig_nanosleep();
			else
				marcel_sig_pause();
#endif
		}
#ifdef MARCEL_IDLE_PAUSE
		ma_set_thread_flag(TIF_POLLING_NRFLAG);
#endif
	}
}
#ifndef MA__ACT
static any_t idle_func(any_t hlwp)
{
	for(;;) {
		pause();
		marcel_yield_intern();
	}
	return NULL;
}
#endif
#endif

static void marcel_sched_lwp_init(marcel_lwp_t* lwp)
{
#ifdef MA__LWPS
	marcel_attr_t attr;
	char name[MARCEL_MAXNAMESIZE];
#endif
	LOG_IN();

	if (!IS_FIRST_LWP(lwp))
		/* run_task DOIT démarrer en contexte d'irq */
		ma_per_lwp(run_task, lwp)->preempt_count=MA_HARDIRQ_OFFSET+MA_PREEMPT_OFFSET;
	else
		/* ajout du thread principal à la liste des threads */
		list_add(&SELF_GETMEM(all_threads),&__ma_get_lwp_var(all_threads));

#ifdef MA__LWPS
	/*****************************************/
	/* Création de la tâche Idle (idle_task) */
	/*****************************************/
	marcel_attr_init(&attr);
	snprintf(name,MARCEL_MAXNAMESIZE,"idle/%u",LWP_NUMBER(lwp));
	marcel_attr_setname(&attr,name);
	marcel_attr_setdetachstate(&attr, tbx_true);
	marcel_attr_setvpmask(&attr, MARCEL_VPMASK_ALL_BUT_VP(LWP_NUMBER(lwp)));
	marcel_attr_setflags(&attr, MA_SF_POLL | /*MA_SF_NOSCHEDLOCK |*/
			     MA_SF_NORUN);
#ifdef PM2
	{
		char *stack = __TBX_MALLOC(2*THREAD_SLOT_SIZE, __FILE__, __LINE__);
		
		marcel_attr_setstackaddr(&attr, (void*)((unsigned long)(stack + THREAD_SLOT_SIZE) & ~(THREAD_SLOT_SIZE-1)));
	}
#endif
	marcel_attr_setprio(&attr, MA_IDLE_PRIO);
	marcel_attr_setinitrq(&attr, ma_dontsched_rq(lwp));
	marcel_create_special(&(ma_per_lwp(idle_task, lwp)), &attr,
			LWP_NUMBER(lwp)<get_nb_lwps()?idle_poll_func:idle_func,
			(void*)(ma_lwp_t)lwp);
	MTRACE("IdleTask", ma_per_lwp(idle_task, lwp));
#endif

	LOG_OUT();
}


static void marcel_sched_lwp_start(ma_lwp_t lwp)
{
	LOG_IN();

	MA_BUG_ON(!ma_in_irq());

#ifdef MA__LWPS
	marcel_wake_up_created_thread(ma_per_lwp(idle_task,lwp));
#endif

	ma_irq_exit();
	MA_BUG_ON(ma_in_atomic());
	ma_preempt_count()=0;
	ma_barrier();

	LOG_OUT();
}

MA_DEFINE_LWP_NOTIFIER_START_PRIO(generic_sched, 100, "Sched generic",
				  marcel_sched_lwp_init, "Création de idle",
				  marcel_sched_lwp_start, "Réveil de idle et démarrage de la préemption");

MA_LWP_NOTIFIER_CALL_UP_PREPARE(generic_sched, MA_INIT_GENSCHED_IDLE);
MA_LWP_NOTIFIER_CALL_ONLINE_PRIO(generic_sched, MA_INIT_GENSCHED_PREEMPT, MA_INIT_GENSCHED_PREEMPT_PRIO);

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

__ma_initfunc(marcel_gensched_start_lwps, MA_INIT_GENSCHED_START_LWPS, "Création et démarrage des LWPs");
#endif /* MA__LWPS */
