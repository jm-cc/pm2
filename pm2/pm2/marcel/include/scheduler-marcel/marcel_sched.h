
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

#section default [no-include-in-main,no-include-in-master-section]

#section sched_marcel_functions [no-depend-previous]

#section sched_marcel_inline

/****************************************************************/
/* Protections
 */
#section common
#ifndef MARCEL_SCHED_INTERNAL_INCLUDE
#error This file should not be directely included. Use "marcel_sched.h" instead
#endif
/****************************************************************/

#section marcel_macros

#section marcel_types
typedef struct marcel_sched_internal_task marcel_sched_internal_task_t;
#section marcel_structures
#depend "pm2_list.h"
#depend "scheduler/linux_sched.h[marcel_types]"
struct marcel_sched_internal_task {
	struct list_head run_list;              /* List chainée des threads prêts */
	//unsigned long lwps_runnable;             
#ifdef MA__LWPS
	ma_runqueue_t *init_rq,*cur_rq;
#endif
	int sched_policy;
	int prio;
	ma_prio_array_t *array;
};

#section marcel_functions
int ma_wake_up_task(marcel_task_t * p);

#section sched_marcel_functions
inline static void 
marcel_sched_internal_init_marcel_thread(marcel_task_t* t, 
					 const marcel_attr_t *attr);
#section sched_marcel_inline
#include <asm/bitops.h>
inline static void 
marcel_sched_internal_init_marcel_thread(marcel_task_t* t, 
					 const marcel_attr_t *attr)
{
	LOG_IN();
	INIT_LIST_HEAD(&t->sched.internal.run_list);
	//t->sched.internal.lwps_runnable=~0UL;
#ifdef MA__LWPS
	if (attr->vpmask==MARCEL_VPMASK_EMPTY)
		t->sched.internal.init_rq=&ma_main_runqueue;
	else if (attr->vpmask==MARCEL_VPMASK_FULL) {
		t->sched.internal.init_rq=&ma_idle_runqueue;
		t->sched.internal.prio=MAX_PRIO-1;
	} else {
		int first_vp;
		first_vp=ma_ffz(attr->vpmask);
		MA_BUG_ON(attr->vpmask!=MARCEL_VPMASK_ALL_BUT_VP(first_vp));
		MA_BUG_ON(first_vp && first_vp>=marcel_nbvps());
		t->sched.internal.init_rq=ma_lwp_rq(GET_LWP_BY_NUM(first_vp));
	}
	t->sched.internal.cur_rq=NULL;
#endif
	t->sched.internal.sched_policy = attr->__schedpolicy;
#warning RT_TASK à ajuster
	t->sched.internal.prio=0;
	t->sched.internal.array=NULL;
	LOG_OUT();
}



#section marcel_functions
//unsigned marcel_sched_add_vp(void);

#section marcel_macros
/* ==== SMP scheduling policies ==== */

#define MARCEL_MAX_USER_SCHED    16

#define MARCEL_SCHED_OTHER       0
#define MARCEL_SCHED_AFFINITY    1
#define MARCEL_SCHED_BALANCE     2
#define __MARCEL_SCHED_AVAILABLE 3

#section marcel_types
#depend "sys/marcel_lwp.h[marcel_types]"
#depend "marcel_threads.h[types]"
typedef marcel_lwp_t *(*marcel_schedpolicy_func_t)(marcel_t pid,
						   marcel_lwp_t *current_lwp);
#section marcel_functions
void marcel_schedpolicy_create(int *policy, marcel_schedpolicy_func_t func);

/* ==== SMP scheduling directives ==== */

void marcel_change_vpmask(marcel_vpmask_t mask);


/* ==== `sleep' functions ==== */

void marcel_delay(unsigned long millisecs);


/* ==== scheduler status ==== */

unsigned marcel_nbvps(void);
unsigned marcel_nbthreads(void);
unsigned marcel_activethreads(void);
unsigned marcel_sleepingthreads(void);
unsigned marcel_blockedthreads(void);
unsigned marcel_frozenthreads(void);
unsigned long marcel_createdthreads(void);


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


#section marcel_variables
/* ==== Scheduler locking ==== */

#ifdef MARCEL_RT
extern unsigned __rt_task_exist;
#endif


#section marcel_functions

/* ==== miscelaneous private defs ==== */

#ifdef MA__DEBUG
extern void breakpoint();
#endif

int __marcel_tempo_give_hand(unsigned long timeout, boolean *blocked, marcel_sem_t *s);

marcel_t marcel_unchain_task_and_find_next(marcel_t t, 
						     marcel_t find_next);
void marcel_insert_task(marcel_t t);
marcel_t marcel_radical_next_task(void);
marcel_t marcel_give_hand_from_upcall_new(marcel_t cur, marcel_lwp_t *lwp);

int marcel_check_sleeping(void);

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Création d'un nouveau thread                                   */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

#section sched_marcel_functions
static inline int marcel_sched_internal_create(marcel_task_t *cur, 
					       marcel_task_t *new_task,
					       __const marcel_attr_t *attr,
					       __const int special_mode,
					       __const unsigned int base_stack);
#section sched_marcel_inline
static inline 
int marcel_sched_internal_create(marcel_task_t *cur, marcel_task_t *new_task,
				 __const marcel_attr_t *attr,
				 __const int special_mode,
				 __const unsigned int base_stack)
{ 
	LOG_IN();

	/* On est encore dans le père. On doit démarrer le fils */
#warning No auto start...
	ma_preempt_disable();
	if(
		// On ne peut pas céder la main maintenant
		(ma_in_atomic())
		// On n'a pas encore de scheduler...
		|| special_mode
#ifdef MA__LWPS
		// On ne peut pas placer ce thread sur le LWP courant
		|| (!lwp_isset(LWP_NUMBER(LWP_SELF), THREAD_GETMEM(new_task, sched.lwps_allowed)))
/* TODO: vieux code */
#if 0
#ifndef MA__ONE_QUEUE
		// Si la politique est du type 'placer sur le LWP le moins
		// chargé', alors on ne peut pas placer ce thread sur le LWP
		// courant
		|| (new_task->sched_policy != MARCEL_SCHED_OTHER)
#endif
#endif
#endif
		) {
		// Ici, le père _ne_doit_pas_ donner la main au fils
		// immédiatement car : 
		// - ou bien lock_task() a été appelé,
		// - ou bien le fils va être inséré sur un autre LWP,
		// La conséquence est que le thread fils est créé et
		// initialisé, mais pas "inséré" dans une quelconque
		// file pour l'instant.

		PROF_IN_EXT(newborn_thread);


		// Si on doit appeler marcel_run, le thread se
		// bloquera sur un sémaphore
		new_task->father->child = (special_mode?
					   NULL: /* On ne fait rien */
					   new_task); /* insert asap */

		/* On sauve l'état du père sachant qu'on va y revenir
		 * tout de suite 
		 */
		if(marcel_ctx_setjmp(cur->ctx_yield) == NORMAL_RETURN) {
			ma_preempt_enable();
			MTRACE("Father Restart", cur);
			LOG_OUT();
			return 0;
		}
		
		ma_set_task_lwp(new_task, LWP_SELF);
		/* Ne pas oublier de laisser de la place pour les
		 * variables locales/empilement de fonctions On prend
		 * la taille entre le plus haut argument de cette
		 * fonction dans la pile et la position courante
		 */
		marcel_ctx_set_new_stack(new_task, 
					 new_task->initial_sp-
					 (base_stack-get_sp()));

		LOG_IN();
		MTRACE("On new stack", marcel_self());
		
		PROF_OUT_EXT(newborn_thread);

		if(MA_THR_SETJMP(marcel_self()) == FIRST_RETURN) {
			// On rend la main au père
			call_ST_FLUSH_WINDOWS();
			marcel_ctx_longjmp(marcel_self()->father->ctx_yield,
					   NORMAL_RETURN);
		}
		MA_THR_RESTARTED(MARCEL_SELF, "Preemption");
		/* Drop preempt_count with ma_spin_unlock_softirq */
		ma_schedule_tail(MARCEL_SELF);
		
	} else {
		// Pas de démarrage direct pour l'instant. Il faudra
		// que le scheduler soit au courrant pour pouvoir le faire
		RAISE(PROGRAM_ERROR);
		// Cas le plus favorable (sur le plan de
		// l'efficacité) : le père sauve son contexte et on
		// démarre le fils immédiatement.
		// Note : si le thread est un 'real-time thread', cela
		// ne change rien ici...

		if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
			MA_THR_RESTARTED(cur, "Father Preemption");
			LOG_OUT();
			return 0;
		}

		PROF_SWITCH_TO(cur->number, new_task->number);
		
		PROF_IN_EXT(newborn_thread);
		
		new_task->father->child = NULL;
		
		//TODO:SET_STATE_RUNNING(NULL, new_task, GET_LWP(new_task));
		ma_wake_up_thread(new_task);
		
		marcel_ctx_set_new_stack(new_task, new_task->initial_sp -
					 (base_stack-get_sp()));
		//SET_STATE_READY(marcel_self()->father);
		
		MTRACE("Preemption", marcel_self());
		
		PROF_OUT_EXT(newborn_thread);
	}
	
	/* pas de unlock_task ici : le preempt a déjà été mangé dans ma_schedule_tail */
	//unlock_task();
	LOG_OUT();
	marcel_exit((*marcel_self()->f_to_call)(marcel_self()->arg));
	return 0;
}
#section marcel_macros

/* effectue un setjmp. On doit être RUNNING avant et après
 * */
#define MA_THR_SETJMP(current) \
  marcel_ctx_setjmp(current->ctx_yield)

/* On vient de reprendre la main. On doit déjà être RUNNING. On enlève
 * le flags RUNNING au thread qui tournait avant.
 * */
#define MA_THR_RESTARTED(current, info) \
  do {                                 \
    MTRACE(info, current);             \
  } while(0)

/* on effectue un longjmp. Le thread courrant ET le suivant doivent
 * être RUNNING. La variable previous_task doit être correctement
 * positionnée pour pouvoir annuler la propriété RUNNING du thread
 * courant.
 * */
#define MA_THR_LONGJMP(cur_num, next, ret) \
  do {                                     \
    PROF_SWITCH_TO(cur_num, next->number); \
    call_ST_FLUSH_WINDOWS();               \
    marcel_ctx_longjmp(next->ctx_yield, ret);              \
  } while(0)



#section marcel_variables
MA_DECLARE_PER_LWP(marcel_task_t *, previous_thread);

#section marcel_functions

inline static marcel_task_t *marcel_switch_to(marcel_task_t *cur, marcel_task_t *next);
#section marcel_inline

inline static marcel_task_t *marcel_switch_to(marcel_task_t *cur, marcel_task_t *next)
{
	if (cur != next) {
		MA_BUG_ON(!ma_in_atomic());
		if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
			MA_BUG_ON(!ma_in_atomic());
			MA_THR_RESTARTED(cur, "Preemption");
			return __ma_get_lwp_var(previous_thread);
		}
		debug_printf(&marcel_mdebug, "switchto(%p, %p) on LWP(%d)\n",
		       cur, next, LWP_NUMBER(GET_LWP(cur)));
		__ma_get_lwp_var(previous_thread)=cur;
		MA_THR_LONGJMP(cur->number, (next), NORMAL_RETURN);
	}
	return cur;
}

