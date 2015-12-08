/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2012 "the PM2 team" (see AUTHORS file)
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

/** @file piom_private.h internal PIOMan definitions.
 */

#ifndef PIOM_PRIVATE_H
#define PIOM_PRIVATE_H

#ifdef PIOMAN_TRACE
#include <GTG.h>
#endif /* PIOMAN_TRACE */

#include <tbx.h>
#include "piom_lfqueue.h"
#include "piom_log.h"
#include "pioman.h"

/** initialize ltask system */
extern void piom_init_ltasks(void);

/** destroy internal structures, stop task execution, etc. */
TBX_INTERNAL void piom_exit_ltasks(void);

TBX_INTERNAL void piom_io_task_init(void);
TBX_INTERNAL void piom_io_task_stop(void);

/* todo: get a dynamic value here !
 * it could be based on:
 * - application hints
 * - the history of previous request
 * - compiler hints
 */

/** pioman internal parameters, tuned through env variables */
TBX_INTERNAL struct piom_parameters_s
{
    int busy_wait_usec;     /**< time to do a busy wait before blocking, in usec; default: 5 */
    int busy_wait_granularity; /**< number of busy wait loops between timestamps to amortize clock_gettime() */
    int enable_progression; /**< whether to enable background progression (idle thread and sighandler); default 1 */
    enum piom_topo_level_e binding_level; /**< hierarchy level where to bind queues; default: socket */
    int idle_granularity;   /**< time granularity for polling on idle, in usec. */
    enum piom_bind_distrib_e
	{
	    PIOM_BIND_DISTRIB_NONE = 0, /**< no value given */
	    PIOM_BIND_DISTRIB_ALL,      /**< bind on all entities in level (default) */
	    PIOM_BIND_DISTRIB_ODD,      /**< bind on odd-numbered entities */
	    PIOM_BIND_DISTRIB_EVEN,     /**< bind on even-numbered entities */
	    PIOM_BIND_DISTRIB_FIRST     /**< bind on single (first) entity in level*/
	} idle_distrib;     /**< binding distribution for idle threads */
    int timer_period;       /**< period for timer-based polling (in usec); default: 4000 */
    int spare_lwp;          /**< number of spare LWPs for blocking calls; default: 0 */
} piom_parameters;

enum piom_trace_event_e
    {
	PIOM_TRACE_EVENT_TIMER_POLL, PIOM_TRACE_EVENT_IDLE_POLL,
	PIOM_TRACE_EVENT_SUBMIT, PIOM_TRACE_EVENT_SUCCESS,
	PIOM_TRACE_STATE_INIT, PIOM_TRACE_STATE_POLL, PIOM_TRACE_STATE_NONE,
	PIOM_TRACE_VAR_LTASKS
    } event;

/* forward declaration for trace functions */
struct piom_ltask_locality_s;

#ifdef PIOMAN_TRACE
struct piom_trace_info_s
{
    enum piom_topo_level_e level;
    int rank;
    const char*cont_type;
    const char*cont_name;
    struct piom_trace_info_s*parent;
};

TBX_INTERNAL void piom_trace_flush(void);

TBX_INTERNAL void piom_trace_local_new(struct piom_trace_info_s*trace_info);

TBX_INTERNAL void piom_trace_local_event(enum piom_trace_event_e _event, void*_value);

TBX_INTERNAL void piom_trace_remote_event(piom_topo_obj_t obj, enum piom_trace_event_e _event, void*_value);

#else /* PIOMAN_TRACE */

struct piom_trace_info_s
{ /* empty */ };

static inline void piom_trace_local_event(enum piom_trace_event_e _event, void*_value)
{ /* empty */ }
static inline void piom_trace_remote_event(piom_topo_obj_t obj, enum piom_trace_event_e _event, void* _value)
{ /* empty */ }
#endif /* PIOMAN_TRACE */

static inline void piom_trace_local_state(enum piom_trace_event_e _event)
{
    piom_trace_local_event(_event, NULL);
}
static inline void piom_trace_remote_state(piom_topo_obj_t obj, enum piom_trace_event_e _event)
{
    piom_trace_remote_event(obj, _event, NULL);
}
static inline void piom_trace_remote_var(piom_topo_obj_t obj, enum piom_trace_event_e _event, int _value)
{
    void*value = (void*)((uintptr_t)_value);
    piom_trace_remote_event(obj, _event, value);
}

/** locality information, for a given hwloc obj */
struct piom_ltask_locality_s
{
    struct piom_ltask_queue*queue;       /**< local queue, NULL if none at this level */
    struct piom_trace_info_s trace_info; /**< context for trace subsystem */
    struct piom_ltask_locality_s*parent; /**< shortcut to parent local info */
};

#endif /* PIOM_PRIVATE_H */
