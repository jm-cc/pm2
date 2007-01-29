
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

#include <stdlib.h>

/** \file
 * \brief Bubble interface
 */

/******************************************************************************
 * user view
 */

/** \defgroup marcel_bubble_user Bubble User Interface
 *
 * This is the user interface for manipulating bubbles.
 *
 * @{
 */

#section types
/** \brief Type of a bubble */
typedef struct marcel_bubble marcel_bubble_t;

#section variables
/** \brief The root bubble (default holding bubble) */
extern marcel_bubble_t marcel_root_bubble;

#section functions
#depend "marcel_topology.h[types]"
#depend "scheduler/marcel_bubble_sched.h[types]"
#depend "scheduler/marcel_holder.h[types]"
#depend "marcel_descr.h[types]"
/**
 * \brief Initializes bubble \e bubble.
 */
int marcel_bubble_init(marcel_bubble_t *bubble);
/**
 * \brief Destroys resources associated with bubble \e bubble
 */
int marcel_bubble_destroy(marcel_bubble_t *bubble);
#define marcel_bubble_destroy(bubble) 0

/** \brief Inserts entity \e entity into bubble \e bubble. */
int marcel_bubble_insertentity(marcel_bubble_t *bubble, marcel_entity_t *entity);
/** \brief Inserts bubble \e little_bubble into bubble \e bubble. */
int marcel_bubble_insertbubble(marcel_bubble_t *bubble, marcel_bubble_t *little_bubble);
/** \brief Inserts thread \e task into bubble \e bubble. */
int marcel_bubble_inserttask(marcel_bubble_t *bubble, marcel_task_t *task);

/** \brief Removes entity \e entity from bubble \e bubble. */
int marcel_bubble_removeentity(marcel_bubble_t *bubble, marcel_entity_t *entity);
/** \brief Removes bubble \e little_bubble from bubble \e bubble. */
int marcel_bubble_removebubble(marcel_bubble_t *bubble, marcel_bubble_t *little_bubble);
/** \brief Removes thread \e task from bubble \e bubble. */
int marcel_bubble_removetask(marcel_bubble_t *bubble, marcel_task_t *task);

#define marcel_bubble_insertbubble(bubble, littlebubble) marcel_bubble_insertentity(bubble, &(littlebubble)->sched)
#define marcel_bubble_inserttask(bubble, task) marcel_bubble_insertentity(bubble, &(task)->sched.internal.entity)

#define marcel_bubble_removebubble(bubble, littlebubble) marcel_bubble_removeentity(bubble, &(littlebubble)->sched)
#define marcel_bubble_removetask(bubble, task) marcel_bubble_removeentity(bubble, &(task)->sched.internal.entity)

/** \brief Sets the "scheduling level" of entity \e entity to \e level. This
 * information is used by the Burst Scheduler.
 */
int marcel_entity_setschedlevel(marcel_entity_t *entity, int level);
int marcel_entity_getschedlevel(__const marcel_entity_t *entity, int *level);

#define marcel_bubble_setschedlevel(bubble,level) marcel_entity_setschedlevel(&(bubble)->sched,level)
#define marcel_bubble_getschedlevel(bubble,level) marcel_entity_getschedlevel(&(bubble)->sched,level)
#define marcel_task_setschedlevel(task,level) marcel_entity_setschedlevel(&(task)->sched.internal.entity,level)
#define marcel_task_getschedlevel(task,level) marcel_entity_getschedlevel(&(task)->sched.internal.entity,level)

/** \brief Sets the priority of bubble \e bubble to \e prio. */
int marcel_bubble_setprio(marcel_bubble_t *bubble, int prio);
/** \brief Gets the priority of bubble \e bubble, put into \e *prio. */
int marcel_bubble_getprio(__const marcel_bubble_t *bubble, int *prio);

/** \brief Sets the initial runqueue of bubble \e bubble to \e rq. When woken up (see
 * marcel_wake_up_bubble), \e bubble will be put on runqueue \e rq.
 */
int marcel_bubble_setinitrq(marcel_bubble_t *bubble, ma_runqueue_t *rq);
/** \brief Sets the initial topology level of bubble \e bubble to \e level. When woken
 * up (see marcel_wake_up_bubble), \e bubble will be put on the runqueue of
 * topology level \e level.
 */
int marcel_bubble_setinitlevel(marcel_bubble_t *bubble, marcel_topo_level_t *level);

/** \brief
 * Wakes bubble \e bubble up, i.e. puts it on its initial runqueue, and let the
 * currently running bubble scheduler and the basic self scheduler schedule the
 * threads it holds.
 */
void marcel_wake_up_bubble(marcel_bubble_t *bubble);

/** \brief Joins bubble \e bubble, i.e. waits for all of its threads to terminate. */
void marcel_bubble_join(marcel_bubble_t *bubble);

/** \brief Executes the barrier of bubble \e bubble, which synchronizes all
 * threads of the bubble.
 */
int marcel_bubble_barrier(marcel_bubble_t *bubble);
#define marcel_bubble_barrier(b) marcel_barrier_wait(&(b)->barrier)

/** \brief Returns the bubble that holds thread \e task. */
marcel_bubble_t *marcel_bubble_holding_task(marcel_task_t *task);
/** \brief Returns the bubble that holds bubble \e bubble. */
marcel_bubble_t *marcel_bubble_holding_bubble(marcel_bubble_t *bubble);
/** \brief Returns the bubble that holds entity \e entity. */
marcel_bubble_t *marcel_bubble_holding_entity(marcel_entity_t *entity);

#define marcel_bubble_holding_bubble(b) marcel_bubble_holding_entity(&(b)->sched)
#define marcel_bubble_holding_task(t) marcel_bubble_holding_entity(&(t)->sched.internal.entity)

#section functions
#ifdef MARCEL_BUBBLE_EXPLODE
/** \brief Closes bubble \e bubble, only useful with the Burst Scheduler. */
void marcel_close_bubble(marcel_bubble_t *bubble);
#endif

/* @} */

/**
 ******************************************************************************
 * scheduler view
 * \defgroup marcel_bubble_sched Bubble Scheduler Interface
 *
 * This is the scheduler interface for manipulating bubbles.
 *
 * @{
 */

#section marcel_variables
/** \brief Whether an idle scheduler is running */
extern int ma_idle_scheduler;
/** \brief Central lock for the idle scheduler */
extern ma_rwlock_t ma_idle_scheduler_lock;

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

/** Structure of a bubble */
struct marcel_bubble {
	/* garder en premier, pour que les conversions bubble / entity soient
	 * triviales */
	/** \brief Entity information */
	struct ma_sched_entity sched;
	/** \brief Holder information */
	struct ma_holder hold;
	/** \brief List of the held entities */
	struct list_head heldentities;
	/** \brief Number of held entities */
	unsigned nbentities;
	/** \brief Semaphore for the join operation */
	marcel_sem_t join;
#ifdef MARCEL_BUBBLE_EXPLODE
	/** \brief Burst Scheduler status */
	ma_bubble_status status;
	/** \brief Number of entities released by the bubble (i.e. currently in runqueues) */
	int nbrunning;
#endif
#ifdef MARCEL_BUBBLE_STEAL
	/** \brief Next entity to try to run in this bubble */
	struct ma_sched_entity *next;
	/** \brief List of running entities */
	struct list_head runningentities;
	/** \brief Whether the bubble settled somewhere */
	int settled;
#endif
	/** \brief Barrier for the barrier operation */
	marcel_barrier_t barrier;
};

#section macros
/*  LIST_HEAD_INIT */
#depend "pm2_list.h"
/*  MA_ATOMIC_INIT */
#depend "asm/linux_atomic.h"
#depend "scheduler/marcel_holder.h[macros]"

/* Attention: ne doit �tre utilis� que par marcel_bubble_init */
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


#section marcel_macros
/** \brief Get a statistic value of a bubble */
#define ma_bubble_stats_get(b,offset) ma_stats_get(&(b)->sched, (offset))
/** \brief Get a synthesis of a statistic value of the content of a bubble, as
 * collected during the last call to ma_bubble_synthesize_stats() */
#define ma_bubble_hold_stats_get(b,offset) ma_stats_get(&(b)->hold, (offset))

#section marcel_functions
/**
 * called from the Marcel Self Scheduler to achieve bubble scheduling. \e prevrq and \e rq
 * must already be locked.
 *
 * \param nextent Entity found by the Self Scheduler.
 * \param prevh Holder of the currently running thread (will be the "previous" thread).
 * \param rq Runqueue where \e nextent was found.
 * \param nexth Holder of the next entity (filled by ma_bubble_sched()).
 * \param prio Priority of \e nextent.
 *
 * \return the next thread to schedule (\e rq is still locked), if any. Else,
 * returns NULL, and \e rq is not locked any more.
 */
marcel_entity_t *ma_bubble_sched(marcel_entity_t *nextent,
		ma_holder_t *prevh, ma_runqueue_t *rq,
		ma_holder_t **nexth, int prio);

/** \brief called from ma_scheduler_tick() when the timeslice for the currently
 * running bubble has expired.
 */
void ma_bubble_tick(marcel_bubble_t *bubble);

/** \brief Synthesize statistics of the entities contained in bubble \e bubble.
 * The synthesis of statistics can be read thanks to ma_bubble_hold_stats_get()
 */
void ma_bubble_synthesize_stats(marcel_bubble_t *bubble);

/** \brief
 * Detaches bubbles \e bubble from its holding bubble, i.e. put \e bubble on
 * the runqueue of the holding bubble.
 */
int marcel_bubble_detach(marcel_bubble_t *bubble);

/** \brief
 * Gathers bubbles contained in \e b back into it, i.e. get them from their
 * runqueues and put them back into \e b.
 */
void ma_bubble_gather(marcel_bubble_t *b);
/* Internal version (preemption and local bottom halves are already disabled) */
void __ma_bubble_gather(marcel_bubble_t *b, marcel_bubble_t *rootbubble);

/******************************************************************************
 * internal view
 */
#ifdef MARCEL_BUBBLE_STEAL
static __tbx_inline__ void __ma_bubble_enqueue_entity(marcel_entity_t *e, marcel_bubble_t *b);
static __tbx_inline__ void __ma_bubble_dequeue_entity(marcel_entity_t *e, marcel_bubble_t *b);

/* enqueue entity \e e in bubble \b */
static __tbx_inline__ void ma_bubble_enqueue_entity(marcel_entity_t *e, marcel_bubble_t *b);
void ma_bubble_enqueue_task(marcel_task_t *t, marcel_bubble_t *b);
void ma_bubble_enqueue_bubble(marcel_bubble_t *sb, marcel_bubble_t *b);
#define ma_bubble_enqueue_task(t,b) ma_bubble_enqueue_entity(&t->sched.internal.entity,b)
#define ma_bubble_enqueue_bubble(sb,b) ma_bubble_enqueue_entity(&sb->sched,b)
/* dequeue entity \e e from bubble \b */
static __tbx_inline__ void ma_bubble_dequeue_entity(marcel_entity_t *e, marcel_bubble_t *b);
void ma_bubble_dequeue_task(marcel_task_t *t, marcel_bubble_t *b);
void ma_bubble_dequeue_bubble(marcel_bubble_t *sb, marcel_bubble_t *b);
#define ma_bubble_dequeue_task(t,b) ma_bubble_dequeue_entity(&t->sched.internal.entity,b)
#define ma_bubble_dequeue_bubble(sb,b) ma_bubble_dequeue_entity(&sb->sched,b)
#endif

#section functions
/* Endort la bulle (qui est verrouill�e) */
int marcel_bubble_sleep_locked(marcel_bubble_t *bubble);
/* Endort la bulle (qui est verrouill�e, ainsi que la runqueue la contenant) */
int marcel_bubble_sleep_rq_locked(marcel_bubble_t *bubble);
/* R�veille la bulle (qui est verrouill�e) */
int marcel_bubble_wake_locked(marcel_bubble_t *bubble);
/* R�veille la bulle (qui est verrouill�e, ainsi que la runqueue la contenant) */
int marcel_bubble_wake_rq_locked(marcel_bubble_t *bubble);

/* Called on marcel shutdown */
void marcel_sched_exit(marcel_t t);
#ifndef MA__BUBBLES
#define marcel_sched_exit(t)
#endif

#section marcel_inline
/* cette version (avec __) suppose que la runqueue contenant b est d�j� verrouill�e */
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
#if 0 // On ne peut pas se le permettre */
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

/* @} */
