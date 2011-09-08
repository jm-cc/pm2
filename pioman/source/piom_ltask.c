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


#ifdef MARCEL
#define piom_ltask_current_vp() marcel_current_vp()
#define piom_ltask_nvp()        marcel_nbvps()
#else
#define piom_ltask_current_vp() (0)
#define piom_ltask_nvp()        (1)
#endif

void piom_ltask_pause(void)
{
    __paused ++;
    int i = 0;
    int current_vp = piom_ltask_current_vp();
    
    for (i = 0; i < piom_ltask_nvp(); i++)
	{
	    if(i != current_vp)
		while(being_processed[i] != 0)
		    ;
	}
}

void piom_ltask_resume(void)
{
    __paused --;
}

/* Get the queue that matches a vpset:
 * - a local queue if only one vp is set in the vpset
 * - the global queue otherwise
 */
static inline piom_ltask_queue_t*__piom_get_queue(piom_vpset_t vpset)
{
#ifdef USE_GLOBAL_QUEUE
    return &global_queue;
#else /* USE_GLOBAL_QUEUE */
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
static inline void __piom_init_queue(piom_ltask_queue_t * queue)
{
    queue->state = PIOM_LTASK_QUEUE_STATE_NONE;
    int i;
    for (i = 0; i < PIOM_MAX_LTASK; i++)
	queue->ltask_array[i] = NULL;
    queue->ltask_array_tail = 0;
    queue->ltask_array_head = 0;
    queue->ltask_array_nb_items = 0;

    for (i = 0; i < piom_ltask_nvp(); i++)
	being_processed[i] = 0;

    piom_spin_lock_init (&queue->ltask_lock);
    queue->state = PIOM_LTASK_QUEUE_STATE_RUNNING;
}

/* Remove a task from a queue and return it
 * Return NULL if the queue is empty
 */
static inline struct piom_ltask*__piom_ltask_get_from_queue(piom_ltask_queue_t * queue)
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


/* Try to schedule a task from a given queue
 * Returns the task that have been scheduled (or NULL if no task)
 * If it returns a task, this task have been freed.
 */
static inline void* __piom_ltask_schedule(piom_ltask_queue_t *queue)
{
    if(! (queue->state & PIOM_LTASK_QUEUE_STATE_RUNNING)
       && ! (queue->state & PIOM_LTASK_QUEUE_STATE_STOPPING))
	return NULL;
    const int current_vp = piom_ltask_current_vp();
    being_processed[current_vp] = 1;
    struct piom_ltask *task = __piom_ltask_get_from_queue (queue);
    if(task)
	{
	    if(task->masked)
		{
		    /* re-submit to poll later when task will be unmasked  */
		    __piom_ltask_submit_in_queue(task, queue);
		}
	    else if(task->state == PIOM_LTASK_STATE_READY)
		{
		    /* wait is pending: poll */
		    task->state |= PIOM_LTASK_STATE_SCHEDULED;
#ifdef MARCEL
		    /* todo: utiliser marcel_disable_preemption */
		    marcel_tasklet_disable();
#endif

		    (*task->func_ptr)(task->data_ptr);

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
				{
				    fprintf(stderr, "PIOMan: WARNING- task %p is leaved uncompleted\n", task);
				}
			}
		    else
			{
			    task->state = PIOM_LTASK_STATE_TERMINATED;
			    piom_ltask_completed (task);
			    if(task->continuation_ptr)
				(*task->continuation_ptr)(task->continuation_data_ptr);
#ifdef MARCEL
			    marcel_tasklet_enable();
#endif
			}
		} 
	    else 
		{
		    if(task->state & PIOM_LTASK_STATE_DONE) 
			{
			    task->state |= PIOM_LTASK_STATE_TERMINATED;
			    if(task->continuation_ptr)
				task->continuation_ptr(task->continuation_data_ptr);
			}
		}
	}
    /* no more task to run, set the queue as stopped */
    if( (queue->ltask_array_nb_items == 0) && 
	(queue->state >= PIOM_LTASK_QUEUE_STATE_STOPPING) )
	{
	    queue->state = PIOM_LTASK_QUEUE_STATE_STOPPED;
	}
    being_processed[current_vp] = 0;
    return task;
}


static inline void __piom_exit_queue(piom_ltask_queue_t *queue)
{
    queue->state = PIOM_LTASK_QUEUE_STATE_STOPPING;

    /* empty the list of tasks */
    while(queue->state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	{
#ifdef MARCEL
	    piom_vpset_t local_vpset = 1<<marcel_current_vp();
	    if(queue->vpset & local_vpset)
#endif
		__piom_ltask_schedule (queue);
	}
}


void piom_init_ltasks(void)
{
    if (!piom_ltask_initialized)
	{
#ifdef MARCEL
	    /* register a callback to Marcel */
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_TIMER);
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_YIELD);
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_IDLE);
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_LIB_ENTRY);
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_CTX_SWITCH);

	    being_processed = TBX_MALLOC (sizeof(int) * marcel_nbvps ());
#else /* MARCEL */
	    being_processed = TBX_MALLOC (sizeof(int) * 1);
#endif /* MARCEL */

#ifdef USE_GLOBAL_QUEUE
	    __piom_init_queue (&global_queue);
	    global_queue.vpset= ~0;
#else /* USE_GLOBAL_QUEUE */
 	    int i, j;
	    for(i = 0; i < marcel_topo_nblevels; i++)
		{
		    for(j = 0; j < marcel_topo_level_nbitems[i]; j++)
			{
			    struct marcel_topo_level *l = &marcel_topo_levels[i][j];
			    l->ltask_data = TBX_MALLOC(sizeof (piom_ltask_queue_t));
			    __piom_init_queue ((piom_ltask_queue_t*)l->ltask_data);
			    ((piom_ltask_queue_t*)l->ltask_data)->vpset = l->vpset;
			}
		}
#endif	/* USE_GLOBAL_QUEUE */
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
	    for(i = marcel_topo_nblevels -1 ; i >= 0; i--)
		{
		    for(j = 0; j < marcel_topo_level_nbitems[i]; j++)
			{
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

void __piom_ltask_submit_in_queue(struct piom_ltask *task, piom_ltask_queue_t *queue)
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
	{
	    fprintf(stderr, "PIOMan: WARNING- submitting a task (%p) to a queue (%p) that is being stopped\n", 
		    task, queue);
	}
    queue->ltask_array_nb_items++;
    task->state = PIOM_LTASK_STATE_READY;
    task->queue = queue;
    queue->ltask_array[queue->ltask_array_tail] = task;
    queue->ltask_array_tail = (queue->ltask_array_tail + 1) % PIOM_MAX_LTASK;
    queue->state = PIOM_LTASK_QUEUE_STATE_RUNNING;

    piom_spin_unlock(&queue->ltask_lock);
}

void piom_ltask_submit(struct piom_ltask *task)
{
    piom_ltask_queue_t *queue;
#ifdef USE_GLOBAL_QUEUE
    queue = &global_queue;
#else /* USE_GLOBAL_QUEUE */
    queue = __piom_get_queue (task->vp_mask);
#endif /* USE_GLOBAL_QUEUE */
    assert(task != NULL);
    assert(queue != NULL);
    __piom_ltask_submit_in_queue(task, queue);
}

void*piom_ltask_schedule(void)
{
    struct piom_ltask *task = NULL;
    if(piom_ltask_initialized && !__paused) {
#ifdef USE_GLOBAL_QUEUE
	task = __piom_ltask_schedule(&global_queue);
#else /* USE_GLOBAL_QUEUE */
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
#endif /* USE_GLOBAL_QUEUE */
    }
    return task;
}

void piom_ltask_wait_success(struct piom_ltask *task)
{
    piom_cond_wait(&task->done, PIOM_LTASK_STATE_DONE);
}

void piom_ltask_wait(struct piom_ltask *task)
{
    piom_cond_wait(&task->done, PIOM_LTASK_STATE_DONE);
    while (!(task->state & PIOM_LTASK_STATE_TERMINATED))
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

void piom_ltask_cancel(struct piom_ltask*task)
{
    piom_ltask_pause();
    if(task)
	{
	    struct piom_ltask_queue*queue = task->queue;
	    if(task->queue)
		{
		    int i;
		    for(i = 0; i < PIOM_MAX_LTASK; i++)
			{
			    if(queue->ltask_array[i] == task)
				queue->ltask_array[i] = NULL;
			}
		}
	    task->state = PIOM_LTASK_STATE_TERMINATED;
	    piom_ltask_completed (task);
	    if(task->continuation_ptr)
		task->continuation_ptr(task->continuation_data_ptr);
	}
    piom_ltask_pause();
}

int piom_ltask_polling_is_required(void)
{
    int vp = piom_ltask_current_vp();
    if (piom_ltask_initialized && !being_processed[vp])
	{
#ifdef USE_GLOBAL_QUEUE
	    if(global_queue.ltask_array_nb_items && 
	       (global_queue.state == PIOM_LTASK_QUEUE_STATE_RUNNING))
		return 1;
#else /* USE_GLOBAL_QUEUE */
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
#endif /* USE_GLOBAL_QUEUE */
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
#else  /* MA__NUMA */
piom_vpset_t piom_get_parent_machine(unsigned vp) { return piom_vpset_full; }
piom_vpset_t piom_get_parent_node(unsigned vp) { return piom_vpset_full; }
piom_vpset_t piom_get_parent_die(unsigned vp) { return piom_vpset_full; }
piom_vpset_t piom_get_parent_l3(unsigned vp) { return piom_vpset_full; }
piom_vpset_t piom_get_parent_l2(unsigned vp) { return piom_vpset_full; }
piom_vpset_t piom_get_parent_core(unsigned vp) { return piom_vpset_full; }

#endif /* MA__NUMA */

