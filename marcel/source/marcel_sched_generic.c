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
			MA_THR_DESTROYJMP(cur);
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
   struct marcel_topo_level *vp;
   for_all_vp(vp)
      num += vp->nb_tasks-1;
   return num + 1;   /* + 1 pour le main */
}

/* TODO: utiliser plut�t le num�ro de slot ? (le profilage sait se d�brouiller lors de la r�utilisation des num�ros) (probl�me avec les piles allou�es statiquement) */
#define MAX_MAX_VP_THREADS 1000000

unsigned long marcel_createdthreads(void)
{
   unsigned long num = 0;
   struct marcel_topo_level *vp;
   for_all_vp(vp)
      num += vp->task_number-1;
   return num;
}

// Appele a chaque fois qu'une tache est creee (y compris par le biais
// de end_hibernation).
void marcel_one_more_task(marcel_t pid)
{
	unsigned oldnbtasks;
	struct marcel_topo_level *vp;

	/* record this thread on _this_ lwp */
	ma_local_bh_disable();
	ma_preempt_disable();
	vp = &marcel_topo_vp_level[LWP_NUMBER(LWP_SELF)];
	_ma_raw_spin_lock(&vp->threadlist_lock);

	pid->number = LWP_NUMBER(LWP_SELF) * MAX_MAX_VP_THREADS + vp->task_number++;
	MA_BUG_ON(vp->task_number == MAX_MAX_VP_THREADS);
	list_add(&pid->all_threads,&vp->all_threads);
	oldnbtasks = vp->nb_tasks++;

	if (!oldnbtasks && vp->main_is_waiting)
		a_new_thread = tbx_true;

	_ma_raw_spin_unlock(&vp->threadlist_lock);
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
}

// Appele a chaque fois qu'une tache est terminee.
void marcel_one_task_less(marcel_t pid)
{
	unsigned vpnum = pid->number/MAX_MAX_VP_THREADS;
	struct marcel_topo_level *vp;

	MA_BUG_ON(vpnum >= marcel_nbvps());
	vp = &marcel_topo_vp_level[vpnum];

	ma_spin_lock_softirq(&vp->threadlist_lock);

	list_del(&pid->all_threads);
	if(((--(vp->nb_tasks)) == 0) && (vp->main_is_waiting))
		ma_wake_up_thread(__main_thread);

	ma_spin_unlock_softirq(&vp->threadlist_lock);
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
	DEFINE_CUR_LWP(, TBX_UNUSED =, LWP_SELF);
	struct marcel_topo_level *vp;


	if( ((which & MIGRATABLE_ONLY) && (which & NOT_MIGRATABLE_ONLY)) ||
		((which & DETACHED_ONLY) && (which & NOT_DETACHED_ONLY)) ||
		((which & BLOCKED_ONLY) && (which & NOT_BLOCKED_ONLY)) ||
		((which & SLEEPING_ONLY) && (which & NOT_SLEEPING_ONLY)))
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);

	for_all_vp(vp) {
		ma_spin_lock_softirq(&vp->threadlist_lock);
		list_for_each_entry(t, &vp->all_threads, all_threads) {
			if (want_to_see(t, which)) {
				if (nb_pids < max)
					pids[nb_pids++] = t;
				else
					nb_pids++;
			}
		}
		ma_spin_unlock_softirq(&vp->threadlist_lock);
	}
	*nb = nb_pids;
}

void marcel_snapshot(snapshot_func_t f)
{
	marcel_t t;
	struct marcel_topo_level *vp;
	DEFINE_CUR_LWP(, TBX_UNUSED =, LWP_SELF);

	for_all_vp(vp) {
		ma_spin_lock_softirq(&vp->threadlist_lock);
		list_for_each_entry(t, &vp->all_threads, all_threads)
			(*f)(t);
		ma_spin_unlock_softirq(&vp->threadlist_lock);
	}
}

// Attend que toutes les taches soient terminees. Cette fonction
// _doit_ etre appelee par la tache "main".
static void wait_all_tasks_end(void)
{
	struct marcel_topo_level *vp_first_wait = NULL;
	struct marcel_topo_level *vp;
	LOG_IN();

#ifdef MA__DEBUG
	if (marcel_self() != __main_thread) {
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
	}
#endif 

	for_all_vp(vp)
		vp->main_is_waiting = tbx_true;
retry:
	a_new_thread = tbx_false;
	for_all_vp(vp) {//, vp_first_wait)
		ma_spin_lock_softirq(&vp->threadlist_lock);
		if (vp->nb_tasks) {
			ma_set_current_state(MA_TASK_INTERRUPTIBLE);
			ma_spin_unlock_softirq(&vp->threadlist_lock);
			ma_schedule();
			vp_first_wait = vp;
			goto retry;
		}
		ma_spin_unlock_softirq(&vp->threadlist_lock);
	}
	if (a_new_thread)
		goto retry;

	LOG_OUT();
}

void marcel_gensched_shutdown(void)
{
#ifdef MA__SMP
	marcel_lwp_t *lwp, *lwp_found;
#endif
	marcel_vpmask_t mask;

	LOG_IN();

	if(MARCEL_SELF != __main_thread)
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
	MA_BUG_ON(ma_in_atomic());

	wait_all_tasks_end();

	ma_write_lock(&ma_idle_scheduler_lock);
	ma_idle_scheduler = 0;
	ma_write_unlock(&ma_idle_scheduler_lock);

#ifdef MA__SMP

	marcel_sig_stop_itimer();

	mdebug("blocking this LWP\n");
	ma_lwp_block();

	mdebug("stopping LWPs from %p\n", LWP_SELF);
	while ((lwp = ma_lwp_wait_vp_active())) {
		if (lwp == &__main_lwp) {
			mdebug("main LWP is active, jumping to it\n");
			mask = MARCEL_VPMASK_ALL_BUT_VP(LWP_NUMBER(&__main_lwp));
			marcel_change_vpmask(&mask);
			lwp = LWP_SELF;
			marcel_leave_blocking_section();
			MA_BUG_ON(LWP_SELF != &__main_lwp);
			mdebug("block it too\n");
			ma_lwp_block();
		} else marcel_lwp_stop_lwp(lwp);
	}

	MA_BUG_ON(LWP_SELF != &__main_lwp);

	mdebug("stopping LWPs for supplementary VPs\n");
	for(;;) {
		lwp_found=NULL;
		lwp_list_lock_read();
		for_all_lwp(lwp) {
			if (lwp != &__main_lwp) {
				lwp_found=lwp;
				break;
			}
		}
		lwp_list_unlock_read();
		if (!lwp_found) {
			break;
		}
		marcel_lwp_stop_lwp(lwp_found);
	}
	ma_preempt_enable();

#elif defined(MA__ACTIVATION)
	// TODO : arr�ter les autres activations...

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
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
	}
	for(;;) {
		ma_lwp_wait_active();
		/* we are the active LWP of this VP */

		/* let wakers now that we will shortly poll need_resched and
		 * thus they don't need to send a kill */
		ma_set_thread_flag(TIF_POLLING_NRFLAG);
		dopoll = marcel_polling_is_required(MARCEL_EV_POLL_AT_IDLE);
		if (dopoll)
		        __marcel_check_polling(MARCEL_EV_POLL_AT_IDLE);
		ma_clear_thread_flag(TIF_POLLING_NRFLAG);
		/* make sure people see that we won't poll it afterwards */
		ma_smp_mb__after_clear_bit();
		if (!ma_need_resched() || !ma_schedule()) {
#ifdef MARCEL_IDLE_PAUSE
			if (dopoll)
				marcel_sig_nanosleep();
			else
				marcel_sig_pause();
#endif
		}
	}
}
#ifndef MA__ACT
static any_t idle_func(any_t hlwp)
{
	for(;;) {
		/* let wakers now that we will shortly poll need_resched and
		 * thus they don't need to send a kill */
		ma_clear_thread_flag(TIF_POLLING_NRFLAG);
		ma_smp_mb__after_clear_bit();
		pause();
		ma_set_thread_flag(TIF_POLLING_NRFLAG);
		if (ma_need_resched())
			ma_schedule();
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
		/* run_task DOIT d�marrer en contexte d'irq */
		ma_per_lwp(run_task, lwp)->preempt_count=MA_HARDIRQ_OFFSET+MA_PREEMPT_OFFSET;
	else
		/* ajout du thread principal � la liste des threads */
		list_add(&SELF_GETMEM(all_threads),&marcel_topo_vp_level[0].all_threads);

#ifdef MA__LWPS
	/*****************************************/
	/* Cr�ation de la t�che Idle (idle_task) */
	/*****************************************/
	marcel_attr_init(&attr);
	snprintf(name,MARCEL_MAXNAMESIZE,"idle/%u",LWP_NUMBER(lwp));
	marcel_attr_setname(&attr,name);
	marcel_attr_setdetachstate(&attr, tbx_true);
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
			LWP_NUMBER(lwp) == -1 || LWP_NUMBER(lwp)<marcel_nbvps()?idle_poll_func:idle_func,
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
				  marcel_sched_lwp_init, "Cr�ation de idle",
				  marcel_sched_lwp_start, "R�veil de idle et d�marrage de la pr�emption");

MA_LWP_NOTIFIER_CALL_UP_PREPARE(generic_sched, MA_INIT_GENSCHED_IDLE);
MA_LWP_NOTIFIER_CALL_ONLINE_PRIO(generic_sched, MA_INIT_GENSCHED_PREEMPT, MA_INIT_GENSCHED_PREEMPT_PRIO);

#ifdef MA__LWPS
void __marcel_init marcel_gensched_start_lwps(void)
{
	int i;
	LOG_IN();
	for(i=1; i<marcel_nbvps(); i++)
		marcel_lwp_add_vp();
	
	mdebug("marcel_sched_init  : %i lwps created\n", marcel_nbvps());
	LOG_OUT();
}

__ma_initfunc(marcel_gensched_start_lwps, MA_INIT_GENSCHED_START_LWPS, "Cr�ation et d�marrage des LWPs");
#endif /* MA__LWPS */
