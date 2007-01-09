
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 "the PM2 team" (see AUTHORS file)
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

/******************************************************************************
 *
 * Bubble gang scheduler.
 *
 */

#ifdef MA__BUBBLES
ma_runqueue_t ma_gang_rq;

any_t marcel_gang_scheduler(any_t runqueue) {
	marcel_entity_t *e, *ee;
	marcel_bubble_t *b;
	ma_runqueue_t *rq, *work_rq = (void*) runqueue;
	struct list_head *queue;
	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ,2),-1,&ma_gang_rq);
	/* Attendre un tout petit peu que la création de threads se fasse */
	marcel_usleep(1);
	while(1) {
		/* D'abord enlever les jobs de la work_rq */
		rq = work_rq;
		PROF_EVENT1(rq_lock,work_rq);
		ma_holder_lock_softirq(&rq->hold);
		PROF_EVENT1(rq_lock,&ma_gang_rq);
		ma_holder_rawlock(&ma_gang_rq.hold);
		/* Les bulles pleines */
		queue = ma_rq_queue(rq, MA_BATCH_PRIO);
		ma_queue_for_each_entry_safe(e, ee, queue) {
			if (e->type == MA_BUBBLE_ENTITY) {
				b = ma_bubble_entity(e);
				ma_deactivate_entity(&b->sched, &rq->hold);
				PROF_EVENT2(bubble_sched_switchrq, b, &ma_gang_rq);
				ma_activate_entity(&b->sched, &ma_gang_rq.hold);
			}
		}
		/* Les bulles vides */
		queue = ma_rq_queue(rq, MA_NOSCHED_PRIO);
		ma_queue_for_each_entry_safe(e, ee, queue) {
			if (e->type == MA_BUBBLE_ENTITY) {
				b = ma_bubble_entity(e);
				ma_deactivate_entity(&b->sched, &rq->hold);
				PROF_EVENT2(bubble_sched_switchrq, b, &ma_gang_rq);
				ma_activate_entity(&b->sched, &ma_gang_rq.hold);
			}
		}
		/* Mettre ensuite un job sur la work_rq */
		rq = &ma_gang_rq;
		queue = ma_rq_queue(rq, MA_BATCH_PRIO);
		if (!ma_queue_empty(queue)) {
			e = ma_queue_entry(queue);
			MA_BUG_ON(e->type != MA_BUBBLE_ENTITY);
			b = ma_bubble_entity(e);
			ma_deactivate_entity(&b->sched, &rq->hold);
			PROF_EVENT2(bubble_sched_switchrq, b, work_rq);
			ma_activate_entity(&b->sched, &work_rq->hold);
		}
		ma_holder_rawunlock(&rq->hold);
		PROF_EVENT1(rq_unlock,&ma_gang_rq);
		ma_holder_unlock_softirq(&work_rq->hold);
		PROF_EVENT1(rq_unlock,work_rq);
		ma_lwp_t lwp;
		/* Et préempter les threads de cette rq */
		for_each_lwp_begin(lwp)
			if (lwp != LWP_SELF && ma_rq_covers(work_rq,LWP_NUMBER(lwp))) {
				ma_holder_rawlock(&ma_lwp_vprq(lwp)->hold);
				ma_set_tsk_need_togo(ma_per_lwp(current_thread,lwp));
				ma_resched_task(ma_per_lwp(current_thread,lwp),lwp);
				ma_holder_rawunlock(&ma_lwp_vprq(lwp)->hold);
			}
		for_each_lwp_end();
		marcel_delay(MARCEL_BUBBLE_TIMESLICE*marcel_gettimeslice()/1000);
	}
	return NULL;
}

/* Nettoyeur: enlève les jobs de la work_rq */
any_t marcel_gang_cleaner(any_t foo) {
	marcel_entity_t *e, *ee;
	marcel_bubble_t *b;
	ma_runqueue_t *rq, *work_rq = (void*) foo;
	struct list_head *queue;
	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ,2),-1,&ma_gang_rq);
	marcel_usleep(1);
	while(1) {
		rq = work_rq;
		ma_holder_lock_softirq(&rq->hold);
		ma_holder_rawlock(&ma_gang_rq.hold);
		/* Les bulles pleines */
		queue = ma_rq_queue(rq, MA_BATCH_PRIO);
		ma_queue_for_each_entry_safe(e, ee, queue) {
			if (e->type == MA_BUBBLE_ENTITY) {
				b = ma_bubble_entity(e);
				ma_deactivate_entity(&b->sched, &rq->hold);
				PROF_EVENT2(bubble_sched_switchrq, b, &ma_gang_rq);
				ma_activate_entity(&b->sched, &ma_gang_rq.hold);
			}
		}
		/* Les bulles vides */
		queue = ma_rq_queue(rq, MA_NOSCHED_PRIO);
		ma_queue_for_each_entry_safe(e, ee, queue) {
			if (e->type == MA_BUBBLE_ENTITY) {
				b = ma_bubble_entity(e);
				ma_deactivate_entity(&b->sched, &rq->hold);
				PROF_EVENT2(bubble_sched_switchrq, b, &ma_gang_rq);
				ma_activate_entity(&b->sched, &ma_gang_rq.hold);
			}
		}
		ma_holder_rawunlock(&rq->hold);
		ma_holder_unlock_softirq(&ma_gang_rq.hold);
		ma_lwp_t lwp;
		/* Et préempter les threads de cette rq */
		for_each_lwp_begin(lwp)
			if (lwp != LWP_SELF && ma_rq_covers(work_rq,LWP_NUMBER(lwp))) {
				ma_holder_rawlock(&ma_lwp_vprq(lwp)->hold);
				ma_set_tsk_need_togo(ma_per_lwp(current_thread,lwp));
				ma_resched_task(ma_per_lwp(current_thread,lwp),lwp);
				ma_holder_rawunlock(&ma_lwp_vprq(lwp)->hold);
			}
		for_each_lwp_end();
		marcel_delay(MARCEL_BUBBLE_TIMESLICE*marcel_gettimeslice()/1000);
	}
	return NULL;
}

#ifdef MARCEL_GANG_SCHEDULER
marcel_t __marcel_start_gang_scheduler(marcel_func_t f, ma_runqueue_t *rq, int sys) {
	marcel_attr_t attr;
	marcel_t t;
	marcel_attr_init(&attr);
	marcel_attr_setname(&attr, "gang scheduler");
	marcel_attr_setdetachstate(&attr, tbx_true);
	marcel_attr_setinitrq(&attr, rq);
	marcel_attr_setprio(&attr, MA_SYS_RT_PRIO);
	if (sys) {
		marcel_attr_setflags(&attr, MA_SF_NORUN);
		marcel_create_special(&t, &attr, f, rq);
		marcel_wake_up_created_thread(t);
	} else {
		marcel_create(&t, &attr, f, rq);
	}
	return t;
}
marcel_t marcel_start_gang_scheduler(ma_runqueue_t *rq, int sys) {
	return __marcel_start_gang_scheduler(marcel_gang_scheduler, rq, sys);
}
#endif

void __marcel_init ma_bubble_sched_start(void) {
#ifdef MARCEL_GANG_SCHEDULER
	ma_idle_scheduler = 0;
#if 1
	/* un seul gang scheduler */
	marcel_start_gang_scheduler(&ma_main_runqueue, 0);
#else
	/* un nettoyeur */
	__marcel_start_gang_scheduler(marcel_gang_cleaner, &ma_main_runqueue, 1);
#if 1
	/* deux gang schedulers */
	marcel_start_gang_scheduler(&marcel_topo_levels[1][0].sched, 0);
	marcel_start_gang_scheduler(&marcel_topo_levels[1][1].sched, 0);
#else
	/* quatre gang schedulers */
	marcel_start_gang_scheduler(&marcel_topo_levels[2][0].sched, 0);
	marcel_start_gang_scheduler(&marcel_topo_levels[2][1].sched, 0);
	marcel_start_gang_scheduler(&marcel_topo_levels[2][2].sched, 0);
	marcel_start_gang_scheduler(&marcel_topo_levels[2][3].sched, 0);
#endif
#endif
#endif
}


void ma_bubble_gang_sched_init() {
	ma_init_rq(&ma_gang_rq, "gang", MA_DONTSCHED_RQ);
}
#endif
