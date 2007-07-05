
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
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*         Création d'un nouveau thread                                   */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

#ifdef PROFILE
#include "fut_pm2.h"
static volatile unsigned * volatile __pm2_profile_active_p = &__pm2_profile_active;
#define __pm2_profile_active (*__pm2_profile_active_p)
#endif

void marcel_sched_internal_create_dontstart_son(void) TBX_NORETURN;

int marcel_sched_internal_create_dontstart(marcel_task_t *cur,
		marcel_task_t *new_task,
		 __const marcel_attr_t *attr,
		 __const int dont_schedule,
		 __const unsigned long base_stack) {
	LOG_IN();

	// Ici, le père _ne_doit_pas_ donner la main au fils
	// immédiatement car : 
	// - ou bien ma_preempt_disable() a été appelé,
	// - ou bien le fils va être inséré sur un autre LWP,
	// La conséquence est que le thread fils est créé et
	// initialisé, mais pas "inséré" dans une quelconque
	// file pour l'instant.

	//PROF_IN_EXT(newborn_thread);


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
		marcel_ctx_destroyjmp(cur->ctx_yield);
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
				 new_task->initial_sp,
				 base_stack);

	marcel_sched_internal_create_dontstart_son();
}
void marcel_sched_internal_create_dontstart_son (void) {
	/* départ du fils, en mode interruption */
	MTRACE("On new stack", marcel_self());
	
	//PROF_OUT_EXT(newborn_thread);
	PROF_SET_THREAD_NAME(MARCEL_SELF);

	if(MA_THR_SETJMP(marcel_self()) == FIRST_RETURN) {
		// On rend la main au père
		PROF_SWITCH_TO(SELF_GETMEM(number), SELF_GETMEM(father));
		call_ST_FLUSH_WINDOWS();
		marcel_ctx_set_tls_reg(SELF_GETMEM(father));
		marcel_ctx_longjmp(SELF_GETMEM(father)->ctx_yield,
				   NORMAL_RETURN);
	}
	MA_THR_DESTROYJMP(marcel_self());
	MA_THR_RESTARTED(MARCEL_SELF, "Start");
	/* Drop preempt_count with ma_spin_unlock_softirq */
	ma_schedule_tail(__ma_get_lwp_var(previous_thread));

	/* pas de ma_preempt_enable ici : le preempt a déjà été mangé dans ma_schedule_tail */
	//ma_preempt_enable();
	LOG_OUT();
	marcel_exit((*SELF_GETMEM(f_to_call))(SELF_GETMEM(arg)));
}

void marcel_sched_internal_create_start_son(void) TBX_NORETURN;
int marcel_sched_internal_create_start(marcel_task_t *cur,
		marcel_task_t *new_task,
		 __const marcel_attr_t *attr,
		 __const int dont_schedule,
		 __const unsigned long base_stack) {
	ma_holder_t *h;
#ifdef MA__BUBBLES
	ma_holder_t *bh;
#endif
	LOG_IN();

#ifdef MA__BUBBLES
	if ((bh=ma_task_init_holder(new_task)) && bh->type != MA_BUBBLE_HOLDER)
		bh = NULL;
#endif

	// Cas le plus favorable (sur le plan de
	// l'efficacité) : le père sauve son contexte et on
	// démarre le fils immédiatement.
	// Note : si le thread est un 'real-time thread', cela
	// ne change rien ici...

	if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
		MA_THR_DESTROYJMP(cur);
		ma_schedule_tail(__ma_get_lwp_var(previous_thread));
		MA_THR_RESTARTED(cur, "Father Preemption");
		LOG_OUT();
		return 0;
	}

	/* le fils sera déjà démarré */
	new_task->father->child = NULL;

#ifdef MA__BUBBLES
	if (bh)
		marcel_bubble_inserttask(ma_bubble_holder(bh),new_task);
#endif

	//PROF_IN_EXT(newborn_thread);
	
	/* activer le fils */
	h = ma_task_sched_holder(new_task);
	ma_holder_lock_softirq(h); // passage en mode interruption
	ma_set_task_lwp(new_task, LWP_SELF);
	MA_BUG_ON(new_task->sched.state != MA_TASK_BORNING);
	ma_set_task_state(new_task, MA_TASK_RUNNING);
	ma_activate_running_task(new_task,h);
	ma_holder_rawunlock(h);

	*(long*)ma_task_stats_get(MARCEL_SELF, ma_stats_last_ran_offset) = marcel_clock();
	*(long*)ma_task_stats_get(new_task, ma_stats_nbrunning_offset) = 1;
	*(long*)ma_task_stats_get(MARCEL_SELF, ma_stats_nbrunning_offset) = 0;

	PROF_SWITCH_TO(cur->number, new_task);
	marcel_ctx_set_new_stack(new_task,
				 new_task->initial_sp,
				 base_stack);

	marcel_sched_internal_create_start_son();
}
void marcel_sched_internal_create_start_son(void) {
	ma_holder_t *h;
	/* départ du fils, en mode interruption */
	/* Signaler le changement de thread aux activations */
	MA_ACT_SET_THREAD(MARCEL_SELF);

	/* ré-enqueuer le père */
	h = ma_task_sched_holder(SELF_GETMEM(father));
	ma_holder_rawlock(h);
	ma_enqueue_task(SELF_GETMEM(father), h);
	ma_holder_unlock_softirq(h); // sortie du mode interruption

	MTRACE("Early start", marcel_self());
	
	//PROF_OUT_EXT(newborn_thread);
	PROF_SET_THREAD_NAME(MARCEL_SELF);

	/* pas de ma_preempt_enable ici : le preempt a déjà été mangé dans ma_schedule_tail */
	//ma_preempt_enable();
	LOG_OUT();
	marcel_exit((*SELF_GETMEM(f_to_call))(SELF_GETMEM(arg)));
}

void *marcel_sched_ghost_runner(void *arg) {
	ma_holder_t *h, *h2;
	marcel_t next;
	void *ret;

	marcel_setname(MARCEL_SELF, "ghost runner");

	/* first ghost thread */
	SELF_GETMEM(cur_ghost_thread) = arg;
	ma_sched_change_prio(MARCEL_SELF, MA_BATCH_PRIO);

	ma_preempt_disable();
	ma_local_bh_disable();

	marcel_ctx_setjmp(SELF_GETMEM(ctx_restart));
	/* restart a new ghost thread */
restart:
	SELF_SETMEM(f_to_call, marcel_sched_ghost_runner);

	next = SELF_GETMEM(cur_ghost_thread);

	/* mimic his scheduling situation */
#ifdef MA__BUBBLES
	h = ma_task_init_holder(next);
	if (h && h->type == MA_BUBBLE_HOLDER) {
		marcel_bubble_t *bubble = ma_bubble_holder(h);
		/* this order prevents marcel_bubble_join() from returning */
		marcel_bubble_inserttask(bubble, MARCEL_SELF);
#endif
		PROF_EVENT1(ghost_thread_run, MA_PROFILE_TID(next));
#ifdef MA__BUBBLES
		marcel_bubble_removetask(bubble, next);
	}
#endif

	/* get out of here */
	h = ma_task_run_holder(MARCEL_SELF);
	h2 = ma_task_run_holder(next);
	if (h2 != h) {
		ma_holder_rawlock(h);
		ma_deactivate_running_entity(ma_entity_task(MARCEL_SELF), h);
		ma_holder_rawunlock(h);
	}

	/* and go there */
	ma_task_sched_holder(MARCEL_SELF) = ma_task_sched_holder(next);
	ma_holder_rawlock(h2);
	ma_deactivate_running_entity(ma_entity_task(next), h2);
	if (h2 != h)
		ma_activate_running_entity(ma_entity_task(MARCEL_SELF), h2);
	ma_holder_rawunlock(h2);

	/* now we're ready */
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();

	/* TODO: transférer les stats aussi? */

	ret = next->f_to_call(next->arg);

	/* we returned from the caller function, we can avoid longjumping */
	SELF_SETMEM(f_to_call, NULL);

	marcel_exit_canreturn(ret);

	goto restart;
	return NULL;
}
