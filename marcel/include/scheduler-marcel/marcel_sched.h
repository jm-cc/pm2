
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

#section common
#include "tbx_compiler.h"
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
/* Attributs
 */

#section types
typedef struct marcel_sched_attr marcel_sched_attr_t;

#section structures
#depend "scheduler/linux_runqueues.h[marcel_types]"
struct marcel_sched_attr {
	ma_runqueue_t *init_rq;
	int prio;
	int stayinbubble;
};

#section variables
extern marcel_sched_attr_t marcel_sched_attr_default;

#section macros
#define MARCEL_SCHED_ATTR_DEFAULT { \
	.init_rq = NULL, \
	.prio = MA_DEF_PRIO, \
	.stayinbubble = FALSE, \
}

#section functions
int marcel_sched_attr_init(marcel_sched_attr_t *attr);
#define marcel_sched_attr_destroy(attr_ptr)	0

int marcel_sched_attr_setinitrq(marcel_sched_attr_t *attr, ma_runqueue_t *rq) __THROW;
int marcel_sched_attr_getinitrq(__const marcel_sched_attr_t *attr, ma_runqueue_t **rq) __THROW;

int marcel_sched_attr_setprio(marcel_sched_attr_t *attr, int prio) __THROW;
int marcel_sched_attr_getprio(__const marcel_sched_attr_t *attr, int *prio) __THROW;

int marcel_sched_attr_setstayinbubble(marcel_sched_attr_t *attr, int stay) __THROW;
int marcel_sched_attr_getstayinbubble(__const marcel_sched_attr_t *attr, int *stay) __THROW;

#section macros
#define marcel_attr_setinitrq(attr,rq) marcel_sched_attr_setinitrq(&(attr)->sched,rq)
#define marcel_attr_getinitrq(attr,rq) marcel_sched_attr_getinitrq(&(attr)->sched,rq)

#define marcel_attr_setprio(attr,prio) marcel_sched_attr_setprio(&(attr)->sched,prio)
#define marcel_attr_getprio(attr,prio) marcel_sched_attr_getprio(&(attr)->sched,prio)

#define marcel_attr_setstayinbubble(attr,stay) marcel_sched_attr_setstayinbubble(&(attr)->sched,stay)
#define marcel_attr_getstayinbubble(attr,stay) marcel_sched_attr_getstayinbubble(&(attr)->sched,stay)

/****************************************************************/
/* Structure interne pour une tâche
 */

#section marcel_types
#depend "scheduler/marcel_bubble_sched.h[marcel_structures]"
typedef struct marcel_bubble_sched_entity marcel_sched_internal_task_t;

#section marcel_functions
int ma_wake_up_task(marcel_task_t * p);

#section marcel_functions
#depend "scheduler/linux_runqueues.h[marcel_types]"
__tbx_inline__ static ma_runqueue_t *
marcel_sched_vpmask_init_rq(marcel_vpmask_t mask);

#section marcel_inline
#depend "scheduler/linux_runqueues.h[marcel_types]"
__tbx_inline__ static ma_runqueue_t *
marcel_sched_vpmask_init_rq(marcel_vpmask_t mask)
{
	if (tbx_unlikely(mask==MARCEL_VPMASK_EMPTY))
		return &ma_main_runqueue;
	else if (tbx_unlikely(mask==MARCEL_VPMASK_FULL))
		return &ma_dontsched_runqueue;
	else {
		int first_vp;
		first_vp=ma_ffs(~mask)-1;
		/* pour l'instant, on ne gère qu'un vp activé */
		MA_BUG_ON(mask!=MARCEL_VPMASK_ALL_BUT_VP(first_vp));
		MA_BUG_ON(first_vp && first_vp>=marcel_nbvps());
		return ma_lwp_rq(GET_LWP_BY_NUM(first_vp));
	}
}

#section sched_marcel_functions
__tbx_inline__ static void 
marcel_sched_internal_init_marcel_thread(marcel_task_t* t, 
					 const marcel_attr_t *attr);
#section sched_marcel_inline
#depend "asm/linux_bitops.h[marcel_inline]"
#depend "scheduler/linux_runqueues.h[variables]"
#depend "scheduler/marcel_bubble_sched.h[types]"
__tbx_inline__ static void 
marcel_sched_internal_init_marcel_thread(marcel_task_t* t, 
					 const marcel_attr_t *attr)
{
	LOG_IN();
	INIT_LIST_HEAD(&t->sched.internal.run_list);
	//t->sched.internal.lwps_runnable=~0UL;
	t->sched.internal.type = MARCEL_TASK_ENTITY;
	if (attr->sched.stayinbubble) {
		/* TODO: on suppose ici que la bulle est éclatée et qu'elle ne sera pas refermée d'ici qu'on wake_up_created ce thread */
		marcel_bubble_t *bubble=marcel_bubble_holding_task(MARCEL_SELF);
		MA_BUG_ON(!bubble);
		t->sched.internal.holdingbubble=bubble;
		t->sched.internal.init_rq=MARCEL_SELF->sched.internal.init_rq;
	} else {
		t->sched.internal.holdingbubble=NULL;
		if (attr->sched.init_rq)
			t->sched.internal.init_rq=attr->sched.init_rq;
		else
			t->sched.internal.init_rq=marcel_sched_vpmask_init_rq(attr->vpmask);
	}
	t->sched.internal.cur_rq=NULL;
	t->sched.internal.sched_policy = attr->__schedpolicy;
	t->sched.internal.prio=attr->sched.prio;
	ma_atomic_set(&t->sched.internal.time_slice,10); /* TODO: utiliser les priorités pour le calculer */
	t->sched.internal.array=NULL;
#ifdef MA__LWPS
	t->sched.internal.sched_level=MARCEL_LEVEL_DEFAULT;
#endif
	sched_debug("%p(%s)'s init_rq is %s (prio %d)\n", t, t->name, t->sched.internal.init_rq->name, t->sched.internal.prio);
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

/* ==== scheduling policies ==== */
#section types
struct marcel_sched_param {
	int __sched_priority;
};
#ifndef sched_priority
#define sched_priority __sched_priority
#endif

#section functions
int marcel_sched_setparam(marcel_t t, const struct marcel_sched_param *p);
int marcel_sched_getparam(marcel_t t, struct marcel_sched_param *p);
int marcel_sched_setscheduler(marcel_t t, int policy, const struct marcel_sched_param *p);
int marcel_sched_getscheduler(marcel_t t);

/* ==== SMP scheduling directives ==== */

void marcel_change_vpmask(marcel_vpmask_t mask);

/* ==== scheduler status ==== */

unsigned marcel_activethreads(void);
unsigned marcel_sleepingthreads(void);
unsigned marcel_blockedthreads(void);
unsigned marcel_frozenthreads(void);



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
static
// gcc 4.0 doesn't implement setjmp in an inline function.
#if (__GNUC__ != 4 || __GNUC_MINOR__ != 0 || __GNUC_PATCHLEVEL__ != 0)
__tbx_inline__
#endif
int marcel_sched_internal_create(marcel_task_t *cur, 
					       marcel_task_t *new_task,
					       __const marcel_attr_t *attr,
					       __const int dont_schedule,
					       __const unsigned long base_stack);
#section sched_marcel_inline
static
#if (__GNUC__ != 4 || __GNUC_MINOR__ != 0 || __GNUC_PATCHLEVEL__ != 0)
__tbx_inline__
#endif
int marcel_sched_internal_create(marcel_task_t *cur, marcel_task_t *new_task,
				 __const marcel_attr_t *attr,
				 __const int dont_schedule,
				 __const unsigned long base_stack)
{ 
	LOG_IN();

	/* On est encore dans le père. On doit démarrer le fils */
	if(
		// On ne peut pas céder la main maintenant
		(ma_in_atomic())
		// ou bien on ne veut pas
		|| ma_thread_preemptible()
		// On n'a pas encore de scheduler...
		|| dont_schedule
#ifdef MA__LWPS
		// On ne peut pas placer ce thread sur le LWP courant
		|| (!lwp_isset(LWP_NUMBER(LWP_SELF), THREAD_GETMEM(new_task, sched.lwps_allowed)))
#warning TODO: MARCEL_SCHED_OTHER
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


		/* on ne doit pas démarrer nous-même les processus spéciaux */
		new_task->father->child = (dont_schedule?
					   NULL: /* On ne fait rien */
					   new_task); /* insert asap */

		ma_preempt_disable();
		/* On sauve l'état du père sachant qu'on va y revenir
		 * tout de suite
		 *
		 * On ne modifie pas l'état enregistré des activations
		 * car les appels bloquants sont déjà désactivés
		 */
		if(marcel_ctx_setjmp(cur->ctx_yield) == NORMAL_RETURN) {
			/* retour dans le père*/
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
		PROF_SWITCH_TO(cur->number, new_task);
		marcel_ctx_set_new_stack(new_task, 
					 new_task->initial_sp-
					 (MAL(base_stack-get_sp()))
/* XXX FIXME: base_stack n'est pas correct ici pour l'ia 64 !! */
					 -0x200);
		/* départ du fils, en mode interruption */

		LOG_IN();
		MTRACE("On new stack", marcel_self());
		
		PROF_OUT_EXT(newborn_thread);
		PROF_SET_THREAD_NAME();

		if(MA_THR_SETJMP(marcel_self()) == FIRST_RETURN) {
			// On rend la main au père
			PROF_SWITCH_TO(marcel_self()->number, marcel_self()->father);
			call_ST_FLUSH_WINDOWS();
			marcel_ctx_longjmp(marcel_self()->father->ctx_yield,
					   NORMAL_RETURN);
		}
		MA_THR_RESTARTED(MARCEL_SELF, "Start");
		/* Drop preempt_count with ma_spin_unlock_softirq */
		ma_schedule_tail(MARCEL_SELF);
		
	} else {
		ma_runqueue_t *rq;
		marcel_bubble_t *b;
		// Cas le plus favorable (sur le plan de
		// l'efficacité) : le père sauve son contexte et on
		// démarre le fils immédiatement.
		// Note : si le thread est un 'real-time thread', cela
		// ne change rien ici...

		if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
			ma_schedule_tail(MARCEL_SELF);
			MA_THR_RESTARTED(cur, "Father Preemption");
			LOG_OUT();
			return 0;
		}

		/* le fils sera déjà démarré */
		new_task->father->child = NULL;

		if ((b=new_task->sched.internal.holdingbubble))
			marcel_bubble_inserttask(b,new_task);

		PROF_SWITCH_TO(cur->number, new_task);
		PROF_IN_EXT(newborn_thread);
		
		/* activer le fils */
		rq = ma_task_init_rq(new_task);
		ma_spin_lock_softirq(&rq->lock); // passage en mode interruption
		ma_set_task_lwp(new_task, LWP_SELF);
		new_task->sched.internal.timestamp = marcel_clock();
		activate_running_task(new_task,rq);
		_ma_raw_spin_unlock(&rq->lock);

		/* passage de rq de la pile du père au fils */
		__ma_get_lwp_var(prev_rq)=task_rq(MARCEL_SELF);

		marcel_ctx_set_new_stack(new_task,
					 new_task->initial_sp -
					 (MAL(base_stack-get_sp()))
/* XXX FIXME: base_stack n'est pas correct ici pour l'ia 64 !! */
					 -0x200);
		/* départ du fils, en mode interruption */

		/* Signaler le changement de thread aux activations */
		MA_ACT_SET_THREAD(MARCEL_SELF);
		/* ré-enqueuer le père */
		rq = ma_prev_rq();
		_ma_raw_spin_lock(&rq->lock);
		enqueue_task(marcel_self()->father, rq->active);
		ma_spin_unlock_softirq(&rq->lock); // sortie du mode interruption
		
		MTRACE("Early start", marcel_self());
		
		PROF_OUT_EXT(newborn_thread);
		PROF_SET_THREAD_NAME();
	}
	
	/* pas de unlock_task ici : le preempt a déjà été mangé dans ma_schedule_tail */
	//unlock_task();
	LOG_OUT();
	marcel_exit((*marcel_self()->f_to_call)(marcel_self()->arg));
}

#section marcel_structures
struct ma_lwp_usage_stat {
	unsigned long long user;
	unsigned long long nice;
	unsigned long long system;
	unsigned long long softirq;
	unsigned long long irq;
	unsigned long long idle;
	unsigned long long iowait;
};

#section marcel_variables
MA_DECLARE_PER_LWP(ma_runqueue_t *, prev_rq);

MA_DECLARE_PER_LWP(struct ma_lwp_usage_stat, lwp_usage);
