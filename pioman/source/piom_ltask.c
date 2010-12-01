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

#include <assert.h>

#include <pioman.h>

#ifndef PIOM_DISABLE_LTASKS

#define PIOM_MAX_LTASK 256

//#define USE_GLOBAL_QUEUE 1

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
    volatile int                      ltask_array_head;
    volatile int                      ltask_array_tail;
    volatile int                      ltask_array_nb_items;
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

/** refcounter on piom_ltask */
static int piom_ltask_initialized = 0;
#ifdef USE_GLOBAL_QUEUE
static struct piom_ltask_queue global_queue;
#endif
static volatile int *being_processed = NULL;

#define GET_QUEUE(vp) ((piom_ltask_queue_t *)(marcel_topo_levels[marcel_topo_nblevels-1][vp])->ltask_data)
#define LOCAL_QUEUE (GET_QUEUE(marcel_current_vp()))

void 
piom_ltask_pause()
{
    __paused ++;
    int i = 0;
    int current_vp = 0;
#ifdef MARCEL
    current_vp = marcel_current_vp();
#else
    current_vp = 0;
#endif
    
#ifdef MARCEL
    for (i = 0; i < marcel_nbvps (); i++)
#endif
	{
	    if(i != current_vp)
		while(being_processed[i] != 0)
		    ;
	}
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
    int vp1 = marcel_vpset_first(&vpset);
    int vp2 = marcel_vpset_last(&vpset);

    if(vp2 > marcel_nbvps())
	/* deal with the MARCEL_VPSET_FULL case */
	vp2 = marcel_nbvps()-1;
    if(vp1 < 0)
	vp1 = 0;
    marcel_topo_level_t *l1 = &marcel_topo_levels[marcel_topo_nblevels-1][vp1];
    marcel_topo_level_t *l2 = &marcel_topo_levels[marcel_topo_nblevels-1][vp2];
    marcel_topo_level_t *common_ancestor = marcel_topo_common_ancestor(l1, l2);
    assert(common_ancestor != NULL);
    piom_ltask_queue_t*queue = (piom_ltask_queue_t *)common_ancestor->ltask_data;
    assert(queue != NULL);
    return queue;
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
	being_processed[i] = 0;
#else
    being_processed[0] = 0;
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
		queue->ltask_array_head = (queue->ltask_array_head + 1 ) % PIOM_MAX_LTASK;
		queue->ltask_array_nb_items--;
		assert(queue->ltask_array_nb_items >= 0 && queue->ltask_array_nb_items < PIOM_MAX_LTASK);
		assert(queue->ltask_array_head >= 0 && queue->ltask_array_head < PIOM_MAX_LTASK);
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
	    cur_queue = (piom_ltask_queue_t *)(l->ltask_data);
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
    return task && (task->state == PIOM_LTASK_STATE_WAITING) && !task->masked;
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
    if(! (queue->state & PIOM_LTASK_QUEUE_STATE_RUNNING)
       && ! (queue->state & PIOM_LTASK_QUEUE_STATE_STOPPING))
	return NULL;
#ifdef MARCEL
    const int current_vp = marcel_current_vp();
#else
    const int current_vp = 0;
#endif
    being_processed[current_vp] = 1;
    struct piom_ltask *task = __piom_ltask_get_from_queue (queue);
    if(task)
	{
	    if(task->masked)
		{
		    /* re-submit to poll later when task will be unmasked  */
		    __piom_ltask_submit_in_queue(task, queue);
		}
	    else if(task->state == PIOM_LTASK_STATE_WAITING)
		{
		    /* wait is pending: poll */
		    task->state |= PIOM_LTASK_STATE_SCHEDULED;
#ifdef MARCEL
		    /* todo: utiliser marcel_disable_preemption */
		    marcel_tasklet_disable();
#endif
		    
		    (*task->func_ptr) (task->data_ptr);
		    
		    task->state ^= PIOM_LTASK_STATE_SCHEDULED;
		    if ((task->options & PIOM_LTASK_OPTION_REPEAT)
			&& !(task->state & PIOM_LTASK_STATE_DONE))
			{
#ifdef MARCEL
			    marcel_tasklet_enable();
#endif
			    /* If another thread is currently stopping the queue don't repost the task */
			    if(queue->state == PIOM_LTASK_QUEUE_STATE_RUNNING)
				{
				    __piom_ltask_submit_in_queue (task, queue);
				}
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
			    marcel_tasklet_enable();
#endif
			}
		} 
	    else 
		{
		    if(task->state & PIOM_LTASK_STATE_DONE) 
			{
			    task->state |= PIOM_LTASK_STATE_COMPLETELY_DONE;
			    if(task->continuation_ptr)
				task->continuation_ptr(task->continuation_data_ptr);
			}
		}
	}
    /* no more task to run, set the queue as stopped */
    if(__piom_ltask_queue_is_done(queue))
	queue->state = PIOM_LTASK_QUEUE_STATE_STOPPED;
    being_processed[current_vp] = 0;
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

extern unsigned long volatile piom_jiffies;
struct marcel_timer_list ltask_poll_timer;

static void __piom_ltask_update_timer()
{
    marcel_mod_timer(&ltask_poll_timer, piom_jiffies);
}

#else

static void __piom_ltask_update_timer() { }

#endif	/* MARCEL */

void piom_init_ltasks(void)
{
    if (!piom_ltask_initialized)
	{
	    /* register a callback to Marcel */
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_TIMER);
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_YIELD);
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_IDLE);
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_LIB_ENTRY);
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_CTX_SWITCH);
#ifdef MARCEL
	    being_processed = TBX_MALLOC (sizeof(int) * marcel_nbvps ());
#else
	    being_processed = TBX_MALLOC (sizeof(int) * 1);
#endif
#ifdef USE_GLOBAL_QUEUE
	    __piom_init_queue (&global_queue);
	    global_queue.vpset= ~0;
#else
 	    int i, j;
	    for (i=0; i<marcel_topo_nblevels; i++) {
		for (j=0; j<marcel_topo_level_nbitems[i]; j++) {
		    struct marcel_topo_level *l = &marcel_topo_levels[i][j];
		    l->ltask_data=TBX_MALLOC(sizeof (piom_ltask_queue_t));
		    __piom_init_queue ((piom_ltask_queue_t*)l->ltask_data);
		    ((piom_ltask_queue_t*)l->ltask_data)->vpset = l->vpset;
		}
	    }
#endif	/* USE_GLOBAL_QUEUE */
	    marcel_init_timer(&ltask_poll_timer, piom_ltask_poll_timer, 0, 0);
	    __piom_ltask_update_timer();

	}
    piom_ltask_initialized++;
}

void piom_exit_ltasks(void)
{
    piom_ltask_initialized--;
    if(piom_ltask_initialized == 0)
	{
#ifdef USE_GLOBAL_QUEUE
	    __piom_exit_queue((piom_ltask_queue_t*)&global_queue);
#else
	    int i, j;
	    for (i=marcel_topo_nblevels -1 ; i>=0; i--) {
		for (j=0; j<marcel_topo_level_nbitems[i]; j++) {
		    struct marcel_topo_level *l = &marcel_topo_levels[i][j];
		    __piom_exit_queue((piom_ltask_queue_t*)l->ltask_data);
		    TBX_FREE(l->ltask_data);
		}
	    }
#endif
	    TBX_FREE((void*)being_processed);
	}

}

int piom_ltask_test_activity(void)
{
    return (piom_ltask_initialized != 0);
}

void 
__piom_ltask_submit_in_queue(struct piom_ltask *task, piom_ltask_queue_t *queue)
{
    piom_spin_lock(&queue->ltask_lock);

    assert(queue->ltask_array_nb_items >= 0 && queue->ltask_array_nb_items < PIOM_MAX_LTASK);
    assert(queue->ltask_array_head >= 0 && queue->ltask_array_head < PIOM_MAX_LTASK);
    assert(queue->ltask_array_tail >= 0 && queue->ltask_array_tail < PIOM_MAX_LTASK);

    /* wait until a task is removed from the list */
    while(queue->ltask_array_nb_items + 1 >= PIOM_MAX_LTASK)
	{
	    piom_spin_unlock(&queue->ltask_lock);
	    __piom_ltask_schedule(queue);
	    piom_spin_lock(&queue->ltask_lock);
	}
    if(queue->state & PIOM_LTASK_QUEUE_STATE_STOPPING)
	fprintf(stderr, "[PIOMan] warning: submitting a task (%p) to a queue (%p) that is being stopped\n", 
		task, queue);
    queue->ltask_array_nb_items++;
    task->state = PIOM_LTASK_STATE_WAITING;
    queue->ltask_array[queue->ltask_array_tail] = task;
    queue->ltask_array_tail = (queue->ltask_array_tail + 1) % PIOM_MAX_LTASK;
    queue->state = PIOM_LTASK_QUEUE_STATE_RUNNING;

    piom_spin_unlock(&queue->ltask_lock);
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
    assert(task != NULL);
    assert(queue != NULL);
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
		cur_queue = (piom_ltask_queue_t *)(l->ltask_data);
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

void piom_ltask_mask(struct piom_ltask *task)
{
    assert(!task->masked);
    task->masked = 1;
    while(task->state & PIOM_LTASK_STATE_SCHEDULED)
	{
	    piom_ltask_schedule();
	}
}

void piom_ltask_unmask(struct piom_ltask *task)
{
    assert(task->masked);
    task->masked = 0;
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
		    cur_queue = (piom_ltask_queue_t *)(l->ltask_data);
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
