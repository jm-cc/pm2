
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
#section types
#include <stdlib.h>
#section common
#depend "tbx_compiler.h"

#section marcel_types
typedef struct marcel_lwp marcel_lwp_t;
#section types
typedef struct marcel_lwp *ma_lwp_t;

#section marcel_structures
#depend "tbx_fast_list.h"
#depend "marcel_sem.h[structures]"
#depend "sys/marcel_kthread.h[marcel_types]"
#depend "linux_timer.h[marcel_types]"
#depend "scheduler/linux_runqueues.h[types]"
#depend "scheduler/linux_runqueues.h[marcel_structures]"
#depend "marcel_topology.h[marcel_structures]"
#depend "scheduler/marcel_sched.h[marcel_structures]"
#ifdef MA__LWPS
#include <hwloc.h>
#endif

struct marcel_lwp {
	struct tbx_fast_list_head lwp_list;
#ifdef MA__LWPS
	marcel_sem_t kthread_stop;
	marcel_kthread_t pid;
#endif
	/*Polling par LWP*/
	struct marcel_per_lwp_polling_s* polling_list;

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
#endif /* MARCEL_POSTEXIT_ENABLED */

	marcel_task_t *ksoftirqd_task;
	unsigned long softirq_pending;

#if defined(IA64_ARCH) && !defined(MA__PROVIDE_TLS)
	unsigned long *ma_ia64_tp;
#endif

	ma_tvec_base_t tvec_bases;

#ifdef MA__LWPS
	int vpnum;
#endif
	unsigned online;
	marcel_task_t *run_task;

	struct marcel_topo_level
#ifdef MA__NUMA
		*node_level,
		*core_level,
		*cpu_level,
#endif
		*vp_level;

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
};

#ifdef MA__LWPS
#  define MA_LWP_INITIALIZER(lwp) { }
#else
#  define MA_LWP_INITIALIZER(lwp) { \
	.vp_level = &marcel_machine_level[0], \
}
#endif

#section macros

#define MARCEL_LWP_EST_DEF
#ifdef MA__LWPS
#  define MA_NR_VPS (MARCEL_NBMAXCPUS+MARCEL_NBMAXVPSUP)
#else
#  define MA_NR_VPS 1
#endif

#section variables
#ifdef MA__LWPS
extern ma_lwp_t ma_vp_lwp[MA_NR_VPS];
extern unsigned  ma__nb_vp;
#endif

#section marcel_variables

extern TBX_EXTERN marcel_lwp_t __main_lwp;
extern TBX_EXTERN ma_atomic_t ma__last_vp;

#ifdef MA__LWPS

#depend "asm/linux_rwlock.h[marcel_types]"
// Verrou protégeant la liste chaînée des LWPs
extern TBX_EXTERN ma_rwlock_t __ma_lwp_list_lock;
extern struct tbx_fast_list_head ma_list_lwp_head;

#if defined(MA__SELF_VAR) && (!defined(MA__LWPS) || !defined(MARCEL_DONT_USE_POSIX_THREADS))
extern TBX_EXTERN __thread marcel_lwp_t *ma_lwp_self;
#endif

#endif

#section marcel_functions
static __tbx_inline__ void ma_lwp_list_lock_read(void);
#section marcel_inline
#depend "linux_spinlock.h[marcel_macros]"
#depend "linux_spinlock.h[marcel_inline]"
static __tbx_inline__ void ma_lwp_list_lock_read(void)
{
#ifdef MA__LWPS
  ma_read_lock(&__ma_lwp_list_lock);
#endif
}

#section marcel_functions
static __tbx_inline__ void ma_lwp_list_unlock_read(void);
#section marcel_inline
static __tbx_inline__ void ma_lwp_list_unlock_read(void)
{
#ifdef MA__LWPS
  ma_read_unlock(&__ma_lwp_list_lock);
#endif
}

#section marcel_functions
static __tbx_inline__ void ma_lwp_list_lock_write(void);
#section marcel_inline
static __tbx_inline__ void ma_lwp_list_lock_write(void)
{
#ifdef MA__LWPS
  ma_write_lock(&__ma_lwp_list_lock);
#endif
}

#section marcel_functions
static __tbx_inline__ void ma_lwp_list_unlock_write(void);
#section marcel_inline
static __tbx_inline__ void ma_lwp_list_unlock_write(void)
{
#ifdef MA__LWPS
  ma_write_unlock(&__ma_lwp_list_lock);
#endif
}

/*
 * Expected number of VPs, i.e. number of CPUs or argument of --marcel-nvp
 */
#section functions
static __tbx_inline__ unsigned marcel_nbvps(void);
#section inline
#depend "[variables]"
static __tbx_inline__ unsigned marcel_nbvps(void)
{
#ifdef MA__LWPS
  return ma__nb_vp;
#else
  return 1;
#endif
}

/*
 * Current number of VPs, can be less than marcel_nbvps() during the start of
 * Marcel, can be more than marcel_nbvps() if supplementary VPs have been added.
 */
#section marcel_functions
static __tbx_inline__ unsigned marcel_nballvps(void);
#section marcel_inline
#depend "[marcel_variables]"
static __tbx_inline__ unsigned marcel_nballvps(void)
{
#ifdef MA__LWPS
  return ma_atomic_read(&ma__last_vp) + 1;
#else
  return 1;
#endif
}

#section marcel_functions
__tbx_inline__ static marcel_lwp_t* marcel_lwp_next_lwp(marcel_lwp_t* lwp);
#section marcel_inline
__tbx_inline__ static marcel_lwp_t* marcel_lwp_next_lwp(marcel_lwp_t* lwp)
{
  return tbx_fast_list_entry(lwp->lwp_list.next, marcel_lwp_t, lwp_list);
}

#section marcel_functions
__tbx_inline__ static marcel_lwp_t* marcel_lwp_prev_lwp(marcel_lwp_t* lwp);
#section marcel_inline
__tbx_inline__ static marcel_lwp_t* marcel_lwp_prev_lwp(marcel_lwp_t* lwp)
{
  return tbx_fast_list_entry(lwp->lwp_list.prev, marcel_lwp_t, lwp_list);
}

#section marcel_functions
/****************************************************************
 * Départ
 */
#if defined(MA__LWPS)
void marcel_lwp_fix_nb_vps(unsigned nb_lwp);
#endif

#ifdef MA__LWPS
unsigned marcel_lwp_add_vp(void);
unsigned marcel_lwp_add_lwp(int num);
void marcel_lwp_stop_lwp(marcel_lwp_t *lwp);
void ma_lwp_wait_active(void);
int ma_lwp_block(void);
marcel_lwp_t *ma_lwp_wait_vp_active(void);
#endif

#section functions
#ifdef MARCEL_BLOCKING_ENABLED
#  ifdef MA__LWPS
void marcel_enter_blocking_section(void);
void marcel_leave_blocking_section(void);
#  else
#    define marcel_enter_blocking_section() (void)0
#    define marcel_leave_blocking_section() (void)0
#  endif
#endif

#section functions
#ifdef MA__LWPS
/** Tell Marcel to stop using the given vpset, i.e.
 * migrate threads up and stop burning cpu time in idle. */
void marcel_disable_vps(const marcel_vpset_t *vpset);
/** Tell Marcel to use again the given vpset, i.e.
 * let idle restart burning cpu time. */
void marcel_enable_vps(const marcel_vpset_t *vpset);
#else /* MA__LWPS */
#  define marcel_disable_vps(vpset) (void)0
#  define marcel_enable_vps(vpset) (void)0
#endif /* MA__LWPS */

#section variables
extern marcel_vpset_t marcel_disabled_vpset;

#section marcel_macros
#include "tbx_compiler.h"
#depend "asm/linux_perlwp.h[marcel_macros]"
/****************************************************************
 * Accès aux LWP
 */

#define ma_get_task_vpnum(task)			(ma_vpnum(THREAD_GETMEM(task,lwp)))
#ifdef MA__LWPS
#  define ma_vpnum(lwp)				(ma_per_lwp(vpnum, lwp))
#  define ma_get_lwp_by_vpnum(vpnum)		(ma_vp_lwp[vpnum])
#  define ma_get_task_lwp(task)			((task)->lwp)
#  define ma_set_task_lwp(task, value)		((task)->lwp=(value))
#  define ma_init_lwp_vpnum(vpnum, lwp)		do { \
	if ((vpnum) == -1) { \
		ma_vpnum(lwp) = -1; \
		ma_lwp_rq(lwp)->father = NULL; \
	} else { \
		ma_vp_lwp[vpnum] = (lwp); \
	        ma_vpnum(lwp) = vpnum; \
		ma_lwp_rq(lwp)->father = &marcel_topo_vp_level[vpnum].rq; \
		ma_per_lwp(vp_level, lwp) = &marcel_topo_vp_level[vpnum]; \
	} \
} while(0)
#  define ma_clr_lwp_vpnum(lwp)			do { \
	MA_BUG_ON(ma_vpnum(lwp) == -1); \
	MA_BUG_ON(!ma_vp_lwp[ma_vpnum(lwp)]); \
	MA_BUG_ON(!ma_lwp_rq(lwp)->father); \
	ma_vp_lwp[ma_vpnum(lwp)] = NULL; \
	ma_init_lwp_vpnum(-1, lwp); \
} while(0)
#  define ma_set_lwp_vpnum(vpnum, lwp)		do { \
	if ((vpnum) == -1) { \
		ma_clr_lwp_vpnum(lwp); \
	} else { \
		MA_BUG_ON(ma_vp_lwp[vpnum]); \
		MA_BUG_ON(ma_vpnum(lwp) > 0); \
		MA_BUG_ON(ma_lwp_rq(lwp)->father); \
		ma_init_lwp_vpnum(vpnum, lwp); \
	} \
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

#section marcel_functions
static __tbx_inline__ unsigned ma_lwp_os_node(marcel_lwp_t *lwp);
#section marcel_inline
#depend "marcel_topology.h[]"
static __tbx_inline__ unsigned ma_lwp_os_node(marcel_lwp_t *lwp)
{
	unsigned vp = ma_vpnum(lwp);
	struct marcel_topo_level *l;
	if (vp == -1)
		vp = 0;
	l = ma_vp_node_level[vp];
	if (!l)
		return 0;
	return l->os_node;
}

#section marcel_macros

#define MA_DEFINE_LWP_NOTIFIER_START(name, help, \
			             prepare, prepare_help, \
			             online, online_help) \
  MA_DEFINE_LWP_NOTIFIER_START_PRIO(name, 0, help, \
				    prepare, prepare_help, \
				    online, online_help)
#define MA_DEFINE_LWP_NOTIFIER_START_PRIO(name, prio, help, \
				          prepare, prepare_help, \
				          online, online_help) \
  MA_DEFINE_LWP_NOTIFIER_TWO_PRIO(name, prio, help, \
				  UP_PREPARE, prepare, prepare_help, \
				  ONLINE, online, online_help, \
				  0, 1, 4, 5)
#define MA_DEFINE_LWP_NOTIFIER_ONOFF(name, help, \
			             online, online_help, \
			             offline, offline_help) \
  MA_DEFINE_LWP_NOTIFIER_ONOFF_PRIO(name, 0, help, \
			            online, online_help, \
			            offline, offline_help)
#define MA_DEFINE_LWP_NOTIFIER_ONOFF_PRIO(name, prio, help, \
			                  online, online_help, \
			                  offline, offline_help) \
  MA_DEFINE_LWP_NOTIFIER_TWO_PRIO(name, prio, help, \
				  ONLINE, online, online_help, \
				  OFFLINE, offline, offline_help, \
		  		  0, 1, 3, 4)

#define MA_DEFINE_LWP_NOTIFIER_TWO_PRIO(name, prio, help, \
				        ONE, one, one_help, \
				        TWO, two, two_help, \
					a, b, c, d \
					) \
  static int name##_notify(struct ma_notifier_block *self TBX_UNUSED, \
		           unsigned long action, void *hlwp) \
  { \
	ma_lwp_t lwp = (ma_lwp_t)hlwp; \
	switch(action) { \
	case MA_LWP_##ONE: \
		one(lwp); \
		break; \
	case MA_LWP_##TWO: \
		two(lwp); \
		break; \
	default: \
		break; \
	} \
	return 0; \
  } \
  static const char * name##_helps[6] = { \
	  [MA_LWP_##ONE]="Notifier [" help "] " one_help, \
	  [MA_LWP_##TWO]="Notifier [" help "] " two_help, \
	  [a]="Notifier [" help "] [none]", \
	  [b]="Notifier [" help "] [none]", \
	  [c]="Notifier [" help "] [none]", \
	  [d]="Notifier [" help "] [none]", \
  }; \
  static MA_DEFINE_NOTIFIER_BLOCK_INTERNAL(name##_nb, name##_notify, \
	prio, help, 4, name##_helps); \
  static void __marcel_init marcel_##name##_notifier_register(void) \
  { \
        ma_register_lwp_notifier(&name##_nb); \
  } \
  __ma_initfunc_prio(marcel_##name##_notifier_register, \
                MA_INIT_REGISTER_LWP_NOTIFIER, \
                MA_INIT_REGISTER_LWP_NOTIFIER_PRIO, \
                "Registering notifier " #name " (" help ") at prio "#prio)

#define MA_LWP_NOTIFIER_CALL_UP_PREPARE(name, section) \
  MA_LWP_NOTIFIER_CALL(name, section, MA_INIT_PRIO_BASE, UP_PREPARE)
#define MA_LWP_NOTIFIER_CALL_UP_PREPARE_PRIO(name, section, prio) \
  MA_LWP_NOTIFIER_CALL(name, section, prio, UP_PREPARE)
#define MA_LWP_NOTIFIER_CALL_ONLINE(name, section) \
  MA_LWP_NOTIFIER_CALL(name, section, MA_INIT_PRIO_BASE, ONLINE)
#define MA_LWP_NOTIFIER_CALL_ONLINE_PRIO(name, section, prio) \
  MA_LWP_NOTIFIER_CALL(name, section, prio, ONLINE)
#define MA_LWP_NOTIFIER_CALL(name, section, prio, PART) \
  static void __marcel_init marcel_##name##_call_##PART(void) \
  { \
	name##_notify(&name##_nb, (unsigned long)MA_LWP_##PART, \
		   (void *)(ma_lwp_t)MA_LWP_SELF); \
  } \
  __ma_initfunc_prio_internal(marcel_##name##_call_##PART, section, prio, \
                              &name##_helps[MA_LWP_##PART])


#section marcel_functions
static __tbx_inline__ int ma_lwp_online(ma_lwp_t lwp);
#ifdef MA__LWPS
/* Need to know about LWPs going up/down? */
extern int ma_register_lwp_notifier(struct ma_notifier_block *nb);
extern void ma_unregister_lwp_notifier(struct ma_notifier_block *nb);

#else
static __tbx_inline__ int ma_register_lwp_notifier(struct ma_notifier_block *nb);
static __tbx_inline__ void ma_unregister_lwp_notifier(struct ma_notifier_block *nb);
#endif /* MA__LWPS */
#section marcel_inline
#ifndef MA__LWPS
static __tbx_inline__ int ma_register_lwp_notifier(struct ma_notifier_block *nb)
{
        return 0;
}
static __tbx_inline__ void ma_unregister_lwp_notifier(struct ma_notifier_block *nb)
{
}
#endif /* MA__LWPS */

static __tbx_inline__ int ma_lwp_online(ma_lwp_t lwp)
{
#ifdef MA__LWPS
	return ma_per_lwp(online,lwp);
#else
	return 1;
#endif
}

