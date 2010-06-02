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

#ifndef PIOM_DISABLE_LTASKS

#define PIOM_MAX_LTASK 256

#ifndef MA__NUMA
#define USE_GLOBAL_QUEUE 1
#endif

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
    piom_vpset_t                      vpset;
#ifdef DEBUG
    int                               id;
#endif
} piom_ltask_queue_t;

void __piom_ltask_submit_in_queue(struct piom_ltask *task, piom_ltask_queue_t *queue);

/* when __paused != 0, no scheduling operation can be performed, except during __exit_queue */
static volatile unsigned __paused = 0;

static tbx_bool_t piom_ltask_initialized = tbx_false;
#ifdef USE_GLOBAL_QUEUE
static struct piom_ltask_queue global_queue;
#endif
static tbx_bool_t *being_processed;

#define GET_QUEUE(vp) ((piom_ltask_queue_t *)(marcel_topo_levels[marcel_topo_nblevels-1][vp])->piom_ltask_data)
#define LOCAL_QUEUE (GET_QUEUE(marcel_current_vp()))

void 
piom_ltask_pause()
{
    __paused ++;
    int i = 0;
    int was_processing = 0;
    int current_vp = 0;
#ifdef MARCEL
    current_vp = marcel_current_vp();
#else
    current_vp = 0;
#endif

    was_processing = being_processed[current_vp];
    if(was_processing)
	{
	    being_processed[current_vp] = tbx_false;
	}
#ifdef MARCEL
    for (i = 0; i < marcel_nbvps (); i++)
#endif
	{
	    while(being_processed[i] == tbx_true);
	}
    if(was_processing)
	being_processed[current_vp] = tbx_true;
}

void 
piom_ltask_resume()
{
    __paused --;
}

/* Get the queue that matches a vpset:
 * - a local queue if only one vp is set in the vpset
 * - the global queue otherwise
 */
static inline piom_ltask_queue_t *
__piom_get_queue (piom_vpset_t vpset)
{
#ifdef USE_GLOBAL_QUEUE
    return &global_queue;
#else
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
#endif	/* USE_GLOBAL_QUEUE */
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
#ifdef MARCEL
    for (i = 0; i < marcel_nbvps (); i++)
	being_processed[i] = tbx_false;
#else
	being_processed[0] = tbx_false;
#endif
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

#ifdef USE_GLOBAL_QUEUE
    result = __piom_ltask_get_from_queue (&global_queue);
    *queue = &global_queue;    
#else
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
 * If it returns a task, this task have been freed.
 */
static __tbx_inline__ void *
__piom_ltask_schedule (piom_ltask_queue_t *queue)
{
    struct piom_ltask *task = NULL;
    if(! (queue->state & PIOM_LTASK_QUEUE_STATE_RUNNING)
       && ! (queue->state & PIOM_LTASK_QUEUE_STATE_STOPPING))
	return NULL;
#ifdef MARCEL
    being_processed[marcel_current_vp ()] = tbx_true;
#else
    being_processed[0] = tbx_true;
#endif
    task = __piom_ltask_get_from_queue (queue);
    /* Make sure the task is runnable */
    if (__piom_ltask_is_runnable (task))
	{
	    task->state |= PIOM_LTASK_STATE_SCHEDULED;
#ifdef MARCEL
	    /* todo: utiliser marcel_disable_preemption */
	    ma_local_bh_disable();
#endif
	 
	    (*task->func_ptr) (task->data_ptr);

	    task->state ^= PIOM_LTASK_STATE_SCHEDULED;
	    if ((task->options & PIOM_LTASK_OPTION_REPEAT)
		&& !(task->state & PIOM_LTASK_STATE_DONE))
		{
#ifdef MARCEL
		    ma_local_bh_enable();
#endif
		    /* If another thread is currently stopping the queue don't repost the task */
		    if(queue->state == PIOM_LTASK_QUEUE_STATE_RUNNING)
			__piom_ltask_submit_in_queue (task, queue);
		    else
			fprintf(stderr, "warning: task %p is leaved uncompleted\n", task);
		}
	    else
		{
		    task->state = PIOM_LTASK_STATE_COMPLETELY_DONE;
		    task->state |= PIOM_LTASK_STATE_DONE;
		    if(task->continuation_ptr)
			task->continuation_ptr(task->continuation_data_ptr);
#ifdef MARCEL
		    ma_local_bh_enable();
#endif
		}
	} else {
	if(task && task->state & PIOM_LTASK_STATE_DONE) {
	    task->state |= PIOM_LTASK_STATE_COMPLETELY_DONE;
	    if(task->continuation_ptr)
		task->continuation_ptr(task->continuation_data_ptr);
	}
    }

    /* no more task to run, set the queue as stopped */
    if(__piom_ltask_queue_is_done(queue))
	queue->state = PIOM_LTASK_QUEUE_STATE_STOPPED;
    else
#ifdef MARCEL
	being_processed[marcel_current_vp ()] = tbx_false;
#else
	being_processed[0] = tbx_false;
#endif
    return task;
}


static __tbx_inline__ void
__piom_exit_queue(piom_ltask_queue_t *queue)
{
    queue->state = PIOM_LTASK_QUEUE_STATE_STOPPING;

    /* empty the list of tasks */
    while(queue->state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	{
	    piom_vpset_t local_vpset = 1<<marcel_current_vp();
	    if(queue->vpset & local_vpset)
		__piom_ltask_schedule (queue);
	}
}

static void __piom_ltask_update_timer();

void 
piom_ltask_poll_timer(unsigned long hid)
{
    if (piom_ltask_initialized && !being_processed[marcel_current_vp()] && !__paused) {
	piom_ltask_schedule();
    }
    __piom_ltask_update_timer();
}

#ifdef MARCEL

extern unsigned long volatile ma_jiffies;
struct ma_timer_list ltask_poll_timer = MA_TIMER_INITIALIZER(piom_ltask_poll_timer, 0, 0);

static void __piom_ltask_update_timer()
{
    ma_mod_timer(&ltask_poll_timer, ma_jiffies);
}

#else

static void __piom_ltask_update_timer() { }

#endif	/* MARCEL */

void
piom_init_ltasks ()
{
    if (!piom_ltask_initialized)
	{
#ifdef USE_GLOBAL_QUEUE
	    being_processed = TBX_MALLOC (sizeof (tbx_bool_t) * 1);
	    __piom_init_queue (&global_queue);
	    global_queue.vpset= ~0;
#else
	    being_processed = TBX_MALLOC (sizeof (tbx_bool_t) * marcel_nbvps ());
 	    int i, j;
	    for (i=0; i<marcel_topo_nblevels; i++) {
		for (j=0; j<marcel_topo_level_nbitems[i]; j++) {
		    struct marcel_topo_level *l = &marcel_topo_levels[i][j];
		    l->piom_ltask_data=TBX_MALLOC(sizeof (piom_ltask_queue_t));
		    __piom_init_queue ((piom_ltask_queue_t*)l->piom_ltask_data);
		    ((piom_ltask_queue_t*)l->piom_ltask_data)->vpset = l->vpset;
		}
	    }
#endif	/* USE_GLOBAL_QUEUE */
	    __piom_ltask_update_timer();

 	    piom_ltask_initialized = tbx_true;
	}
}

void
piom_exit_ltasks()
{
    if (piom_ltask_initialized)
	{
#ifdef USE_GLOBAL_QUEUE
	    __piom_exit_queue((piom_ltask_queue_t*)&global_queue);
#else
	    int i, j;
	    for (i=marcel_topo_nblevels -1 ; i>=0; i--) {
		for (j=0; j<marcel_topo_level_nbitems[i]; j++) {
		    struct marcel_topo_level *l = &marcel_topo_levels[i][j];
		    __piom_exit_queue((piom_ltask_queue_t*)l->piom_ltask_data);
		    TBX_FREE(l->piom_ltask_data);
		}
	    }
#endif
	    TBX_FREE(being_processed);
	    piom_ltask_initialized = tbx_false;

	}

}

int 
piom_ltask_test_activity()
{
    return piom_ltask_initialized;
}

void 
__piom_ltask_submit_in_queue(struct piom_ltask *task, piom_ltask_queue_t *queue)
{
 begin:
    /* wait until a task is removed from the list */
    while (queue->ltask_array_nb_items + 1 == PIOM_MAX_LTASK)
	piom_ltask_schedule ();

    piom_spin_lock (&queue->ltask_lock);
    {
	if(queue->state & PIOM_LTASK_QUEUE_STATE_STOPPING)
	    fprintf(stderr, "[PIOMan] warning: submitting a task (%p) to a queue (%p) that is being stopped\n", 
		    task, queue);
	/* the list is still full, wait until a task is removed from the list */
	if (queue->ltask_array_nb_items + 1 == PIOM_MAX_LTASK)
	    {
		piom_spin_unlock (&queue->ltask_lock);
		goto begin;
	    }
	queue->ltask_array_nb_items++;
	task->state = PIOM_LTASK_STATE_WAITING;
	queue->ltask_array[queue->ltask_array_tail++] = task;
	queue->state = PIOM_LTASK_QUEUE_STATE_RUNNING;
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
    struct piom_ltask *task = NULL;
    if(piom_ltask_initialized && !__paused) {
#ifdef USE_GLOBAL_QUEUE
	task = __piom_ltask_schedule(&global_queue);
#else
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
#endif
    }
    return task;
}

void
piom_ltask_wait_success (struct piom_ltask *task)
{
    while (!(task->state & PIOM_LTASK_STATE_DONE))
	if(!__paused)
	    piom_ltask_schedule ();
}

void
piom_ltask_wait (struct piom_ltask *task)
{
    while (!(task->state & PIOM_LTASK_STATE_COMPLETELY_DONE))
	if(!__paused)
	    piom_ltask_schedule ();
}

int
piom_ltask_polling_is_required ()
{
#ifdef MARCEL
    int vp = marcel_current_vp();
#else
    int vp = 0;
#endif
    if (piom_ltask_initialized && !being_processed[vp])
	{
#ifdef USE_GLOBAL_QUEUE
	    if(global_queue.ltask_array_nb_items && 
	       (global_queue.state == PIOM_LTASK_QUEUE_STATE_RUNNING))
		return 1;
#else
	    struct marcel_topo_level *l;
	    piom_ltask_queue_t *cur_queue;
	    for(l=&marcel_topo_levels[marcel_topo_nblevels-1][vp];
		l;			/* do this as long as there's a topo level */
		l=l->father)
		{
		    cur_queue = (piom_ltask_queue_t *)(l->piom_ltask_data);
		    if(cur_queue->ltask_array_nb_items && 
		       (cur_queue->state == PIOM_LTASK_QUEUE_STATE_RUNNING))
			return 1;
		    if(cur_queue->state == PIOM_LTASK_QUEUE_STATE_STOPPING)
			return 1;
		}
#endif	/* USE_GLOBAL_QUEUE */
	}

    return 0;
}


#ifdef MA__NUMA
piom_vpset_t piom_get_parent_machine(unsigned vp) {
    marcel_topo_level_t *l = &marcel_topo_levels[marcel_topo_nblevels-1][vp];
    while(l->type > MARCEL_LEVEL_MACHINE)
	l=l->father;
    return l->vpset;
}

piom_vpset_t piom_get_parent_node(unsigned vp) {
    marcel_topo_level_t *l = &marcel_topo_levels[marcel_topo_nblevels-1][vp];
    while(l->type > MARCEL_LEVEL_NODE)
	l=l->father;
    return l->vpset;
}

piom_vpset_t piom_get_parent_die(unsigned vp) {
    marcel_topo_level_t *l = &marcel_topo_levels[marcel_topo_nblevels-1][vp];
    while(l->type > MARCEL_LEVEL_DIE)
	l=l->father;
    return l->vpset;
}

piom_vpset_t piom_get_parent_l3(unsigned vp) {
    marcel_topo_level_t *l = &marcel_topo_levels[marcel_topo_nblevels-1][vp];
    while(l->type > MARCEL_LEVEL_L2)
	l=l->father;
    return l->vpset;
}

piom_vpset_t piom_get_parent_l2(unsigned vp) {
    marcel_topo_level_t *l = &marcel_topo_levels[marcel_topo_nblevels-1][vp];
    while(l->type > MARCEL_LEVEL_L2)
	l=l->father;
    return l->vpset;
}

piom_vpset_t piom_get_parent_core(unsigned vp) {
    marcel_topo_level_t *l = &marcel_topo_levels[marcel_topo_nblevels-1][vp];
    while(l->type > MARCEL_LEVEL_VP)
	l=l->father;
    return l->vpset;
}
#else  /* MARCEL */
piom_vpset_t piom_get_parent_machine(unsigned vp) { return piom_vpset_full; }
piom_vpset_t piom_get_parent_node(unsigned vp) { return piom_vpset_full; }
piom_vpset_t piom_get_parent_die(unsigned vp) { return piom_vpset_full; }
piom_vpset_t piom_get_parent_l3(unsigned vp) { return piom_vpset_full; }
piom_vpset_t piom_get_parent_l2(unsigned vp) { return piom_vpset_full; }
piom_vpset_t piom_get_parent_core(unsigned vp) { return piom_vpset_full; }

#endif /* MARCEL */

#endif /* PIOM_DISABLE_LTASKS */
