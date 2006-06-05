
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
/* Liste des sections
 */
#section sched_marcel_functions [no-include-in-main,no-include-in-master-section]

#section sched_marcel_inline [no-include-in-main,no-include-in-master-section]
#depend "[sched_marcel_functions]"

/****************************************************************/
/* Include
 */
#section common
/* inclusion du fichier scheduler particulier */
#define MARCEL_SCHED_INTERNAL_INCLUDE
#depend "scheduler/marcel_sched.h[]"
#undef MARCEL_SCHED_INTERNAL_INCLUDE

/****************************************************************/
#section functions
/* ==== `sleep' functions ==== */

int marcel_usleep(unsigned long usec);
#define marcel_delay(millisec) marcel_usleep((millisec)*1000)

/* ==== scheduler status ==== */

unsigned marcel_nbthreads(void);
unsigned long marcel_createdthreads(void);

#section marcel_functions
void marcel_gensched_shutdown(void);

void marcel_one_task_less(marcel_t pid);
void marcel_one_more_task(marcel_t pid);

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
#define SLEEPING_ONLY           64
#define NOT_SLEEPING_ONLY      128

#section functions
void marcel_threadslist(int max, marcel_t *pids, int *nb, int which);

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
DEC_MARCEL_GNU(int, yield, (void) __THROW);

/****************************************************************/
/*               Scheduler et threads                           */
/****************************************************************/

#section marcel_types
typedef struct marcel_sched_task marcel_sched_task_t;

#section marcel_structures
#depend "scheduler/marcel_sched.h[marcel_types]"
struct marcel_sched_task {
	struct marcel_lwp *lwp; /* LWP sur lequel s'ex�cute la t�che */
	unsigned long lwps_allowed; /* Contraintes sur le placement sur les LWP */
	unsigned int state; /* �tat du thread */
	marcel_sched_internal_task_t internal;
};

#section marcel_macros
/* Ces valeurs repr�sentent ce que l'utilisateur voudrait comme �tat */

#define MA_TASK_RUNNING		0 /* _doit_ rester 0 */
#define MA_TASK_INTERRUPTIBLE	1
#define MA_TASK_UNINTERRUPTIBLE	2
//#define MA_TASK_STOPPED	4 pas utilis�
//#define MA_TASK_TRACED	  pas utilis�
//#define MA_TASK_ZOMBIE	8 pas utilis�
#define MA_TASK_DEAD		16
//#define MA_TASK_GHOST		32 pas utilis�
#define MA_TASK_MOVING		64
#define MA_TASK_FROZEN		128
#define MA_TASK_BORNING		256

#define __ma_set_task_state(tsk, state_value)		\
	do { (tsk)->sched.state = (state_value); } while (0)
#define ma_set_task_state(tsk, state_value)		\
	ma_set_mb((tsk)->sched.state, (state_value))

#define __ma_set_current_state(state_value)			\
	do { SELF_GETMEM(sched).state = (state_value); } while (0)
#define ma_set_current_state(state_value)		\
	ma_set_mb(SELF_GETMEM(sched).state, (state_value))

#define MA_TASK_IS_FROZEN(tsk) (!(tsk)->sched.state == MA_TASK_FROZEN)

#section sched_marcel_functions
__tbx_inline__ static void 
marcel_sched_init_marcel_thread(marcel_task_t* __restrict t,
				const marcel_attr_t* __restrict attr);
#section sched_marcel_inline
__tbx_inline__ static void 
marcel_sched_init_marcel_thread(marcel_task_t* __restrict t,
				const marcel_attr_t* __restrict attr)
{
	//t->lwp
	t->sched.lwps_allowed = ~attr->vpmask; 
	ma_set_task_state(t, MA_TASK_BORNING);
	marcel_sched_internal_init_marcel_thread(t, &t->sched.internal, attr);
}

#section sched_marcel_functions
/* D�marrage d'un thread par le scheduler */
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
#define TIF_RT_THREAD           12      /* ce thread doit �tre
					 * ordonnanc� en tant que
					 * thread "Real Time" */
#define TIF_RUNTASK             13      /* ce thread d�marre un lwp */

#define _TIF_UPCALL_NEW         (1<<TIF_UPCALL_NEW)
#define _TIF_POLL               (1<<TIF_POLL)
#define _TIF_IDLE               (1<<TIF_IDLE)
#define _TIF_NORUN              (1<<TIF_NORUN)
#define _TIF_RT_THREAD          (1<<TIF_RT_THREAD)
#define _TIF_RUNTASK            (1<<TIF_RUNTASK)


// NORMAL : Thread marcel "tout bete"
//#define MA_SF_NORMAL       0
// UPCALL_NEW : no comment
#define MA_SF_UPCALL_NEW   _TIF_UPCALL_NEW
// POLL : le thread "idle_task" qui fait plein de choses
#define MA_SF_POLL         _TIF_POLL
// IDLE : le thread "wait_and_yield" ne consomme pas de CPU...
#define MA_SF_IDLE         _TIF_IDLE
// NORUN : ne pas prendre en compte ce thread dans le calcul des
// taches actives
#define MA_SF_NORUN        _TIF_NORUN
// NOSCHEDLOCK : ne pas appeler "sched_lock" dans insert_task...
//#define MA_SF_NOSCHEDLOCK  16
// RT_THREAD : ce thread doit �tre ordonnanc� en tant que thread "Real Time"
#define MA_SF_RT_THREAD    _TIF_RT_THREAD
// RUNTASK : ce thread d�marre un lwp
#define MA_SF_RUNTASK      _TIF_RUNTASK

//#define MA_TASK_TYPE_NORMAL       MA_SF_NORMAL
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

#section types
#section marcel_inline
#section variables
