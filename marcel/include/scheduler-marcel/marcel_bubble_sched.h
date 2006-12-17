
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

/******************************************************************************
 * user view
 */

#include <stdlib.h>

#section types
typedef struct marcel_bubble marcel_bubble_t;

#section variables
extern marcel_bubble_t marcel_root_bubble;

#section marcel_variables
extern int ma_idle_scheduler;
extern ma_rwlock_t ma_idle_scheduler_lock;

#section functions
#depend "marcel_topology.h[types]"
#depend "scheduler/marcel_bubble_sched.h[types]"
#depend "scheduler/marcel_holder.h[types]"
#depend "marcel_descr.h[types]"
int marcel_bubble_init(marcel_bubble_t *bubble);
int marcel_bubble_destroy(marcel_bubble_t *bubble);
#define marcel_bubble_destroy(bubble) 0

int marcel_entity_setschedlevel(marcel_entity_t *entity, int level);
int marcel_entity_getschedlevel(__const marcel_entity_t *entity, int *level);

#define marcel_bubble_setschedlevel(bubble,level) marcel_entity_setschedlevel(&(bubble)->sched,level)
#define marcel_bubble_getschedlevel(bubble,level) marcel_entity_getschedlevel(&(bubble)->sched,level)
#define marcel_task_setschedlevel(task,level) marcel_entity_setschedlevel(&(task)->sched.internal.entity,level)
#define marcel_task_getschedlevel(task,level) marcel_entity_getschedlevel(&(task)->sched.internal.entity,level)

int marcel_bubble_setprio(marcel_bubble_t *bubble, int prio);
int marcel_bubble_sleep_locked(marcel_bubble_t *bubble);
int marcel_bubble_sleep_rq_locked(marcel_bubble_t *bubble);
int marcel_bubble_wake_locked(marcel_bubble_t *bubble);
int marcel_bubble_wake_rq_locked(marcel_bubble_t *bubble);
int marcel_bubble_getprio(__const marcel_bubble_t *bubble, int *prio);

int marcel_bubble_setinitrq(marcel_bubble_t *bubble, ma_runqueue_t *rq);
int marcel_bubble_setinitlevel(marcel_bubble_t *bubble, marcel_topo_level_t *level);

int marcel_bubble_insertentity(marcel_bubble_t *bubble, marcel_entity_t *entity);
int marcel_bubble_insertbubble(marcel_bubble_t *bubble, marcel_bubble_t *little_bubble);
int marcel_bubble_inserttask(marcel_bubble_t *bubble, marcel_task_t *task);

int marcel_bubble_removeentity(marcel_bubble_t *bubble, marcel_entity_t *entity);
int marcel_bubble_removebubble(marcel_bubble_t *bubble, marcel_bubble_t *little_bubble);
int marcel_bubble_removetask(marcel_bubble_t *bubble, marcel_task_t *task);

#define marcel_bubble_insertbubble(bubble, littlebubble) marcel_bubble_insertentity(bubble, &(littlebubble)->sched)
#define marcel_bubble_inserttask(bubble, task) marcel_bubble_insertentity(bubble, &(task)->sched.internal.entity)

#define marcel_bubble_removebubble(bubble, littlebubble) marcel_bubble_removeentity(bubble, &(littlebubble)->sched)
#define marcel_bubble_removetask(bubble, task) marcel_bubble_removeentity(bubble, &(task)->sched.internal.entity)

void marcel_wake_up_bubble(marcel_bubble_t *bubble);

void marcel_bubble_join(marcel_bubble_t *bubble);

int marcel_bubble_detach(marcel_bubble_t *bubble);

void marcel_sched_exit(marcel_t t);
#ifndef MA__BUBBLES
#define marcel_sched_exit(t)
#endif

int marcel_bubble_barrier(marcel_bubble_t *bubble);
#define marcel_bubble_barrier(b) marcel_barrier_wait(&(b)->barrier)

marcel_bubble_t *marcel_bubble_holding_task(marcel_task_t *task);
marcel_bubble_t *marcel_bubble_holding_bubble(marcel_bubble_t *bubble);
marcel_bubble_t *marcel_bubble_holding_entity(marcel_entity_t *entity);

#define marcel_bubble_holding_bubble(b) marcel_bubble_holding_entity(&(b)->sched)
#define marcel_bubble_holding_task(t) marcel_bubble_holding_entity(&(t)->sched.internal.entity)

void ma_bubble_synthesize_stats(marcel_bubble_t *bubble);

#section marcel_macros
#define ma_bubble_stats_get(b,offset) ma_stats_get(&(b)->sched, (offset))
#define ma_bubble_hold_stats_get(b,offset) ma_stats_get(&(b)->hold, (offset))

#section functions
#ifdef MARCEL_BUBBLE_EXPLODE
void marcel_close_bubble(marcel_bubble_t *bubble);
#endif

/******************************************************************************
 * scheduler view
 */

#section structures
#depend "pm2_list.h"
#depend "scheduler/marcel_holder.h[structures]"
#depend "marcel_sem.h[types]"
#depend "marcel_sem.h[structures]"

#ifdef MARCEL_BUBBLE_EXPLODE
typedef enum {
	MA_BUBBLE_CLOSED,
	MA_BUBBLE_OPENED,
	MA_BUBBLE_CLOSING,
} ma_bubble_status;
#endif

struct marcel_bubble {
	/* garder en premier, pour que les conversion bubble / entity soient
	 * triviales */
	struct ma_sched_entity sched;
	struct ma_holder hold;
	struct list_head heldentities;
	unsigned nbentities;
	marcel_sem_t join;
#ifdef MARCEL_BUBBLE_EXPLODE
	ma_bubble_status status;
	int nbrunning; /* number of entities released by the bubble (i.e. currently in runqueues) */
#endif
#ifdef MARCEL_BUBBLE_STEAL
	/* next entity to try to run in this bubble */
	struct ma_sched_entity *next;
	/* list of running entities */
	struct list_head runningentities;
	int settled;
#endif
	marcel_barrier_t barrier;
};

#section macros
/*  LIST_HEAD_INIT */
#depend "pm2_list.h"
/*  MA_ATOMIC_INIT */
#depend "asm/linux_atomic.h"
#depend "scheduler/marcel_holder.h[macros]"

/* Attention: ne doit être utilisé que par marcel_bubble_init */
#ifdef MARCEL_BUBBLE_EXPLODE
#define MARCEL_BUBBLE_SCHED_INITIALIZER(b) \
	.status = MA_BUBBLE_CLOSED, \
	.nbrunning = 0,
#endif
#ifdef MARCEL_BUBBLE_STEAL
#define MARCEL_BUBBLE_SCHED_INITIALIZER(b) \
	.next = NULL, \
	.runningentities = LIST_HEAD_INIT((b).runningentities), \
	.settled = 0,
#endif

#define MARCEL_BUBBLE_INITIALIZER(b) { \
	.sched = MA_SCHED_ENTITY_INITIALIZER((b).sched, MA_BUBBLE_ENTITY, MA_BATCH_PRIO), \
	.hold = MA_HOLDER_INITIALIZER(MA_BUBBLE_HOLDER), \
	.heldentities = LIST_HEAD_INIT((b).heldentities), \
	.nbentities = 0, \
	.join = MARCEL_SEM_INITIALIZER(1), \
	MARCEL_BUBBLE_SCHED_INITIALIZER(b) \
	.barrier = MARCEL_BARRIER_INITIALIZER(0), \
}


#section marcel_functions
/* called from ma_schedule() to achieve bubble scheduling. prevrq and rq are
 * locked, returns 1 if ma_schedule() should restart (prevrq and rq have then
 * been released), returns 0 if ma_schedule() can continue with entity nextent
 * (prevrq and rq have been kept locked) */
marcel_entity_t *ma_bubble_sched(marcel_entity_t *nextent,
		ma_holder_t *prevh, ma_runqueue_t *rq,
		ma_holder_t **nexth, int idx);

/* called from ma_scheduler_tick() when the timeslice for the currently running
 * bubble has expired */
void ma_bubble_tick(marcel_bubble_t *bubble);

#section marcel_functions
#ifdef MARCEL_BUBBLE_STEAL
static __tbx_inline__ void __ma_bubble_enqueue_entity(marcel_entity_t *e, marcel_bubble_t *b);
static __tbx_inline__ void __ma_bubble_dequeue_entity(marcel_entity_t *e, marcel_bubble_t *b);

static __tbx_inline__ void ma_bubble_enqueue_entity(marcel_entity_t *e, marcel_bubble_t *b);
void ma_bubble_enqueue_task(marcel_task_t *t, marcel_bubble_t *b);
void ma_bubble_enqueue_bubble(marcel_bubble_t *sb, marcel_bubble_t *b);
#define ma_bubble_enqueue_task(t,b) ma_bubble_enqueue_entity(&t->sched.internal.entity,b)
#define ma_bubble_enqueue_bubble(sb,b) ma_bubble_enqueue_entity(&sb->sched,b)
static __tbx_inline__ void ma_bubble_dequeue_entity(marcel_entity_t *e, marcel_bubble_t *b);
void ma_bubble_dequeue_task(marcel_task_t *t, marcel_bubble_t *b);
void ma_bubble_dequeue_bubble(marcel_bubble_t *sb, marcel_bubble_t *b);
#define ma_bubble_dequeue_task(t,b) ma_bubble_dequeue_entity(&t->sched.internal.entity,b)
#define ma_bubble_dequeue_bubble(sb,b) ma_bubble_dequeue_entity(&sb->sched,b)
#endif

#section marcel_inline
/* cette version suppose que la runqueue contenant b est déjà verrouillée */
static __tbx_inline__ void __ma_bubble_enqueue_entity(marcel_entity_t *e, marcel_bubble_t *b) {
#ifdef MA__BUBBLES
	bubble_sched_debugl(7,"enqueuing %p in bubble %p\n",e,b);
#ifdef MARCEL_BUBBLE_STEAL
	if (list_empty(&b->runningentities) && b->sched.prio == MA_NOSCHED_PRIO) {
		bubble_sched_debugl(7,"first running entity in bubble %p\n",b);
		marcel_bubble_wake_rq_locked(b);
	}
	list_add_tail(&e->run_list, &b->runningentities);
#endif
	MA_BUG_ON(e->holder_data);
	e->holder_data = (void *)1;
#endif
}
static __tbx_inline__ void __ma_bubble_dequeue_entity(marcel_entity_t *e, marcel_bubble_t *b) {
#ifdef MA__BUBBLES
	bubble_sched_debugl(7,"dequeuing %p from bubble %p\n",e,b);
#ifdef MARCEL_BUBBLE_STEAL
	list_del(&e->run_list);
	if (list_empty(&b->runningentities) && b->sched.prio != MA_NOSCHED_PRIO) {
		bubble_sched_debugl(7,"last running entity in bubble %p\n",b);
		marcel_bubble_sleep_rq_locked(b);
	}
#endif
	MA_BUG_ON(!e->holder_data);
	e->holder_data = NULL;
#endif
}

static __tbx_inline__ void ma_bubble_enqueue_entity(marcel_entity_t *e, marcel_bubble_t *b) {
#ifdef MA__BUBBLES
	bubble_sched_debugl(7,"enqueuing %p in bubble %p\n",e,b);
#ifdef MARCEL_BUBBLE_STEAL
	if (list_empty(&b->runningentities) && b->sched.prio == MA_NOSCHED_PRIO) {
		bubble_sched_debugl(7,"first running entity in bubble %p\n",b);
		marcel_bubble_wake_locked(b);
	}
	list_add_tail(&e->run_list, &b->runningentities);
#endif
	MA_BUG_ON(e->holder_data);
	e->holder_data = (void *)1;
#endif
}
static __tbx_inline__ void ma_bubble_dequeue_entity(marcel_entity_t *e, marcel_bubble_t *b) {
#ifdef MA__BUBBLES
	bubble_sched_debugl(7,"dequeuing %p from bubble %p\n",e,b);
#ifdef MARCEL_BUBBLE_STEAL
	list_del(&e->run_list);
#if 0
	if (list_empty(&b->runningentities) && b->sched.prio != MA_NOSCHED_PRIO) {
		bubble_sched_debugl(7,"last running entity in bubble %p\n",b);
		marcel_bubble_sleep_locked(b);
	}
#endif
#endif
	MA_BUG_ON(!e->holder_data);
	e->holder_data = NULL;
#endif
}
