
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

struct marcel_bubble_gang_sched {
	marcel_bubble_sched_t scheduler;
};

ma_runqueue_t ma_gang_rq;
static int keep_running;
static marcel_sem_t schedulers = MARCEL_SEM_INITIALIZER(0);

any_t marcel_gang_scheduler(any_t runqueue)
{
	marcel_entity_t *e, *ee;
	marcel_bubble_t *b;
	ma_runqueue_t *work_rq = (void *) runqueue;
	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ, 2), -1, &ma_gang_rq);
	/* Attendre un tout petit peu que la création de threads se fasse */
	marcel_usleep(1);
	while (keep_running) {
		/* First clean the work_rq runqueue */
		PROF_EVENT1(rq_lock, work_rq);
		ma_holder_lock_softirq(&work_rq->as_holder);
		PROF_EVENT1(rq_lock, &ma_gang_rq);
		ma_holder_rawlock(&ma_gang_rq.as_holder);
		PROF_EVENTSTR(sched_status, "gang scheduler: cleaning gang runqueue");
		tbx_fast_list_for_each_entry_safe(e, ee,
						  &work_rq->as_holder.ready_entities,
						  ready_entities_item) {
			if (e->type == MA_BUBBLE_ENTITY) {
				int state = ma_get_entity(e);
				ma_put_entity(e, &ma_gang_rq.as_holder, state);
			}
		}
		/* Then put one job on work_rq */
		PROF_EVENTSTR(sched_status, "gang scheduler: putting one job");
		tbx_fast_list_for_each_entry(e, &ma_gang_rq.as_holder.ready_entities,
					     ready_entities_item) {
			MA_BUG_ON(e->type != MA_BUBBLE_ENTITY);
			b = ma_bubble_entity(e);
			if (b->as_holder.nb_ready_entities) {
				int state = ma_get_entity(e);
				ma_put_entity(e, &work_rq->as_holder, state);
				break;
			}
		}
		ma_holder_rawunlock(&ma_gang_rq.as_holder);
		PROF_EVENT1(rq_unlock, &ma_gang_rq);
		ma_holder_unlock_softirq(&work_rq->as_holder);
		PROF_EVENT1(rq_unlock, work_rq);
		ma_lwp_t lwp;
		/* And eventually preempt currently running thread */
		ma_for_each_lwp_begin(lwp)
		    if (lwp != MA_LWP_SELF && ma_rq_covers(work_rq, ma_vpnum(lwp))) {
			ma_holder_rawlock(&ma_lwp_vprq(lwp)->as_holder);
			ma_set_tsk_need_togo(ma_per_lwp(current_thread, lwp));
			ma_resched_task(ma_per_lwp(current_thread, lwp), ma_vpnum(lwp),
					lwp);
			ma_holder_rawunlock(&ma_lwp_vprq(lwp)->as_holder);
		}
		ma_for_each_lwp_end();
		PROF_EVENTSTR(sched_status, "gang scheduler: done");
		marcel_delay(MARCEL_BUBBLE_TIMESLICE * marcel_gettimeslice() / 1000);
	}
	marcel_sem_V(&schedulers);
	return NULL;
}

#if 0
/* Cleaner: cleans jobs from work_rq */
static any_t TBX_NORETURN marcel_gang_cleaner(any_t foo)
{
	marcel_entity_t *e, *ee;
	ma_runqueue_t *work_rq = (void *) foo;
	PROF_ALWAYS_PROBE(FUT_CODE(FUT_RQS_NEWRQ, 2), -1, &ma_gang_rq);
	marcel_usleep(1);
	while (1) {
		ma_holder_lock_softirq(&work_rq->as_holder);
		ma_holder_rawlock(&ma_gang_rq.as_holder);
		PROF_EVENTSTR(sched_status, "gang cleaner: cleaning gang runqueue");
		tbx_fast_list_for_each_entry_safe(e, ee,
						  &work_rq->as_holder.ready_entities,
						  ready_entities_item) {
			if (e->type == MA_BUBBLE_ENTITY) {
				int state = ma_get_entity(e);
				ma_put_entity(e, &ma_gang_rq.as_holder, state);
			}
		}
		ma_holder_rawunlock(&work_rq->as_holder);
		ma_holder_unlock_softirq(&ma_gang_rq.as_holder);
		ma_lwp_t lwp;
		/* And eventually preempt currently running thread */
		ma_for_each_lwp_begin(lwp)
		    if (lwp != MA_LWP_SELF && ma_rq_covers(work_rq, ma_vpnum(lwp))) {
			ma_holder_rawlock(&ma_lwp_vprq(lwp)->as_holder);
			ma_set_tsk_need_togo(ma_per_lwp(current_thread, lwp));
			ma_resched_task(ma_per_lwp(current_thread, lwp), ma_vpnum(lwp),
					lwp);
			ma_holder_rawunlock(&ma_lwp_vprq(lwp)->as_holder);
		}
		ma_for_each_lwp_end();
		PROF_EVENTSTR(sched_status, "gang cleaner: done");
		marcel_delay(MARCEL_BUBBLE_TIMESLICE * marcel_gettimeslice() / 1000);
	}
	return NULL;
}
#endif

static marcel_t __marcel_start_gang_scheduler(marcel_func_t f, ma_runqueue_t * rq,
					      int sys)
{
	marcel_attr_t attr;
	marcel_t t;
	marcel_attr_init(&attr);
	marcel_attr_setname(&attr, "gang scheduler");
	marcel_attr_setdetachstate(&attr, tbx_true);
	marcel_attr_setschedrq(&attr, rq);
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

static marcel_t marcel_start_gang_scheduler(ma_runqueue_t * rq, int sys)
{
	return __marcel_start_gang_scheduler(marcel_gang_scheduler, rq, sys);
}

static int gang_sched_init(marcel_bubble_sched_t * self TBX_UNUSED)
{
	ma_init_rq(&ma_gang_rq, "gang");
	return 0;
}

static marcel_t gang_thread, clean_thread, gang_thread1, gang_thread2, gang_thread3,
    gang_thread4;

static int gang_sched_start(marcel_bubble_sched_t * self TBX_UNUSED)
{
	keep_running = 1;
#if 1
	/* un seul gang scheduler */
	gang_thread = marcel_start_gang_scheduler(&ma_main_runqueue, 1);
#else
	/* un nettoyeur */
	clean_thread =
	    __marcel_start_gang_scheduler(marcel_gang_cleaner, &ma_main_runqueue, 1);
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

static int gang_sched_exit(marcel_bubble_sched_t * self TBX_UNUSED)
{
	marcel_entity_t *e, *ee;
	keep_running = 0;

#define CANCEL(var) if (var) { marcel_sem_P(&schedulers); var = NULL; }
	CANCEL(gang_thread)
	    CANCEL(clean_thread)
	    CANCEL(gang_thread1)
	    CANCEL(gang_thread2)
	    CANCEL(gang_thread3)
	    CANCEL(gang_thread4)
	    PROF_EVENT1(rq_lock, &ma_main_runqueue);
	ma_holder_lock_softirq(&ma_main_runqueue.as_holder);
	PROF_EVENT1(rq_lock, &ma_gang_rq);
	ma_holder_rawlock(&ma_gang_rq.as_holder);
	tbx_fast_list_for_each_entry_safe(e, ee, &ma_gang_rq.as_holder.ready_entities,
					  ready_entities_item) {
		int state = ma_get_entity(e);
		ma_put_entity(e, &ma_main_runqueue.as_holder, state);
	}
	ma_holder_rawunlock(&ma_gang_rq.as_holder);
	ma_holder_unlock_softirq(&ma_main_runqueue.as_holder);
	return 0;
}

static int
gang_vp_is_idle(marcel_bubble_sched_t * self TBX_UNUSED, unsigned vp TBX_UNUSED)
{
	/* This lwp is idle... TODO: check other that other LWPs of the same controller are idle too, and get another gang (that gang terminated) */
	return 0;
}


static int make_default_scheduler(marcel_bubble_sched_t * scheduler)
{
	scheduler->klass = &marcel_bubble_gang_sched_class;
	scheduler->init = gang_sched_init;
	scheduler->start = gang_sched_start;
	scheduler->exit = gang_sched_exit;
	scheduler->vp_is_idle = gang_vp_is_idle;
	return 0;
}

MARCEL_DEFINE_BUBBLE_SCHEDULER_CLASS(gang, make_default_scheduler);

#endif
