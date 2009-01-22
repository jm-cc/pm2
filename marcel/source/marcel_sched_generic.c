
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
#ifdef PIOMAN
#include "pioman.h"
#endif /* PIOMAN */
#include "tbx_compiler.h"
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <sched.h>

/* marcel_nanosleep
 */
DEF_MARCEL_POSIX(int,nanosleep,(const struct timespec *rqtp,struct timespec *rmtp),(rqtp,rmtp),
{
        unsigned long long nsec = rqtp->tv_nsec + 1000000000*rqtp->tv_sec;
	int todosleep;
        LOG_IN();

        if ((rqtp->tv_nsec<0)||(rqtp->tv_nsec > 999999999)||(rqtp->tv_sec < 0)) {
                mdebug("(p)marcel_nanosleep : valeur nsec(%ld) invalide\n",rqtp->tv_nsec);
  	   errno = EINVAL;
                LOG_RETURN(-1);
   }

	ma_set_current_state(MA_TASK_INTERRUPTIBLE);
   todosleep = ma_schedule_timeout(nsec/(1000*marcel_gettimeslice()));

        if (rmtp) {
	   nsec = todosleep*(1000*marcel_gettimeslice());
      rmtp->tv_sec = nsec/1000000000;
      rmtp->tv_nsec = nsec%1000000000;
	}

        if (todosleep) {
	   errno = EINTR;
				    LOG_RETURN(-1);
	}
   else
      LOG_RETURN(0);
})

DEF___C(int,nanosleep,(const struct timespec *rqtp,struct timespec *rmtp),(rqtp,rmtp));
DEF_C(int,nanosleep,(const struct timespec *rqtp,struct timespec *rmtp),(rqtp,rmtp));

/* marcel_usleep
 */
DEF_MARCEL(int,usleep,(unsigned long usec),(usec),
{
	     LOG_IN();
	ma_set_current_state(MA_TASK_INTERRUPTIBLE);
	ma_schedule_timeout((usec+marcel_gettimeslice()-1)/marcel_gettimeslice());

	LOG_RETURN(0);
})

DEF_POSIX(int,usleep,(unsigned long usec),(usec),
{
	LOG_IN();

	if (usec > 1000000) {
		mdebug("(p)marcel_usleep : valeur usec(%ld) invalide\n", usec);
		errno = EINVAL;
		LOG_RETURN(-1);
	}

	marcel_usleep(usec);
	LOG_RETURN(0);
})

DEF_C(int,usleep,(unsigned long usec),(usec));
DEF___C(int,usleep,(unsigned long usec),(usec));

/* marcel_sleep
 */
DEF_MARCEL_POSIX(int,sleep,(unsigned long sec),(sec),
{
	LOG_IN();
	ma_set_current_state(MA_TASK_INTERRUPTIBLE);
	ma_schedule_timeout((1000000*sec)/marcel_gettimeslice());

	LOG_RETURN(0);
})

#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, pmarcel_sleep, sleep, GLIBC_2_0);
#endif
DEF___C(int,sleep,(unsigned long sec),(sec));

DEF_MARCEL_POSIX(int,sched_get_priority_max,(int policy),(policy),
{
	LOG_IN();
	if ((policy == SCHED_RR) || (policy == SCHED_FIFO))
		LOG_RETURN(MA_MAX_USER_RT_PRIO);
	else if (policy == SCHED_OTHER)
		LOG_RETURN(0);

	mdebug("sched_get_priority_max : valeur policy(%d) invalide\n", policy);
	errno = EINVAL;
	LOG_RETURN(-1);
})

DEF_C(int,sched_get_priority_max,(int policy),(policy));
DEF___C(int,sched_get_priority_max,(int policy),(policy));

DEF_MARCEL_POSIX(int,sched_get_priority_min,(int policy),(policy),
{
	LOG_IN();
	if ((policy == SCHED_RR) || (policy == SCHED_OTHER)
	    || (policy == SCHED_FIFO))
		LOG_RETURN(0);
	mdebug("sched_get_priority_min : valeur policy(%d) invalide\n", policy);
	errno = EINVAL;
	LOG_RETURN(-1);
})

DEF_C(int,sched_get_priority_min,(int policy),(policy));
DEF___C(int,sched_get_priority_min,(int policy),(policy));


#ifdef MA__SELF_VAR
#ifdef MA__USE_TLS
__thread
#endif
	marcel_t ma_self
#ifdef STANDARD_MAIN
		= &__main_thread_struct
#endif
		;
#endif
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
		       cur, next, ma_vpnum(ma_get_task_lwp(cur)));
		__ma_get_lwp_var(previous_thread)=cur;
		MA_THR_LONGJMP(cur->number, (next), NORMAL_RETURN);
	}
	return cur;
}

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
	    num += ma_topo_vpdata_l(vp, nb_tasks);
	return num + 1;		/* + 1 pour le main */
}

unsigned marcel_per_lwp_nbthreads(void)
{
	unsigned num = 0;
	struct marcel_topo_level *vp;

	vp = ma_get_task_lwp(MARCEL_SELF)->vp_level;
	num += ma_topo_vpdata_l(vp, nb_tasks);
	return num + 1;		/* + 1 pour le main */
}

/* TODO: utiliser plutôt le numéro de slot ? (le profilage sait se débrouiller lors de la réutilisation des numéros) (problème avec les piles allouées statiquement) */
#define MA_MAX_VP_THREADS 10000000

unsigned long marcel_createdthreads(void)
{
	unsigned long num = 0;
	struct marcel_topo_level *vp;
	for_all_vp(vp)
	    num += ma_topo_vpdata_l(vp, task_number);
	return num;
}

void marcel_one_more_task(marcel_t pid)
{
	unsigned oldnbtasks;
	struct marcel_topo_level *vp;
	unsigned task_number;

	/* record this thread on _this_ lwp */
	ma_local_bh_disable();
	ma_preempt_disable();
	vp = &marcel_topo_vp_level[ma_vpnum(MA_LWP_SELF)];
	_ma_raw_spin_lock(&ma_topo_vpdata_l(vp,threadlist_lock));

	task_number = ++ma_topo_vpdata_l(vp,task_number);
	MA_BUG_ON(task_number == MA_MAX_VP_THREADS);
	pid->number = ma_vpnum(MA_LWP_SELF) * MA_MAX_VP_THREADS + task_number;
	MA_BUG_ON(ma_topo_vpdata_l(vp,task_number) == MA_MAX_VP_THREADS);
	list_add(&pid->all_threads,&ma_topo_vpdata_l(vp,all_threads));
	oldnbtasks = ma_topo_vpdata_l(vp,nb_tasks)++;

	if (!oldnbtasks && ma_topo_vpdata_l(vp,main_is_waiting))
		a_new_thread = tbx_true;

	_ma_raw_spin_unlock(&ma_topo_vpdata_l(vp,threadlist_lock));
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
}

void marcel_one_task_less(marcel_t pid)
{
	unsigned vpnum = pid->number/MA_MAX_VP_THREADS;
	struct marcel_topo_level *vp;

	MA_BUG_ON(vpnum >= marcel_nbvps());
	vp = &marcel_topo_vp_level[vpnum];

	ma_spin_lock_softirq(&ma_topo_vpdata_l(vp,threadlist_lock));

	list_del(&pid->all_threads);
	if(((--(ma_topo_vpdata_l(vp,nb_tasks))) == 0) && (ma_topo_vpdata_l(vp,main_is_waiting)))
		ma_wake_up_thread(__main_thread);

	ma_spin_unlock_softirq(&ma_topo_vpdata_l(vp,threadlist_lock));
}

static __tbx_inline__ int want_to_see(marcel_t t, int which)
{
	if (t->detached) {
		if (which & NOT_DETACHED_ONLY)
			return 0;
	} else if (which & DETACHED_ONLY)
		return 0;

#ifdef MARCEL_MIGRATION_ENABLED
	if (t->not_migratable) {
		if (which & MIGRATABLE_ONLY)
			return 0;
	} else if (which & NOT_MIGRATABLE_ONLY)
		return 0;
#endif /* MARCEL_MIGRATION_ENABLED */

	if (MA_TASK_IS_BLOCKED(t)) {
		if (which & NOT_BLOCKED_ONLY)
			return 0;
	} else if (which & BLOCKED_ONLY)
		return 0;

	if (MA_TASK_IS_READY(t)) {
		if (which & NOT_READY_ONLY)
			return 0;
	} else if (which & READY_ONLY)
		return 0;

	return 1;
}

void marcel_threadslist(int max, marcel_t *pids, int *nb, int which)
{
	marcel_t t;
	int nb_pids = 0;
	struct marcel_topo_level *vp;


	     if(((which & MIGRATABLE_ONLY) && (which & NOT_MIGRATABLE_ONLY)) ||
		((which & DETACHED_ONLY) && (which & NOT_DETACHED_ONLY)) ||
		((which & BLOCKED_ONLY) && (which & NOT_BLOCKED_ONLY)) ||
		((which & READY_ONLY) && (which & NOT_READY_ONLY)))
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);

	for_all_vp(vp) {
		ma_spin_lock_softirq(&ma_topo_vpdata_l(vp,threadlist_lock));
		list_for_each_entry(t, &ma_topo_vpdata_l(vp,all_threads), all_threads) {
			if (want_to_see(t, which)) {
				if (nb_pids < max)
					pids[nb_pids++] = t;
				else
					nb_pids++;
			}
		}
		ma_spin_unlock_softirq(&ma_topo_vpdata_l(vp,threadlist_lock));
	}
	*nb = nb_pids;
}

void marcel_snapshot(snapshot_func_t f)
{
	marcel_t t;
	struct marcel_topo_level *vp;

	for_all_vp(vp) {
		ma_spin_lock_softirq(&ma_topo_vpdata_l(vp,threadlist_lock));
		list_for_each_entry(t, &ma_topo_vpdata_l(vp,all_threads), all_threads)
			(*f)(t);
		ma_spin_unlock_softirq(&ma_topo_vpdata_l(vp,threadlist_lock));
	}
}

// Attend que toutes les taches soient terminees. Cette fonction
// _doit_ etre appelee par la tache "main".
static void wait_all_tasks_end(void)
{
	struct marcel_topo_level *vp;
	LOG_IN();

#ifdef MA__DEBUG
	if (marcel_self() != __main_thread) {
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
	}
#endif 

	for_all_vp(vp)
		ma_topo_vpdata_l(vp,main_is_waiting) = tbx_true;
retry:
	a_new_thread = tbx_false;
	for_all_vp(vp) {
		ma_spin_lock_softirq(&ma_topo_vpdata_l(vp,threadlist_lock));
		if (ma_topo_vpdata_l(vp,nb_tasks)) {
			ma_set_current_state(MA_TASK_INTERRUPTIBLE);
			ma_spin_unlock_softirq_no_resched(&ma_topo_vpdata_l(vp,threadlist_lock));
			ma_schedule();
			goto retry;
		}
		ma_spin_unlock_softirq(&ma_topo_vpdata_l(vp,threadlist_lock));
	}
	if (a_new_thread)
		goto retry;

	LOG_OUT();
}

void marcel_gensched_shutdown(void)
{
#ifdef MA__LWPS
	marcel_lwp_t *lwp, *lwp_found;
	marcel_vpset_t vpset;
#endif

	LOG_IN();

	if(MARCEL_SELF != __main_thread)
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
	if (ma_in_atomic()) {
		pm2debug("bad: shutdown while atomic (%06x)! Did you forget to unlock a spinlock?\n", ma_preempt_count());
		ma_show_preempt_backtrace();
		MA_BUG();
	}

	wait_all_tasks_end();

	marcel_exit_top();

#ifdef MA__BUBBLES
	ma_deactivate_idle_scheduler();
	if (current_sched->exit)
		current_sched->exit();
#endif

#ifdef MA__LWPS
	/* Stop timer before stopping kernel threads, to avoid running
	 * pthread_kill() in the middle of pthread_exit() */
	marcel_sig_stop_itimer();

	mdebug("blocking this LWP\n");
	ma_lwp_block();

	mdebug("stopping LWPs (1)\n");
	/* We must switch to the main kernel thread for proper
	 * termination. However, it may be currently a spare LWP, so we
	 * have to wait for it to become active. */
	while ((lwp = ma_lwp_wait_vp_active())) {
		if (lwp == &__main_lwp) {
			mdebug("main LWP is active, jumping to it at vp %d\n", ma_vpnum(lwp));
			marcel_vpset_vp(&vpset, ma_vpnum(lwp));
			/* To match ma_lwp_block() above */
			ma_preempt_enable();
			marcel_apply_vpset(&vpset);
			MA_BUG_ON(MA_LWP_SELF != &__main_lwp);
			/* Ok, we're in the main kernel thread now */
			mdebug("block it too\n");
			ma_lwp_block();
		} else marcel_lwp_stop_lwp(lwp);
	}

	MA_BUG_ON(MA_LWP_SELF != &__main_lwp);

	mdebug("stopping LWPs (2)\n");
	for(;;) {
		lwp_found=NULL;
		ma_lwp_list_lock_read();
		ma_for_all_lwp(lwp) {
			if (lwp != &__main_lwp) {
				lwp_found=lwp;
				break;
			}
		}
		ma_lwp_list_unlock_read();
		if (!lwp_found) {
			break;
		}

		MA_BUG(); /* should not happen (tm) */

		marcel_lwp_stop_lwp(lwp_found);
	}
#else
	/* Destroy master-sched's stack */
	//marcel_cancel(__main_lwp.sched.idle_task);
#ifdef PM2
	/* __sched_task is detached, so we can free its stack now */
	//__TBX_FREE(marcel_stackbase(ma_per_lwp(idle_task,&__main_lwp)), __FILE__, __LINE__);
#endif
#endif
	marcel_sig_exit();

	LOG_OUT();
}

#ifdef MA__LWPS
static any_t TBX_NORETURN idle_poll_func(any_t hlwp)
{
	int dopoll;
	struct marcel_lwp *lwp = hlwp;
	if (lwp == NULL) {
		/* upcall_new_task est venue ici ? */
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
	}
        ma_set_thread_flag(TIF_POLLING_NRFLAG);
	for(;;) {
		MA_BUG_ON(lwp != MA_LWP_SELF);
		if (ma_vpnum(lwp) == -1) {
			ma_clear_thread_flag(TIF_POLLING_NRFLAG);
			ma_smp_mb__after_clear_bit();

			if (!ma_get_need_resched())
				ma_lwp_wait_active();
			ma_set_thread_flag(TIF_POLLING_NRFLAG);
		}
		/* we are the active LWP of this VP */

		/* schedule threads */
		//PROF_EVENT(idle_tests_need_resched);
		if (ma_get_need_resched()) {
			PROF_EVENT(idle_does_schedule);
			if (ma_schedule())
				continue;
		}
		//PROF_EVENT(idle_tested_need_resched);

		/* no more threads, now poll */
#ifdef PIOMAN
		dopoll = piom_polling_is_required(PIOM_POLL_AT_IDLE);
		if (dopoll) {
		        __piom_check_polling(PIOM_POLL_AT_IDLE);
		}
#else
		dopoll = marcel_polling_is_required(MARCEL_EV_POLL_AT_IDLE);
		if (dopoll) {
		        __marcel_check_polling(MARCEL_EV_POLL_AT_IDLE);
		}

#endif
#ifdef MARCEL_IDLE_PAUSE
		if (dopoll)
			marcel_sig_nanosleep();
		else {
			marcel_sig_disable_interrupts();
			ma_sched_sig_pause();
			marcel_sig_enable_interrupts();
		}
#endif
	}
}
/* Idle function for supplementary VPs: don't poll on idle */
static any_t idle_func(any_t hlwp TBX_UNUSED)
{
	ma_set_thread_flag(TIF_POLLING_NRFLAG);
	for(;;) {
		/* let wakers know that we will shortly poll need_resched and
		 * thus they don't need to send a kill */
		if (ma_get_need_resched()) {
			PROF_EVENT(idle_does_schedule);
			if (ma_schedule())
				continue;
		}
		marcel_sig_disable_interrupts();
		ma_sched_sig_pause();
		marcel_sig_enable_interrupts();
	}
	return NULL;
}
#endif

static void marcel_sched_lwp_init(marcel_lwp_t* lwp)
{
#ifdef MA__LWPS
	marcel_attr_t attr;
	char name[MARCEL_MAXNAMESIZE];
#endif
	LOG_IN();

	if (!ma_is_first_lwp(lwp))
		/* run_task DOIT démarrer en contexte d'irq */
		ma_per_lwp(run_task, lwp)->preempt_count=MA_HARDIRQ_OFFSET+MA_PREEMPT_OFFSET;
	else
		/* ajout du thread principal à la liste des threads */
		list_add(&SELF_GETMEM(all_threads),&ma_topo_vpdata(0,all_threads));

#ifdef MA__LWPS
	/*****************************************/
	/* Création de la tâche Idle (idle_task) */
	/*****************************************/
	marcel_attr_init(&attr);
	snprintf(name,MARCEL_MAXNAMESIZE,"idle/%2d",ma_vpnum(lwp));
	marcel_attr_setname(&attr,name);
	marcel_attr_setdetachstate(&attr, tbx_true);
	marcel_attr_setflags(&attr, MA_SF_POLL|MA_SF_NORUN);
#ifdef PM2
	{
		char *stack = __TBX_MALLOC(2*THREAD_SLOT_SIZE, __FILE__, __LINE__);
		
		marcel_attr_setstackaddr(&attr, (void*)((unsigned long)(stack + THREAD_SLOT_SIZE) & ~(THREAD_SLOT_SIZE-1)));
	}
#endif
	marcel_attr_setprio(&attr, MA_IDLE_PRIO);
	marcel_attr_setinitrq(&attr, ma_dontsched_rq(lwp));
	marcel_create_special(&(ma_per_lwp(idle_task, lwp)), &attr,
			ma_vpnum(lwp) == -1 || ma_vpnum(lwp)<marcel_nbvps()?idle_poll_func:idle_func,
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
static void __marcel_init marcel_gensched_start_lwps(void)
{
	int i;
	LOG_IN();
	for(i=1; i<marcel_nbvps(); i++)
		marcel_lwp_add_vp();
	mdebug("marcel_sched_init  : %i lwps created\n", marcel_nbvps());
	LOG_OUT();
}

__ma_initfunc(marcel_gensched_start_lwps, MA_INIT_GENSCHED_START_LWPS, "Création et démarrage des LWPs");
#endif /* MA__LWPS */
