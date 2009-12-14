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

#include "pioman.h"

#ifdef PIOM_ENABLE_LTASKS

#define PIOM_MAX_LTASK 256

//#define USE_GLOBAL_QUEUE 1

typedef enum
    {
	/* not yet initialized */
	PIOM_LTASK_QUEUE_STATE_NONE     = 0x00,
	/* running */
	PIOM_LTASK_QUEUE_STATE_RUNNING  = 0x01,
	/* being stopped */
	PIOM_LTASK_QUEUE_STATE_STOPPING = 0x02,
	/* stopped: no more request in the queue, 
	   no more task from this queue being executed */
	PIOM_LTASK_QUEUE_STATE_STOPPED  = 0x04	
    } piom_ltask_queue_state_t;

typedef struct piom_ltask_queue
{
    struct piom_ltask                *ltask_array[PIOM_MAX_LTASK];
    volatile uint8_t                  ltask_array_head;
    volatile uint8_t                  ltask_array_tail;
    volatile uint8_t                  ltask_array_nb_items;
    volatile piom_ltask_queue_state_t state;
    piom_spinlock_t                   ltask_lock;
#ifdef DEBUG
    int                               id;
#endif
} piom_ltask_queue_t;

void __piom_ltask_submit_in_queue(struct piom_ltask *task, piom_ltask_queue_t *queue);

static tbx_bool_t piom_ltask_initialized = tbx_false;
#ifdef USE_GLOBAL_QUEUE
static struct piom_ltask_queue* global_queue;
#endif
static tbx_bool_t *being_processed;

#define GET_QUEUE(vp) ((piom_ltask_queue_t *)(marcel_topo_levels[marcel_topo_nblevels-1][vp])->piom_ltask_data)
#define LOCAL_QUEUE (GET_QUEUE(marcel_current_vp()))

/* Get the queue that matches a vpset:
 * - a local queue if only one vp is set in the vpset
 * - the global queue otherwise
 */
static inline piom_ltask_queue_t *
__piom_get_queue (piom_vpset_t vpset)
{
#ifdef USE_GLOBAL_QUEUE
    return &global_queue;
#endif
    int vp1, vp2;
    vp1 = marcel_vpset_first(&vpset);
    vp2 = marcel_vpset_last(&vpset);

    if(vp2 > marcel_nbvps())
	/* deal with the MARCEL_VPSET_FULL case */
	vp2 = marcel_nbvps()-1;

    marcel_topo_level_t *l1 = &marcel_topo_levels[marcel_topo_nblevels-1][vp1];
    marcel_topo_level_t *l2 = &marcel_topo_levels[marcel_topo_nblevels-1][vp2];
    marcel_topo_level_t *common_ancestor=ma_topo_common_ancestor (l1, l2);

    return (piom_ltask_queue_t *)common_ancestor->piom_ltask_data;
}

/* Initialize a queue */
static inline void
__piom_init_queue (piom_ltask_queue_t * queue)
{
    queue->state = PIOM_LTASK_QUEUE_STATE_NONE;
    int i;
    for (i = 0; i < PIOM_MAX_LTASK; i++)
	queue->ltask_array[i] = NULL;
    queue->ltask_array_tail = 0;
    queue->ltask_array_head = 0;
    queue->ltask_array_nb_items = 0;
    for (i = 0; i < marcel_nbvps (); i++)
	being_processed[i] = tbx_false;
    piom_spin_lock_init (&queue->ltask_lock);
    queue->state = PIOM_LTASK_QUEUE_STATE_RUNNING;
}
/* Remove a task from a queue and return it
 * Return NULL if the queue is empty
 */
static __tbx_inline__ struct piom_ltask *
__piom_ltask_get_from_queue (piom_ltask_queue_t * queue)
{
    if (!queue->ltask_array_nb_items)
	return NULL;
    struct piom_ltask *result = NULL;
    piom_spin_lock (&queue->ltask_lock);
    {
	if (!queue->ltask_array_nb_items)
	    result = NULL;
	else
	    {
		result = queue->ltask_array[queue->ltask_array_head];
		queue->ltask_array[queue->ltask_array_head] = NULL;
		queue->ltask_array_head++;
		queue->ltask_array_nb_items--;
	    }
    }
    piom_spin_unlock (&queue->ltask_lock);
    return result;
}


/* Remove a task from the list and returns it
 * returns NULL if the list is empty
 */
static __tbx_inline__ struct piom_ltask *
__piom_ltask_get (piom_ltask_queue_t ** queue)
{
    /* Try to get a local job */
    struct piom_ltask *result=NULL;
#ifndef USE_GLOBAL_QUEUE
    struct marcel_topo_level *l;
    piom_ltask_queue_t *cur_queue;
    for(l=&marcel_topo_levels[marcel_topo_nblevels-1][marcel_current_vp()];
	l;			/* do this as long as there's a topo level */
	l=l->father)
	{	    
	    cur_queue = (piom_ltask_queue_t *)(l->piom_ltask_data);
	    result = __piom_ltask_get_from_queue (cur_queue);
	    if (result)
		{
		    *queue=cur_queue;
		    return result;
		}
	}
#else
    result = __piom_ltask_get_from_queue (&global_queue);
    *queue = &global_queue;    
#endif	/* USE_GLOBAL_QUEUE */
    return result;
}

/* Test whether a task is compledted */
static __tbx_inline__ int
__piom_ltask_is_runnable (struct piom_ltask *task)
{
    return task && (task->state == PIOM_LTASK_STATE_WAITING);
}

/* Test wether the queue can be stopped */
static __tbx_inline__ int
__piom_ltask_queue_is_done (piom_ltask_queue_t *queue)
{
    return (!queue->ltask_array_nb_items) && 
	(queue->state >= PIOM_LTASK_QUEUE_STATE_STOPPING);

}

/* Try to schedule a task from a given queue
 * Returns the task that have been scheduled (or NULL if no task)
 */
static __tbx_inline__ void *
__piom_ltask_schedule (piom_ltask_queue_t *queue)
{
    struct piom_ltask *task = NULL;
    if(! (queue->state & PIOM_LTASK_QUEUE_STATE_RUNNING)
       && ! (queue->state & PIOM_LTASK_QUEUE_STATE_STOPPING))
	return NULL;
    being_processed[marcel_current_vp ()] = tbx_true;
    task = __piom_ltask_get_from_queue (queue);
    /* Make sure the task is runnable */
    if (__piom_ltask_is_runnable (task))
	{
	    task->state = PIOM_LTASK_STATE_SCHEDULED;

	    ma_local_bh_disable();
	    (*task->func_ptr) (task->data_ptr);
	    
	    if ((task->options & PIOM_LTASK_OPTION_REPEAT)
		&& !(task->state & PIOM_LTASK_STATE_DONE))
		{
		    ma_local_bh_enable();
		    __piom_ltask_submit_in_queue (task, queue);
		}
	    else
		{
		    task->state = PIOM_LTASK_STATE_COMPLETELY_DONE;
		    task->state |= PIOM_LTASK_STATE_DONE;
		    ma_local_bh_enable();
		}
	}

    /* no more task to run, set the queue as stopped */
    if(__piom_ltask_queue_is_done(queue))
	queue->state = PIOM_LTASK_QUEUE_STATE_STOPPED;
    else
	being_processed[marcel_current_vp ()] = tbx_false;
    return task;
}


static __tbx_inline__ void
__piom_exit_queue(piom_ltask_queue_t *queue)
{
    queue->state = PIOM_LTASK_QUEUE_STATE_STOPPING;
    /* empty the list of tasks */
    while(queue->state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	__piom_ltask_schedule (queue);
}

void
piom_init_ltasks ()
{
    if (!piom_ltask_initialized)
	{
#ifdef DEBUG
	    fprintf(stderr, "Using LTasks\n");
#endif
	    being_processed = TBX_MALLOC (sizeof (tbx_bool_t) * marcel_nbvps ());
	    int i, j;
	    for (i=0; i<marcel_topo_nblevels; i++) {
		for (j=0; j<marcel_topo_level_nbitems[i]; j++) {
		    struct marcel_topo_level *l = &marcel_topo_levels[i][j];
		    l->piom_ltask_data=TBX_MALLOC(sizeof (piom_ltask_queue_t));
		    __piom_init_queue ((piom_ltask_queue_t*)l->piom_ltask_data);
		}
	    }
	    piom_ltask_initialized = tbx_true;
	}
}

void
piom_exit_ltasks()
{
    if (piom_ltask_initialized)
	{
	    int i, j;
	    for (i=0; i<marcel_topo_nblevels; i++) {
		for (j=0; j<marcel_topo_level_nbitems[i]; j++) {
		    struct marcel_topo_level *l = &marcel_topo_levels[i][j];
		    __piom_exit_queue((piom_ltask_queue_t*)l->piom_ltask_data);
		    TBX_FREE(l->piom_ltask_data);
		}
	    }

	    TBX_FREE(being_processed);
	    piom_ltask_initialized = tbx_false;
	}

}

void __piom_ltask_submit_in_queue(struct piom_ltask *task, piom_ltask_queue_t *queue)
{
 begin:
    /* wait until a task is removed from the list */
    while (queue->ltask_array_nb_items + 1 == PIOM_MAX_LTASK)
	piom_ltask_schedule ();

    piom_spin_lock (&queue->ltask_lock);
    {
	/* the list is still full, wait until a task is removed from the list */
	if (queue->ltask_array_nb_items + 1 == PIOM_MAX_LTASK)
	    {
		piom_spin_unlock (&queue->ltask_lock);
		goto begin;
	    }
	queue->ltask_array_nb_items++;
	task->state = PIOM_LTASK_STATE_WAITING;
	queue->ltask_array[queue->ltask_array_tail++] = task;
    }
    piom_spin_unlock (&queue->ltask_lock);
}

void
piom_ltask_submit (struct piom_ltask *task)
{
    piom_ltask_queue_t *queue;
#ifdef USE_GLOBAL_QUEUE
    queue=&global_queue;
#else
    queue = __piom_get_queue (task->vp_mask);
#endif
    __piom_ltask_submit_in_queue(task, queue);
}

void *
piom_ltask_schedule ()
{
    struct piom_ltask *task;
#ifndef USE_GLOBAL_QUEUE
    struct marcel_topo_level *l;
    piom_ltask_queue_t *cur_queue;
    for(l=&marcel_topo_levels[marcel_topo_nblevels-1][marcel_current_vp()];
	l;			/* do this as long as there's a topo level */
	l=l->father)
	{
	    cur_queue = (piom_ltask_queue_t *)(l->piom_ltask_data);
	    task = __piom_ltask_schedule(cur_queue);
	    if(task)
		break;
	}
#else
    task = __piom_ltask_schedule(&global_queue);
#endif
    return task;
}

void
piom_ltask_wait_success (struct piom_ltask *task)
{
    while (!(task->state & PIOM_LTASK_STATE_DONE))
	piom_ltask_schedule ();
}

void
piom_ltask_wait (struct piom_ltask *task)
{
    while (!(task->state & PIOM_LTASK_STATE_COMPLETELY_DONE))
	piom_ltask_schedule ();
}

int
piom_ltask_polling_is_required ()
{
    int plop=marcel_current_vp();
    if (piom_ltask_initialized && !being_processed[plop])
	{
	    struct marcel_topo_level *l;
	    piom_ltask_queue_t *cur_queue;
	    for(l=&marcel_topo_levels[marcel_topo_nblevels-1][plop];
		l;			/* do this as long as there's a topo level */
		l=l->father)
		{
		    cur_queue = (piom_ltask_queue_t *)(l->piom_ltask_data);
		    if(cur_queue->ltask_array_nb_items && 
		       (cur_queue->state == PIOM_LTASK_QUEUE_STATE_RUNNING))
			return 1;
		}
	}

    return 0;
}
#endif /* PIOM_ENABLE_LTASKS */