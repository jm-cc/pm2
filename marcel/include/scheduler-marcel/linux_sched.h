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


#ifndef __LINUX_SCHED_H__
#define __LINUX_SCHED_H__


#include "tbx_compiler.h"
#include "sys/marcel_flags.h"
#include "marcel_types.h"
#ifdef __MARCEL_KERNEL__
#include "linux_linkage.h"
#include "marcel_rwlock.h"
#endif


/** Public functions **/
extern void marcel_wake_up_created_thread(marcel_task_t * tsk);
/** Force a LWP to be rescheduled */
extern void ma_resched_task(marcel_task_t *p, int vp, ma_lwp_t lwp);
/** Force a whole VPset to be rescheduled */
extern void ma_resched_vpset(const marcel_vpset_t *vpset);
/** Same, but lwp rqs of the VPset are already locked */
extern void __ma_resched_vpset(const marcel_vpset_t *vpset);
/** Force VPs of a whole level to be rescheduled */
extern void ma_resched_topo_level(struct marcel_topo_level *l);
/** Same, but lwp rqs of the level VPset are already locked */
extern void __ma_resched_topo_level(struct marcel_topo_level *l);


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
#define	MA_MAX_SCHEDULE_TIMEOUT	LONG_MAX


/** Internal global variables **/
extern int nr_threads;

/*
 * This serializes "schedule()" and also protects
 * the run-queue from deletions/modifications (but
 * _adding_ to the beginning of the run-queue has
 * a separate lock).
 */
extern ma_rwlock_t tasklist_lock;
extern TBX_EXTERN void ma_scheduler_tick(int user_tick, int system);


/** Internal functions **/
extern TBX_EXTERN void __ma_cond_resched(void);

extern void ma_linux_sched_init0(void);

extern unsigned long ma_nb_ready_entities(void);
asmlinkage TBX_EXTERN int ma_schedule(void);
asmlinkage void ma_schedule_tail(marcel_task_t *prev);
extern int ma_try_to_wake_up(marcel_task_t * p, unsigned int state, int sync);
extern int FASTCALL(ma_wake_up_state(marcel_task_t * tsk, unsigned int state));
extern TBX_EXTERN int FASTCALL(ma_wake_up_thread(marcel_task_t * tsk));
extern int FASTCALL(ma_wake_up_thread_async(marcel_task_t * tsk));
#ifdef MA__LWPS
extern void ma_kick_process(marcel_task_t * tsk);
#else
static __tbx_inline__ void ma_kick_process(marcel_task_t *tsk) { }
#endif
int ma_sched_change_prio(marcel_t t, int prio);
#ifdef MA__LWPS
extern void ma_wait_task_inactive(marcel_task_t * p);
#else
#  define ma_wait_task_inactive(p)	do { } while (0)
#endif

extern void marcel_bind_to_topo_level(marcel_topo_level_t *level);

int marcel_idle_lwp(ma_lwp_t lwp);
int marcel_task_prio(marcel_task_t *p);
int marcel_task_curr(marcel_task_t *p);


#endif /** __MARCEL_KERNEL__ **/


#endif /** __LINUX_SCHED_H__ **/
