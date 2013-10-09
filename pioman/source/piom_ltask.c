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
#include <signal.h>
#include <time.h>

#include "piom_private.h"
#include <tbx_topology.h>

#define PIOM_MAX_LTASK 256

PIOM_LFQUEUE_TYPE(piom_ltask, struct piom_ltask*, NULL, PIOM_MAX_LTASK);

typedef enum
    {
	/* not yet initialized */
	PIOM_LTASK_QUEUE_STATE_NONE,
	/* running */
	PIOM_LTASK_QUEUE_STATE_RUNNING,
	/* being stopped */
	PIOM_LTASK_QUEUE_STATE_STOPPING,
	/* stopped: no more request in the queue, 
	   no more task from this queue being executed */
	PIOM_LTASK_QUEUE_STATE_STOPPED
    } piom_ltask_queue_state_t;

typedef struct piom_ltask_queue
{
    struct piom_ltask_lfqueue_s       ltask_queue;
    volatile piom_ltask_queue_state_t state;
    volatile int                      processing;
    piom_topo_obj_t                   binding;
    struct piom_ltask_queue          *parent;
} piom_ltask_queue_t;

static void __piom_ltask_submit_in_queue(struct piom_ltask *task, piom_ltask_queue_t *queue);
static int __piom_ltask_submit_in_lwp(struct piom_ltask*task);


/** refcounter on piom_ltask */
static int piom_ltask_initialized = 0;

#if defined(PIOMAN_TOPOLOGY_NONE)
static struct piom_ltask_queue global_queue;
#elif defined(PIOMAN_TOPOLOGY_HWLOC)
/** node topology */
hwloc_topology_t __piom_ltask_topology = NULL;
/** locality information */
struct piom_ltask_local_s
{
    pthread_t tid;              /**< thread this info is about */
    hwloc_cpuset_t cpuset;      /**< last cpuset location for thread  */
    hwloc_obj_t obj;            /**< last obj encompassing cpuset */
    tbx_tick_t timestamp;
    int count;
};
pthread_key_t __piom_ltask_local_key;
#endif /* PIOMAN_TOPOLOGY_* */

#ifdef PIOMAN_PTHREAD
/** number of spare LWPs running */
static int                __piom_ltask_lwps_num = 0;
/** number of available LWPs */
static int                __piom_ltask_lwps_avail = 0;
/** signal new tasks to LWPs */
static sem_t              __piom_ltask_lwps_ready;
/** ltasks queue to feed LWPs */
static struct piom_ltask_lfqueue_s __piom_ltask_lwps_queue;

static void*__piom_ltask_lwp_worker(void*_dummy);
#endif /* PIOMAN_PTHREAD */

/* ********************************************************* */

#if defined(PIOMAN_PTHREAD)
static void piom_tasklet_mask(void)
{
}
static void piom_tasklet_unmask(void)
{
}
#elif defined(PIOMAN_MARCEL)
static void piom_tasklet_mask(void)
{
    marcel_tasklet_disable();
}
static void piom_tasklet_unmask(void)
{
    marcel_tasklet_enable();
}
#else /* PIOMAN_MARCEL */
static void piom_tasklet_mask(void)
{ }
static void piom_tasklet_unmask(void)
{ }
#endif /* PIOMAN_PTHREAD/MARCEL */


/* Get the queue that matches a topology object
 */
static inline piom_ltask_queue_t*__piom_get_queue(piom_topo_obj_t obj)
{
    assert(obj != NULL);
#if defined(PIOMAN_TOPOLOGY_HWLOC)
    while(obj != NULL && obj->userdata == NULL)
	{
	    obj = obj->parent;
	}
    return (obj != NULL) ? obj->userdata : NULL;
#elif defined(PIOMAN_TOPOLOGY_MARCEL)
    return obj->ltask_data;
#elif defined(PIOMAN_TOPOLOGY_NONE)
    return &global_queue;
#else /* PIOMAN_TOPOLOGY_* */
#error
    return NULL;
#endif	/* PIOMAN_TOPOLOGY_* */
}

/* Initialize a queue */
static inline void __piom_init_queue(piom_ltask_queue_t*queue)
{
    queue->state = PIOM_LTASK_QUEUE_STATE_NONE;
    piom_ltask_lfqueue_init(&queue->ltask_queue);
    queue->processing = 0;
    queue->parent = NULL;
    queue->state = PIOM_LTASK_QUEUE_STATE_RUNNING;
}

/* Remove a task from a queue and return it
 * Return NULL if the queue is empty
 */
static inline struct piom_ltask*__piom_ltask_get_from_queue(piom_ltask_queue_t*queue)
{
    struct piom_ltask*task = piom_ltask_lfqueue_dequeue(&queue->ltask_queue);
    return task;
}


/* Try to schedule a task from a given queue
 * Returns the task that have been scheduled (or NULL if no task)
 * If it returns a task, this task have been freed.
 */
static inline void* __piom_ltask_queue_schedule(piom_ltask_queue_t *queue)
{
    int success = 0;
    if( (queue->processing) || 
	( (queue->state != PIOM_LTASK_QUEUE_STATE_RUNNING)
	  && (queue->state != PIOM_LTASK_QUEUE_STATE_STOPPING)) )
	{
	    return NULL;
	}
    if(__sync_fetch_and_add(&queue->processing, 1) != 0)
	{
	    __sync_fetch_and_sub(&queue->processing, 1);
	    return NULL;
	}
    assert(queue->processing >= 1);
    struct piom_ltask *task = __piom_ltask_get_from_queue(queue);
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
		    piom_ltask_state_set(task, PIOM_LTASK_STATE_SCHEDULED);
		    piom_tasklet_mask();
		    const int options = task->options;
		    (*task->func_ptr)(task->data_ptr);
		    if(options & PIOM_LTASK_OPTION_DESTROY)
			{
			    /* ltask was destroyed by handler */
			    piom_tasklet_unmask();
			    success = 1;
			}
		    else if((options & PIOM_LTASK_OPTION_REPEAT) && !(task->state & PIOM_LTASK_STATE_SUCCESS))
			{
			    piom_ltask_state_unset(task, PIOM_LTASK_STATE_SCHEDULED);
			    piom_tasklet_unmask();
			    /* If another thread is currently stopping the queue don't repost the task */
			    if(queue->state == PIOM_LTASK_QUEUE_STATE_RUNNING)
				{
				    __piom_ltask_submit_in_queue(task, queue);
				}
			    else
				{
				    fprintf(stderr, "PIOMan: WARNING- task %p is left uncompleted\n", task);
				}
			}
		    else
			{
			    task->state = PIOM_LTASK_STATE_TERMINATED;
			    piom_ltask_completed (task);
			    piom_tasklet_unmask();
			    success = 1;
			}
		} 
	    else if(task->state & PIOM_LTASK_STATE_SUCCESS) 
		{
		    success = 1;
		    piom_ltask_state_set(task, PIOM_LTASK_STATE_TERMINATED);
		}
	    else if(task->state == PIOM_LTASK_STATE_NONE)
		{
		    fprintf(stderr, "PIOMan: FATAL- wrong state for scheduled ltask.\n");
		    abort();
		}
	}
    /* no more task to run, set the queue as stopped */
    if( (task == NULL) && 
	(queue->state == PIOM_LTASK_QUEUE_STATE_STOPPING) )
	{
	    queue->state = PIOM_LTASK_QUEUE_STATE_STOPPED;
	}
    __sync_fetch_and_sub(&queue->processing, 1);
    return success ? task : NULL;
}


static inline void __piom_exit_queue(piom_ltask_queue_t *queue)
{
    queue->state = PIOM_LTASK_QUEUE_STATE_STOPPING;

    /* empty the list of tasks */
    while(queue->state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	{
	    __piom_ltask_queue_schedule (queue);
	}
}

#ifdef PIOMAN_PTHREAD
static void __piom_pthread_setname(const char*name)
{
    pthread_setname_np(pthread_self(), name);
}
static void*__piom_ltask_timer_worker(void*_dummy)
{
    const long timeslice_nsec = piom_parameters.timer_period * 1000;
    const int policy = SCHED_OTHER;
    const int prio = sched_get_priority_max(policy);
    int rc = pthread_setschedprio(pthread_self(), prio);
    if(rc != 0)
	{
	    fprintf(stderr, "# pioman: WARNING- timer thread could not get priority %d.\n", prio);
	}
    __piom_pthread_setname("_pioman_timer");
    piom_ltask_queue_t*global_queue = __piom_get_queue(piom_topo_full);
    while(global_queue->state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	{
	    struct timespec t = { .tv_sec = 0, .tv_nsec = timeslice_nsec };
	    nanosleep(&t, NULL);
	    piom_check_polling(PIOM_POLL_AT_TIMER_SIG);
	}
    return NULL;
}
static void*__piom_ltask_idle_worker(void*_dummy)
{
    piom_ltask_queue_t*queue = _dummy;
    __piom_pthread_setname("_pioman_idle");
    while(queue->state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	{
	    piom_check_polling(PIOM_POLL_AT_IDLE);
	    if(piom_parameters.idle_granularity > 0)
		usleep(piom_parameters.idle_granularity);
	    else
		sched_yield();
	}
    return NULL;
}
static void*__piom_ltask_lwp_worker(void*_dummy)
{
    __piom_pthread_setname("_pioman_lwp");
    for(;;)
	{
	    sem_wait(&__piom_ltask_lwps_ready);
	    struct piom_ltask*task = piom_ltask_lfqueue_dequeue(&__piom_ltask_lwps_queue);
	    const int options = task->options;
	    assert(task != NULL);
	    piom_ltask_state_set(task, PIOM_LTASK_STATE_BLOCKED);
	    (*task->blocking_func)(task->data_ptr);
	    if(!(options & PIOM_LTASK_OPTION_DESTROY))
		piom_ltask_state_unset(task, PIOM_LTASK_STATE_BLOCKED);
	    __sync_fetch_and_add(&__piom_ltask_lwps_avail, 1);
	}
    return NULL;
}

#endif /* PIOMAN_PTHREAD */

void piom_init_ltasks(void)
{
    if (!piom_ltask_initialized)
	{

#if defined(PIOMAN_TOPOLOGY_NONE)
	    __piom_init_queue(&global_queue);
	    global_queue.binding = piom_topo_full;
#elif defined(PIOMAN_TOPOLOGY_MARCEL)
 	    int i, j;
	    for(i = 0; i < marcel_topo_nblevels; i++)
		{
		    for(j = 0; j < marcel_topo_level_nbitems[i]; j++)
			{
			    struct marcel_topo_level *l = &marcel_topo_levels[i][j];
			    piom_ltask_queue_t*queue = TBX_MALLOC(sizeof (piom_ltask_queue_t));
			    __piom_init_queue(queue);
			    queue->binding = l;
			    queue->parent = (l->father != NULL) ? l->father->ltask_data : NULL;
			    l->ltask_data = queue;
			}
		}
#elif defined(PIOMAN_TOPOLOGY_HWLOC)
	    hwloc_topology_init(&__piom_ltask_topology);
	    hwloc_topology_load(__piom_ltask_topology);
	    const int depth = hwloc_topology_get_depth(__piom_ltask_topology);
	    int d, i;
	    for(d = 0; d < depth; d++)
		{
		    hwloc_obj_t o = hwloc_get_obj_by_depth(__piom_ltask_topology, d, 0);
		    if(o->type == HWLOC_OBJ_MACHINE ||
		       o->type == HWLOC_OBJ_NODE ||
		       o->type == HWLOC_OBJ_SOCKET ||
		       o->type == HWLOC_OBJ_CORE)
			{
			    const int nb = hwloc_get_nbobjs_by_depth(__piom_ltask_topology, d);
			    for (i = 0; i < nb; i++)
				{
				    o = hwloc_get_obj_by_depth(__piom_ltask_topology, d, i);
				    /* TODO- allocate memory on given obj */
				    piom_ltask_queue_t*queue = TBX_MALLOC(sizeof(piom_ltask_queue_t));
				    __piom_init_queue(queue);
				    queue->binding = o;
				    queue->parent = (o->parent == NULL) ? NULL : __piom_get_queue(o->parent);
				    o->userdata = queue;
				}
			}
		}
	    pthread_key_create(&__piom_ltask_local_key, NULL); /* TODO- detructor */
#endif	/* PIOMAN_TOPOLOGY_* */

#ifdef PIOMAN_MARCEL
	    /* register a callback to Marcel */
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_TIMER);
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_YIELD);
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_IDLE);
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_LIB_ENTRY);
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_CTX_SWITCH);
#endif /* PIOMAN_MARCEL */

#ifdef PIOMAN_PTHREAD 
	    /* ** timer-based polling */
	    if(piom_parameters.timer_period > 0)
		{
		    if(piom_parameters.timer_period > 0)
			{
			    pthread_t timer_thread;
			    pthread_create(&timer_thread, NULL, &__piom_ltask_timer_worker, NULL);
			}
		}
	    /* ** idle polling */
	    if(piom_parameters.idle_granularity >= 0)
		{
#if defined(PIOMAN_TOPOLOGY_NONE)
		    pthread_t idle_thread;
		    piom_ltask_queue_t*queue = __piom_get_queue(piom_topo_full);
		    pthread_create(&idle_thread, NULL, &__piom_ltask_idle_worker, queue);
		    pthread_setschedprio(idle_thread, sched_get_priority_min(SCHED_OTHER));
#elif defined(PIOMAN_TOPOLOGY_HWLOC)
		    const hwloc_obj_type_t level = PIOM_TOPO_SOCKET;
		    hwloc_obj_t o = NULL;
		    int i = 0;
		    do
			{
			    o = hwloc_get_obj_by_type(__piom_ltask_topology, level, i);
			    if(o == NULL)
				break;
#ifdef DEBUG
			    char string[128];
			    hwloc_obj_snprintf(string, sizeof(string), __piom_ltask_topology, o, "#", 0);
			    printf("# pioman: idle #%d on %s\n", i, string);
#endif /* DEBUG */
			    piom_ltask_queue_t*queue = __piom_get_queue(o);
			    pthread_t idle_thread;
			    pthread_create(&idle_thread, NULL, &__piom_ltask_idle_worker, queue);
			    pthread_setschedprio(idle_thread, sched_get_priority_min(SCHED_OTHER));
			    int rc = hwloc_set_thread_cpubind(__piom_ltask_topology, idle_thread, o->cpuset, HWLOC_CPUBIND_THREAD);
			    if(rc != 0)
				{
				    fprintf(stderr, "# pioman: WARNING- hwloc_set_thread_cpubind failed.\n");
				}
			    i++;
			}
		    while(o != NULL);
#endif /* PIOMAN_TOPOLOGY_* */
		}
	    /* ** spare LWPs for blocking calls */
	    if(piom_parameters.spare_lwp)
		{
		    __piom_ltask_lwps_num = piom_parameters.spare_lwp;
		    fprintf(stderr, "# pioman: starting %d spare LWPs.\n", piom_parameters.spare_lwp);
		    sem_init(&__piom_ltask_lwps_ready, 0, 0);
		    piom_ltask_lfqueue_init(&__piom_ltask_lwps_queue);
		    int i;
		    for(i = 0; i < __piom_ltask_lwps_num; i++)
			{
			    pthread_t tid;
			    int err = pthread_create(&tid, NULL, &__piom_ltask_lwp_worker, NULL);
			    if(err)
				{
				    fprintf(stderr, "PIOMan: FATAL- cannot create spare LWP #%d\n", i);
				    abort();
				}
			    __piom_ltask_lwps_avail++;
			}
		}
#endif /* PIOMAN_PTHREAD */

	}
    piom_ltask_initialized++;
}

void piom_exit_ltasks(void)
{
    piom_ltask_initialized--;
    if(piom_ltask_initialized == 0)
	{
#if defined(PIOMAN_TOPOLOGY_NONE)
	    __piom_exit_queue((piom_ltask_queue_t*)&global_queue);
#elif defined(PIOMAN_TOPOLOGY_MARCEL)
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
#elif defined(PIOMAN_TOPOLOGY_HWLOC)
	    const int depth = hwloc_topology_get_depth(__piom_ltask_topology);
	    int d, i;
	    for(d = 0; d < depth; d++)
		{
		    for (i = 0; i < hwloc_get_nbobjs_by_depth(__piom_ltask_topology, d); i++)
			{
			    hwloc_obj_t o = hwloc_get_obj_by_depth(__piom_ltask_topology, d, i);
			    piom_ltask_queue_t*queue = o->userdata;
			    __piom_exit_queue(queue);
			    TBX_FREE(queue);
			    o->userdata = NULL;
			}
		}
#endif /* PIOMAN_TOPOLOGY_* */
	}

}

int piom_ltask_test_activity(void)
{
    return (piom_ltask_initialized != 0);
}

static void __piom_ltask_submit_in_queue(struct piom_ltask *task, piom_ltask_queue_t *queue)
{
    if(queue->state == PIOM_LTASK_QUEUE_STATE_STOPPING)
	{
	    fprintf(stderr, "PIOMan: WARNING- submitting a task (%p) to a queue (%p) that is being stopped\n", 
		    task, queue);
	}
    if(task->options & PIOM_LTASK_OPTION_BLOCKING)
	{
	    struct timespec t;
	    clock_gettime(CLOCK_MONOTONIC, &t);
	    const long d_usec = (t.tv_sec - task->origin.tv_sec) * 1000000 + (t.tv_nsec - task->origin.tv_nsec) / 1000;
	    if(d_usec > task->blocking_delay)
		{
		    if(__piom_ltask_submit_in_lwp(task) == 0)
			return;
		}
	}
    task->state = PIOM_LTASK_STATE_READY;
    task->queue = queue;
    /* wait until a task is removed from the list */
    int rc = -1;
    while(rc != 0)
	{
	    rc = piom_ltask_lfqueue_enqueue(&queue->ltask_queue, task);
	}
}

static inline int __piom_ltask_submit_in_lwp(struct piom_ltask*task)
{
#ifdef PIOMAN_PTHREAD
    if(__sync_fetch_and_sub(&__piom_ltask_lwps_avail, 1) > 0)
	{
	    piom_ltask_lfqueue_enqueue(&__piom_ltask_lwps_queue, task);
	    sem_post(&__piom_ltask_lwps_ready);
	    return 0;
	}
    else
	{
	    /* rollback */
	    __sync_fetch_and_add(&__piom_ltask_lwps_avail, 1);
	}
#endif /* PIOMAN_PTHREAD */
    return -1;
}


void piom_ltask_submit(struct piom_ltask *task)
{
    piom_ltask_queue_t *queue;
    assert(task->state == PIOM_LTASK_STATE_NONE || task->state == PIOM_LTASK_STATE_SUCCESS);
    task->state = PIOM_LTASK_STATE_NONE;
    queue = __piom_get_queue(task->binding);
    assert(task != NULL);
    assert(queue != NULL);
    __piom_ltask_submit_in_queue(task, queue);
}

void*piom_ltask_schedule(void)
{
    struct piom_ltask*ltask = NULL;
    if(piom_ltask_initialized)
	{
	    piom_ltask_queue_t*queue = __piom_get_queue(piom_ltask_current_obj());
	    while(queue != NULL)
		{
		    ltask = __piom_ltask_queue_schedule(queue);
		    if(ltask)
			{
			    break;
			}
		    queue = queue->parent;
		}
	}
    return ltask;
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
    while (!(task->state & PIOM_LTASK_STATE_TERMINATED))
	{
	    piom_ltask_schedule();
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
    __sync_fetch_and_add(&task->masked, 1);
    while(task->state & PIOM_LTASK_STATE_SCHEDULED)
	{
	    piom_ltask_schedule();
	}
}

void piom_ltask_unmask(struct piom_ltask *task)
{
    assert(task->masked > 0);
    __sync_fetch_and_sub(&task->masked, 1);
    assert(task->masked >= 0);
}

void piom_ltask_cancel(struct piom_ltask*ltask)
{
    int found = 0;
    assert(ltask->state != PIOM_LTASK_STATE_NONE);
    const int options = ltask->options;
    ltask->masked = 1;
    /* loop to retry, since we iterate without stopping the queue
     * and without lock. Some tasklet may steal the ltask between
     * test and dequeue. *Theoretically* starvation is possible.
     */
    piom_ltask_state_set(ltask, PIOM_LTASK_STATE_CANCELLED);
    while(!found)
	{
	    while(ltask->state & PIOM_LTASK_STATE_BLOCKED)
		{ }
	    while(ltask->state & PIOM_LTASK_STATE_SCHEDULED)
		{ }
	    struct piom_ltask_queue*queue = ltask->queue;
	    if(queue)
		{
		    int i;
		    for(i = 0; i < PIOM_MAX_LTASK; i++)
			{
			    struct piom_ltask*ltask2 = __piom_ltask_get_from_queue(queue);
			    if(ltask2 == ltask)
				{
				    found = 1;
				    break;
				}
			    else if(ltask2 != NULL)
				__piom_ltask_submit_in_queue(ltask2, queue);
			}
		}
	    else
		found = 1;
	}
    ltask->state = PIOM_LTASK_STATE_TERMINATED;
    if(!(options & PIOM_LTASK_OPTION_DESTROY))
	{
	    piom_ltask_completed(ltask);
	}
}


#if defined(PIOMAN_TOPOLOGY_MARCEL)
piom_topo_obj_t piom_get_parent_obj(piom_topo_obj_t obj, enum piom_topo_level_e _level)
{
    const enum marcel_topo_level_e level = _level;
    marcel_topo_level_t *l = obj; /* &marcel_topo_levels[marcel_topo_nblevels - 1][marcel_current_vp()]; */
    while(l != NULL && l->type > level)
	{
	    l = l->father;
	}
    assert(l != NULL);
    return l;
}
#elif defined(PIOMAN_TOPOLOGY_HWLOC)
piom_topo_obj_t piom_ltask_current_obj(void)
{
    int update = 0;
    struct piom_ltask_local_s*local = pthread_getspecific(__piom_ltask_local_key);
    if(local == NULL)
	{
	    local = TBX_MALLOC(sizeof(struct piom_ltask_local_s));
	    pthread_setspecific(__piom_ltask_local_key, local);
	    local->tid = pthread_self();
	    local->cpuset = hwloc_bitmap_alloc();
	    local->count = 0;
	    update = 1;
	}
    else
	{
	    local->count++;
	    if(local->count % 100 == 0)
		{
		    tbx_tick_t now;
		    TBX_GET_TICK(now);
		    double delay = TBX_TIMING_DELAY(local->timestamp, now);
		    if(delay > 1000)
			update = 1;
		}
	}
    if(update)
	{
	    int rc = hwloc_get_last_cpu_location(__piom_ltask_topology, local->cpuset, HWLOC_CPUBIND_THREAD);
	    if(rc != 0)
		abort();
	    local->obj = hwloc_get_obj_covering_cpuset(__piom_ltask_topology, local->cpuset);
	    if(local->obj == NULL)
		abort();
	    TBX_GET_TICK(local->timestamp);
	}
    return local->obj;
}
piom_topo_obj_t piom_get_parent_obj(piom_topo_obj_t obj, enum piom_topo_level_e _level)
{
    const hwloc_obj_type_t level = _level;
    hwloc_obj_t parent = hwloc_get_ancestor_obj_by_type(topology, level, obj);
    return parent;
}
#else
piom_topo_obj_t piom_get_parent_obj(piom_topo_obj_t obj, enum piom_topo_level_e level)
{
    return piom_topo_full;
}
#endif

