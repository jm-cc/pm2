
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
#include <sys/time.h>
#include <errno.h>
#include <sched.h>

/* marcel_nanosleep
 */
DEF_MARCEL_PMARCEL(int,nanosleep,(const struct timespec *rqtp,struct timespec *rmtp),(rqtp,rmtp),
{
	int old;
	unsigned long long todosleep;
	int nsec;

	MARCEL_LOG_IN();

	if ((rqtp->tv_nsec<0)||(rqtp->tv_nsec > 999999999)||(rqtp->tv_sec < 0)) {
		MARCEL_SCHED_LOG("(p)marcel_nanosleep : valeur nsec(%ld) invalide\n",rqtp->tv_nsec);
		errno = EINVAL;
		MARCEL_LOG_RETURN(-1);
	}

	old = __pmarcel_enable_asynccancel();
	ma_set_current_state(MA_TASK_INTERRUPTIBLE);
	todosleep = ma_schedule_timeout(ma_jiffies_from_us(MA_TIMESPEC_TO_USEC(rqtp)));
	__pmarcel_disable_asynccancel(old);

	if (rmtp) {
		nsec = todosleep * 1000 * MARCEL_CLOCK_RATE;
		rmtp->tv_sec = nsec/1000000000;
		rmtp->tv_nsec = nsec%1000000000;
	}

	if (todosleep) {
		errno = EINTR;
		MARCEL_LOG_RETURN(-1);
	}
	
	MARCEL_LOG_RETURN(0);
})
DEF___C(int,nanosleep,(const struct timespec *rqtp,struct timespec *rmtp),(rqtp,rmtp))
DEF_C(int,nanosleep,(const struct timespec *rqtp,struct timespec *rmtp),(rqtp,rmtp))

/* marcel_usleep
 */
DEF_MARCEL(int,usleep,(unsigned long usec),(usec),
{
	int old;

	MARCEL_LOG_IN();
	old = __pmarcel_enable_asynccancel();
	ma_set_current_state(MA_TASK_INTERRUPTIBLE);
	ma_schedule_timeout(ma_jiffies_from_us((usec)));
	__pmarcel_disable_asynccancel(old);

	MARCEL_LOG_RETURN(0);
})

DEF_PMARCEL(int,usleep,(unsigned long usec),(usec),
{
	MARCEL_LOG_IN();

	if (usec > 1000000) {
		MARCEL_SCHED_LOG("(p)marcel_usleep : valeur usec(%ld) invalide\n", usec);
		errno = EINVAL;
		MARCEL_LOG_RETURN(-1);
	}
	marcel_usleep(usec);

	MARCEL_LOG_RETURN(0);
})
DEF_C(int,usleep,(unsigned long usec),(usec))
DEF___C(int,usleep,(unsigned long usec),(usec))

/* marcel_sleep
 */
DEF_MARCEL_PMARCEL(int,sleep,(unsigned long sec),(sec),
{
	int old;
	unsigned long long todosleep;
 
	MARCEL_LOG_IN();

	old = __pmarcel_enable_asynccancel();
	ma_set_current_state(MA_TASK_INTERRUPTIBLE);
	todosleep = ma_schedule_timeout(ma_jiffies_from_us((1000000*sec)));
	__pmarcel_disable_asynccancel(old);

	MARCEL_LOG_RETURN(todosleep * MARCEL_CLOCK_RATE / 1000000);
})

#ifdef MA__LIBPTHREAD
versioned_symbol(libpthread, pmarcel_sleep, sleep, GLIBC_2_0);
#endif
DEF___C(int,sleep,(unsigned long sec),(sec))

DEF_MARCEL_PMARCEL(int,sched_get_priority_max,(int policy),(policy),
{
	MARCEL_LOG_IN();

	errno = 0;
	if ((policy == SCHED_RR) || (policy == SCHED_FIFO))
		MARCEL_LOG_RETURN(MA_MAX_USER_RT_PRIO);
	else if (policy == SCHED_OTHER)
		MARCEL_LOG_RETURN(0);

	MARCEL_SCHED_LOG("sched_get_priority_max : valeur policy(%d) invalide\n", policy);
	errno = EINVAL;
	MARCEL_LOG_RETURN(-1);
})
DEF_C(int,sched_get_priority_max,(int policy),(policy))
DEF___C(int,sched_get_priority_max,(int policy),(policy))

DEF_MARCEL_PMARCEL(int,sched_get_priority_min,(int policy),(policy),
{
	MARCEL_LOG_IN();

	errno = 0;
	if ((policy == SCHED_RR) || (policy == SCHED_OTHER) || (policy == SCHED_FIFO))
		MARCEL_LOG_RETURN(0);
	
	MARCEL_SCHED_LOG("sched_get_priority_min : valeur policy(%d) invalide\n", policy);
	errno = EINVAL;
	MARCEL_LOG_RETURN(-1);
})
DEF_C(int,sched_get_priority_min,(int policy),(policy))
DEF___C(int,sched_get_priority_min,(int policy),(policy))


#ifdef MA__SELF_VAR
#ifdef MA__SELF_VAR_TLS
__thread
#endif
marcel_t __ma_self
#ifdef STANDARD_MAIN
= &__main_thread_struct
#endif
	;
#endif
marcel_task_t *marcel_switch_to(marcel_task_t *cur, marcel_task_t *next)
{
	MA_BUG_ON(!ma_in_atomic());
	if (cur != next) {
		if (cur->f_pre_switch) {
			cur->f_pre_switch(cur->arg);
		}
		if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
			MA_THR_DESTROYJMP(cur);
			MA_THR_RESTARTED(cur, "Switch_to");
			MA_BUG_ON(!ma_in_atomic());
			ma_update_lwp_blocked_signals();
			if (cur->f_post_switch) {
				cur->f_post_switch(cur->arg);
			}
			return __ma_get_lwp_var(previous_thread);
		}
		MARCEL_SCHED_LOG("switchto(`%s' [%p], `%s' [%p]) on LWP(%d)\n",
				 cur->as_entity.name, cur, next->as_entity.name, next, ma_vpnum(ma_get_task_lwp(cur)));
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
	unsigned num;
	struct marcel_topo_level *vp;

	vp = ma_get_task_lwp(MARCEL_SELF)->vp_level;
	num = ma_topo_vpdata_l(vp, nb_tasks);
	return num + 1;		/* + 1 pour le main */
}

/* TODO: utiliser plut�t le num�ro de slot ? (le profilage sait se d�brouiller lors de la r�utilisation des num�ros) (probl�me avec les piles allou�es statiquement) */
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

	/** retrieve the task id */
	task_number = ++ma_topo_vpdata_l(vp,task_number);
	MA_BUG_ON(task_number == MA_MAX_VP_THREADS);

	/** save the vpnum in marcel_t data structure */
	pid->number = ma_vpnum(MA_LWP_SELF) * MA_MAX_VP_THREADS + task_number;
	MA_BUG_ON(ma_topo_vpdata_l(vp,task_number) == MA_MAX_VP_THREADS);

	/** register the thread on this LWP */
	tbx_fast_list_add(&pid->all_threads,&ma_topo_vpdata_l(vp,all_threads));
	oldnbtasks = ma_topo_vpdata_l(vp,nb_tasks)++;
	if (!oldnbtasks && ma_topo_vpdata_l(vp,main_is_waiting))
		a_new_thread = tbx_true;

	_ma_raw_spin_unlock(&ma_topo_vpdata_l(vp,threadlist_lock));
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
}

void marcel_one_task_less(marcel_t pid)
{
	unsigned vpnum;
	unsigned nb_tasks, main_is_waiting;
	struct marcel_topo_level *vp;

	vpnum = pid->number/MA_MAX_VP_THREADS;
	MA_BUG_ON(vpnum >= marcel_nbvps());
	vp = &marcel_topo_vp_level[vpnum];

	ma_spin_lock_softirq(&ma_topo_vpdata_l(vp, threadlist_lock));

	tbx_fast_list_del(&pid->all_threads);

	/** main_thread is not in nb_tasks **/
	main_is_waiting = ma_topo_vpdata_l(vp,main_is_waiting);
	if (tbx_likely(pid != __main_thread))
		nb_tasks = --(ma_topo_vpdata_l(vp,nb_tasks)) ;
	else
		nb_tasks = ma_topo_vpdata_l(vp,nb_tasks);

	ma_spin_unlock_softirq(&ma_topo_vpdata_l(vp, threadlist_lock));

	if (0 == nb_tasks && main_is_waiting)
		ma_wake_up_thread(__main_thread);
}

static __tbx_inline__ int want_to_see(marcel_t t, int which)
{
	if (t->detached == MARCEL_CREATE_DETACHED) {
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
		tbx_fast_list_for_each_entry(t, &ma_topo_vpdata_l(vp,all_threads), all_threads) {
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
		tbx_fast_list_for_each_entry(t, &ma_topo_vpdata_l(vp,all_threads), all_threads)
			(*f)(t);
		ma_spin_unlock_softirq(&ma_topo_vpdata_l(vp,threadlist_lock));
	}
}

// Attend que toutes les taches soient terminees. Cette fonction
// _doit_ etre appelee par la tache "main".
void ma_wait_all_tasks_end(void)
{
	struct marcel_topo_level *vp;
	MARCEL_LOG_IN();

#ifdef MA__DEBUG
	if (ma_self() != __main_thread) {
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

	MARCEL_LOG_OUT();
}

void ma_gensched_shutdown(void)
{
#ifdef MA__LWPS
	marcel_lwp_t *lwp, *lwp_found;
	marcel_vpset_t vpset;
#endif

	MARCEL_LOG_IN();

	if(MARCEL_SELF != __main_thread)
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);

	marcel_exit_top();

#ifdef MA__BUBBLES
	ma_deactivate_idle_scheduler();
	ma_bubble_exit();
#endif

#ifdef MA__LWPS
	/* Stop timer before stopping kernel threads, to avoid running
	 * pthread_kill() in the middle of pthread_exit() */
	marcel_sig_stop_itimer();

	MARCEL_SCHED_LOG("blocking this LWP\n");
	ma_lwp_block(tbx_false);

	MARCEL_SCHED_LOG("stopping LWPs (1)\n");
	/* We must switch to the main kernel thread for proper
	 * termination. However, it may be currently a spare LWP, so we
	 * have to wait for it to become active. */
	while ((lwp = ma_lwp_wait_vp_active())) {
		if (lwp == &__main_lwp) {
			MARCEL_SCHED_LOG("main LWP is active, jumping to it at vp %d\n", ma_vpnum(lwp));
			vpset = MARCEL_VPSET_VP(ma_vpnum(lwp));
			/* To match ma_lwp_block() above */
			ma_preempt_enable();
			marcel_enable_vps(&vpset);
			marcel_apply_vpset(&vpset);
			MA_BUG_ON(MA_LWP_SELF != &__main_lwp);
			/* Ok, we're in the main kernel thread now */
			MARCEL_SCHED_LOG("block it too\n");
			ma_lwp_block(tbx_false);
		} else marcel_lwp_stop_lwp(lwp);
	}

	MA_BUG_ON(MA_LWP_SELF != &__main_lwp);

	MARCEL_SCHED_LOG("stopping LWPs (2)\n");
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
#endif
	marcel_sig_exit();

	MARCEL_LOG_OUT();
}

#ifdef MA__LWPS
static any_t TBX_NORETURN idle_poll_func(any_t hlwp)
{
	int dopoll;
	struct marcel_lwp *lwp;

	lwp = hlwp;
	if (lwp == NULL) {
		/* upcall_new_task est venue ici ? */
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
	}

        ma_set_thread_flag(TIF_POLLING_NRFLAG);
	while (1) {
		MA_BUG_ON(lwp != MA_LWP_SELF);

		if (tbx_unlikely(-1 == ma_vpnum(lwp))) {
			ma_clear_thread_flag(TIF_POLLING_NRFLAG);
			ma_smp_mb__after_clear_bit();
			if (!ma_get_need_resched())
				ma_lwp_wait_active();
			ma_set_thread_flag(TIF_POLLING_NRFLAG);
		}
		/* we are the active LWP of this VP */

		if (tbx_unlikely(marcel_vp_is_disabled(ma_vpnum(lwp)))) {
			int vp;

			ma_preempt_disable();
			vp = ma_vpnum(lwp);

		        /* FIXME: we still need a VP to account time, we always
			 * keep VP0 for that */
			if (vp != 0)
			        /* Not only sleep, but also disable the timer,
				 * to completely avoid disturbing the CPU */
			        marcel_sig_stop_perlwp_itimer();
			marcel_sig_disable_interrupts();
			while (marcel_vpset_isset(&marcel_disabled_vpset, vp) && !ma_get_need_resched())
				ma_sched_sig_pause();
			marcel_sig_enable_interrupts();
			if (vp != 0)
			        marcel_sig_reset_perlwp_timer();

			ma_preempt_enable_no_resched();
		}

		/* schedule threads */
		while (ma_get_need_resched()) {
			PROF_EVENT(idle_does_schedule);
			if (ma_schedule())
				continue;
		}

		/* no more threads, now poll */
		dopoll = ma_schedule_hooks(MARCEL_SCHEDULING_POINT_IDLE);

#ifdef MARCEL_IDLE_PAUSE
		ma_preempt_disable();

		if (dopoll)
			marcel_sig_nanosleep();
		else {
			marcel_sig_disable_interrupts();
			if (!ma_get_need_resched())
				ma_sched_sig_pause();
			marcel_sig_enable_interrupts();
		}

		ma_preempt_enable_no_resched();
#else
		if (!dopoll)
			ma_still_idle();
#endif
	}
}

/* Idle function for supplementary VPs: don't poll on idle */
static any_t idle_func(any_t hlwp TBX_UNUSED)
{
	ma_set_thread_flag(TIF_POLLING_NRFLAG);
	while (1) {
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

#  ifndef __INTEL_COMPILER
	return NULL;
#  endif
}
#endif

static void marcel_sched_lwp_init(marcel_lwp_t* lwp)
{
#ifdef MA__LWPS
	int num = ma_vpnum(lwp);
	marcel_attr_t attr;
	char name[MARCEL_MAXNAMESIZE];
#endif
	MARCEL_LOG_IN();

	if (!ma_is_first_lwp(lwp))
		/* run_task DOIT d�marrer en contexte d'irq */
		ma_per_lwp(run_task, lwp)->preempt_count=MA_HARDIRQ_OFFSET+MA_PREEMPT_OFFSET;
	else
		/* ajout du thread principal � la liste des threads */
		tbx_fast_list_add(&SELF_GETMEM(all_threads),&ma_topo_vpdata(0,all_threads));

#ifdef MA__LWPS
	/*****************************************/
	/* Cr�ation de la t�che Idle (idle_task) */
	/*****************************************/
	marcel_attr_init(&attr);
	marcel_snprintf(name,MARCEL_MAXNAMESIZE,"idle/%2d",ma_vpnum(lwp));
	marcel_attr_setname(&attr,name);
	marcel_attr_setdetachstate(&attr, tbx_true);
	marcel_attr_setflags(&attr, MA_SF_POLL|MA_SF_NORUN);
#ifdef PM2
	{
		char *stack = marcel_malloc(2*THREAD_SLOT_SIZE) ;
		
		marcel_attr_setstackaddr(&attr, (void*)((unsigned long)(stack + THREAD_SLOT_SIZE) & ~(THREAD_SLOT_SIZE-1)));
	}
#endif
	marcel_attr_setprio(&attr, MA_IDLE_PRIO);
	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ,2),-1,&ma_per_lwp(dontsched_runqueue,lwp));
	marcel_snprintf(name,sizeof(name),"dontsched%d",num);
	ma_init_rq(ma_dontsched_rq(lwp),name);
	marcel_vpset_zero(&ma_dontsched_rq(lwp)->vpset);
	marcel_attr_setschedrq(&attr, ma_dontsched_rq(lwp));
	marcel_create_special(&(ma_per_lwp(idle_task, lwp)), &attr,
			      ma_vpnum(lwp) == -1 || ma_vpnum(lwp)<(int)marcel_nbvps()?idle_poll_func:idle_func,
			      (void*)(ma_lwp_t)lwp);
	MTRACE("IdleTask", ma_per_lwp(idle_task, lwp));
#endif

	MARCEL_LOG_OUT();
}


static void marcel_sched_lwp_start(ma_lwp_t lwp LWPS_VAR_UNUSED)
{
        MARCEL_LOG_IN();
	MA_BUG_ON(!ma_in_irq());

#ifdef MA__LWPS
	marcel_wake_up_created_thread(ma_per_lwp(idle_task,lwp));
#endif

	ma_irq_exit();
	MA_BUG_ON(ma_in_atomic());
	ma_preempt_count()=0;
	ma_barrier();

	MARCEL_LOG_OUT();
}

MA_DEFINE_LWP_NOTIFIER_START_PRIO(generic_sched, MA_NOTIFIER_PRIO_RUNTASKS, "Sched generic",
				  marcel_sched_lwp_init, "Creation de idle",
				  marcel_sched_lwp_start, "Reveil de idle et demarrage de la preemption");

MA_LWP_NOTIFIER_CALL_UP_PREPARE(generic_sched, MA_INIT_GENSCHED_IDLE);
MA_LWP_NOTIFIER_CALL_ONLINE_PRIO(generic_sched, MA_INIT_GENSCHED_PREEMPT, MA_INIT_GENSCHED_PREEMPT_PRIO);

#ifdef MA__LWPS
static void marcel_gensched_start_lwps(void)
{
        unsigned int i;
	MARCEL_LOG_IN();
	for(i=1; i<marcel_nbvps(); i++)
		marcel_lwp_add_vp(MARCEL_VPSET_VP(i));
	MARCEL_SCHED_LOG("marcel_sched_init  : %i lwps created\n", marcel_nbvps());
	MARCEL_LOG_OUT();
}

__ma_initfunc(marcel_gensched_start_lwps, MA_INIT_GENSCHED_START_LWPS, "Creation et demarrage des LWPs");
#endif /* MA__LWPS */
