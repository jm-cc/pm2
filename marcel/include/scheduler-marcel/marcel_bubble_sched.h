
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

#section marcel_variables
#depend "scheduler/marcel_bubble_sched_interface.h[types]"

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
#define marcel_bubble_destroy(bubble) ((void)0)

/** \brief Sets an id for the bubble, useful for traces. */
int marcel_bubble_setid(marcel_bubble_t *bubble, int id);

/** Permanently bring the entity \e entity inside the bubble \e bubble by
 * setting its sched_holder and natural_holder accordingly. */
int marcel_bubble_insertentity(marcel_bubble_t *bubble, marcel_entity_t *entity);
/** Permanently bring the bubble \e little_bubble inside the bubble \e bubble by
 * setting its sched_holder and natural_holder accordingly. */
int marcel_bubble_insertbubble(marcel_bubble_t *bubble, marcel_bubble_t *little_bubble);
/** Permanently bring the task \e task inside the bubble \e bubble by
 * setting its sched_holder and natural_holder accordingly. */
int marcel_bubble_inserttask(marcel_bubble_t *bubble, marcel_task_t *task);

/** \brief Removes entity \e entity from bubble \e bubble.
 * Put \e entity on \e bubble's holding runqueue as a fallback */
int marcel_bubble_removeentity(marcel_bubble_t *bubble, marcel_entity_t *entity);
/** \brief Removes bubble \e little_bubble from bubble \e bubble.
 * Put \e little_bubble on \e bubble's holding runqueue as a fallback  */
int marcel_bubble_removebubble(marcel_bubble_t *bubble, marcel_bubble_t *little_bubble);
/** \brief Removes thread \e task from bubble \e bubble.  
 *  Put \e task on \e bubble's holding runqueue as a fallback */
int marcel_bubble_removetask(marcel_bubble_t *bubble, marcel_task_t *task);

#define marcel_bubble_insertbubble(bubble, littlebubble) marcel_bubble_insertentity(bubble, &(littlebubble)->as_entity)
#define marcel_bubble_inserttask(bubble, task) marcel_bubble_insertentity(bubble, &(task)->as_entity)

#define marcel_bubble_removebubble(bubble, littlebubble) marcel_bubble_removeentity(bubble, &(littlebubble)->as_entity)
#define marcel_bubble_removetask(bubble, task) marcel_bubble_removeentity(bubble, &(task)->as_entity)

/** \brief Sets the "scheduling level" of entity \e entity to \e level. This
 * information is used by the Burst Scheduler.
 */
int marcel_entity_setschedlevel(marcel_entity_t *entity, int level);
int marcel_entity_getschedlevel(__const marcel_entity_t *entity, int *level);

#define marcel_bubble_setschedlevel(bubble,level) marcel_entity_setschedlevel(&(bubble)->as_entity,level)
#define marcel_bubble_getschedlevel(bubble,level) marcel_entity_getschedlevel(&(bubble)->as_entity,level)
#define marcel_task_setschedlevel(task,level) marcel_entity_setschedlevel(&(task)->as_entity,level)
#define marcel_task_getschedlevel(task,level) marcel_entity_getschedlevel(&(task)->as_entity,level)

/** \brief Sets the priority of bubble \e bubble to \e prio. */
int marcel_bubble_setprio(marcel_bubble_t *bubble, int prio);
/* \brief Sets the priority of bubble \e bubble to \e prio, holder is already locked.  */
int marcel_bubble_setprio_locked(marcel_bubble_t *bubble, int prio);
/** \brief Gets the priority of bubble \e bubble, put into \e *prio. */
int marcel_bubble_getprio(__const marcel_bubble_t *bubble, int *prio);

/** \brief Temporarily bring bubble \e bubble on the runqueue \e rq by setting its sched_holder accordingly.
 * . The bubble's natural holder is left unchange.d
 * . The actual move is deferred until the bubble is woken up.  When woken up (see
 *   marcel_wake_up_bubble()), \e bubble will be put on runqueue \e rq.*/
int marcel_bubble_scheduleonrq(marcel_bubble_t *bubble, ma_runqueue_t *rq);
/** \briefTemporarily bring the bubble \e bubble on level \e level 's runqueue by setting its sched_holder accordingly.
 * . The bubble's natural holder is left unchanged.
 * . The actual move is deferred until the bubble is woken up.
 *   When woken up (see marcel_wake_up_bubble()), \e bubble will be put on the
 *   runqueue of topology level \e level. */
int marcel_bubble_scheduleonlevel(marcel_bubble_t *bubble, marcel_topo_level_t *level);
/** \brief Temporarily bring the bubble \e bubble on the current thread's holder by setting its sched_holder accordingly.
 * Inheritence occurs only if the thread sched holder is a runqueue.
 * . The bubble's natural holder is left unchanged.
 * . The actual move is deferred until the bubble is woken up. When woken up
 * (see marcel_wake_up_bubble()), \e bubble will be put on the corresponding
 * runqueue. */
int marcel_bubble_scheduleonthreadholder(marcel_bubble_t *bubble);

/** \brief
 * Wakes bubble \e bubble up, i.e. puts it on its initial runqueue, and let the
 * currently running bubble scheduler and the basic self scheduler schedule the
 * threads it holds.
 */
void marcel_wake_up_bubble(marcel_bubble_t *bubble);

/** \brief Joins bubble \e bubble, i.e. waits for all of its sub-bubbles and threads to terminate. */
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

#define marcel_bubble_holding_bubble(b) marcel_bubble_holding_entity(&(b)->as_entity)
#define marcel_bubble_holding_task(t) marcel_bubble_holding_entity(&(t)->as_entity)

/** \brief Change the current bubble scheduler to \e new_sched, return the
 * previous one.  */
marcel_bubble_sched_t *marcel_bubble_change_sched(marcel_bubble_sched_t *new_sched);
/** \brief Change the current bubble scheduler to \e new_sched, return the
 * previous one.  This variant should only be used by marcel_init() before
 * Marcel is fully initialized.  */
marcel_bubble_sched_t *marcel_bubble_set_sched(marcel_bubble_sched_t *new_sched);

/** \brief Informs the scheduler that the application initialization
    phase has terminated */
void marcel_bubble_sched_begin (void);
/** \brief Informs the scheduler that the application is entering
    ending phase */
void marcel_bubble_sched_end (void);
/** \brief Re-distributes the active entities on the topology */
void marcel_bubble_shake (void);
/** \brief Submits the bubble _b_ to the underlying bubble scheduler. */
int marcel_bubble_submit (marcel_bubble_t *b);

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
/* \brief Whether an idle scheduler is running */
extern ma_atomic_t ma_idle_scheduler;
/* \brief Central lock for the idle scheduler */
extern ma_spinlock_t ma_idle_scheduler_lock;

#section functions
#depend "marcel_bubble_sched_interface.h[types]"

/* \brief Turns idle scheduler on. */
int ma_activate_idle_scheduler (void);
/* \brief Turns idle scheduler off. */
int ma_deactivate_idle_scheduler (void);
/* \brief Checks whether an idle scheduler is running. */
int ma_idle_scheduler_is_running (void);

/* \brief Return the bubble scheduler named \param name, or \code NULL if not
 * found.  */
extern const marcel_bubble_sched_t *
marcel_lookup_bubble_scheduler(const char *name);


#section structures
#depend "pm2_list.h"
#depend "scheduler/marcel_holder.h[structures]"
#depend "marcel_sem.h[types]"
#depend "marcel_sem.h[structures]"

/** Structure of a bubble */
struct marcel_bubble {
#ifdef MA__BUBBLES
	/* garder en premier, pour que les conversions bubble / entity soient
	 * triviales */
	/** \brief Entity information */
	struct ma_entity as_entity;
	/** \brief Holder information */
	struct ma_holder as_holder;
	/** \brief List of entities for which the bubble is a natural holder */
	struct list_head natural_entities;
	/** \brief Number of held entities */
	unsigned nb_natural_entities;
	/** \brief Semaphore for the join operation */
	marcel_sem_t join;

	/** \brief List of entities queued in the so-called "thread cache" */
	struct list_head cached_entities;
	/** \brief Number of threads that we ran in a row, shouldn't be greater than hold->nb_ready_entities. */
	int num_schedules;

	/** \brief Whether the bubble settled somewhere */
	int settled;
        /** \brief Barrier for the barrier operation */
	marcel_barrier_t barrier;

	/** \brief Whether the bubble is preemptible, i.e., whether one of its
			threads can be preempted by a thread from another bubble.  */
	tbx_bool_t not_preemptible;

        /** \brief Dead bubbles (temporary field)*/
	int old;

        /** bubble identifier */
        int id;
#endif

};

#section macros
/*  LIST_HEAD_INIT */
#depend "pm2_list.h"
/*  MA_ATOMIC_INIT */
#depend "asm/linux_atomic.h"
#depend "scheduler/marcel_holder.h[macros]"

/* Attention: ne doit être utilisé que par marcel_bubble_init */
#define MARCEL_BUBBLE_INITIALIZER(b) { \
	.as_entity = MA_SCHED_ENTITY_INITIALIZER((b).as_entity, MA_BUBBLE_ENTITY, MA_DEF_PRIO), \
	.as_holder = MA_HOLDER_INITIALIZER((b).as_holder, MA_BUBBLE_HOLDER), \
	.natural_entities = LIST_HEAD_INIT((b).natural_entities), \
	.nb_natural_entities = 0, \
	.join = MARCEL_SEM_INITIALIZER(1), \
	.cached_entities = LIST_HEAD_INIT((b).cached_entities), \
	.num_schedules = 0, \
	.settled = 0, \
	.barrier = MARCEL_BARRIER_INITIALIZER(0), \
	.not_preemptible = tbx_false, \
        .old = 0,			      \
        .id = 0,			      \
}



#section marcel_macros
/** \brief Gets a statistic value of a bubble */
#define ma_bubble_stats_get(b,offset) ma_stats_get(&(b)->as_entity, (offset))
/** \brief Gets a synthesis of a statistic value of the content of a bubble, as
 * collected during the last call to ma_bubble_synthesize_stats() */
#define ma_bubble_hold_stats_get(b,offset) ma_stats_get(&(b)->as_holder, (offset))

#section marcel_functions
/**
 * called from the Marcel Self Scheduler to achieve bubble scheduling. \e
 * prevrq and \e rq are already be locked.
 *
 * \param nextent Entity found by the Self Scheduler.
 * \param rq Runqueue where \e nextent was found.
 * \param nexth Holder of the next entity (filled by ma_bubble_sched()).
 * \param prio Priority of \e nextent.
 *
 * \return the next thread to schedule (\e rq is still locked), if any. Else,
 * returns NULL, and \e rq is not locked any more.
 */
marcel_entity_t *ma_bubble_sched(marcel_entity_t *nextent,
		ma_runqueue_t *rq, ma_holder_t **nexth, int prio);

/** \brief Frobnicate \e bubble.  FIXME: This is unused, undefined, and thus
 * ought to be removed.  */
int ma_bubble_tick(marcel_bubble_t *bubble);

/** \brief Terminate the current scheduler.  Return zero on success.  */
int ma_bubble_exit(void);

/** \brief Notify the current bubble scheduler that VP \e vp has become idle.
 * Return non-zero if the bubble scheduler's work stealing algorithm
 * succeeded, zero otherwise.  */
int ma_bubble_notify_idle_vp(unsigned int vp);

/** \brief Synthesizes statistics of the entities contained in bubble \e bubble.
 * The synthesis of statistics can be read thanks to ma_bubble_hold_stats_get()
 * This locks the bubble hierarchy.
 */
void ma_bubble_synthesize_stats(marcel_bubble_t *bubble);

/** \brief Remember the current threads and bubble distribution by
    updating the last_topo_level statistics on every entity currently
    scheduled. */
void ma_bubble_snapshot (void);

/** \brief
 * Detaches bubbles \e bubble from its holding bubble, i.e. put \e bubble on
 * the first runqueue encountering when following holding bubbles.
 */
int ma_bubble_detach(marcel_bubble_t *bubble);

/** \brief
 * Recursively gathers bubbles contained in \e b back into it, i.e. get them
 * from their holding runqueues and put them back into their holders.
 */
void ma_bubble_gather(marcel_bubble_t *b);
/* Internal version (bubble hierarchy is already locked) */
void __ma_bubble_gather(marcel_bubble_t *b, marcel_bubble_t *rootbubble);
/* Same as ma_bubble_gather, but asserts that the bubble hierarchy is held under level \e level and \e level is already locked */
void ma_bubble_gather_here(marcel_bubble_t *b, struct marcel_topo_level *level);

/** \brief Gathers bubble b, move it to the top-level runqueue, and
    call the bubble scheduler distribution algorithm from there. */
void ma_bubble_move_top_and_submit (marcel_bubble_t *b);

/** \brief Locks a whole bubble hierarchy.  Also locks the whole level hierarchy.  */
void ma_bubble_lock_all(marcel_bubble_t *b, struct marcel_topo_level *level);
/** \brief Unlocks a whole bubble hierarchy.  Also unlocks the whole level hierarchy. */
void ma_bubble_unlock_all(marcel_bubble_t *b, struct marcel_topo_level *level);
/** \brief Same as ma_bubble_lock_all, but the top level is already locked.  */
void ma_bubble_lock_all_but_level(marcel_bubble_t *b, struct marcel_topo_level *level);
/** \brief Same as ma_bubble_unlock_all, but the top level is already locked.  */
void ma_bubble_unlock_all_but_level(marcel_bubble_t *b, struct marcel_topo_level *level);
/** \brief Locks a whole bubble hierarchy. Levels must be already locked.  */
void ma_bubble_lock_all_but_levels(marcel_bubble_t *b);
/** \brief Unlocks a whole bubble hierarchy. Levels must be already locked.  */
void ma_bubble_unlock_all_but_levels(marcel_bubble_t *b);
/** \brief Locks the whole level hierarchy. */
void ma_topo_lock_levels(struct marcel_topo_level *level);
/** \brief Unlocks the whole level hierarchy. */
void ma_topo_unlock_levels(struct marcel_topo_level *level);

/** \brief Locks the whole level hierarchy and the bubbles on it.  */
void ma_topo_lock_all(struct marcel_topo_level *level);
/** \brief Unlocks the whole level hierarchy and the bubbles on it.  */
void ma_topo_unlock_all(struct marcel_topo_level *level);

/** \brief Locks a bubble hierarchy.  Stops at bubbles held on other runqueues */
void ma_bubble_lock(marcel_bubble_t *b);
void __ma_bubble_lock(marcel_bubble_t *b);

/** \brief Unlocks a bubble hierarchy.  Stops at bubbles held on other runqueues */
void ma_bubble_unlock(marcel_bubble_t *b);
void __ma_bubble_unlock(marcel_bubble_t *b);


/******************************************************************************
 * internal view
 */
static __tbx_inline__ void __ma_bubble_enqueue_entity(marcel_entity_t *e, marcel_bubble_t *b);
static __tbx_inline__ void __ma_bubble_dequeue_entity(marcel_entity_t *e, marcel_bubble_t *b);

/* enqueue entity \e e in bubble \b, which must be locked, and may temporarily be unlocked */
static __tbx_inline__ void ma_bubble_enqueue_entity(marcel_entity_t *e, marcel_bubble_t *b);
void ma_bubble_enqueue_task(marcel_task_t *t, marcel_bubble_t *b);
void ma_bubble_enqueue_bubble(marcel_bubble_t *sb, marcel_bubble_t *b);
#define ma_bubble_enqueue_task(t,b) ma_bubble_enqueue_entity(&t->entity,b)
#define ma_bubble_enqueue_bubble(sb,b) ma_bubble_enqueue_entity(&sb->as_entity,b)
/* dequeue entity \e e from bubble \b, which must be locked */
static __tbx_inline__ void ma_bubble_dequeue_entity(marcel_entity_t *e, marcel_bubble_t *b);
void ma_bubble_dequeue_task(marcel_task_t *t, marcel_bubble_t *b);
void ma_bubble_dequeue_bubble(marcel_bubble_t *sb, marcel_bubble_t *b);
#define ma_bubble_dequeue_task(t,b) ma_bubble_dequeue_entity(&t->entity,b)
#define ma_bubble_dequeue_bubble(sb,b) ma_bubble_dequeue_entity(&sb->as_entity,b)
/* to call after ma_bubble_enqueue_task instead of unlocking the bubble */
static __tbx_inline__ void ma_bubble_try_to_wake_up_and_rawunlock(marcel_bubble_t *b);
static __tbx_inline__ void ma_bubble_try_to_wake_up_and_unlock(marcel_bubble_t *b);
static __tbx_inline__ void ma_bubble_try_to_wake_up_and_unlock_softirq(marcel_bubble_t *b);

#section functions
/* Endort la bulle (qui est verrouillée) */
int marcel_bubble_sleep_locked(marcel_bubble_t *bubble);
/* Endort la bulle (qui est verrouillée, ainsi que la runqueue la contenant) */
int marcel_bubble_sleep_rq_locked(marcel_bubble_t *bubble);
/* Réveille la bulle (qui est verrouillée) */
int marcel_bubble_wake_locked(marcel_bubble_t *bubble);
/* Réveille la bulle (qui est verrouillée, ainsi que la runqueue la contenant) */
int marcel_bubble_wake_rq_locked(marcel_bubble_t *bubble);

extern void __ma_bubble_sched_start(void);
extern void ma_bubble_sched_init2(void);

/* Called on marcel shutdown */
void marcel_sched_exit(marcel_t t);
#ifndef MA__BUBBLES
#define marcel_sched_exit(t)
#endif

#section marcel_inline
/* This version (with __) assumes that the runqueue holding b is already locked  */
static __tbx_inline__ void __ma_bubble_enqueue_entity(marcel_entity_t *e, marcel_bubble_t *b) {
#ifdef MA__BUBBLES
	bubble_sched_debugl(7,"enqueuing %p in bubble %p\n",e,b);
	MA_BUG_ON(e->ready_holder != &b->as_holder);
	MA_BUG_ON(!ma_holder_check_locked(&b->as_holder));
	if (list_empty(&b->cached_entities)) {
		ma_holder_t *h = b->as_entity.ready_holder;
		bubble_sched_debugl(7,"first running entity in bubble %p\n",b);
		if (h) {
			MA_BUG_ON(ma_holder_type(h) != MA_RUNQUEUE_HOLDER);
			MA_BUG_ON(!ma_holder_check_locked(h));
			if (!b->as_entity.ready_holder_data)
				ma_rq_enqueue_entity(&b->as_entity, ma_rq_holder(h));
		}
	}
	if ((e->prio >= MA_BATCH_PRIO) && (e->prio != MA_LOWBATCH_PRIO))
		list_add(&e->run_list, &b->cached_entities);
	else
		list_add_tail(&e->run_list, &b->cached_entities);
	MA_BUG_ON(e->ready_holder_data);
	e->ready_holder_data = (void *)1;
#endif
}
static __tbx_inline__ void __ma_bubble_dequeue_entity(marcel_entity_t *e, marcel_bubble_t *b) {
#ifdef MA__BUBBLES
	bubble_sched_debugl(7,"dequeuing %p from bubble %p\n",e,b);
	MA_BUG_ON(!ma_holder_check_locked(&b->as_holder));
	list_del(&e->run_list);
	if (list_empty(&b->cached_entities)) {
		ma_holder_t *h = b->as_entity.ready_holder;
		bubble_sched_debugl(7,"last running entity in bubble %p\n",b);
		if (h && ma_holder_type(h) == MA_RUNQUEUE_HOLDER) {
			MA_BUG_ON(!ma_holder_check_locked(h));
			if (b->as_entity.ready_holder_data)
				ma_rq_dequeue_entity(&b->as_entity, ma_rq_holder(h));
		}
	}
	MA_BUG_ON(!e->ready_holder_data);
	e->ready_holder_data = NULL;
	MA_BUG_ON(e->ready_holder != &b->as_holder);
#endif
}

/* This version doesn't assume that the runqueue holding b is already locked.
 * It thus can not wake the bubble when needed, so the caller has to call
 * ma_bubble_try_to_wake_and_rawunlock() instead of unlocking it when it is done
 * with it.
 */
static __tbx_inline__ void ma_bubble_enqueue_entity(marcel_entity_t *e, marcel_bubble_t *b) {
#ifdef MA__BUBBLES
	bubble_sched_debugl(7,"enqueuing %p in bubble %p\n",e,b);
	MA_BUG_ON(!ma_holder_check_locked(&b->as_holder));
	if ((e->prio >= MA_BATCH_PRIO) && (e->prio != MA_LOWBATCH_PRIO))
		list_add(&e->run_list, &b->cached_entities);
	else
		list_add_tail(&e->run_list, &b->cached_entities);
	MA_BUG_ON(e->ready_holder_data);
	e->ready_holder_data = (void *)1;
#endif
}

/*
 * B, still locked, has seen entities being enqueued on it by the
 * ma_bubble_enqueue_entity() function above. We may have to wake b if it
 * wasn't awake and these entities are awake. In that case we have to unlock
 * the holding runqueue.
 *
 * Note: this function raw-unlocks b.
 */
static __tbx_inline__ void ma_bubble_try_to_wake_up_and_rawunlock(marcel_bubble_t *b)
{
#ifdef MA__BUBBLES
	ma_holder_t *h = b->as_entity.ready_holder;
	MA_BUG_ON(!ma_holder_check_locked(&b->as_holder));

	if (list_empty(&b->cached_entities) || b->as_entity.ready_holder_data || !h) {
		/* No awake entity or B already awake, just unlock */
		ma_holder_rawunlock(&b->as_holder);
		return;
	}

	/* B holds awake entities but is not awake, we have to unlock it so as
	 * to be able to lock the runqueue because of the locking conventions.
	 */
	bubble_sched_debugl(7,"waking up bubble %p because of its containing awake entities\n",b);

	MA_BUG_ON(ma_holder_type(h) != MA_RUNQUEUE_HOLDER);
	ma_holder_rawunlock(&b->as_holder);
	h = ma_bubble_holder_rawlock(b);
	ma_holder_rawlock(&b->as_holder);
	if (!list_empty(&b->cached_entities) && h) {
		MA_BUG_ON(ma_holder_type(h) != MA_RUNQUEUE_HOLDER);
		if (!b->as_entity.ready_holder_data)
			ma_rq_enqueue_entity(&b->as_entity, ma_rq_holder(h));
	}
	ma_bubble_holder_rawunlock(h);
	ma_holder_rawunlock(&b->as_holder);
#endif
}

static __tbx_inline__ void ma_bubble_try_to_wake_up_and_unlock(marcel_bubble_t *b) {
	ma_bubble_try_to_wake_up_and_rawunlock(b);
	ma_preempt_enable();
}

static __tbx_inline__ void ma_bubble_try_to_wake_up_and_unlock_softirq(marcel_bubble_t *b) {
	ma_bubble_try_to_wake_up_and_rawunlock(b);
	ma_preempt_enable_no_resched();
	ma_local_bh_enable();
}

static __tbx_inline__ void ma_bubble_dequeue_entity(marcel_entity_t *e, marcel_bubble_t *b TBX_UNUSED) {
#ifdef MA__BUBBLES
	MA_BUG_ON(!ma_holder_check_locked(&b->as_holder));
	bubble_sched_debugl(7,"dequeuing %p from bubble %p\n",e,b);
	list_del(&e->run_list);
	MA_BUG_ON(!e->ready_holder_data);
	e->ready_holder_data = NULL;
	MA_BUG_ON(e->ready_holder != &b->as_holder);
#endif
}

/** \brief Tell whether bubble \e b is preemptible, i.e., whether one of its
 * threads can be preempted by a thread from another bubble.  */
static __tbx_inline__ void marcel_bubble_set_preemptible(marcel_bubble_t *b, tbx_bool_t preemptible) {
#ifdef MA__BUBBLES
	b->not_preemptible = !preemptible;
#endif
}

/** \brief Return true if bubble \e b is preemptible, i.e., if one of its
 * threads can be preempted by a thread from another bubble.  */
static __tbx_inline__ tbx_bool_t marcel_bubble_is_preemptible(const marcel_bubble_t *b) {
#ifdef MA__BUBBLES
	return !b->not_preemptible;
#else
	return tbx_true;
#endif
}

/* @} */
