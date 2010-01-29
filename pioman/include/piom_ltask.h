/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

#ifndef PIOM_LTASK_H
#define PIOM_LTASK_H

#include "pioman.h"
#ifdef MARCEL
#include "marcel.h"
#endif

#ifdef PIOM_ENABLE_LTASKS
#ifdef MARCEL
#define piom_vpset_t marcel_vpset_t
#define piom_vpset_full MARCEL_VPSET_FULL
#define PIOM_VPSET_VP(vp) MARCEL_VPSET_VP(vp)
#define piom_vpset_set(vpset, vp) marcel_vpset_set(vpset, vp)
#else
#define piom_vpset_t unsigned 
#define piom_vpset_full (unsigned) 1
#define PIOM_VPSET_VP(vp) piom_vpset_full
#define piom_vpset_set(vpset, vp) (vpset = vp)
#endif /* MARCEL */

typedef unsigned piom_ltask_option_t;
#define PIOM_LTASK_OPTION_NULL 0
#define PIOM_LTASK_OPTION_REPEAT 1

typedef enum
    {
	PIOM_LTASK_STATE_NONE = 0x00,	/* not yet submitted */
	PIOM_LTASK_STATE_WAITING = 0x01,	/* submitted to the task manager */
	PIOM_LTASK_STATE_SCHEDULED = 0x02,	/* being scheduled */
	PIOM_LTASK_STATE_DONE = 0x04,	/* the task is successfull */
	PIOM_LTASK_STATE_COMPLETELY_DONE = 0x08	/* the task is successfull and its execution is over  */
    } piom_ltask_state_t;


typedef int (piom_ltask_func) (void *arg);

struct piom_ltask
{
    piom_ltask_func *func_ptr;
    void *data_ptr;
    piom_ltask_option_t options;
    piom_ltask_state_t state;
    piom_vpset_t vp_mask;
};

/* initialize internal structures, etc. */
void piom_init_ltasks ();

/* destroy internal structures, stop task execution, etc. */
void piom_exit_ltasks();

/* submit a task
 * Beyond this point, the task may be scheduled at any time
 */
void piom_ltask_submit (struct piom_ltask *task);

/* Wait for the completion of a task
 * (ie. wait until the task state matches PIOM_LTASK_STATE_DONE)
 */
void piom_ltask_wait_success (struct piom_ltask *task);


/* Wait for the terminaison of a task
 * (ie. wait until the task state matches PIOM_LTASK_STATE_COMPLETELY_DONE)
 */
void piom_ltask_wait(struct piom_ltask *task);


/* Try to schedule a task
 * Returns the task that have been scheduled (or NULL if no task)
 */
void *piom_ltask_schedule ();

/* Check whether a task should be executed on the current vp
 * (ie: the local queue or the global queue is not empty)
 */
int piom_ltask_polling_is_required ();

/* Test whether a task is completed */
static __tbx_inline__ int
piom_ltask_test (struct piom_ltask *task)
{
    return task && (task->state == PIOM_LTASK_STATE_DONE);
}

/* Set a task as completed */
static __tbx_inline__ void
piom_ltask_completed (struct piom_ltask *task)
{
    if(task->state!=PIOM_LTASK_STATE_DONE)
	task->state = PIOM_LTASK_STATE_DONE;
}

/* initialize a task */
static __tbx_inline__ void
piom_ltask_init (struct piom_ltask *task)
{
    task->func_ptr = NULL;
    task->data_ptr = NULL;
    task->options = PIOM_LTASK_OPTION_NULL;
    task->state = PIOM_LTASK_STATE_NONE;
    task->vp_mask = piom_vpset_full;
}

/* change the func_ptr of a task */
static __tbx_inline__ void
piom_ltask_set_func (struct piom_ltask *task, piom_ltask_func * func_ptr)
{
    task->func_ptr = func_ptr;
}

/* change the data_ptr of a task */
static __tbx_inline__ void
piom_ltask_set_data (struct piom_ltask *task, void *data_ptr)
{
    task->data_ptr = data_ptr;
}

/* change the options of a task */
static __tbx_inline__ void
piom_ltask_set_options (struct piom_ltask *task, piom_ltask_option_t option)
{
    task->options |= option;
}

/* change the vpmask of a task 
 * This function should not be called if the task is already submitted
 */
static __tbx_inline__ void
piom_ltask_set_vpmask (struct piom_ltask *task, piom_vpset_t mask)
{
    task->vp_mask = mask;
}

/* create a new task (initialize it with the params) */
static __tbx_inline__ void
piom_ltask_create (struct piom_ltask *task,
		   piom_ltask_func * func_ptr,
		   void *data_ptr,
		   piom_ltask_option_t options, piom_vpset_t vp_mask)
{
    task->func_ptr = func_ptr;
    task->data_ptr = data_ptr;
    task->options = options;
    task->state = PIOM_LTASK_STATE_NONE;
    task->vp_mask = vp_mask;
}
#endif /* PIOM_ENABLE_LTASKS */
#endif /* PIOM_LTASK_H */
