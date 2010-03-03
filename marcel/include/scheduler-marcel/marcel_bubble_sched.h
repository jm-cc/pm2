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


#ifndef __MARCEL_BUBBLE_SCHED_H__
#define __MARCEL_BUBBLE_SCHED_H__


#include "tbx_compiler.h"
#include "scheduler-marcel/marcel_sched_types.h"
#include "scheduler-marcel/linux_runqueues.h"
#include "marcel_types.h"


/** Public macros **/
/* Attention: ne doit être utilisé que par marcel_bubble_init
 *
 * Note: - only the root_bubble is statically initialized, thus we assume the root bubble for initializing the name fields,
 * for other bubbles, these fields will be overwritten by marcel_bubble_init. */
#define MARCEL_BUBBLE_INITIALIZER(b) { \
	.as_entity = MA_SCHED_ENTITY_INITIALIZER((b).as_entity, MA_BUBBLE_ENTITY, MA_DEF_PRIO), \
	.as_holder = MA_HOLDER_INITIALIZER((b).as_holder, MA_BUBBLE_HOLDER), \
	.natural_entities = TBX_FAST_LIST_HEAD_INIT((b).natural_entities), \
	.nb_natural_entities = 0, \
	.join = MARCEL_SEM_INITIALIZER(1), \
	.cached_entities = TBX_FAST_LIST_HEAD_INIT((b).cached_entities), \
	.num_schedules = 0, \
	.barrier = MARCEL_BARRIER_INITIALIZER(0), \
	.not_preemptible = tbx_false, \
        .old = 0,			      \
        .id = 0,			      \
	.as_holder.name = "root_bubble_h", \
	.as_entity.name = "root_bubble_e" \
}


/** Public global variables **/
/** \brief The root bubble (default holding bubble) */
extern marcel_bubble_t marcel_root_bubble;


/** Public functions **/
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

/** \brief Sets the "scheduling level" of entity \e entity to \e level, i.e.
 * the depth (within the machine's topology) where it should be scheduled or
 * burst. This information is used by the Burst Scheduler.
 */
int marcel_entity_setschedlevel(marcel_entity_t *entity, int level);
/** \brief Gets the "scheduling level" of entity \e entity. */
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
/** \brief Temporarily bring the bubble \e bubble on level \e level 's runqueue by setting its sched_holder accordingly.
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

/** \brief Return the current bubble scheduler.  */
marcel_bubble_sched_t *marcel_bubble_current_sched (void);
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
/** \brief Call the bubble scheduler #ma_bubble_sched_struct::shake method to reset
 * the current distribution of activities and start over a fresh new
 * distribution of active entities on the topology.
 *
 * If the current bubble scheduler does not export a \p shake method, the
 * default behaviour is to move the root bubble (#marcel_root_bubble) back to
 * the top of the topology and then to call the \p submit method on that root
 * bubble. */
void marcel_bubble_shake (void);
#ifndef MA__BUBBLES
#define marcel_bubble_shake() ((void)0)
#endif
/** \brief Submits the bubble _b_ to the underlying bubble scheduler. */
int marcel_bubble_submit (marcel_bubble_t *b);
/** \brief Submits the bubble _b_ to the _sched_ bubble scheduler. */
int marcel_bubble_submit_to_sched (marcel_bubble_sched_t *sched, marcel_bubble_t *b);
/**
 ******************************************************************************
 * scheduler view
 * \addtogroup marcel_bubble_sched
 */
/**
 * \brief Return the bubble scheduler class named \param name or \p NULL
 * if not found.  */
extern const marcel_bubble_sched_class_t *
marcel_lookup_bubble_scheduler_class(const char *name);

/**
 * \brief Return the size of an instance of \param klass .  */
static __tbx_inline__ size_t 
marcel_bubble_sched_instance_size(const marcel_bubble_sched_class_t *klass) ;

/**
 * \brief Return the name of \param klass .  */
static __tbx_inline__ const char *
marcel_bubble_sched_class_name(const marcel_bubble_sched_class_t *klass) ;

/**
 * \brief Initialize \param scheduler as an instance of \param klass .  This
 * assumes \param scheduler points to a memory region at least as large as
 * specified by marcel_bubble_sched_instance_size().  */
extern int
marcel_bubble_sched_instantiate(const marcel_bubble_sched_class_t *klass,
	marcel_bubble_sched_t *scheduler);

/**
 * \brief Return the bubble scheduler named \param name or \p NULL if not
 * found.  */
extern const marcel_bubble_sched_t *
marcel_lookup_bubble_scheduler(const char *name);

/**
 * \brief Return the \param scheduler class. */
static __tbx_inline__ const marcel_bubble_sched_class_t *
marcel_bubble_sched_class(const marcel_bubble_sched_t *scheduler) ;

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


#ifdef __MARCEL_KERNEL__


/** Internal macros **/
/** \brief Gets a statistic value of a bubble */
#define ma_bubble_stats_get(b,offset) ma_stats_get(&(b)->as_entity, (offset))
/** \brief Gets a synthesis of a statistic value of the content of a bubble, as
 * collected during the last call to ma_bubble_synthesize_stats() */
#define ma_bubble_hold_stats_get(b,offset) ma_stats_get(&(b)->as_holder, (offset))


/** Internal global variables **/
/* \brief Whether an idle scheduler is running */
extern ma_atomic_t ma_idle_scheduler;
/* \brief Central lock for the idle scheduler */
extern ma_spinlock_t ma_idle_scheduler_lock;


/** Internal functions **/
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
void ma_bubble_snapshot (struct marcel_topo_level *from);

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

/** \brief Push entities out from a given vpset that becomes disabled. */
void ma_push_entities(const marcel_vpset_t *vpset);
#ifndef MA__BUBBLES
#define ma_push_entities(vpset) ((void)0)
#endif

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

/* \brief Turns idle scheduler on. */
int ma_activate_idle_scheduler (void);
/* \brief Turns idle scheduler off. */
int ma_deactivate_idle_scheduler (void);
/* \brief Checks whether an idle scheduler is running. */
int ma_idle_scheduler_is_running (void);

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


#endif /** __MARCEL_KERNEL__ **/


#endif /** __MARCEL_BUBBLE_SCHED_H__ **/
