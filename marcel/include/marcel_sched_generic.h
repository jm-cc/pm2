
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

#section common
#include "tbx_compiler.h"

/****************************************************************/
/* List of sections
 */
#section sched_marcel_functions [no-include-in-main,no-include-in-master-section]

#section sched_marcel_inline [no-include-in-main,no-include-in-master-section]
#depend "[sched_marcel_functions]"

/****************************************************************/
/* Include
 */
#section common
/* include the specific scheduler file */
#define MARCEL_SCHED_INTERNAL_INCLUDE
#depend "scheduler/marcel_sched.h[]"
#undef MARCEL_SCHED_INTERNAL_INCLUDE

/****************************************************************/
#section functions
/* ==== `sleep' functions ==== */
int pmarcel_nanosleep(const struct timespec *rqtp,struct timespec *rmtp);
/** \fn int marcel_nanosleep(const struct timespec *rqtp,struct timespec *rmtp)
 */
DEC_MARCEL_POSIX(int,nanosleep,(const struct timespec *rqtp,struct timespec *rmtp));

int pmarcel_usleep(unsigned long usec);
/** \fn int marcel_usleep(unsigned long usec)
 */
DEC_MARCEL_POSIX(int,usleep,(unsigned long usec));

int pmarcel_sleep(unsigned long sec);
/** \fn int marcel_sleep(unsigned long sec)
 */
DEC_MARCEL_POSIX(int,sleep,(unsigned long sec));

/** \brief Pause the calling thread for a given number of milliseconds */
int marcel_delay(unsigned long msec);
#define marcel_delay(millisec) marcel_usleep((millisec)*1000)

/* ==== scheduler status ==== */

/** \brief Return the number of threads created by all the
 * vps that are still alive (sum of marcel_topo_level::nb_tasks)
 */
unsigned marcel_nbthreads(void);

/** \brief Return the number of threads that have been created on all the
 * vps including dead threads (sum of marcel_topo_level::task_number)
 */
unsigned long marcel_createdthreads(void);

#section marcel_functions
/** \brief Wait for all application threads to terminate.  */
void ma_wait_all_tasks_end(void);
/** \brief Handler for graceful termination of a marcel session */
void ma_gensched_shutdown(void);

#ifdef MA__LWPS
/** \brief Called by schedule before switching to idle thread. Last chance for
 * PIOMan to avoid switching to the idle Thread. Preemption and bottom halves
 * are disabled. Useless in mono since there's no actual switch to mono.
 */
static __tbx_inline__ void ma_about_to_idle(void);
#endif

/** \brief Called by scheduler when having switched to idle thread.  Preemption is
 * disabled
 */
static __tbx_inline__ void ma_entering_idle(void);

/** \brief Called by scheduler when idle calls schedule().  Preemption is disabled.
 */
static __tbx_inline__ void ma_still_idle(void);

/** \brief Called by scheduler just before switching from idle thread.  Preemption is
 * disabled
 */
static __tbx_inline__ void ma_leaving_idle(void);

#section marcel_inline
#ifdef MA__LWPS
static __tbx_inline__ void ma_about_to_idle(void) {
#  ifdef PIOMAN
	/* TODO: appeler PIOMan */
#  endif
}
#endif

#ifdef MA__LWPS
/* To be called from idle only, call marcel_sig_pause after making sure that
 * we have announced other LWPs that we stopped polling */
static __tbx_inline__ void ma_sched_sig_pause(void) {
	MA_BUG_ON(MARCEL_SELF != __ma_get_lwp_var(idle_task));
	/* Tell other LWPs that we will stop polling need_resched */
	ma_clear_thread_flag(TIF_POLLING_NRFLAG);
	/* Make sure people see that we won't poll any more after that */
	ma_smp_mb__after_clear_bit();
	/* Make a last check once we've announced that */
	if (!ma_get_need_resched())
		marcel_sig_pause();
	/* Either we have need_resched or got a signal, re-announce that we
	 * will be polling */
	ma_set_thread_flag(TIF_POLLING_NRFLAG);
}
#endif

static __tbx_inline__ void ma_entering_idle(void) {
#ifdef MA__LWPS
	PROF_EVENT1(sched_idle_start,ma_vpnum(MA_LWP_SELF));
#  ifdef MARCEL_SMT_IDLE
	if (!(ma_preempt_count() & MA_PREEMPT_ACTIVE)) {
		marcel_sig_disable_interrupts();
		ma_topology_lwp_idle_start(MA_LWP_SELF);
		if (!(ma_topology_lwp_idle_core(MA_LWP_SELF)))
			ma_sched_sig_pause();
		marcel_sig_enable_interrupts();
	}
#  endif
#endif
#ifdef PIOMAN
	/* TODO: appeler PIOMan */
#endif
}

static __tbx_inline__ void ma_still_idle(void) {
#ifdef MA__LWPS
#  ifdef MARCEL_SMT_IDLE
	if (!(ma_preempt_count() & MA_PREEMPT_ACTIVE)) {
		marcel_sig_disable_interrupts();
		if (!ma_topology_lwp_idle_core(MA_LWP_SELF))
			ma_sched_sig_pause();
		marcel_sig_enable_interrupts();
	}
#  endif
#endif
}

static __tbx_inline__ void ma_leaving_idle(void) {
#ifdef MA__LWPS
	PROF_EVENT1(sched_idle_stop, ma_vpnum(MA_LWP_SELF));
#  ifdef MARCEL_SMT_IDLE
	ma_topology_lwp_idle_end(MA_LWP_SELF);
#  endif
#endif
#ifdef PIOMAN
	/* TODO: appeler PIOMan */
#endif
}

#section marcel_functions

/** \brief Update the vp struct each time a task is created
 * (also called for tasks created through end_hibernation
 */
void marcel_one_more_task(marcel_t pid);

/** \brief Update the vp struct each time a task is destructed */
void marcel_one_task_less(marcel_t pid);

#section structures
/* ==== snapshot ==== */

typedef void (*snapshot_func_t)(marcel_t pid);

#section functions
void marcel_snapshot(snapshot_func_t f);

#section macros
#define ALL_THREADS		 0
#define MIGRATABLE_ONLY		 1
#define NOT_MIGRATABLE_ONLY	 2
#define DETACHED_ONLY		 4
#define NOT_DETACHED_ONLY	 8
#define BLOCKED_ONLY		16
#define NOT_BLOCKED_ONLY	32
#define READY_ONLY           64
#define NOT_READY_ONLY      128

#section functions
void marcel_threadslist(int max, marcel_t *pids, int *nb, int which);
void marcel_per_lwp_threadslist(int max, marcel_t *pids, int *nb, int which);
unsigned marcel_per_lwp_nbthreads(void);
int pmarcel_sched_get_priority_max(int policy);
DEC_MARCEL_POSIX(int,sched_get_priority_max,(int policy) __THROW);
int pmarcel_sched_get_priority_min(int policy);
DEC_MARCEL_POSIX(int,sched_get_priority_min,(int policy) __THROW);

#section marcel_functions
extern TBX_EXTERN signed long FASTCALL(ma_schedule_timeout(signed long timeout));
extern void ma_process_timeout(unsigned long __data);

#section macros
#define TIMED_SLEEP_ON_STATE_CONDITION_RELEASING(STATE, cond, release, get, timeout) \
	while((cond)) { \
		ma_set_current_state(MA_TASK_##STATE); \
		release; \
		ma_schedule_timeout(timeout*1000/marcel_gettimeslice()); \
		get; \
	}

#define SLEEP_ON_STATE_CONDITION_RELEASING(STATE, cond, release, get) \
	while((cond)) { \
		ma_set_current_state(MA_TASK_##STATE); \
		release; \
		ma_schedule(); \
		get; \
	}

#define INTERRUPTIBLE_SLEEP_ON_CONDITION_RELEASING(cond, release, get) \
        SLEEP_ON_STATE_CONDITION_RELEASING( \
                INTERRUPTIBLE, \
                cond, \
                release, \
                { ma_check_work(); get; } )

#section functions
/* ==== explicit preemption ==== */
int marcel_yield_to(marcel_t next);
DEC_MARCEL_POSIX(int, yield, (void) __THROW);
int marcel_yield_to_team(marcel_t *team, char *mask, unsigned padding, unsigned nb_teammates);

/****************************************************************/
/*               Scheduler et threads                           */
/****************************************************************/

#section marcel_macros
/* Ces valeurs représentent ce que l'utilisateur voudrait comme état */

#define MA_TASK_RUNNING		0 /* _doit_ rester 0 */
#define MA_TASK_INTERRUPTIBLE	1
#define MA_TASK_UNINTERRUPTIBLE	2
#define MA_TASK_DEAD		16
#define MA_TASK_MOVING		64
#define MA_TASK_FROZEN		128
#define MA_TASK_BORNING		256

#define __ma_set_task_state(tsk, state_value)		\
	do { (tsk)->state = (state_value); } while (0)
#define ma_set_task_state(tsk, state_value)		\
	ma_set_mb((tsk)->state, (state_value))

#define __ma_set_current_state(state_value)			\
	do { SELF_GETMEM(state) = (state_value); } while (0)
#define ma_set_current_state(state_value)		\
	ma_set_mb(SELF_GETMEM(state), (state_value))

#define MA_TASK_IS_FROZEN(tsk) (!(tsk)->state == MA_TASK_FROZEN)

#section sched_marcel_functions
__tbx_inline__ static void 
marcel_sched_init_marcel_thread(marcel_task_t* __restrict t,
				const marcel_attr_t* __restrict attr);
#section sched_marcel_inline
__tbx_inline__ static void 
marcel_sched_init_marcel_thread(marcel_task_t* __restrict t,
				const marcel_attr_t* __restrict attr)
{
	/* t->lwp */
	if (attr->topo_level)
		t->vpset = attr->topo_level->vpset;
	else if (attr->__cpuset)
		t->vpset = *attr->__cpuset; 
	else
		t->vpset = attr->vpset; 
	ma_set_task_state(t, MA_TASK_BORNING);
	marcel_sched_internal_init_marcel_thread(t, attr);
}

#section marcel_functions
__tbx_inline__ static void 
marcel_get_vpset(marcel_task_t* __restrict t, marcel_vpset_t *vpset);
#section marcel_inline
__tbx_inline__ static void 
marcel_get_vpset(marcel_task_t* __restrict t, marcel_vpset_t *vpset)
{
	     *vpset = t->vpset;
}

#section sched_marcel_functions
/* Démarrage d'un thread par le scheduler */
__tbx_inline__ static int marcel_sched_create(marcel_task_t* __restrict cur,
				      marcel_task_t* __restrict new_task,
				      __const marcel_attr_t * __restrict attr,
				      __const int dont_schedule,
				      __const unsigned long base_stack);
#section sched_marcel_inline
__tbx_inline__ static int marcel_sched_create(marcel_task_t* __restrict cur,
				      marcel_task_t* __restrict new_task,
				      __const marcel_attr_t * __restrict attr,
				      __const int dont_schedule,
				      __const unsigned long base_stack)
{
	LOG_IN();
	LOG_RETURN(marcel_sched_internal_create(cur, new_task, attr, 
						dont_schedule, base_stack));
}

/****************************************************************/
/*               Scheduler et TIF                               */
/****************************************************************/


#section marcel_macros
/* Manipulation des champs de task->flags */

#define TIF_UPCALL_NEW          8       /* no comment */
#define TIF_POLL                9       /* le thread "idle_task" qui
					 * fait plein de choses */
#define TIF_IDLE                10      /* le thread "wait_and_yield"
					 * ne consomme pas de
					 * CPU... */
#define TIF_NORUN               11      /* ne pas prendre en compte ce
					 * thread dans le calcul des
					 * taches actives */
#define TIF_RT_THREAD           12      /* ce thread doit être
					 * ordonnancé en tant que
					 * thread "Real Time" */
#define TIF_RUNTASK             13      /* ce thread démarre un lwp */

#define _TIF_UPCALL_NEW         (1<<TIF_UPCALL_NEW)
#define _TIF_POLL               (1<<TIF_POLL)
#define _TIF_IDLE               (1<<TIF_IDLE)
#define _TIF_NORUN              (1<<TIF_NORUN)
#define _TIF_RT_THREAD          (1<<TIF_RT_THREAD)
#define _TIF_RUNTASK            (1<<TIF_RUNTASK)


/*  NORMAL : Thread marcel "tout bete" */
/* #define MA_SF_NORMAL       0 */
/*  UPCALL_NEW : no comment */
#define MA_SF_UPCALL_NEW   _TIF_UPCALL_NEW
/*  POLL : le thread "idle_task" qui fait plein de choses */
#define MA_SF_POLL         _TIF_POLL
/*  IDLE : le thread "wait_and_yield" ne consomme pas de CPU... */
#define MA_SF_IDLE         _TIF_IDLE
/*  NORUN : ne pas prendre en compte ce thread dans le calcul des */
/*  taches actives */
#define MA_SF_NORUN        _TIF_NORUN
/*  RT_THREAD : ce thread doit être ordonnancé en tant que thread "Real Time" */
#define MA_SF_RT_THREAD    _TIF_RT_THREAD
/*  RUNTASK : ce thread démarre un lwp */
#define MA_SF_RUNTASK      _TIF_RUNTASK

/* #define MA_TASK_TYPE_NORMAL       MA_SF_NORMAL */
#define MA_TASK_TYPE_UPCALL_NEW   MA_SF_UPCALL_NEW
#define MA_TASK_TYPE_POLL         MA_SF_POLL
#define MA_TASK_TYPE_IDLE         MA_SF_IDLE
#define MA_TASK_TYPE_RUNTASK      MA_SF_RUNTASK

#define MA_GET_TASK_TYPE(task) \
         (((task)->flags) & (_TIF_UPCALL_NEW|_TIF_POLL|_TIF_IDLE))

#define IS_TASK_TYPE_UPCALL_NEW(task) \
   ma_test_ti_thread_flag(task, TIF_UPCALL_NEW)

#define IS_TASK_TYPE_IDLE(task) \
   (ma_test_ti_thread_flag(task, TIF_POLL) | \
   ma_test_ti_thread_flag(task, TIF_IDLE))

#define IS_TASK_TYPE_RUNTASK(task) \
   ma_test_ti_thread_flag(task, TIF_RUNTASK)

#define MA_TASK_NOT_COUNTED_IN_RUNNING(task) \
   ma_test_ti_thread_flag(task, TIF_NORUN)

#define MA_TASK_NO_USE_SCHEDLOCK(task) \
   ma_test_ti_thread_flag(task, TIF_NOSCHEDLOCK)

#define MA_TASK_REAL_TIME(task) \
   ma_test_ti_thread_flag(task, TIF_RT_THREAD)

/* Is the current VP in a polling loop ? */
#define MA_VP_IS_POLLING(vp) \
   ma_test_tsk_thread_flag(ma_per_lwp(current_thread, GET_LWP_BY_NUM(vp)), TIF_POLLING_NRFLAG)

#section types
#section marcel_inline
#section variables
