

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

#warning "[1;33m<<< [1;37mScheduler [1;32mmarcel[1;37m selected[1;33m >>>[0m"

marcel_sched_attr_t marcel_sched_attr_default = MARCEL_SCHED_ATTR_INITIALIZER;

int marcel_sched_attr_init(marcel_sched_attr_t * attr)
{
	*attr = marcel_sched_attr_default;
	return 0;
}

int marcel_sched_attr_setinitholder(marcel_sched_attr_t * attr, ma_holder_t * h)
{
	attr->init_holder = h;
	return 0;
}

int marcel_sched_attr_getinitholder(__const marcel_sched_attr_t * attr,
    ma_holder_t ** h)
{
	*h = attr->init_holder;
	return 0;
}

int marcel_sched_attr_getinitrq(__const marcel_sched_attr_t * attr,
    ma_runqueue_t ** rq)
{
	ma_holder_t *h = attr->init_holder;
	MA_BUG_ON(h->type != MA_RUNQUEUE_HOLDER);
	*rq = ma_rq_holder(h);
	return 0;
}

#ifdef MA__BUBBLES
int marcel_sched_attr_getinitbubble(__const marcel_sched_attr_t * attr,
    marcel_bubble_t ** b)
{
	ma_holder_t *h = attr->init_holder;
	MA_BUG_ON(h->type != MA_BUBBLE_HOLDER);
	*b = ma_bubble_holder(h);
	return 0;
}
#endif

int marcel_sched_attr_setinheritholder(marcel_sched_attr_t * attr, int yes)
{
	attr->inheritholder = yes;
	return 0;
}

int marcel_sched_attr_getinheritholder(__const marcel_sched_attr_t * attr,
    int *yes)
{
	*yes = attr->inheritholder;
	return 0;
}

int marcel_sched_attr_setschedpolicy(marcel_sched_attr_t * attr, int policy)
{
	attr->sched_policy = policy;
	return 0;
}

int marcel_sched_attr_getschedpolicy(__const marcel_sched_attr_t *
    __restrict attr, int *__restrict policy)
{
	*policy = attr->sched_policy;
	return 0;
}

static unsigned next_schedpolicy_available = __MARCEL_SCHED_AVAILABLE;
marcel_schedpolicy_func_t ma_user_policy[MARCEL_MAX_USER_SCHED];
void marcel_schedpolicy_create(int *policy, marcel_schedpolicy_func_t func)
{
	if (next_schedpolicy_available >=
	    MARCEL_MAX_USER_SCHED + __MARCEL_SCHED_AVAILABLE)
		MARCEL_EXCEPTION_RAISE(MARCEL_CONSTRAINT_ERROR);

	ma_user_policy[next_schedpolicy_available - __MARCEL_SCHED_AVAILABLE] =
	    func;
	*policy = next_schedpolicy_available++;
}

int marcel_sched_setparam(marcel_t t, const struct marcel_sched_param *p)
{
	return ma_sched_change_prio(t, p->sched_priority);
}

int marcel_sched_getparam(marcel_t t, struct marcel_sched_param *p)
{
	p->sched_priority = t->sched.internal.entity.prio;
	return 0;
}

int marcel_sched_setscheduler(marcel_t t, int policy,
    const struct marcel_sched_param *p)
{
	t->sched.internal.entity.sched_policy = policy;
	return ma_sched_change_prio(t, p->sched_priority);
}

int marcel_sched_getscheduler(marcel_t t)
{
	return t->sched.internal.entity.sched_policy;
}

#ifdef MA__LWPS
unsigned marcel_add_lwp(void)
{
	return marcel_lwp_add_vp();
}
#endif

static int frozen_scheduler;

void ma_freeze_thread(marcel_task_t * p)
{
	ma_holder_t *h;
	if (!frozen_scheduler)
		h = ma_task_holder_lock_softirq(p);
	else
		h = ma_task_run_holder(p);

	if (MA_TASK_IS_FROZEN(p)) {
		if (!frozen_scheduler)
			ma_task_holder_unlock_softirq(h);
		MARCEL_EXCEPTION_RAISE(MARCEL_PROGRAM_ERROR);
	}
	if (MA_TASK_IS_RUNNING(p)) {
		if (!frozen_scheduler)
			ma_task_holder_unlock_softirq(h);
		MARCEL_EXCEPTION_RAISE(MARCEL_NOT_IMPLEMENTED);
	}

	if (!MA_TASK_IS_BLOCKED(p))
		ma_deactivate_task(p, h);
	MA_BUG_ON(!MA_TASK_IS_BLOCKED(p));
	if (!frozen_scheduler)
		ma_task_holder_unlock_softirq(h);
	__ma_set_task_state(p, MA_TASK_FROZEN);
}

void ma_unfreeze_thread(marcel_task_t * p)
{
	ma_try_to_wake_up(p, MA_TASK_FROZEN, 0);
}

void TBX_EXTERN marcel_freeze_sched(void)
{
	ma_holder_lock_softirq(&ma_main_runqueue.as_holder);
	/* TODO: other levels */
#ifdef MA__LWPS
	{
		ma_lwp_t lwp;
		ma_for_all_lwp(lwp)
		    ma_holder_rawlock(&ma_lwp_rq(lwp)->as_holder);
	}
#endif
	frozen_scheduler++;
}

void TBX_EXTERN marcel_unfreeze_sched(void)
{
	MA_BUG_ON(!frozen_scheduler);
	frozen_scheduler--;
#ifdef MA__LWPS
	{
		ma_lwp_t lwp;
		ma_for_all_lwp(lwp)
		    ma_holder_rawunlock(&ma_lwp_rq(lwp)->as_holder);
	}
#endif
	ma_holder_unlock_softirq(&ma_main_runqueue.as_holder);
}
