/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001-2015 "the PM2 team" (see AUTHORS file)
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

#ifndef PIOM_LTASK_H
#define PIOM_LTASK_H

/** @defgroup piom_ltask PIOMan ltask interface
 * This is the main PIOMan interface, used to submit light tasks
 * @ingroup pioman
 * @{
 */

#ifdef PIOMAN_TOPOLOGY_MARCEL
#define piom_topo_obj_t   struct marcel_topo_level*
#define piom_topo_full    marcel_topo_levels[0]
#define piom_topo_current_obj()  &marcel_topo_levels[marcel_topo_nblevels - 1][marcel_current_vp()]
enum piom_topo_level_e
    {
	PIOM_TOPO_NONE    = -1,
	PIOM_TOPO_MACHINE = MARCEL_LEVEL_MACHINE,
	PIOM_TOPO_NODE    = MARCEL_LEVEL_NODE,
	PIOM_TOPO_SOCKET  = MARCEL_LEVEL_DIE,
	PIOM_TOPO_CORE    = MARCEL_LEVEL_CORE,
	PIOM_TOPO_PU      = MARCEL_LEVEL_PROC
    };
#elif defined(PIOMAN_TOPOLOGY_HWLOC)
#include <hwloc.h>
#define piom_topo_obj_t   hwloc_obj_t
#define piom_topo_full    hwloc_get_root_obj(piom_topo_get())
enum piom_topo_level_e
    {
	PIOM_TOPO_NONE    = -1,
	PIOM_TOPO_MACHINE = HWLOC_OBJ_MACHINE,
	PIOM_TOPO_NODE    = HWLOC_OBJ_NODE,
	PIOM_TOPO_SOCKET  = HWLOC_OBJ_SOCKET,
	PIOM_TOPO_CORE    = HWLOC_OBJ_CORE,
	PIOM_TOPO_PU      = HWLOC_OBJ_PU
    };
extern hwloc_topology_t piom_topo_get(void);
extern piom_topo_obj_t piom_topo_current_obj(void);
#else /* PIOMAN_TOPOLOGY_* */
#define piom_topo_obj_t          void* 
#define piom_topo_full           NULL
#define piom_topo_current_obj()  NULL
enum piom_topo_level_e
    {
	PIOM_TOPO_NONE    = -1,
	PIOM_TOPO_MACHINE,
	PIOM_TOPO_NODE,
	PIOM_TOPO_SOCKET,
	PIOM_TOPO_CORE,
	PIOM_TOPO_PU
    };
#endif /* PIOMAN_TOPOLOGY_* */


typedef unsigned piom_ltask_option_t;
#define PIOM_LTASK_OPTION_NONE     0x00
#define PIOM_LTASK_OPTION_REPEAT   0x01 /**< repeat until task completion */
#define PIOM_LTASK_OPTION_ONESHOT  0x02 /**< task is scheduled once */
#define PIOM_LTASK_OPTION_DESTROY  0x04 /**< handler destroys the ltask- must be oneshot, without wait */
#define PIOM_LTASK_OPTION_NOWAIT   0x08 /**< no wait will be performed on the task- bypass completion notification */
#define PIOM_LTASK_OPTION_BLOCKING 0x10 /**< task has a blocking function */

typedef enum
    {
	PIOM_LTASK_STATE_NONE       = 0x00,  /**< not yet submitted */
	PIOM_LTASK_STATE_READY      = 0x01,  /**< submitted to a queue, ready to be scheduled */
	PIOM_LTASK_STATE_SCHEDULED  = 0x02,  /**< being scheduled */
	PIOM_LTASK_STATE_SUCCESS    = 0x04,  /**< task has been scheduled and is successfull */
	PIOM_LTASK_STATE_TERMINATED = 0x08,  /**< task is successfull and processing is completed; resources may be freed */
	PIOM_LTASK_STATE_DESTROYED  = 0x10,  /**< task destroyed by user handler */
	PIOM_LTASK_STATE_BLOCKED    = 0x20,  /**< task is blocked on syscall */
	PIOM_LTASK_STATE_CANCELLED  = 0x40
    } piom_ltask_state_t;

/** type for ltask callback function */
typedef int (*piom_ltask_func_t)(void*arg);

struct piom_ltask
{
    piom_ltask_state_t state;          /**< current state of ltask (bitmask) */
    piom_ltask_func_t func_ptr;        /**< function used for polling */
    void *data_ptr;                    /**< user-supplied pointer given to polling function */
    piom_ltask_option_t options;       /**< special options */
    piom_cond_t done;                  /**< condition to wait for task completion */
    piom_ltask_func_t blocking_func;   /**< function used for blocking system call (passive wait) */
    struct timespec origin;            /**< time origin for blocking call timeout */
    int blocking_delay;                /**< time (in usec.) to poll before switching to blocking call */
    piom_topo_obj_t binding;           /**< location binding for the task */
    struct piom_ltask_queue*queue;     /**< current queue where ltask has been submitted */
    const char*name;                   /**< name given to the ltask, usefull for debug */
    void (*destructor)(struct piom_ltask*);
};


/** Tests whether the ltask system is running.
    return 1 if running, 0 otherwise  */
extern int piom_ltask_test_activity(void);

/** submit a task
 * Beyond this point, the task may be scheduled at any time
 */
extern void piom_ltask_submit(struct piom_ltask *task);

/** Wait for the completion of a task
 * (ie. wait until the task state matches PIOM_LTASK_STATE_SUCCESS)
 */
extern void piom_ltask_wait_success(struct piom_ltask *task);


/** Wait for the termination of a task
 * (ie. wait until the task state matches PIOM_LTASK_STATE_TERMINATED)
 */
extern void piom_ltask_wait(struct piom_ltask *task);

/** Cancel a task.
 * Slow! Use only for clean shutdown.
 */
extern void piom_ltask_cancel(struct piom_ltask*task);

/** sets a given bit in a task state.
 * @return the new state
 */
static inline piom_ltask_state_t piom_ltask_state_set(struct piom_ltask*ltask, piom_ltask_state_t state)
{
#ifdef PIOMAN_MULTITHREAD
    return __sync_or_and_fetch(&ltask->state, state);
#else /* PIOMAN_MULTITHREAD */
    ltask->state |= state;
    return ltask->state;
#endif /* PIOMAN_MULTITHREAD */
}
/** unsets a given bit in a task state.
 * @return the new state
 */
static inline piom_ltask_state_t piom_ltask_state_unset(struct piom_ltask*ltask, piom_ltask_state_t state)
{
#ifdef PIOMAN_MULTITHREAD
    return __sync_and_and_fetch(&ltask->state, ~state);
#else /* PIOMAN_MULTITHREAD */
    ltask->state &= ~state;
    return ltask->state;
#endif /* PIOMAN_MULTITHREAD */
}
/** test a given bit in a task state.
 */
static inline int piom_ltask_state_test(struct piom_ltask*ltask, piom_ltask_state_t state)
{
    return ((ltask->state & state) != 0);
}

/** Notify task completion. */
static inline void piom_ltask_completed(struct piom_ltask *task)
{
    piom_ltask_state_set(task, PIOM_LTASK_STATE_SUCCESS);
    if(!(task->options & PIOM_LTASK_OPTION_NOWAIT))
	{
	    piom_cond_signal(&task->done, PIOM_LTASK_STATE_SUCCESS);
	}
}

/** Notify task destruction, if option DESTROY was set. */
static inline void piom_ltask_destroy(struct piom_ltask*ltask)
{
    assert((ltask->state & PIOM_LTASK_STATE_SCHEDULED) || (ltask->state & PIOM_LTASK_STATE_BLOCKED));
    ltask->state = PIOM_LTASK_STATE_DESTROYED;
}

static inline void piom_ltask_init(struct piom_ltask*ltask)
{
    ltask->state = PIOM_LTASK_STATE_NONE;
    ltask->queue = NULL;
}

/** create a new ltask. */
static inline void piom_ltask_create(struct piom_ltask*task,
				     piom_ltask_func_t func_ptr, void*data_ptr,
				     piom_ltask_option_t options)
{
    assert((options & PIOM_LTASK_OPTION_REPEAT) ||
	   (options & PIOM_LTASK_OPTION_ONESHOT));
    assert((options & PIOM_LTASK_OPTION_ONESHOT) ||
	   !(options & PIOM_LTASK_OPTION_DESTROY));
    task->func_ptr      = func_ptr;
    task->data_ptr      = data_ptr;
    task->blocking_func = NULL;
    task->options       = options;
    task->state         = PIOM_LTASK_STATE_NONE;
    task->binding       = NULL;
    task->queue         = NULL;
    task->name          = NULL;
    task->destructor    = NULL;
    piom_cond_init(&task->done, 0);
}
static inline void piom_ltask_set_binding(struct piom_ltask*task, piom_topo_obj_t binding)
{
    task->binding = binding;
}

extern void piom_ltask_set_blocking(struct piom_ltask*task, piom_ltask_func_t func, int delay_usec);

static inline void piom_ltask_set_name(struct piom_ltask*ltask, const char*name)
{
    ltask->name = name;
}

static inline void piom_ltask_set_destructor(struct piom_ltask*ltask, void(*destructor)(struct piom_ltask*))
{
    assert(ltask->destructor == NULL);
    assert(ltask->options & PIOM_LTASK_OPTION_NOWAIT);
    ltask->destructor = destructor;
}

/** set the polling level- 0 for low; 1 for high */
extern void piom_ltask_poll_level_set(int level);

extern int piom_ltask_poll_level_get(void);

/** suspend the ltask scheduling
 * @note blocks if the ltask is currently scheduled
 */
extern void piom_ltask_mask(struct piom_ltask *ltask);

/** re-enable a previously masked ltask */
extern void piom_ltask_unmask(struct piom_ltask *ltask);

/** asynchronously request task cancellation
 * @return 1 if task already completed in-between; 0 otherwise
 */
int piom_ltask_cancel_request(struct piom_ltask*ltask);

/** wait for cancellation completion */
void piom_ltask_cancel_wait(struct piom_ltask*ltask);

/** @} */

#endif /* PIOM_LTASK_H */
