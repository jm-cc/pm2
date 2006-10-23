
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
	ma_holder_lock_softirq(h); // passage en mode interruption
	ma_set_task_lwp(new_task, LWP_SELF);
	MA_BUG_ON(new_task->sched.state != MA_TASK_BORNING);
	*(long*)ma_task_stats_get(new_task, ma_stats_last_ran_offset) = marcel_clock();
	ma_set_task_state(new_task, MA_TASK_RUNNING);
#ifdef MA__BUBBLES
#ifdef MARCEL_BUBBLE_EXPLODE
	if (bh)
		/* le fils est d�j� activ� par l'insertion de la bulle, le rendre runnable */
		ma_dequeue_task(new_task,h);
	else
#endif
#endif
	{
		ma_activate_running_task(new_task,h);
		h->nr_scheduled++;
	}
	ma_holder_rawunlock(h);

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
	h = ma_task_sched_holder(SELF_GETMEM(father));
	ma_holder_rawlock(h);
	ma_enqueue_task(SELF_GETMEM(father), h);
	h->nr_scheduled--;
	ma_holder_unlock_softirq(h); // sortie du mode interruption

	MTRACE("Early start", marcel_self());
	
	//PROF_OUT_EXT(newborn_thread);
	PROF_SET_THREAD_NAME(MARCEL_SELF);

	/* pas de ma_preempt_enable ici : le preempt a d�j� �t� mang� dans ma_schedule_tail */
	//ma_preempt_enable();
	LOG_OUT();
	marcel_exit((*SELF_GETMEM(f_to_call))(SELF_GETMEM(arg)));
}
