
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
#section common
#depend "tbx_compiler.h"

#section marcel_types
typedef struct marcel_lwp marcel_lwp_t;
typedef struct marcel_lwp *ma_lwp_t;

#section marcel_structures
#depend "pm2_list.h"
#depend "marcel_sched_generic.h[marcel_structures]"
#depend "marcel_sem.h[structures]"
#depend "sys/marcel_kthread.h[marcel_types]"
#depend "linux_timer.h[marcel_types]"
#depend "linux_softirq.h[marcel_structures]"
#depend "scheduler/linux_runqueues.h[types]"
#depend "scheduler/linux_runqueues.h[marcel_structures]"
#depend "marcel_topology.h[structures]"
#include <stdlib.h>

struct marcel_lwp {
	struct list_head lwp_list;
#ifdef MA__ACTIVATION
	act_proc_info_t act_infos;
#endif
#ifdef MA__SMP
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

#ifdef MARCEL_SMT_IDLE
	struct marcel_topo_level *core_level;
#endif

#if defined(LINUX_SYS) || defined(GNU_SYS)
	struct drand48_data random_buffer;
#endif

	marcel_task_t *previous_thread;
	boolean main_is_waiting;
	unsigned nb_tasks;
// Utilise par les fonctions one_more_task, wait_all_tasks, etc.
	ma_spinlock_t threadlist_lock;

	volatile sig_atomic_t task_number;
	struct list_head all_threads;

#ifdef MA__LWPS
	marcel_task_t *idle_task;
#endif

	marcel_postexit_func_t postexit_func;
	any_t postexit_arg;
	marcel_sem_t postexit_thread;
	marcel_sem_t postexit_space;

#ifdef MA__ACTIVATION
	marcel_task_t *upcall_new_task;
#endif

#ifdef MA__LWPS
#ifdef IA64_ARCH
	unsigned long ma_ia64_tp;
#endif
#endif
	ma_tvec_base_t tvec_bases;

#ifdef MA__TIMER
	int _no_interrupt;
#endif

	marcel_task_t *ksoftirqd;
	unsigned long softirq_pending;
	struct ma_tasklet_head tasklet_vec, tasklet_hi_vec;

#ifdef MA__LWPS
	unsigned number;
#endif
	unsigned online;
	marcel_task_t *run_task;

	//unsigned long process_counts;
	
#ifdef MA__NUMA
	struct marcel_topo_level *node_level, *cpu_level;
#endif
};

#define MA_LWP_INITIALIZER(lwp) (marcel_lwp_t) { \
	.threadlist_lock = MA_SPIN_LOCK_UNLOCKED, \
	.task_number = 1, \
	.all_threads = LIST_HEAD_INIT((lwp)->all_threads), \
	.postexit_thread = MARCEL_SEM_INITIALIZER(0), \
	.postexit_space = MARCEL_SEM_INITIALIZER(1), \
}

#section marcel_macros

#define MARCEL_LWP_EST_DEF
#ifdef MA__LWPS
#  ifdef MA__ACTIVATION
#    include <asm/act.h>
#    ifndef ACT_NB_MAX_CPU
#      error Perhaps, you should add CFLAGS=-I/lib/modules/`uname -r`/build/include
#      undef MA__ACTIVATION
#    endif
#    undef MAX_LWP
#    define MAX_LWP ACT_NB_MAX_CPU
#  else
#    define MA_NR_LWPS (sizeof(unsigned long)*8)
#  endif
#else
#  define MA_NR_LWPS 1
#endif

#section marcel_variables
#ifdef MA__LWPS
extern marcel_lwp_t* addr_lwp[MA_NR_LWPS];
#endif

extern marcel_lwp_t __main_lwp;

#ifdef MA__LWPS

#depend "asm/linux_rwlock.h[marcel_types]"
// Verrou protégeant la liste chaînée des LWPs
extern ma_rwlock_t __lwp_list_lock;
extern struct list_head list_lwp_head;

extern unsigned  ma__nb_lwp;
//extern marcel_lwp_t* addr_lwp[MA__MAX_LWPS];

#endif

#section marcel_functions
static __tbx_inline__ void lwp_list_lock_read(void);
#section marcel_inline
#depend "linux_spinlock.h[marcel_macros]"
#depend "linux_spinlock.h[marcel_inline]"
static __tbx_inline__ void lwp_list_lock_read(void)
{
#ifdef MA__LWPS
  ma_read_lock(&__lwp_list_lock);
#endif
}

#section marcel_functions
static __tbx_inline__ void lwp_list_unlock_read(void);
#section marcel_inline
static __tbx_inline__ void lwp_list_unlock_read(void)
{
#ifdef MA__LWPS
  ma_read_unlock(&__lwp_list_lock);
#endif
}

#section marcel_functions
static __tbx_inline__ void lwp_list_lock_write(void);
#section marcel_inline
static __tbx_inline__ void lwp_list_lock_write(void)
{
#ifdef MA__LWPS
  ma_write_lock(&__lwp_list_lock);
#endif
}

#section marcel_functions
static __tbx_inline__ void lwp_list_unlock_write(void);
#section marcel_inline
static __tbx_inline__ void lwp_list_unlock_write(void)
{
#ifdef MA__LWPS
  ma_write_unlock(&__lwp_list_lock);
#endif
}

#section functions
static __tbx_inline__ unsigned get_nb_lwps(void);
unsigned marcel_nbvps(void);
#section inline
#depend "[marcel_variables]"
static __tbx_inline__ unsigned get_nb_lwps(void)
{
#ifdef MA__LWPS
  return ma__nb_lwp;
#else
  return 1;
#endif
}

#section functions
static __tbx_inline__ unsigned marcel_get_nb_lwps_np(void);
#section inline
static __tbx_inline__ unsigned marcel_get_nb_lwps_np(void)
{
   return get_nb_lwps();
}

#section marcel_functions
static __tbx_inline__ void set_nb_lwps(unsigned value);
#section marcel_inline
static __tbx_inline__ void set_nb_lwps(unsigned value)
{
#ifdef MA__LWPS
  ma__nb_lwp=value;
#endif
}

#section marcel_functions
__tbx_inline__ static marcel_lwp_t* marcel_lwp_next_lwp(marcel_lwp_t* lwp);
#section marcel_inline
__tbx_inline__ static marcel_lwp_t* marcel_lwp_next_lwp(marcel_lwp_t* lwp)
{
  return list_entry(lwp->lwp_list.next, marcel_lwp_t, lwp_list);
}

#section marcel_functions
__tbx_inline__ static marcel_lwp_t* marcel_lwp_prev_lwp(marcel_lwp_t* lwp);
#section marcel_inline
__tbx_inline__ static marcel_lwp_t* marcel_lwp_prev_lwp(marcel_lwp_t* lwp)
{
  return list_entry(lwp->lwp_list.prev, marcel_lwp_t, lwp_list);
}

#section marcel_functions
/****************************************************************
 * Départ
 */
#if defined(MA__LWPS)
void marcel_lwp_fix_nb_vps(unsigned nb_lwp);
#endif

#ifdef MA__SMP
unsigned marcel_lwp_add_vp(void);
void marcel_lwp_stop_lwp(marcel_lwp_t *lwp);
#endif

#section marcel_macros
#include "tbx_compiler.h"
#depend "asm/linux_perlwp.h[marcel_macros]"
/****************************************************************
 * Accès aux LWP
 */

#define GET_LWP_NUMBER(current)             (LWP_NUMBER(THREAD_GETMEM(current,sched.lwp)))
#ifdef MA__LWPS
#  define LWP_NUMBER(lwp)                     (ma_per_lwp(number, lwp))
#  define GET_LWP_BY_NUM(proc)                (addr_lwp[proc])
#  define GET_LWP(current)                    ((current)->sched.lwp)
#  define SET_LWP(current, value)             ((current)->sched.lwp=(value))
#  define GET_CUR_LWP()                       (cur_lwp)
#  define SET_CUR_LWP(value)                  (cur_lwp=(value))
#  define SET_LWP_NB(number, lwp)             (addr_lwp[number]=(lwp), \
	                                       LWP_NUMBER(lwp)=number)
#  define GET_LWP_BY_NUMBER(number)           (addr_lwp[number])
#  define DEFINE_CUR_LWP(OPTIONS, signe, lwp) \
     OPTIONS marcel_lwp_t *cur_lwp signe lwp
#  define IS_FIRST_LWP(lwp)                   (LWP_NUMBER(lwp)==0)

#  define for_all_lwp(lwp) \
     list_for_each_entry(lwp, &list_lwp_head, lwp_list)
#  define for_all_lwp_from_begin(lwp, lwp_start) \
     list_for_each_entry_from_begin(lwp, &list_lwp_head, lwp_start, lwp_list)
#  define for_all_lwp_from_end() \
     list_for_each_entry_from_end()
#  define lwp_isset(num, map) ma_test_bit(num, &map)
#else
#  define cur_lwp                             (&__main_lwp)
#  define LWP_NUMBER(lwp)                     ((void)(lwp),0)
#  define GET_LWP_BY_NUM(nb)                  (cur_lwp)
#  define GET_LWP(current)                    (cur_lwp)
#  define SET_LWP(current, value)             ((void)0)
#  define GET_CUR_LWP()                       (cur_lwp)
#  define SET_CUR_LWP(value)                  ((void)0)
#  define SET_LWP_NB(proc, value)             ((void)0)
#  define GET_LWP_BY_NUMBER(number)           (cur_lwp)
#  define DEFINE_CUR_LWP(OPTIONS, signe, current) \
     int __cur_lwp_unused__ TBX_UNUSED
#  define IS_FIRST_LWP(lwp)                   (1)

#  define for_all_lwp(lwp) for (lwp=cur_lwp;lwp;lwp=NULL)
#  define for_all_lwp_from_begin(lwp, lwp_start) for(lwp=lwp_start;0;) {
#  define for_all_lwp_from_end() }
#  define lwp_isset(num, map) 1
#endif

#  define for_each_lwp_begin(lwp) \
     for_all_lwp(lwp) {\
        if (ma_lwp_online(lwp)) {
#  define for_each_lwp_end() \
        } \
     }

#  define for_each_lwp_from_begin(lwp, lwp_start) \
     for_all_lwp_from_begin(lwp, lwp_start) \
        if (ma_lwp_online(lwp)) {
#  define for_each_lwp_from_end() \
        } \
     for_all_lwp_from_end()

#define LWP_GETMEM(lwp, member) ((lwp)->member)
#define LWP_SELF                (GET_LWP(MARCEL_SELF))
#define ma_softirq_pending(lwp) \
   (ma_per_lwp(softirq_pending,(lwp)))
#define ma_local_softirq_pending() \
   (__ma_get_lwp_var(softirq_pending))

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
  static int name##_notify(struct ma_notifier_block *self, \
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
  void __marcel_init marcel_##name##_notifier_register(void) \
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
  void __marcel_init marcel_##name##_call_##PART(void) \
  { \
	name##_notify(&name##_nb, (unsigned long)MA_LWP_##PART, \
		   (void *)(ma_lwp_t)LWP_SELF); \
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

