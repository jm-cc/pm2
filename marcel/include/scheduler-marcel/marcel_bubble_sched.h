
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
typedef struct marcel_bubble_sched_entity marcel_bubble_entity_t;

#section variables
extern marcel_bubble_t marcel_root_bubble;

#section functions
#depend "marcel_topology.h[types]"
#depend "scheduler/marcel_bubble_sched.h[types]"
#depend "marcel_descr.h[types]"
int marcel_bubble_init(marcel_bubble_t *bubble);
int marcel_bubble_destroy(marcel_bubble_t *bubble);
#define marcel_bubble_destroy(bubble) 0

int marcel_entity_setschedlevel(marcel_bubble_entity_t *entity, int level);
int marcel_entity_getschedlevel(__const marcel_bubble_entity_t *entity, int *level);

#define marcel_bubble_setschedlevel(bubble,level) marcel_entity_setschedlevel(&(bubble)->sched,level)
#define marcel_bubble_getschedlevel(bubble,level) marcel_entity_getschedlevel(&(bubble)->sched,level)
#define marcel_task_setschedlevel(task,level) marcel_entity_setschedlevel(&(task)->sched.internal,level)
#define marcel_task_getschedlevel(task,level) marcel_entity_getschedlevel(&(task)->sched.internal,level)

int marcel_bubble_setprio(marcel_bubble_t *bubble, int prio);
int marcel_bubble_getprio(__const marcel_bubble_t *bubble, int *prio);

int marcel_bubble_insertentity(marcel_bubble_t *bubble, marcel_bubble_entity_t *entity);
int marcel_bubble_insertbubble(marcel_bubble_t *bubble, marcel_bubble_t *little_bubble);
int marcel_bubble_inserttask(marcel_bubble_t *bubble, marcel_task_t *task);

#define marcel_bubble_insertbubble(bubble, littlebubble) marcel_bubble_insertentity(bubble, &(littlebubble)->sched)
#define marcel_bubble_inserttask(bubble, task) marcel_bubble_insertentity(bubble, &(task)->sched.internal)

void marcel_wake_up_bubble(marcel_bubble_t *bubble);

marcel_bubble_t marcel_bubble_holding_task(marcel_task_t *task);
marcel_bubble_t marcel_bubble_holding_bubble(marcel_bubble_t *bubble);
marcel_bubble_t marcel_bubble_holding_entity(marcel_bubble_entity_t *entity);

#define marcel_bubble_holding_entity(e) ((e)->holdingbubble)
#define marcel_bubble_holding_bubble(b) marcel_bubble_holding_entity(&(b)->sched)
#define marcel_bubble_holding_task(t) marcel_bubble_holding_entity(&(t)->sched.internal)

#ifdef MARCEL_BUBBLE_EXPLODE
void marcel_close_bubble(marcel_bubble_t *bubble);
#endif

/******************************************************************************
 * scheduler view
 */

#section marcel_types

enum marcel_bubble_entity {
	MARCEL_TASK_ENTITY,
	MARCEL_BUBBLE_ENTITY,
};

#section marcel_structures
#depend "pm2_list.h"
#depend "scheduler/linux_sched.h[marcel_types]"
#depend "scheduler/linux_runqueues.h[marcel_types]"
#depend "scheduler/linux_runqueues.h[marcel_variables]"

#ifdef MARCEL_BUBBLE_EXPLODE
typedef enum {
	MA_BUBBLE_CLOSED,
	MA_BUBBLE_OPENED,
	MA_BUBBLE_CLOSING,
} ma_bubble_status;
#endif

struct marcel_bubble_sched_entity {
	enum marcel_bubble_entity type;
	struct list_head run_list; /* liste chaînée des entités prêtes */
	ma_runqueue_t *init_rq;
	ma_runqueue_t *cur_rq;
	ma_prio_array_t *array;
	int sched_policy;
	int prio;
	unsigned long timestamp, last_ran;
	ma_atomic_t time_slice;
	struct list_head entity_list;
	marcel_bubble_t *holdingbubble; /* ne change pas */
#ifdef MA__LWPS
	int sched_level;
#endif
#ifdef MARCEL_BUBBLE_STEAL
	struct list_head runningentities;
#endif
};

struct marcel_bubble {
	/* garder en premier, pour que les conversion bubble / entity soient
	 * triviales */
	struct marcel_bubble_sched_entity sched;
	struct list_head heldentities;
	/* verrouillage: toujours dans l'ordre contenant puis contenu:
	 * runqueue, puis bulle sur la runqueues, puis bulles contenues dans la
	 * bulle, mais sur la même runqueue, puis sous-runqueue, puis bulles
	 * contenues dan bulle, mais sur la sous-runqueue, etc... */
	ma_spinlock_t lock;
#ifdef MARCEL_BUBBLE_EXPLODE
	ma_bubble_status status;
	int nbrunning; /* number of entities released by the bubble (i.e. currently in runqueues) */
#endif
#ifdef MARCEL_BUBBLE_STEAL
	/* number of threads in bubble */
	int nbthreads;
	/* number of LWPs running a thread in bubble */
	int nbactive;
	/* next entity to try to run in this bubble */
	struct marcel_bubble_sched_entity *next;
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
#ifdef MA__LWPS
#define SCHED_LEVEL_INIT .sched_level = MARCEL_LEVEL_DEFAULT,
#else
#define SCHED_LEVEL_INIT
#endif

#ifdef MARCEL_BUBBLE_EXPLODE
#define MARCEL_BUBBLE_SCHED_INITIALIZER(b) \
	.status = MA_BUBBLE_CLOSED, \
	.nbrunning = 0,
#endif
#ifdef MARCEL_BUBBLE_STEAL
#define MARCEL_BUBBLE_SCHED_INITIALIZER(b) \
	.nbthreads = 0, \
	.nbactive = 0, \
	.next = NULL, \
	.runningentities = LIST_HEAD_INIT((b).runningentities),
#endif

#define MARCEL_BUBBLE_INITIALIZER(b) { \
	.heldentities = LIST_HEAD_INIT((b).heldentities), \
	.lock = MA_SPIN_LOCK_UNLOCKED, \
	.sched = { \
		.type = MARCEL_BUBBLE_ENTITY, \
		.run_list = LIST_HEAD_INIT((b).sched.run_list), \
		.init_rq = NULL, .cur_rq = NULL, \
		.array = NULL, \
		.prio = MA_BATCH_PRIO, \
		.timestamp = 0, .last_ran = 0, \
		.time_slice = MA_ATOMIC_INIT(0), \
		SCHED_LEVEL_INIT \
	}, \
	MARCEL_BUBBLE_SCHED_INITIALIZER(b) \
}

#section marcel_variables
MA_DECLARE_PER_LWP(marcel_bubble_t *, bubble_towake);

#section marcel_functions
/* called from ma_schedule() to achieve bubble scheduling. prevrq and rq are
 * locked, returns 1 if ma_schedule() should restart (prevrq and rq have then
 * been released), returns 0 if ma_schedule() can continue with entity nextent
 * (prevrq and rq have been kept locked) */
marcel_bubble_entity_t *ma_bubble_sched(marcel_bubble_entity_t *nextent,
		ma_runqueue_t *prevrq, ma_runqueue_t *rq, int idx);

/* called from ma_scheduler_tick() when the timeslice for the currently running
 * bubble has expired */
void ma_bubble_tick(marcel_bubble_t *bubble);
