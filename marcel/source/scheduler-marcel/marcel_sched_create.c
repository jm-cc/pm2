
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
/*         Cr�ation d'un nouveau thread                                   */
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

	// Ici, le p�re _ne_doit_pas_ donner la main au fils
	// imm�diatement car : 
	// - ou bien ma_preempt_disable() a �t� appel�,
	// - ou bien le fils va �tre ins�r� sur un autre LWP,
	// La cons�quence est que le thread fils est cr�� et
	// initialis�, mais pas "ins�r�" dans une quelconque
	// file pour l'instant.

	//PROF_IN_EXT(newborn_thread);
	
	/* on ne doit pas d�marrer nous-m�me les processus sp�ciaux */
	new_task->father->child = (dont_schedule?
				   NULL: /* On ne fait rien */
				   new_task); /* insert asap */

	ma_preempt_disable();
	/* On sauve l'�tat du p�re sachant qu'on va y revenir
	 * tout de suite
	 *
	 * On ne modifie pas l'�tat enregistr� des activations
	 * car les appels bloquants sont d�j� d�sactiv�s
	 */
	if(marcel_ctx_setjmp(cur->ctx_yield) == NORMAL_RETURN) {
		/* retour dans le p�re*/
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
	/* d�part du fils, en mode interruption */
	MTRACE("On new stack", marcel_self());
	
	//PROF_OUT_EXT(newborn_thread);
	PROF_SET_THREAD_NAME(MARCEL_SELF);

	if(MA_THR_SETJMP(marcel_self()) == FIRST_RETURN) {
		// On rend la main au p�re
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

	/* pas de ma_preempt_enable ici : le preempt a d�j� �t� mang� dans ma_schedule_tail */
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
	// l'efficacit�) : le p�re sauve son contexte et on
	// d�marre le fils imm�diatement.
	// Note : si le thread est un 'real-time thread', cela
	// ne change rien ici...

	ma_local_bh_disable(); // entering interrupt mode
	ma_preempt_disable();
	if(MA_THR_SETJMP(cur) == NORMAL_RETURN) {
		MA_THR_DESTROYJMP(cur);
		ma_schedule_tail(__ma_get_lwp_var(previous_thread));
		MA_THR_RESTARTED(cur, "Father Preemption");
		LOG_OUT();
		return 0;
	}

	/* le fils sera d�j� d�marr� */
	new_task->father->child = NULL;

#ifdef MA__BUBBLES
	if (bh)
		marcel_bubble_inserttask(ma_bubble_holder(bh),new_task);
#endif

	//PROF_IN_EXT(newborn_thread);
	
	/* activer le fils */
	h = ma_task_sched_holder(new_task);
	ma_holder_rawlock(h);
	ma_set_task_lwp(new_task, LWP_SELF);
	MA_BUG_ON(new_task->sched.state != MA_TASK_BORNING);
	ma_set_task_state(new_task, MA_TASK_RUNNING);
	ma_activate_running_task(new_task,h);
	ma_holder_rawunlock(h);

	ma_task_stats_set(long, MARCEL_SELF, ma_stats_last_ran_offset, marcel_clock());
	ma_task_stats_set(long, new_task, ma_stats_nbrunning_offset, 1);
	ma_task_stats_set(long, MARCEL_SELF, ma_stats_nbrunning_offset, 0);
	ma_task_stats_set(long, new_task, ma_stats_nbready_offset, 1);

	PROF_SWITCH_TO(cur->number, new_task);
	marcel_ctx_set_new_stack(new_task,
				 new_task->initial_sp,
				 base_stack);

	marcel_sched_internal_create_start_son();
}
void marcel_sched_internal_create_start_son(void) {
	ma_holder_t *h;
	/* d�part du fils, en mode interruption */
	/* Signaler le changement de thread aux activations */
	MA_ACT_SET_THREAD(MARCEL_SELF);

	/* r�-enqueuer le p�re */
	h = ma_task_holder_rawlock(SELF_GETMEM(father));
	ma_enqueue_task(SELF_GETMEM(father), h);
	ma_holder_unlock_softirq(h); // sortie du mode interruption

	MTRACE("Early start", marcel_self());
	
	MA_THR_DESTROYJMP(MARCEL_SELF);
	//PROF_OUT_EXT(newborn_thread);
	PROF_SET_THREAD_NAME(MARCEL_SELF);

	/* pas de ma_preempt_enable ici : le preempt a d�j� �t� mang� dans ma_schedule_tail */
	//ma_preempt_enable();
	LOG_OUT();
	marcel_exit((*SELF_GETMEM(f_to_call))(SELF_GETMEM(arg)));
}

void *marcel_sched_seed_runner(void *arg) {
	ma_holder_t *h, *h2, *h3;
	marcel_t next;
	void *ret;

	marcel_setname(MARCEL_SELF, "seed runner");

	/* first thread seed */
	SELF_GETMEM(cur_thread_seed) = arg;
	ma_sched_change_prio(MARCEL_SELF, MA_BATCH_PRIO);

	ma_preempt_disable();
	ma_local_bh_disable();

	marcel_ctx_setjmp(SELF_GETMEM(ctx_restart));
	/* restart a new thread seed */
restart:
	SELF_SETMEM(f_to_call, marcel_sched_seed_runner);

	next = SELF_GETMEM(cur_thread_seed);

	PROF_EVENT1(thread_seed_run, MA_PROFILE_TID(next));

	h2 = ma_entity_holder_rawlock(ma_entity_task(next));
	ma_deactivate_running_task(next, h2);
	ma_entity_holder_rawunlock(h2);

	/* mimic his scheduling situation */
#ifdef MA__BUBBLES
	h = ma_task_init_holder(next);
	if (h && h->type == MA_BUBBLE_HOLDER) {
		marcel_bubble_t *bubble = ma_bubble_holder(h);
		/* this order prevents marcel_bubble_join() from returning */
		marcel_bubble_inserttask(bubble, MARCEL_SELF);
		ma_task_sched_holder(MARCEL_SELF) = ma_task_sched_holder(next);
		marcel_bubble_removetask(bubble, next);
	} else
#endif
		ma_task_sched_holder(MARCEL_SELF) = ma_task_sched_holder(next);

	/* get out of here */
	h2 = ma_entity_holder_rawlock(ma_entity_task(MARCEL_SELF));
	ma_deactivate_running_task(MARCEL_SELF, h2);
	ma_entity_holder_rawunlock(h2);

	/* and go there */
	h3 = ma_entity_holder_rawlock(ma_entity_task(MARCEL_SELF));
	ma_activate_running_task(MARCEL_SELF, h3);
	ma_entity_holder_rawunlock(h3);

	/* now we're ready */
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();

	/* TODO: transf�rer les stats aussi? */

	ret = next->f_to_call(next->arg);

	/* we returned from the caller function, we can avoid longjumping */
	SELF_SETMEM(f_to_call, NULL);

	marcel_exit_canreturn(ret);

	goto restart;
	return NULL;
}
