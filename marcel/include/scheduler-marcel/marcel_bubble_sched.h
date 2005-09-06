
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

#section marcel_macros
#define MARCEL_LEVEL_DEFAULT MARCEL_LEVEL_MACHINE

#section types
typedef struct marcel_bubble marcel_bubble_t;

#section variables
extern marcel_bubble_t marcel_root_bubble;

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
#define marcel_task_setschedlevel(task,level) marcel_entity_setschedlevel(&(task)->sched.internal,level)
#define marcel_task_getschedlevel(task,level) marcel_entity_getschedlevel(&(task)->sched.internal,level)

int marcel_bubble_setprio(marcel_bubble_t *bubble, int prio);
int marcel_bubble_getprio(__const marcel_bubble_t *bubble, int *prio);

int marcel_bubble_insertentity(marcel_bubble_t *bubble, marcel_entity_t *entity);
int marcel_bubble_insertbubble(marcel_bubble_t *bubble, marcel_bubble_t *little_bubble);
int marcel_bubble_inserttask(marcel_bubble_t *bubble, marcel_task_t *task);

#define marcel_bubble_insertbubble(bubble, littlebubble) marcel_bubble_insertentity(bubble, &(littlebubble)->sched)
#define marcel_bubble_inserttask(bubble, task) marcel_bubble_insertentity(bubble, &(task)->sched.internal)

void marcel_wake_up_bubble(marcel_bubble_t *bubble);

marcel_bubble_t *marcel_bubble_holding_task(marcel_task_t *task);
marcel_bubble_t *marcel_bubble_holding_bubble(marcel_bubble_t *bubble);
marcel_bubble_t *marcel_bubble_holding_entity(marcel_entity_t *entity);

#define marcel_bubble_holding_bubble(b) marcel_bubble_holding_entity(&(b)->sched)
#define marcel_bubble_holding_task(t) marcel_bubble_holding_entity(&(t)->sched.internal)

#ifdef MARCEL_BUBBLE_EXPLODE
void marcel_close_bubble(marcel_bubble_t *bubble);
#endif

/******************************************************************************
 * scheduler view
 */

#section structures
#depend "pm2_list.h"
#depend "scheduler/marcel_holder.h[structures]"

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
#ifdef MARCEL_BUBBLE_EXPLODE
	ma_bubble_status status;
	int nbrunning; /* number of entities released by the bubble (i.e. currently in runqueues) */
#endif
#ifdef MARCEL_BUBBLE_STEAL
	/* number of LWPs running a thread in bubble */
	int nr_active;
	/* next entity to try to run in this bubble */
	struct ma_sched_entity *next;
	/* holding bubble that has some of our content as next entity to
	 * try to run in it */
	struct marcel_bubble *next_user;
	///* first holding bubble that is on a runqueue */
	//struct marcel_bubble *rq_bubble;
	//en fait non: c'est sched_holder
	/* list of running entities */
	struct list_head runningentities;
#endif
};

#section macros
// LIST_HEAD_INIT
#depend "pm2_list.h"
// MA_SPIN_LOCK_UNLOCKED
#depend "asm/linux_spinlock.h"
// MA_ATOMIC_INIT
#depend "asm/linux_atomic.h"
#depend "scheduler/marcel_holder.h[macros]"

#ifdef MARCEL_BUBBLE_EXPLODE
#define MARCEL_BUBBLE_SCHED_INITIALIZER(b) \
	.status = MA_BUBBLE_CLOSED, \
	.nbrunning = 0,
#endif
#ifdef MARCEL_BUBBLE_STEAL
#define MARCEL_BUBBLE_SCHED_INITIALIZER(b) \
	.nr_active = 0, \
	.next = NULL, \
	.runningentities = LIST_HEAD_INIT((b).runningentities),
#endif

#define MARCEL_BUBBLE_INITIALIZER(b) { \
	.hold = MA_HOLDER_INITIALIZER(MA_BUBBLE_HOLDER), \
	.heldentities = LIST_HEAD_INIT((b).heldentities), \
	.sched = MA_SCHED_ENTITY_INITIALIZER((b).sched, MA_BUBBLE_ENTITY, MA_BATCH_PRIO), \
	MARCEL_BUBBLE_SCHED_INITIALIZER(b) \
}


#section marcel_functions
/* called from ma_schedule() to achieve bubble scheduling. prevrq and rq are
 * locked, returns 1 if ma_schedule() should restart (prevrq and rq have then
 * been released), returns 0 if ma_schedule() can continue with entity nextent
 * (prevrq and rq have been kept locked) */
marcel_entity_t *ma_bubble_sched(marcel_entity_t *nextent,
		ma_runqueue_t *prevrq, ma_runqueue_t *rq,
		ma_holder_t **nexth, int idx);

/* called from ma_scheduler_tick() when the timeslice for the currently running
 * bubble has expired */
void ma_bubble_tick(marcel_bubble_t *bubble);

#ifdef MARCEL_BUBBLE_STEAL
static __tbx_inline__ void ma_bubble_enqueue_entity(marcel_entity_t *e, marcel_bubble_t *b);
void ma_bubble_enqueue_task(marcel_task_t *t, marcel_bubble_t *b);
void ma_bubble_enqueue_bubble(marcel_bubble_t *sb, marcel_bubble_t *b);
#define ma_bubble_enqueue_task(t,b) ma_bubble_enqueue_entity(&t->sched.internal,b)
#define ma_bubble_enqueue_bubble(sb,b) ma_bubble_enqueue_entity(&sb->sched,b)
static __tbx_inline__ void ma_bubble_dequeue_entity(marcel_entity_t *e, marcel_bubble_t *b);
void ma_bubble_dequeue_task(marcel_task_t *t, marcel_bubble_t *b);
void ma_bubble_dequeue_bubble(marcel_bubble_t *sb, marcel_bubble_t *b);
#define ma_bubble_dequeue_task(t,b) ma_bubble_dequeue_entity(&t->sched.internal,b)
#define ma_bubble_dequeue_bubble(sb,b) ma_bubble_dequeue_entity(&sb->sched,b)
#endif

#section marcel_inline
static inline void ma_bubble_enqueue_entity(marcel_entity_t *e, marcel_bubble_t *b) {
	bubble_sched_debug("enqueuing %p in bubble %p\n",e,b);
#ifdef MARCEL_BUBBLE_STEAL
	if (list_empty(&b->runningentities) && b->sched.run_holder && !b->sched.holder_data) {
		ma_holder_t *h;
		bubble_sched_debug("first running entity in bubble %p\n",b);
		ma_holder_rawunlock(&b->hold);
		h = ma_entity_holder_rawlock(&b->sched);
		MA_BUG_ON(h && ma_holder_type(h) != MA_RUNQUEUE_HOLDER);
		ma_holder_rawlock(&b->hold);
		if (h && b->sched.run_holder && !b->sched.holder_data && list_empty(&b->runningentities))
			ma_enqueue_entity_rq(&b->sched, ma_rq_holder(h));
		else
			bubble_sched_debug("Mmm, %p was actually not empty\n", b);
		ma_entity_holder_rawunlock(h);
	}
	list_add_tail(&e->run_list, &b->runningentities);
#endif
	MA_BUG_ON(e->holder_data);
	e->holder_data = (void *)1;
}
static inline void ma_bubble_dequeue_entity(marcel_entity_t *e, marcel_bubble_t *b) {
	bubble_sched_debug("dequeuing %p from bubble %p\n",e,b);
#ifdef MARCEL_BUBBLE_STEAL
	list_del(&e->run_list);
	if (list_empty(&b->runningentities) && b->sched.run_holder && b->sched.holder_data) {
		ma_holder_t *h;
		bubble_sched_debug("last running entity in bubble %p\n",b);
		ma_holder_rawunlock(&b->hold);
		h = ma_entity_holder_rawlock(&b->sched);
		MA_BUG_ON(h && ma_holder_type(h) != MA_RUNQUEUE_HOLDER);
		ma_holder_rawlock(&b->hold);
		if (h && b->sched.run_holder && b->sched.holder_data && list_empty(&b->runningentities))
			ma_dequeue_entity_rq(&b->sched, ma_rq_holder(h));
		else
			bubble_sched_debug("Mmm, %p was actually not empty\n", b);
		ma_entity_holder_rawunlock(h);
	}
#endif
	MA_BUG_ON(!e->holder_data);
	e->holder_data = NULL;
}
