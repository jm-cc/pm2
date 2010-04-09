/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 the PM2 team (see AUTHORS file)
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


#ifndef __INLINEFUNCTIONS_MARCEL_SCHED_GENERIC_H__
#define __INLINEFUNCTIONS_MARCEL_SCHED_GENERIC_H__


#include "marcel_sched_generic.h"
#include "marcel_debug.h"


#ifdef __MARCEL_KERNEL__


/** Internal inline functions **/
#ifdef MA__LWPS
static __tbx_inline__ void ma_about_to_idle(void) {
#  ifdef PIOMAN
	/* TODO: appeler PIOMan */
#  endif
}
#endif

#ifdef MA__LWPS
/* To be called from idle only, call marcel_sig_pause after making sure that
 * we have announced other LWPs that we stopped polling */
static __tbx_inline__ void ma_sched_sig_pause(void) {
	MA_BUG_ON(MARCEL_SELF != __ma_get_lwp_var(idle_task));
	/* Tell other LWPs that we will stop polling need_resched */
	ma_clear_thread_flag(TIF_POLLING_NRFLAG);
	/* Make sure people see that we won't poll any more after that */
	ma_smp_mb__after_clear_bit();
	/* Make a last check once we've announced that */
	if (ma_vpnum(MA_LWP_SELF) >= 0 && !ma_get_need_resched())
		marcel_sig_pause();
	/* Either we have need_resched or got a signal, re-announce that we
	 * will be polling */
	ma_set_thread_flag(TIF_POLLING_NRFLAG);
}
#endif

static __tbx_inline__ void ma_entering_idle(void) {
#ifdef MA__LWPS
	PROF_EVENT1(sched_idle_start,ma_vpnum(MA_LWP_SELF));
#  ifdef MARCEL_SMT_IDLE
	if (!(ma_preempt_count() & MA_PREEMPT_ACTIVE)) {
		marcel_sig_disable_interrupts();
		ma_topology_lwp_idle_start(MA_LWP_SELF);
		if (!(ma_topology_lwp_idle_core(MA_LWP_SELF)))
			ma_sched_sig_pause();
		marcel_sig_enable_interrupts();
	}
#  endif
#endif
#ifdef PIOMAN
	/* TODO: appeler PIOMan */
#endif
}

static __tbx_inline__ void ma_still_idle(void) {
#ifdef MA__LWPS
#  ifdef MARCEL_SMT_IDLE
	if (!(ma_preempt_count() & MA_PREEMPT_ACTIVE)) {
		ma_preempt_disable();
		marcel_sig_disable_interrupts();
		if (!ma_topology_lwp_idle_core(MA_LWP_SELF))
			ma_sched_sig_pause();
		marcel_sig_enable_interrupts();
		ma_preempt_enable_no_resched();
	}
#  endif
#endif
}

static __tbx_inline__ void ma_leaving_idle(void) {
#ifdef MA__LWPS
	PROF_EVENT1(sched_idle_stop, ma_vpnum(MA_LWP_SELF));
#  ifdef MARCEL_SMT_IDLE
	ma_topology_lwp_idle_end(MA_LWP_SELF);
#  endif
#endif
#ifdef PIOMAN
	/* TODO: appeler PIOMan */
#endif
}

__tbx_inline__ static void 
marcel_get_vpset(marcel_task_t* __restrict t, marcel_vpset_t *vpset)
{
	     *vpset = t->vpset;
}

__tbx_inline__ static void 
marcel_sched_init_marcel_thread(marcel_task_t* __restrict t,
				const marcel_attr_t* __restrict attr)
{
#ifdef MA__LWPS
	/* t->lwp */
	if (attr->schedrq)
		t->vpset = attr->schedrq->vpset;
	else if (attr->__cpuset)
		t->vpset = *attr->__cpuset; 
	else
		t->vpset = attr->vpset; 
#else
	marcel_vpset_vp(&t->vpset, 0);
#endif
	ma_set_task_state(t, MA_TASK_BORNING);
	marcel_sched_internal_init_marcel_thread(t, attr);
}

__tbx_inline__ static int marcel_sched_create(marcel_task_t* __restrict cur,
				      marcel_task_t* __restrict new_task,
				      __const marcel_attr_t * __restrict attr,
				      __const int dont_schedule,
				      __const unsigned long base_stack)
{
	LOG_IN();
	LOG_RETURN(marcel_sched_internal_create(cur, new_task, attr, 
						dont_schedule, base_stack));
}


#endif /** __MARCEL_KERNEL__ **/


#endif /** __INLINEFUNCTIONS_MARCEL_SCHED_GENERIC_H__ **/
