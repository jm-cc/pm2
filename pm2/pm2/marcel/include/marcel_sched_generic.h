
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

void marcel_delay(unsigned long millisecs);

/* ==== scheduler status ==== */

unsigned marcel_nbthreads(void);
unsigned long marcel_createdthreads(void);

#section marcel_functions
asmlinkage void marcel_sched_do_work(void);

void marcel_gensched_shutdown(void);

//extern int FASTCALL(marcel_wake_up_state(marcel_task_t * tsk, unsigned int state));
//extern int FASTCALL(marcel_wake_up_thread(marcel_task_t * tsk));
//extern int FASTCALL(marcel_wake_up_thread_kick(task_t * tsk));
//extern void FASTCALL(marcel_wake_up_created_thread(marcel_task_t * tsk));
//extern void FASTCALL(marcel_sched_thread_exit(marcel_task_t * p));

void marcel_one_task_less(marcel_t pid);
void marcel_one_more_task(marcel_t pid);

#section macros
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
                { /*marcel_sched_do_work();*/ get; } )

#section functions
/* ==== explicit preemption ==== */
DEC_MARCEL_POSIX(int, yield, (void) __THROW)

/****************************************************************/
/*               Scheduler et threads                           */
/****************************************************************/

#section marcel_variables
MA_DECLARE_PER_LWP(marcel_task_t *,idle_task);


#section marcel_types
typedef struct marcel_sched_task marcel_sched_task_t;

#section marcel_structures
struct marcel_sched_task {
	struct marcel_lwp *lwp; /* LWP sur lequel s'exécute la tâche */
	unsigned long lwps_allowed; /* Contraintes sur le placement sur les LWP */
	unsigned int state; /* État du thread */
	struct marcel_sched_internal_task internal;
};

#section marcel_macros
//#define MARCEL_SET_STATE(t, STATE) 
//        t->sched.state=MARCEL_TASK_##STATE
#section sched_marcel_functions
inline static void 
marcel_sched_init_marcel_thread(marcel_task_t* t,
				const marcel_attr_t* attr);
#section sched_marcel_inline
inline static void 
marcel_sched_init_marcel_thread(marcel_task_t* t,
				const marcel_attr_t* attr)
{
	//t->lwp
	t->sched.lwps_allowed = ~attr->vpmask; 
	ma_set_task_state(t, MA_TASK_RUNNING);
	marcel_sched_internal_init_marcel_thread(t, attr);
}

#section sched_marcel_functions
/* Démarrage d'un thread par le scheduler */
inline static int marcel_sched_create(marcel_task_t* cur,
				      marcel_task_t* new_task,
				      __const marcel_attr_t *attr,
				      __const int special_mode,
				      __const unsigned long base_stack);
#section sched_marcel_inline
inline static int marcel_sched_create(marcel_task_t* cur,
				      marcel_task_t* new_task,
				      __const marcel_attr_t *attr,
				      __const int special_mode,
				      __const unsigned long base_stack)
{
	LOG_IN();
	LOG_RETURN(marcel_sched_internal_create(cur, new_task, attr, 
						special_mode, base_stack));
}

/****************************************************************/
/*               Virtual Processors                             */
/****************************************************************/

#section types
/* VP mask: useful for selecting the set of "forbiden" LWP for a given thread */
typedef unsigned long marcel_vpmask_t;

#section macros

// Primitives & macros de construction de "masques" de processeurs
// virtuels. 

// ATTENTION : le placement d un thread est autorise sur un 'vp' si le
// bit correspondant est a _ZERO_ dans le masque (similitude avec
// sigset_t pour la gestion des masques de signaux).

#define MARCEL_VPMASK_EMPTY          ((marcel_vpmask_t)0)
#define MARCEL_VPMASK_FULL           ((marcel_vpmask_t)-1)
#define MARCEL_VPMASK_ONLY_VP(vp)    ((marcel_vpmask_t)(1U << (vp)))
#define MARCEL_VPMASK_ALL_BUT_VP(vp) ((marcel_vpmask_t)(~(1U << (vp))))

#define marcel_vpmask_init(m)        marcel_vpmask_empty(m)

#section functions
static __inline__ void marcel_vpmask_empty(marcel_vpmask_t *mask);
#section inline
static __inline__ void marcel_vpmask_empty(marcel_vpmask_t *mask)
{
  *mask = MARCEL_VPMASK_EMPTY;
}
#section functions
static __inline__ void marcel_vpmask_fill(marcel_vpmask_t *mask);
#section inline
static __inline__ void marcel_vpmask_fill(marcel_vpmask_t *mask)
{
  *mask = MARCEL_VPMASK_FULL;
}
#section functions
static __inline__ void marcel_vpmask_add_vp(marcel_vpmask_t *mask,
					    unsigned vp);
#section inline
static __inline__ void marcel_vpmask_add_vp(marcel_vpmask_t *mask,
					    unsigned vp)
{
  *mask |= 1U << vp;
}

#section functions
static __inline__ void marcel_vpmask_only_vp(marcel_vpmask_t *mask,
					     unsigned vp);
#section inline
static __inline__ void marcel_vpmask_only_vp(marcel_vpmask_t *mask,
					     unsigned vp)
{
  *mask = 1U << vp;
}

#section functions
static __inline__ void marcel_vpmask_del_vp(marcel_vpmask_t *mask,
					    unsigned vp);
#section inline
static __inline__ void marcel_vpmask_del_vp(marcel_vpmask_t *mask,
					    unsigned vp)
{
  *mask &= ~(1U << vp);
}

#section functions
static __inline__ void marcel_vpmask_all_but_vp(marcel_vpmask_t *mask,
						unsigned vp);
#section inline
static __inline__ void marcel_vpmask_all_but_vp(marcel_vpmask_t *mask,
						unsigned vp)
{
  *mask = ~(1U << vp);
}

#section functions
static __inline__ int marcel_vpmask_vp_ismember(marcel_vpmask_t mask,
						unsigned vp);
#section inline
static __inline__ int marcel_vpmask_vp_ismember(marcel_vpmask_t mask,
						unsigned vp)
{
  return 1 & (mask >> vp);
}

#section functions
static __inline__ unsigned marcel_current_vp(void);
#section inline
#depend "sys/marcel_lwp.h[marcel_variables]"
static __inline__ unsigned marcel_current_vp(void)
{
  return LWP_NUMBER(LWP_SELF);
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
// RT_THREAD : ce thread doit être ordonnancé en tant que thread "Real Time"
#define MA_SF_RT_THREAD    _TIF_RT_THREAD
// RUNTASK : ce thread démarre un lwp
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
