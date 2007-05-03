
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
	ma_runqueue_t *work_rq = (void*) runqueue;
	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ,2),-1,&ma_gang_rq);
	/* Attendre un tout petit peu que la cr�ation de threads se fasse */
	marcel_usleep(1);
	while(1) {
		/* First clean the work_rq runqueue */
		PROF_EVENT1(rq_lock,work_rq);
		ma_holder_lock_softirq(&work_rq->hold);
		PROF_EVENT1(rq_lock,&ma_gang_rq);
		ma_holder_rawlock(&ma_gang_rq.hold);
		PROF_EVENTSTR(sched_status,"gang scheduler: cleaning gang runqueue");
		list_for_each_entry_safe(e, ee, &work_rq->hold.sched_list, sched_list) {
			if (e->type == MA_BUBBLE_ENTITY) {
				int state = ma_get_entity(e);
				ma_put_entity(e, &ma_gang_rq.hold, state);
			}
		}
		/* Then put one job on work_rq */
		PROF_EVENTSTR(sched_status,"gang scheduler: putting one job");
		list_for_each_entry(e, &ma_gang_rq.hold.sched_list, sched_list) {
			MA_BUG_ON(e->type != MA_BUBBLE_ENTITY);
			b = ma_bubble_entity(e);
			if (b->hold.nr_ready) {
				int state = ma_get_entity(e);
				ma_put_entity(e, &work_rq->hold, state);
				break;
			}
		}
		ma_holder_rawunlock(&ma_gang_rq.hold);
		PROF_EVENT1(rq_unlock,&ma_gang_rq);
		ma_holder_unlock_softirq(&work_rq->hold);
		PROF_EVENT1(rq_unlock,work_rq);
		ma_lwp_t lwp;
		/* And eventually preempt currently running thread */
		for_each_lwp_begin(lwp)
			if (lwp != LWP_SELF && ma_rq_covers(work_rq,LWP_NUMBER(lwp))) {
				ma_holder_rawlock(&ma_lwp_vprq(lwp)->hold);
				ma_set_tsk_need_togo(ma_per_lwp(current_thread,lwp));
				ma_resched_task(ma_per_lwp(current_thread,lwp),LWP_NUMBER(lwp),lwp);
				ma_holder_rawunlock(&ma_lwp_vprq(lwp)->hold);
			}
		for_each_lwp_end();
		PROF_EVENTSTR(sched_status,"gang scheduler: done");
		marcel_delay(MARCEL_BUBBLE_TIMESLICE*marcel_gettimeslice()/1000);
	}
	return NULL;
}

/* Cleaner: cleans jobs from work_rq */
any_t marcel_gang_cleaner(any_t foo) {
	marcel_entity_t *e, *ee;
	ma_runqueue_t *work_rq = (void*) foo;
	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ,2),-1,&ma_gang_rq);
	marcel_usleep(1);
	while(1) {
		ma_holder_lock_softirq(&work_rq->hold);
		ma_holder_rawlock(&ma_gang_rq.hold);
		PROF_EVENTSTR(sched_status,"gang cleaner: cleaning gang runqueue");
		list_for_each_entry_safe(e, ee, &work_rq->hold.sched_list, sched_list) {
			if (e->type == MA_BUBBLE_ENTITY) {
				int state = ma_get_entity(e);
				ma_put_entity(e, &ma_gang_rq.hold, state);
			}
		}
		ma_holder_rawunlock(&work_rq->hold);
		ma_holder_unlock_softirq(&ma_gang_rq.hold);
		ma_lwp_t lwp;
		/* And eventually preempt currently running thread */
		for_each_lwp_begin(lwp)
			if (lwp != LWP_SELF && ma_rq_covers(work_rq,LWP_NUMBER(lwp))) {
				ma_holder_rawlock(&ma_lwp_vprq(lwp)->hold);
				ma_set_tsk_need_togo(ma_per_lwp(current_thread,lwp));
				ma_resched_task(ma_per_lwp(current_thread,lwp),LWP_NUMBER(lwp),lwp);
				ma_holder_rawunlock(&ma_lwp_vprq(lwp)->hold);
			}
		for_each_lwp_end();
		PROF_EVENTSTR(sched_status,"gang cleaner: done");
		marcel_delay(MARCEL_BUBBLE_TIMESLICE*marcel_gettimeslice()/1000);
	}
	return NULL;
}

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

static int gang_sched_init() {
	ma_init_rq(&ma_gang_rq, "gang", MA_DONTSCHED_RQ);
	return 0;
}

static marcel_t gang_thread, clean_thread, gang_thread1, gang_thread2, gang_thread3, gang_thread4;

static int gang_sched_start(void) {
#if 1
	/* un seul gang scheduler */
	gang_thread = marcel_start_gang_scheduler(&ma_main_runqueue, 1);
#else
	/* un nettoyeur */
	clean_thread = __marcel_start_gang_scheduler(marcel_gang_cleaner, &ma_main_runqueue, 1);
#if 1
	/* deux gang schedulers */
	gang_thread1 = marcel_start_gang_scheduler(&marcel_topo_levels[1][0].sched, 1);
	gang_thread2 = marcel_start_gang_scheduler(&marcel_topo_levels[1][1].sched, 1);
#else
	/* quatre gang schedulers */
	gang_thread1 = marcel_start_gang_scheduler(&marcel_topo_levels[2][0].sched, 1);
	gang_thread2 = marcel_start_gang_scheduler(&marcel_topo_levels[2][1].sched, 1);
	gang_thread3 = marcel_start_gang_scheduler(&marcel_topo_levels[2][2].sched, 1);
	gang_thread4 = marcel_start_gang_scheduler(&marcel_topo_levels[2][3].sched, 1);
#endif
#endif
	return 0;
}

static int gang_sched_exit(void) {
	marcel_entity_t *e, *ee;

#define CANCEL(var) if (var) { marcel_cancel(var); var = NULL; }
	CANCEL(gang_thread)
	CANCEL(clean_thread)
	CANCEL(gang_thread1)
	CANCEL(gang_thread2)
	CANCEL(gang_thread3)
	CANCEL(gang_thread4)
	PROF_EVENT1(rq_lock,&ma_main_runqueue);
	ma_holder_lock_softirq(&ma_main_runqueue.hold);
	PROF_EVENT1(rq_lock,&ma_gang_rq);
	ma_holder_rawlock(&ma_gang_rq.hold);
	list_for_each_entry_safe(e, ee, &ma_gang_rq.hold.sched_list, sched_list) {
		int state = ma_get_entity(e);
		ma_put_entity(e, &ma_main_runqueue.hold, state);
	}
	ma_holder_rawunlock(&ma_gang_rq.hold);
	ma_holder_unlock_softirq(&ma_main_runqueue.hold);
	return 0;
}

static int
gang_vp_is_idle(unsigned vp)
{
	/* This lwp is idle... TODO: check other that other LWPs of the same controller are idle too, and get another gang (that gang terminated) */
	return 0;
}
struct ma_bubble_sched_struct marcel_bubble_gang_sched = {
	.init = gang_sched_init,
	.start = gang_sched_start,
	.exit = gang_sched_exit,
	.vp_is_idle = gang_vp_is_idle,
};
#endif
