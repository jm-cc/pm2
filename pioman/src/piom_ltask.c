/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001-2016 "the PM2 team" (see AUTHORS file)
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
#include <signal.h>
#include <time.h>

#include "piom_private.h"
#include <tbx_topology.h>

//#define UNLOCK_QUEUES


/** block of static state for pioman ltask */
static struct
{
    int initialized;                 /**< refcounter on piom_ltask */
    int poll_level;                  /**< 0: low; 1: high (pending req needs progression) */
    piom_ltask_queue_t**all_queues;  /**< all ltask queues, for global polling */
    int n_queues;                    /**< size of array 'all_queues' */
} __piom_ltask =
    {
	.initialized = 0,
	.n_queues    = 0,
	.poll_level  = 0
    };

/* ********************************************************* */

#if defined(PIOMAN_PTHREAD)
static void piom_ltask_hook_preinvoke(void)
{
#ifdef DEBUG
    piom_pthread_preinvoke();
#endif
}
static void piom_ltask_hook_postinvoke(void)
{
#ifdef DEBUG
    piom_pthread_postinvoke();
#endif
}
#elif defined(PIOMAN_MARCEL)
static void piom_ltask_hook_preinvoke(void)
{
    marcel_tasklet_disable();
}
static void piom_ltask_hook_postinvoke(void)
{
    marcel_tasklet_enable();
}
#elif defined(PIOMAN_NOTHREAD)
int piom_nothread_dispatching = 0;
static void piom_ltask_hook_preinvoke(void)
{
#ifdef DEBUG
    piom_nothread_dispatching++;
#endif
}
static void piom_ltask_hook_postinvoke(void)
{
#ifdef DEBUG
    piom_nothread_dispatching--;
#endif
}
#else /* PIOMAN_MARCEL */
static void piom_ltask_hook_preinvoke(void)
{ }
static void piom_ltask_hook_postinvoke(void)
{ }
#endif /* PIOMAN_PTHREAD/MARCEL */


/* Initialize a queue */
void piom_ltask_queue_init(piom_ltask_queue_t*queue, piom_topo_obj_t binding)
{
    queue->state = PIOM_LTASK_QUEUE_STATE_NONE;
    piom_ltask_lfqueue_init(&queue->ltask_queue);
    piom_ltask_lfqueue_init(&queue->submit_queue);
    piom_mask_init(&queue->mask);
    queue->binding = binding;
    __piom_ltask.all_queues = realloc(__piom_ltask.all_queues, sizeof(struct piom_ltask_queue*)*(__piom_ltask.n_queues + 1));
    __piom_ltask.all_queues[__piom_ltask.n_queues] = queue;
#if defined(PIOMAN_TOPOLOGY_HWLOC)
    char s_binding[128];
    hwloc_obj_snprintf(s_binding, sizeof(s_binding), piom_topo_get(), queue->binding, "#", 0);
    PIOM_DISP("queue #%d on %s\n", __piom_ltask.n_queues, s_binding);
#endif /* PIOMAN_TOPOLOGY_HWLOC */
    __piom_ltask.n_queues++;
    queue->state = PIOM_LTASK_QUEUE_STATE_RUNNING;
}


/* Try to schedule a task from a given queue
 * @param full schedule all tasks from if 1, a single task if 0
 */
static void piom_ltask_queue_schedule(piom_ltask_queue_t*queue, int full)
{
    if(piom_mask_acquire(&queue->mask))
	return;
    const int hint1 = (PIOM_MAX_LTASK + queue->ltask_queue._head - queue->ltask_queue._tail) % PIOM_MAX_LTASK;
    const int hint2 = (PIOM_MAX_LTASK + queue->submit_queue._head - queue->submit_queue._tail) % PIOM_MAX_LTASK;
    const int hint = ((hint1 > 0)?hint1:1) + ((hint2 > 0)?hint2:1);
    const int count = (full != 0) ? hint : 1;
    int success = 0;
    int i;
    for(i = 0; (i < count) && !success; i++)
	{
	    int again = 0;
	    struct piom_ltask*task = piom_ltask_lfqueue_dequeue_single_reader(&queue->submit_queue);
	    if(task == NULL)
		task = piom_ltask_lfqueue_dequeue_single_reader(&queue->ltask_queue);
	    piom_trace_remote_var(queue->binding, PIOM_TRACE_VAR_LTASKS,
				  (PIOM_MAX_LTASK + queue->ltask_queue._head - queue->ltask_queue._tail) % PIOM_MAX_LTASK);
	    if(task == NULL)
		{
		    if(queue->state == PIOM_LTASK_QUEUE_STATE_STOPPING) 
			{
			    /* no more task to run, set the queue as stopped */
			    queue->state = PIOM_LTASK_QUEUE_STATE_STOPPED;
			    return; /* purposely return while holding the lock */
			}
		}
	    else
		{
		    piom_trace_local_state(PIOM_TRACE_STATE_INIT);
		    const int prestate = piom_ltask_state_set(task, PIOM_LTASK_STATE_SCHEDULED);
		    if(prestate & PIOM_LTASK_STATE_READY)
			{
			    /* wait is pending: poll */
			    piom_trace_local_state(PIOM_TRACE_STATE_POLL);
			    piom_ltask_hook_preinvoke();
			    const int options = task->options;
#ifdef UNLOCK_QUEUES
			    piom_mask_release(&queue->mask); /* unlock queue to schedule- will be re-acquired later */
#endif /* UNLOCK_QUEUES */
			    (*task->func_ptr)(task->data_ptr);
			    if(options & PIOM_LTASK_OPTION_DESTROY)
				{
				    /* ltask was destroyed by handler */
				    task = NULL;
				    piom_ltask_hook_postinvoke();
				    success = 1;
				}
			    else if((options & PIOM_LTASK_OPTION_REPEAT) && !(task->state & PIOM_LTASK_STATE_SUCCESS))
				{
				    piom_ltask_hook_postinvoke();
				    /* If another thread is currently stopping the queue don't repost the task */
				    piom_ltask_state_unset(task, PIOM_LTASK_STATE_SCHEDULED);
				    if(queue->state == PIOM_LTASK_QUEUE_STATE_RUNNING)
					{
					    again = 1;
					    assert(task->state != PIOM_LTASK_STATE_NONE);
					    assert(!(task->state & PIOM_LTASK_STATE_DESTROYED));
					    if(options & PIOM_LTASK_OPTION_BLOCKING)
						{
						    struct timespec t;
						    clock_gettime(CLOCK_MONOTONIC, &t);
						    const long d_usec = (t.tv_sec - task->origin.tv_sec) * 1000000 +
							(t.tv_nsec - task->origin.tv_nsec) / 1000;
						    if(d_usec > task->blocking_delay)
							{
							    if(piom_ltask_submit_in_lwp(task) == 0)
								again = 0;
							}
						}
					}
				    else
					{
					    PIOM_WARN("task %p is left uncompleted\n", task);
					}
				}
			    else
				{
				    task->state = PIOM_LTASK_STATE_TERMINATED;
				    piom_ltask_completed(task);
				    if((task->options & PIOM_LTASK_OPTION_NOWAIT) && (task->destructor))
					{
					    (*task->destructor)(task);
					}
				    task = NULL;
				    piom_ltask_hook_postinvoke();
				    success = 1;
				}
#ifdef UNLOCK_QUEUES
			    const int relock = piom_mask_acquire(&queue->mask);
			    if(relock != 0)
				{
				    /* didn't get the lock again- enqueue as new, and abort loop */
				    if(again)
					{
					    piom_ltask_lfqueue_enqueue(&queue->submit_queue, task);
					}
				    return;
				}
#endif /* UNLOCK_QUEUES */
			} 
		    else if((prestate & PIOM_LTASK_STATE_SUCCESS) ||
			    (prestate & PIOM_LTASK_STATE_CANCELLED))
			{
			    success = 1;
			    piom_ltask_state_unset(task, PIOM_LTASK_STATE_SCHEDULED);
			    piom_ltask_state_set(task, PIOM_LTASK_STATE_TERMINATED);
			}
		    else
			{
			    PIOM_FATAL("wrong state 0x%4x for scheduled ltask.\n", prestate);
			}
		    piom_trace_local_state(PIOM_TRACE_STATE_NONE);
		    if(success)
			{
			    piom_trace_local_event(PIOM_TRACE_EVENT_SUCCESS, task);
			}
		    if(again)
			{
			    int rc = -1;
			    while(rc != 0)
				{
				    assert(task->state != PIOM_LTASK_STATE_NONE);
				    assert(!(task->state & PIOM_LTASK_STATE_DESTROYED));
				    rc = piom_ltask_lfqueue_enqueue_single_writer(&queue->ltask_queue, task);
				}
			}
		}
	}
    piom_mask_release(&queue->mask);
}


void piom_ltask_queue_exit(piom_ltask_queue_t*queue)
{
    queue->state = PIOM_LTASK_QUEUE_STATE_STOPPING;

    /* empty the list of tasks */
    while(queue->state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	{
	    piom_ltask_queue_schedule(queue, 0);
	}
}

void piom_init_ltasks(void)
{
    if(__piom_ltask.initialized)
	return;
    __piom_ltask.initialized++;

    /* ** create queues */
    piom_topo_init_ltasks();

    /* ** Start polling */
    
#ifdef PIOMAN_PTHREAD 
    piom_pthread_init_ltasks();
#endif /* PIOMAN_PTHREAD */

#ifdef PIOMAN_ABT
    piom_abt_init_ltasks();
#endif /* PIOMAN_ABT */
}

void piom_exit_ltasks(void)
{
    __piom_ltask.initialized--;
    assert(__piom_ltask.initialized >= 0);
    if(__piom_ltask.initialized == 0)
	{
	    int i;
	    for(i = 0; i < __piom_ltask.n_queues; i++)
		{
		    piom_ltask_queue_exit(__piom_ltask.all_queues[i]);
		}
#ifdef PIOMAN_TRACE
	    piom_trace_flush();
#endif /* PIOMAN_TRACE */
#ifdef PIOMAN_PTHREAD 
	    piom_pthread_exit_ltasks();
#endif /* PIOMAN_PTHREAD */
	    piom_topo_exit_ltasks();
	}

}

int piom_ltask_test_activity(void)
{
    return (__piom_ltask.initialized != 0);
}

void piom_ltask_poll_level_set(int level)
{
    /* no need for atomic here- polling level is a hint */
    __piom_ltask.poll_level = level;
}

void piom_ltask_submit(struct piom_ltask*task)
{
    assert(task != NULL);
    assert(task->state == PIOM_LTASK_STATE_NONE || (task->state & PIOM_LTASK_STATE_TERMINATED) );
    task->state = PIOM_LTASK_STATE_NONE;
    piom_ltask_queue_t*queue = piom_topo_get_queue(task->binding);
    assert(queue != NULL);
    piom_trace_remote_event(queue->binding, PIOM_TRACE_EVENT_SUBMIT, task);
    if(queue->state == PIOM_LTASK_QUEUE_STATE_STOPPING)
	{
	    PIOM_WARN("submitting a task (%p) to a queue (%p) that is being stopped\n", task, queue);
	}
    task->state = PIOM_LTASK_STATE_READY;
    task->queue = queue;
    /* wait until a task is removed from the list */
    int rc = -1;
    do
	{
	    rc = piom_ltask_lfqueue_enqueue(&queue->submit_queue, task);
	    if(rc != 0)
		{
		    PIOM_WARN("ltask queue full\n");
		    piom_ltask_schedule(PIOM_POLL_POINT_BUSY);
		}
	}
    while(rc != 0);
    piom_trace_remote_var(queue->binding, PIOM_TRACE_VAR_LTASKS, (PIOM_MAX_LTASK + queue->ltask_queue._head - queue->ltask_queue._tail) % PIOM_MAX_LTASK);
}

void piom_ltask_schedule(int point)
{
    if(__piom_ltask.initialized)
	{
	    if(point == PIOM_POLL_POINT_BUSY)
		{
		    /* busy wait- poll all queues, higher frequency for local queue */
		    piom_ltask_queue_t*local_queue = piom_topo_get_queue(piom_topo_current_obj());
		    int i;
		    for(i = 0; i < __piom_ltask.n_queues; i++)
			{
			    piom_ltask_queue_t*queue = __piom_ltask.all_queues[i];
			    if(queue == local_queue)
				{
				    piom_ltask_queue_schedule(queue, 1);
				}
			    else
				{
				    piom_ltask_queue_schedule(queue, 0);
				}
			}
		}
	    else if(point == PIOM_POLL_POINT_TIMER)
		{
		    /* timer poll- poll all tasks from all queues */
		    int i;
		    for(i = 0; i < __piom_ltask.n_queues; i++)
			{
			    piom_ltask_queue_t*queue = __piom_ltask.all_queues[i];
			    piom_ltask_queue_schedule(queue, 1);
			}
		}
	    else
		{
		    /* other points (idle, forced)- poll local queue */
		    piom_ltask_queue_t*queue = piom_topo_get_queue(piom_topo_current_obj());
		    piom_ltask_queue_schedule(queue, 1);
		}
	}
}

void piom_ltask_wait_success(struct piom_ltask *task)
{
    assert(task->state != PIOM_LTASK_STATE_NONE);
    assert(!(task->options & PIOM_LTASK_OPTION_DESTROY));
    assert(!(task->options & PIOM_LTASK_OPTION_NOWAIT));
    piom_cond_wait(&task->done, PIOM_LTASK_STATE_SUCCESS);
}

void piom_ltask_wait(struct piom_ltask *task)
{
    piom_ltask_wait_success(task);
    while(!piom_ltask_state_test(task, PIOM_LTASK_STATE_TERMINATED))
	{
	    piom_ltask_schedule(PIOM_POLL_POINT_BUSY);
	}
}

void piom_ltask_set_blocking(struct piom_ltask*task, piom_ltask_func_t func, int delay_usec)
{
    task->blocking_func = func;
    task->blocking_delay = delay_usec;
    task->options |= PIOM_LTASK_OPTION_BLOCKING;
    clock_gettime(CLOCK_MONOTONIC, &task->origin);
}

void piom_ltask_mask(struct piom_ltask *task)
{
    piom_ltask_state_unset(task, PIOM_LTASK_STATE_READY);
    while(piom_ltask_state_test(task, PIOM_LTASK_STATE_SCHEDULED))
	{
	    sched_yield();
	}
}

void piom_ltask_unmask(struct piom_ltask *task)
{
    assert(piom_ltask_state_test(task, PIOM_LTASK_STATE_READY) == 0);
    piom_ltask_state_set(task, PIOM_LTASK_STATE_READY);
}

void piom_ltask_cancel(struct piom_ltask*ltask)
{
    if(ltask->state == PIOM_LTASK_STATE_NONE)
	return;
    piom_ltask_state_set(ltask, PIOM_LTASK_STATE_CANCELLED);
    piom_ltask_state_unset(ltask, PIOM_LTASK_STATE_READY);
    struct piom_ltask_queue*queue = ltask->queue;
    if(queue)
	{
	    while(!piom_ltask_state_test(ltask, PIOM_LTASK_STATE_TERMINATED))
		{
		    piom_ltask_queue_schedule(queue, 0);
		}
	}
    ltask->state = PIOM_LTASK_STATE_TERMINATED;
}

