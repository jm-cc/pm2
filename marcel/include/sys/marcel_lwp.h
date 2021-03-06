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


#ifndef __SYS_MARCEL_LWP_H__
#define __SYS_MARCEL_LWP_H__


#include <stdlib.h>
#include "tbx_compiler.h"
#include "marcel_config.h"
#include "marcel_config.h"
#include "tbx_fast_list.h"
#include "asm/linux_atomic.h"
#include "asm/linux_rwlock.h"
#include "scheduler/marcel_sched_types.h"
#include "scheduler/linux_runqueues.h"
#include "sys/linux_notifier.h"
#include "sys/linux_timer.h"
#include "sys/marcel_kthread.h"
#include "marcel_sem.h"
#include "marcel_signal.h"
#include "sys/marcel_types.h"
#include "sys/marcel_hwlocapi.h"


/** Public macros **/
#ifdef MA__LWPS
#define marcel_vp_is_disabled(vp) ((vp) >= 0 && marcel_vpset_isset(&marcel_disabled_vpset, (vp)))
#else
#define marcel_vp_is_disabled(vp) 0
#endif


/** Public variables **/
#ifdef MA__LWPS
extern marcel_vpset_t marcel_disabled_vpset;
#endif
extern unsigned marcel_nb_vp;


/** Public functions **/
tbx_bool_t marcel_vp_is_idle(int vpnum);
static __tbx_inline__ unsigned marcel_nbvps(void);
#ifdef MARCEL_BLOCKING_ENABLED
#  ifdef MA__LWPS
void marcel_enter_blocking_section(void);
void marcel_leave_blocking_section(void);
#  else
#    define marcel_enter_blocking_section() (void)0
#    define marcel_leave_blocking_section() (void)0
#  endif
#endif

#ifdef MA__LWPS
/** Tell Marcel to stop using the given vpset, i.e.
 *  migrate threads up and stop burning cpu time in idle. */
void marcel_disable_vps(const marcel_vpset_t * vpset);
/** Tell Marcel to use again the given vpset, i.e.
 *  let idle restart burning cpu time. */
void marcel_enable_vps(const marcel_vpset_t * vpset);
#else				/* MA__LWPS */
#  define marcel_disable_vps(vpset) (void)0
#  define marcel_enable_vps(vpset) (void)0
#endif				/* MA__LWPS */


#ifdef __MARCEL_KERNEL__
TBX_VISIBILITY_PUSH_INTERNAL


/** Internal marcel_macros **/

/* LWP notifier priority */
#define MA_NOTIFIER_PRIO_RUNTASKS  100
#define MA_NOTIFIER_PRIO_SCHED     (MA_NOTIFIER_PRIO_RUNTASKS+MA_NOTIFIER_PRIO_RUNTASKS)
#define MA_NOTIFIER_PRIO_DEFAULT   (MA_NOTIFIER_PRIO_SCHED+MA_NOTIFIER_PRIO_RUNTASKS)
#define MA_NOTIFIER_PRIO_INITLWP   (MA_NOTIFIER_PRIO_DEFAULT+MA_NOTIFIER_PRIO_RUNTASKS)

/*
 * Acc�s aux LWP
 */
#define ma_get_task_vpnum(task)			(ma_vpnum(THREAD_GETMEM(task,lwp)))

#ifdef MA__LWPS
#  define ma_vpnum(lwp)				(ma_per_lwp(vpnum, lwp))
#  define ma_get_lwp_by_vpnum(vpnum)		(ma_vp_lwp[vpnum])
#  define ma_get_task_lwp(task)			((task)->lwp)
#  define ma_set_task_lwp(task, value)		((task)->lwp=(value))
#  define ma_init_lwp_vpnum(vpnum, lwp)		ma_set_lwp_vpnum(vpnum, lwp)
#  define ma_clr_lwp_vpnum(lwp)				\
	  do {						\
		  MA_BUG_ON(ma_vpnum(lwp) == -1);	\
		  MA_BUG_ON(!ma_vp_lwp[ma_vpnum(lwp)]);	\
		  MA_BUG_ON(!ma_lwp_rq(lwp)->father);	\
		  ma_vp_lwp[ma_vpnum(lwp)] = NULL;	\
		  ma_vpnum(lwp) = -1;			\
		  ma_lwp_rq(lwp)->father = NULL;	\
	  } while(0)
#  define ma_set_lwp_vpnum(vpnum, lwp)					\
	do {								\
		if (tbx_unlikely((vpnum) == -1)) {			\
			ma_vpnum(lwp) = -1;				\
			ma_lwp_rq(lwp)->father = NULL;			\
		} else {						\
			ma_vp_lwp[vpnum] = (lwp);			\
			ma_vpnum(lwp) = vpnum;				\
			ma_lwp_rq(lwp)->father = &marcel_topo_vp_level[vpnum].rq; \
			ma_per_lwp(vp_level, lwp) = &marcel_topo_vp_level[vpnum]; \
		}							\
	} while(0)

#  define ma_is_first_lwp(lwp)			(lwp == &__main_lwp)
#  define ma_any_lwp()				(!tbx_fast_list_empty(&ma_list_lwp_head))
#  define ma_for_all_lwp(lwp) \
     tbx_fast_list_for_each_entry(lwp, &ma_list_lwp_head, lwp_list)
#  define ma_for_all_lwp_from_begin(lwp, lwp_start) \
     tbx_fast_list_for_each_entry_from_begin(lwp, &ma_list_lwp_head, lwp_start, lwp_list)
#  define ma_for_all_lwp_from_end() \
     list_for_each_entry_from_end()
/* Should rather be the node level where this lwp was started */
#  define ma_lwp_vpaffinity_level(lwp)		(&marcel_machine_level[0])
#else
#  define ma_vpnum(lwp)				((void)(lwp),0)
#  define ma_get_lwp_by_vpnum(vpnum)		(&__main_lwp)
#  define ma_get_task_lwp(task)			(&__main_lwp)
#  define ma_set_task_lwp(task, value)		((void)0)
#  define ma_clr_lwp_nb(proc, value)		((void)0)
#  define ma_set_lwp_nb(proc, value)		((void)0)
#  define ma_is_first_lwp(lwp)			(1)
#  define ma_any_lwp()				(1)
#  define ma_for_all_lwp(lwp) \
	for (lwp=&__main_lwp;lwp;lwp=NULL)
#  define ma_for_all_lwp_from_begin(lwp, lwp_start) \
	for(lwp=lwp_start;lwp;lwp=NULL) {
#  define ma_for_all_lwp_from_end() \
	}
#endif

#define ma_spare_lwp_ext(lwp)			(ma_vpnum(lwp)==-1)
#define ma_spare_lwp()				(ma_spare_lwp_ext(MA_LWP_SELF))
#define ma_for_each_lwp_begin(lwp) \
	ma_for_all_lwp(lwp) {\
		if (ma_lwp_online(lwp)) {
#define ma_for_each_lwp_end() \
		} \
	}
#define ma_for_each_lwp_from_begin(lwp, lwp_start) \
	ma_for_all_lwp_from_begin(lwp, lwp_start) \
		if (ma_lwp_online(lwp)) {
#define ma_for_each_lwp_from_end() \
		} \
	ma_for_all_lwp_from_end()

#ifdef MA__LWPS
#if defined(MA__SELF_VAR) && (!defined(MA__LWPS) || !defined(MARCEL_DONT_USE_POSIX_THREADS))
#define MA_LWP_SELF				(ma_lwp_self)
#else
#define MA_LWP_SELF				(ma_get_task_lwp(MARCEL_SELF))
#endif
#else
#define MA_LWP_SELF				(&__main_lwp)
#endif
#define ma_softirq_pending_vp(vp) \
	ma_topo_vpdata(vp,softirq_pending)
#define ma_softirq_pending_lwp(lwp) \
	ma_per_lwp(softirq_pending, (lwp))
#define ma_local_softirq_pending() \
	__ma_get_lwp_var(softirq_pending)
#define MA_DEFINE_LWP_NOTIFIER_START(name, help, prepare, prepare_help, online, online_help) \
	MA_DEFINE_LWP_NOTIFIER_START_PRIO(name, MA_NOTIFIER_PRIO_DEFAULT, help, \
					  prepare, prepare_help,	\
					  online, online_help)
#define MA_DEFINE_LWP_NOTIFIER_START_PRIO(name, prio, help, prepare, prepare_help, online, online_help) \
	MA_DEFINE_LWP_NOTIFIER_TWO_PRIO(name, prio, help,		\
					UP_PREPARE, prepare, prepare_help, \
					ONLINE, online, online_help,	\
					0, 1, 4, 5)
#define MA_DEFINE_LWP_NOTIFIER_ONOFF(name, help, online, online_help, offline, offline_help) \
	MA_DEFINE_LWP_NOTIFIER_ONOFF_PRIO(name, MA_NOTIFIER_PRIO_DEFAULT, help, \
					  online, online_help,		\
					  offline, offline_help)
#define MA_DEFINE_LWP_NOTIFIER_ONOFF_PRIO(name, prio, help, online, online_help, offline, offline_help) \
	MA_DEFINE_LWP_NOTIFIER_TWO_PRIO(name, prio, help,		\
					ONLINE, online, online_help,	\
					OFFLINE, offline, offline_help, \
					0, 1, 3, 4)
#define MA_DEFINE_LWP_NOTIFIER_TWO_PRIO(name, prio, help, ONE, one, one_help, TWO, two, two_help, a, b, c, d) \
	static int name##_notify(struct ma_notifier_block *self TBX_UNUSED, \
				 unsigned long action, void *hlwp)	\
	{								\
		ma_lwp_t lwp = (ma_lwp_t)hlwp;				\
		switch(action) {					\
		case MA_LWP_##ONE:					\
			one(lwp);					\
			break;						\
		case MA_LWP_##TWO:					\
			two(lwp);					\
			break;						\
		default:						\
			break;						\
		}							\
		return 0;						\
	}								\
	static const char * name##_helps[6] = {				\
		[MA_LWP_##ONE]="Notifier [" help "] " one_help,		\
		[MA_LWP_##TWO]="Notifier [" help "] " two_help,		\
		[a]="Notifier [" help "] [none]",			\
		[b]="Notifier [" help "] [none]",			\
		[c]="Notifier [" help "] [none]",			\
		[d]="Notifier [" help "] [none]",			\
	};								\
	static MA_DEFINE_NOTIFIER_BLOCK_INTERNAL(name##_nb, name##_notify, \
						 prio, help, 4, name##_helps); \
	static void marcel_##name##_notifier_register(void)		\
	{								\
		ma_register_lwp_notifier(&name##_nb);			\
	}								\
	__ma_initfunc_prio(marcel_##name##_notifier_register,		\
			   MA_INIT_REGISTER_LWP_NOTIFIER,		\
			   MA_INIT_REGISTER_LWP_NOTIFIER_PRIO,		\
			   "Registering notifier " #name " (" help ") at prio "#prio)

#define MA_LWP_NOTIFIER_CALL_UP_PREPARE(name, section) \
	MA_LWP_NOTIFIER_CALL(name, section, MA_INIT_PRIO_BASE, UP_PREPARE)
#define MA_LWP_NOTIFIER_CALL_UP_PREPARE_PRIO(name, section, prio)	\
	MA_LWP_NOTIFIER_CALL(name, section, prio, UP_PREPARE)
#define MA_LWP_NOTIFIER_CALL_ONLINE(name, section) \
	MA_LWP_NOTIFIER_CALL(name, section, MA_INIT_PRIO_BASE, ONLINE)
#define MA_LWP_NOTIFIER_CALL_ONLINE_PRIO(name, section, prio) \
	MA_LWP_NOTIFIER_CALL(name, section, prio, ONLINE)
#define MA_LWP_NOTIFIER_CALL(name, section, prio, PART) \
	static void marcel_##name##_call_##PART(void)	\
	{								\
		name##_notify(&name##_nb, (unsigned long)MA_LWP_##PART,	\
			      (void *)(ma_lwp_t)MA_LWP_SELF);		\
	}								\
	__ma_initfunc_prio_internal(marcel_##name##_call_##PART, section, prio, \
				    &name##_helps[MA_LWP_##PART])


/** Internal marcel_types **/
typedef struct marcel_lwp marcel_lwp_t;


/** Internal marcel_structures **/
struct marcel_lwp {
	struct tbx_fast_list_head lwp_list;
#ifdef MA__LWPS
	marcel_sem_t kthread_stop;
	marcel_kthread_t pid;
#endif
	/*Polling par LWP */
	struct marcel_per_lwp_polling_s *polling_list;

	marcel_task_t *current_thread;
	struct ma_lwp_usage_stat lwp_usage;

#ifdef MA__LWPS
	ma_runqueue_t runqueue;
	ma_runqueue_t dontsched_runqueue;
#endif

#if defined(LINUX_SYS) || defined(GNU_SYS)
	struct drand48_data random_buffer;
#endif

	marcel_task_t *previous_thread;

#ifdef MA__LWPS
	marcel_task_t *idle_task;
#endif

#ifdef MARCEL_POSTEXIT_ENABLED
	marcel_task_t *postexit_task;
#endif				/* MARCEL_POSTEXIT_ENABLED */

	marcel_task_t *ksoftirqd_task;
	unsigned long softirq_pending;

#ifdef MARCEL_FORK_ENABLED
	marcel_task_t *forkd_task;
#endif

#if defined(IA64_ARCH) && !defined(MA__PROVIDE_TLS)
	/* explanations in include/asm-ia64/marcel_arch_switchto.h */
	unsigned long ma_tls_tp;
#endif

	ma_tvec_base_t tvec_bases;

#ifdef MA__LWPS
	int vpnum;
#endif
	unsigned online;
	marcel_task_t *run_task;

	struct marcel_topo_level *vp_level;

	char data[MA_PER_LWP_ROOM];

#ifdef MA__LWPS
#  ifdef MARCEL_BLOCKING_ENABLED
	int need_resched;
#  endif
#endif
#ifdef MARCEL_SIGNALS_ENABLED
	/* Signal mask currently applied to the LWP.  */
	marcel_sigset_t curmask;
#endif

#ifdef MA__USE_TIMER_CREATE
	timer_t timer;
#endif
#ifdef MA__LWPS
	hwloc_cpuset_t cpuset;
#endif

#ifdef MARCEL_SIGNALS_ENABLED
	/* Mask currently used for the timer and preemption signals, for
	 * marcel_signal.c to apply them as well. Protected by
	 * timer_sigmask_lock below.  */
	sigset_t timer_sigmask;
	ma_spinlock_t timer_sigmask_lock;
#endif
};

#ifdef MA__LWPS
#  define MA_LWP_INITIALIZER {		\
	.vp_level = NULL,		\
	.online = 0,	                \
}
#else
#  define MA_LWP_INITIALIZER {			\
	.vp_level = &marcel_machine_level[0],	\
	.online = 0,				\
}
#endif


/** Internal marcel_variables **/
extern ma_atomic_t ma__last_vp;
extern marcel_lwp_t __main_lwp;
extern ma_lwp_t ma_vp_lwp[MA_NR_VPS];

#ifndef MA__LWPS
/* mono: no idle thread, but on interrupts we need to when whether we're
 * idling. */
extern tbx_bool_t ma_currently_idle;
#endif

#ifdef MA__LWPS
// Verrou prot�geant la liste cha�n�e des LWPs
extern ma_rwlock_t __ma_lwp_list_lock;
extern struct tbx_fast_list_head ma_list_lwp_head;

#if defined(MA__SELF_VAR) && (!defined(MA__LWPS) || !defined(MARCEL_DONT_USE_POSIX_THREADS))
extern __thread marcel_lwp_t *ma_lwp_self;
#endif
#endif				/* MA__LWPS */


/** Internal marcel_functions **/
static __tbx_inline__ void ma_lwp_list_lock_read(void);
static __tbx_inline__ void ma_lwp_list_unlock_read(void);
static __tbx_inline__ void ma_lwp_list_lock_write(void);
static __tbx_inline__ void ma_lwp_list_unlock_write(void);
static __tbx_inline__ unsigned marcel_nballvps(void);
static __tbx_inline__ marcel_lwp_t *marcel_lwp_next_lwp(marcel_lwp_t * lwp);
static __tbx_inline__ marcel_lwp_t *marcel_lwp_prev_lwp(marcel_lwp_t * lwp);


#ifdef MA__LWPS
unsigned marcel_lwp_add_vp(marcel_vpset_t vpset);
void marcel_lwp_add_lwp(marcel_vpset_t vpset, int vpnum);
void marcel_lwp_stop_lwp(marcel_lwp_t * lwp);
void ma_lwp_wait_active(void);
int ma_lwp_block(tbx_bool_t require_spare);
marcel_lwp_t *ma_lwp_wait_vp_active(void);
#endif

static __tbx_inline__ unsigned ma_lwp_os_node(marcel_lwp_t * lwp);
static __tbx_inline__ int ma_lwp_online(ma_lwp_t lwp);

#ifdef MA__LWPS
/* Need to know about LWPs going up/down? */
extern int ma_register_lwp_notifier(struct ma_notifier_block *nb);
extern void ma_unregister_lwp_notifier(struct ma_notifier_block *nb);
#else
static __tbx_inline__ int ma_register_lwp_notifier(struct ma_notifier_block *nb);
static __tbx_inline__ void ma_unregister_lwp_notifier(struct ma_notifier_block *nb);
#endif				/* MA__LWPS */


TBX_VISIBILITY_POP
#endif /** __MARCEL_KERNEL__ **/


#endif /** __SYS_MARCEL_LWP_H__ **/
