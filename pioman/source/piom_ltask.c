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

#include <assert.h>
#include <signal.h>
#include <time.h>

#include "piom_private.h"
#include <tbx_topology.h>

/** Maximum number of ltasks in a queue. 
 * Should be large enough to avoid overflow, but small enough to fit the cache.
 */
#define PIOM_MAX_LTASK 256

/** type for lock-free queue of ltasks */
PIOM_LFQUEUE_TYPE(piom_ltask, struct piom_ltask*, NULL, PIOM_MAX_LTASK);

/** state for a queue of tasks */
typedef enum
    {
	/* not yet initialized */
	PIOM_LTASK_QUEUE_STATE_NONE = 0,
	/* running */
	PIOM_LTASK_QUEUE_STATE_RUNNING,
	/* being stopped */
	PIOM_LTASK_QUEUE_STATE_STOPPING,
	/* stopped: no more request in the queue, 
	   no more task from this queue being executed */
	PIOM_LTASK_QUEUE_STATE_STOPPED
    } piom_ltask_queue_state_t;

/** an ltask queue- one queue instanciated per hwloc object */
typedef struct piom_ltask_queue
{
    /** the queue of tasks ready to be scheduled */
    struct piom_ltask_lfqueue_s       ltask_queue;
    /** separate queue for task submission, to reduce contention */
    struct piom_ltask_lfqueue_s       submit_queue;
    /** flag whether scheduling is disabled for the queue */
    volatile int                      masked;
    /** state to control queue lifecycle */
    volatile piom_ltask_queue_state_t state;
    /** where this queue is located */
    piom_topo_obj_t                   binding;
    /* context for trace subsystem */
    struct piom_trace_info_s          trace_info;
} piom_ltask_queue_t;

static void __piom_ltask_submit_in_queue(struct piom_ltask *task, piom_ltask_queue_t *queue);
static int __piom_ltask_submit_in_lwp(struct piom_ltask*task);

/** block of static state for pioman ltask */
static struct
{
    /** refcounter on piom_ltask */
    int initialized;
#if defined(PIOMAN_TOPOLOGY_NONE)
    /** global queue in case topology is unknown */
    struct piom_ltask_queue global_queue;
#elif defined(PIOMAN_TOPOLOGY_HWLOC)
    /** node topology */
    hwloc_topology_t topology;
    /** thread-local location information  */
    pthread_key_t local_key;
#endif /* PIOMAN_TOPOLOGY_* */
#ifdef PIOMAN_PTHREAD
    /** number of spare LWPs running */
    int lwps_num;
    /** number of available LWPs */
    int lwps_avail;
    /** signal new tasks to LWPs */
    sem_t lwps_ready;
    /** ltasks queue to feed LWPs */
    struct piom_ltask_lfqueue_s lwps_queue;
#endif /* PIOMAN_PTHREAD */
    struct piom_ltask_queue**all_queues;
    int n_queues;
} __piom_ltask =
    {
	.initialized = 0,
	.n_queues = 0
    };

#if defined(PIOMAN_TOPOLOGY_HWLOC)
/** locality information */
struct piom_ltask_local_s
{
    pthread_t tid;              /**< thread this info is about */
    hwloc_cpuset_t cpuset;      /**< last cpuset location for thread  */
    hwloc_obj_t obj;            /**< last obj encompassing cpuset */
    tbx_tick_t timestamp;
    int count;
};
#endif /* PIOMAN_TOPOLOGY_HWLOC */

#ifdef PIOMAN_PTHREAD
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
    if(obj == NULL)
	{
	    obj = piom_ltask_current_obj();
	}
#if defined(PIOMAN_TOPOLOGY_HWLOC)
    while(obj != NULL && obj->userdata == NULL)
	{
	    obj = obj->parent;
	}
    return (obj != NULL) ? obj->userdata : NULL;
#elif defined(PIOMAN_TOPOLOGY_MARCEL)
    return obj->ltask_data;
#elif defined(PIOMAN_TOPOLOGY_NONE)
    return &__piom_ltask.global_queue;
#else /* PIOMAN_TOPOLOGY_* */
#error
    return NULL;
#endif	/* PIOMAN_TOPOLOGY_* */
}

/* Initialize a queue */
static void piom_ltask_queue_init(piom_ltask_queue_t*queue, piom_topo_obj_t binding)
{
    queue->state = PIOM_LTASK_QUEUE_STATE_NONE;
    piom_ltask_lfqueue_init(&queue->ltask_queue);
    piom_ltask_lfqueue_init(&queue->submit_queue);
    queue->masked = 0;
    queue->binding = binding;
    __piom_ltask.all_queues = realloc(__piom_ltask.all_queues, sizeof(struct piom_ltask_queue*)*(__piom_ltask.n_queues + 1));
    __piom_ltask.all_queues[__piom_ltask.n_queues] = queue;
#if defined(PIOMAN_TOPOLOGY_HWLOC)
    char s_binding[128];
    hwloc_obj_snprintf(s_binding, sizeof(s_binding), __piom_ltask.topology, queue->binding, "#", 0);
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
#ifdef PIOMAN_MULTITHREAD
    if(__sync_fetch_and_add(&queue->masked, 1) != 0)
	{
	    __sync_fetch_and_sub(&queue->masked, 1);
	    return;
	}
#else /* PIOMAN_MULTITHREAD */
    if(queue->masked > 0)
	return;
    queue->masked++;
#endif /* PIOMAN_MULTITHREAD */
    const int hint1 = (PIOM_MAX_LTASK + queue->ltask_queue._head - queue->ltask_queue._tail) % PIOM_MAX_LTASK;
    const int hint2 = (PIOM_MAX_LTASK + queue->submit_queue._head - queue->submit_queue._tail) % PIOM_MAX_LTASK;
    const int hint = ((hint1 > 0)?hint1:1) + ((hint2 > 0)?hint2:1);
    const int count = (full != 0) ? hint : 1;
    int i;
    for(i = 0; i < count; i++)
	{
	    int success = 0, again = 0;
	    struct piom_ltask*task = piom_ltask_lfqueue_dequeue_single_reader(&queue->submit_queue);
	    if(task == NULL)
		task = piom_ltask_lfqueue_dequeue_single_reader(&queue->ltask_queue);
	    piom_trace_queue_var(&queue->trace_info, PIOM_TRACE_VAR_LTASKS, (PIOM_MAX_LTASK + queue->ltask_queue._head - queue->ltask_queue._tail) % PIOM_MAX_LTASK);
	    if(task)
		{
		    piom_trace_queue_state(&queue->trace_info, PIOM_TRACE_STATE_INIT);
		    const int prestate = piom_ltask_state_set(task, PIOM_LTASK_STATE_SCHEDULED);
		    if(prestate & PIOM_LTASK_STATE_READY)
			{
			    /* wait is pending: poll */
			    piom_trace_queue_state(&queue->trace_info, PIOM_TRACE_STATE_POLL);
			    piom_tasklet_mask();
			    const int options = task->options;
			    (*task->func_ptr)(task->data_ptr);
			    if(options & PIOM_LTASK_OPTION_DESTROY)
				{
				    /* ltask was destroyed by handler */
				    task = NULL;
				    piom_tasklet_unmask();
				    success = 1;
				}
			    else if((options & PIOM_LTASK_OPTION_REPEAT) && !(task->state & PIOM_LTASK_STATE_SUCCESS))
				{
				    piom_tasklet_unmask();
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
						    const long d_usec = (t.tv_sec - task->origin.tv_sec) * 1000000 + (t.tv_nsec - task->origin.tv_nsec) / 1000;
						    if(d_usec > task->blocking_delay)
							{
							    if(__piom_ltask_submit_in_lwp(task) == 0)
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
				    piom_tasklet_unmask();
				    success = 1;
				}
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
			    PIOM_FATAL("wrong state 0x%x for scheduled ltask.\n", prestate);
			}
		    piom_trace_queue_state(&queue->trace_info, PIOM_TRACE_STATE_NONE);
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
	    /* no more task to run, set the queue as stopped */
	    if( (task == NULL) && 
		(queue->state == PIOM_LTASK_QUEUE_STATE_STOPPING) )
		{
		    queue->state = PIOM_LTASK_QUEUE_STATE_STOPPED;
		    return; /* purposely return while holding the lock */
		}
	    if(success)
		{
		    piom_trace_queue_event(&queue->trace_info, PIOM_TRACE_EVENT_SUCCESS, task);
		}
	}
#ifdef PIOMAN_MULTITHREAD
    __sync_fetch_and_sub(&queue->masked, 1);
#else
    queue->masked--;
#endif /* PIOMAN_MULTITHREAD */
}


static inline void __piom_exit_queue(piom_ltask_queue_t *queue)
{
    queue->state = PIOM_LTASK_QUEUE_STATE_STOPPING;

    /* empty the list of tasks */
    while(queue->state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	{
	    piom_ltask_queue_schedule(queue, 0);
	}
}

#ifdef PIOMAN_PTHREAD
static void __piom_pthread_setname(const char*name)
{
#if defined(__GLIBC__) && defined(__GLIBC_MINOR__) && ((__GLIBC__ == 2 && __GLIBC_MINOR__ >= 12) || (__GLIBC__ > 2))
    pthread_setname_np(pthread_self(), name);
#else
#warning pthread_setname_np not available
#endif
}
static void*__piom_ltask_timer_worker(void*_dummy)
{
    const long timeslice_nsec = piom_parameters.timer_period * 1000;
    int policy = SCHED_OTHER;
    const int max_prio = sched_get_priority_max(policy);
    struct sched_param old_param;
    pthread_getschedparam(pthread_self(), &policy, &old_param);
    const int old_prio = old_param.sched_priority;
    __piom_pthread_setname("_pioman_timer");
    int rc = pthread_setschedprio(pthread_self(), max_prio);
    const int prio_enable = ((rc == 0) && (max_prio != old_prio));
    if(rc != 0)
	{
	    PIOM_WARN("timer thread could not get priority %d.\n", max_prio);
	}
    piom_ltask_queue_t*global_queue = __piom_get_queue(piom_ltask_current_obj());
    while(global_queue->state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	{
	    struct timespec t = { .tv_sec = 0, .tv_nsec = timeslice_nsec };
	    clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL);
	    piom_trace_queue_event(NULL, PIOM_TRACE_EVENT_TIMER_POLL, NULL);
	    if(prio_enable)
		pthread_setschedprio(pthread_self(), old_prio);
	    piom_ltask_schedule(PIOM_POLL_POINT_TIMER);
	    sched_yield();
	    if(prio_enable)
		pthread_setschedprio(pthread_self(), max_prio);
	}
    return NULL;
}

static void*__piom_ltask_idle_worker(void*_dummy)
{
    piom_ltask_queue_t*queue = _dummy;
    int policy = SCHED_OTHER;
    const int min_prio = sched_get_priority_min(policy);
    struct sched_param old_param;
    pthread_getschedparam(pthread_self(), &policy, &old_param);
    const int old_prio = old_param.sched_priority;
    __piom_pthread_setname("_pioman_idle");
    int rc = pthread_setschedprio(pthread_self(), min_prio);
    const int prio_enable = ((rc == 0) && (min_prio != old_prio));
    if(rc != 0)
	{
	    PIOM_WARN("idle thread could not get priority %d.\n", min_prio);
	}
    while(queue->state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	{
	    tbx_tick_t s1, s2;
	    TBX_GET_TICK(s1);
	    piom_trace_queue_event(NULL, PIOM_TRACE_EVENT_IDLE_POLL, NULL);
	    if(prio_enable)
		pthread_setschedprio(pthread_self(), old_prio);
	    piom_ltask_schedule(PIOM_POLL_POINT_IDLE);
	    if(prio_enable)
		pthread_setschedprio(pthread_self(), min_prio);
	    if(piom_parameters.idle_granularity > 0 && piom_parameters.idle_granularity < 10)
		{
		    double d = 0.0;
		    do
			{
			    sched_yield();
			    TBX_GET_TICK(s2);
			    d = TBX_TIMING_DELAY(s1, s2);
			}
		    while(d < piom_parameters.idle_granularity);
		}
	    else if(piom_parameters.idle_granularity > 0)
		{
		    usleep(piom_parameters.idle_granularity);
		}
	    else
		{
		    sched_yield();
		}
	}
    return NULL;
}
static void*__piom_ltask_lwp_worker(void*_dummy)
{
    __piom_pthread_setname("_pioman_lwp");
    for(;;)
	{
	    sem_wait(&__piom_ltask.lwps_ready);
	    struct piom_ltask*task = piom_ltask_lfqueue_dequeue(&__piom_ltask.lwps_queue);
	    const int options = task->options;
	    assert(task != NULL);
	    piom_ltask_state_set(task, PIOM_LTASK_STATE_BLOCKED);
	    (*task->blocking_func)(task->data_ptr);
	    if(!(options & PIOM_LTASK_OPTION_DESTROY))
		piom_ltask_state_unset(task, PIOM_LTASK_STATE_BLOCKED);
	    __sync_fetch_and_add(&__piom_ltask.lwps_avail, 1);
	}
    return NULL;
}

#endif /* PIOMAN_PTHREAD */

void piom_init_ltasks(void)
{
    if(__piom_ltask.initialized)
	return;
    __piom_ltask.initialized++;

    /* ** Create queues */
    
#if defined(PIOMAN_TOPOLOGY_NONE)
    piom_ltask_queue_init(&__piom_ltask.global_queue, piom_topo_full);
#elif defined(PIOMAN_TOPOLOGY_MARCEL)
    int i, j;
    for(i = 0; i < marcel_topo_nblevels; i++)
	{
	    for(j = 0; j < marcel_topo_level_nbitems[i]; j++)
		{
		    struct marcel_topo_level *l = &marcel_topo_levels[i][j];
		    piom_ltask_queue_t*queue = TBX_MALLOC(sizeof (piom_ltask_queue_t));
		    piom_ltask_queue_init(queue, l);
		    queue->parent = (l->father != NULL) ? l->father->ltask_data : NULL;
		    queue->skip_factor = 1;
		    l->ltask_data = queue;
		}
	}
#elif defined(PIOMAN_TOPOLOGY_HWLOC)
    hwloc_topology_init(&__piom_ltask.topology);
    hwloc_topology_load(__piom_ltask.topology);
    const int depth = hwloc_topology_get_depth(__piom_ltask.topology);
    const hwloc_obj_type_t binding_level = piom_parameters.binding_level;
    int d;
    for(d = 0; d < depth; d++)
	{
	    hwloc_obj_t o = hwloc_get_obj_by_depth(__piom_ltask.topology, d, 0);
	    if(o->type == binding_level)
		{
		    const int nb = hwloc_get_nbobjs_by_depth(__piom_ltask.topology, d);
		    int i;
		    for (i = 0; i < nb; i++)
			{
			    o = hwloc_get_obj_by_depth(__piom_ltask.topology, d, i);
			    /* TODO- allocate memory on given obj */
			    piom_ltask_queue_t*queue = TBX_MALLOC(sizeof(piom_ltask_queue_t));
			    piom_ltask_queue_init(queue, o);
			    o->userdata = queue;
#ifdef PIOMAN_TRACE
			    char cont_name[32];
			    char cont_type[64];
			    const char*level_label = NULL;
			    switch(o->type)
				{
				case HWLOC_OBJ_MACHINE:
				    level_label = "Machine";
				    break;
				case HWLOC_OBJ_NODE:
				    level_label = "Node";
				    break;
				case HWLOC_OBJ_SOCKET:
				    level_label = "Socket";
				    break;
				case HWLOC_OBJ_CORE:
				    level_label = "Core";
				    break;
				default:
				    break;
				}
			    sprintf(cont_name, "%s_%d", level_label, o->logical_index);
			    sprintf(cont_type, "Container_%s", level_label);
			    queue->trace_info.cont_name = strdup(cont_name);
			    queue->trace_info.cont_type = strdup(cont_type);
			    queue->trace_info.level = o->type;
			    queue->trace_info.rank = o->logical_index;
			    queue->trace_info.parent = queue->parent ? &queue->parent->trace_info : NULL;
			    piom_trace_queue_new(&queue->trace_info);
			    piom_trace_queue_state(&queue->trace_info, PIOM_TRACE_STATE_NONE);
#endif /* PIOMAN_TRACE */
			}
		}
	}
    pthread_key_create(&__piom_ltask.local_key, NULL); /* TODO- detructor */
#endif	/* PIOMAN_TOPOLOGY_* */

    /* ** Start polling */
    
#ifdef PIOMAN_PTHREAD 
    /* ** timer-based polling */
    if(piom_parameters.timer_period > 0)
	{
	    pthread_t timer_thread;
	    pthread_create(&timer_thread, NULL, &__piom_ltask_timer_worker, NULL);
	}
    /* ** idle polling */
    if(piom_parameters.idle_granularity >= 0)
	{
#if defined(PIOMAN_TOPOLOGY_NONE)
	    pthread_t idle_thread;
	    piom_ltask_queue_t*queue = __piom_get_queue(piom_topo_full);
	    pthread_create(&idle_thread, NULL, &__piom_ltask_idle_worker, queue);
	    pthread_setschedprio(idle_thread, sched_get_priority_min(SCHED_OTHER));
	    PIOM_WARN("no hwloc, using global queue. Running in degraded mode.\n");
#elif defined(PIOMAN_TOPOLOGY_HWLOC)
	    const hwloc_obj_type_t level = piom_parameters.binding_level;
	    hwloc_obj_t o = NULL;
	    int i = 0;
	    do
		{
		    o = hwloc_get_obj_by_type(__piom_ltask.topology, level, i);
		    if(o == NULL)
			break;
		    if( ((piom_parameters.idle_distrib == PIOM_BIND_DISTRIB_FIRST) && (o->logical_index == 1)) ||
			((piom_parameters.idle_distrib == PIOM_BIND_DISTRIB_ODD)   && (o->logical_index % 2 == 1)) ||
			((piom_parameters.idle_distrib == PIOM_BIND_DISTRIB_EVEN)  && (o->logical_index % 2 == 0)) ||
			( piom_parameters.idle_distrib == PIOM_BIND_DISTRIB_ALL)
			)
			{
			    char string[128];
			    hwloc_obj_snprintf(string, sizeof(string), __piom_ltask.topology, o, "#", 0);
			    PIOM_DISP("idle #%d on %s\n", i, string);
			    piom_ltask_queue_t*queue = __piom_get_queue(o);
			    pthread_t idle_thread;
			    pthread_attr_t attr;
			    pthread_attr_init(&attr);
#ifdef SCHED_IDLE
			    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
			    pthread_attr_setschedpolicy(&attr, SCHED_IDLE);
#endif /* SCHED_IDLE */
			    pthread_create(&idle_thread, &attr, &__piom_ltask_idle_worker, queue);
			    int rc = hwloc_set_thread_cpubind(__piom_ltask.topology, idle_thread, o->cpuset, HWLOC_CPUBIND_THREAD);
			    if(rc != 0)
				{
				    PIOM_WARN("hwloc_set_thread_cpubind failed.\n");
				}
			}
		    i++;
		}
	    while(o != NULL);
#endif /* PIOMAN_TOPOLOGY_* */
	}
    /* ** spare LWPs for blocking calls */
    __piom_ltask.lwps_avail = 0;
    __piom_ltask.lwps_num = piom_parameters.spare_lwp;
    if(piom_parameters.spare_lwp)
	{
	    PIOM_DISP("starting %d spare LWPs.\n", piom_parameters.spare_lwp);
	    sem_init(&__piom_ltask.lwps_ready, 0, 0);
	    piom_ltask_lfqueue_init(&__piom_ltask.lwps_queue);
	    int i;
	    for(i = 0; i < __piom_ltask.lwps_num; i++)
		{
		    pthread_t tid;
		    int err = pthread_create(&tid, NULL, &__piom_ltask_lwp_worker, NULL);
		    if(err)
			{
			    PIOM_FATAL("cannot create spare LWP #%d\n", i);
			}
		    __piom_ltask.lwps_avail++;
		}
	}
#endif /* PIOMAN_PTHREAD */
}

void piom_exit_ltasks(void)
{
    __piom_ltask.initialized--;
    if(__piom_ltask.initialized == 0)
	{
#ifdef PIOMAN_TRACE
	    piom_trace_flush();
#endif /* PIOMAN_TRACE */
#if defined(PIOMAN_TOPOLOGY_NONE)
	    __piom_exit_queue((piom_ltask_queue_t*)&__piom_ltask.global_queue);
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
	    const int depth = hwloc_topology_get_depth(__piom_ltask.topology);
	    int d, i;
	    for(d = 0; d < depth; d++)
		{
		    for (i = 0; i < hwloc_get_nbobjs_by_depth(__piom_ltask.topology, d); i++)
			{
			    hwloc_obj_t o = hwloc_get_obj_by_depth(__piom_ltask.topology, d, i);
			    piom_ltask_queue_t*queue = o->userdata;
			    piom_trace_queue_state(&queue->trace_info, PIOM_TRACE_STATE_NONE);
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
    return (__piom_ltask.initialized != 0);
}

static void __piom_ltask_submit_in_queue(struct piom_ltask *task, piom_ltask_queue_t *queue)
{
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
	}
    while(rc != 0);
    piom_trace_queue_var(&queue->trace_info, PIOM_TRACE_VAR_LTASKS, (PIOM_MAX_LTASK + queue->ltask_queue._head - queue->ltask_queue._tail) % PIOM_MAX_LTASK);
}

static inline int __piom_ltask_submit_in_lwp(struct piom_ltask*task)
{
#ifdef PIOMAN_PTHREAD
    if(__sync_fetch_and_sub(&__piom_ltask.lwps_avail, 1) > 0)
	{
	    piom_ltask_lfqueue_enqueue(&__piom_ltask.lwps_queue, task);
	    sem_post(&__piom_ltask.lwps_ready);
	    return 0;
	}
    else
	{
	    /* rollback */
	    __sync_fetch_and_add(&__piom_ltask.lwps_avail, 1);
	}
#endif /* PIOMAN_PTHREAD */
    return -1;
}


void piom_ltask_submit(struct piom_ltask *task)
{
    assert(task != NULL);
    assert(task->state == PIOM_LTASK_STATE_NONE || (task->state & PIOM_LTASK_STATE_TERMINATED) );
    task->state = PIOM_LTASK_STATE_NONE;
    piom_ltask_queue_t*queue = __piom_get_queue(task->binding);
    assert(queue != NULL);
    piom_trace_queue_event(&queue->trace_info, PIOM_TRACE_EVENT_SUBMIT, task);
    __piom_ltask_submit_in_queue(task, queue);
}

void piom_ltask_schedule(int point)
{
    if(__piom_ltask.initialized)
	{
	    if(point == PIOM_POLL_POINT_BUSY)
		{
		    /* busy wait- poll all queues, higher frequency for local queue */
		    piom_ltask_queue_t*local_queue = __piom_get_queue(piom_ltask_current_obj());
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
				    /*
				    queue->skip++;
				    if(queue->skip % queue->skip_factor == 0)
				    */
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
		    piom_ltask_queue_t*queue = __piom_get_queue(piom_ltask_current_obj());
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
    piom_ltask_state_unset(ltask, PIOM_LTASK_STATE_READY);
    piom_ltask_state_set(ltask, PIOM_LTASK_STATE_CANCELLED);
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
hwloc_topology_t piom_ltask_topology(void)
{
    assert(__piom_ltask.topology != NULL);
    return __piom_ltask.topology;
}
piom_topo_obj_t piom_ltask_current_obj(void)
{
    int update = 0;
    struct piom_ltask_local_s*local = pthread_getspecific(__piom_ltask.local_key);
    if(local == NULL)
	{
	    local = TBX_MALLOC(sizeof(struct piom_ltask_local_s));
	    pthread_setspecific(__piom_ltask.local_key, local);
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
		    if(delay > 10000)
			update = 1;
		}
	}
    if(update)
	{
	    int rc = hwloc_get_last_cpu_location(__piom_ltask.topology, local->cpuset, HWLOC_CPUBIND_THREAD);
	    if(rc != 0)
		abort();
	    local->obj = hwloc_get_obj_covering_cpuset(__piom_ltask.topology, local->cpuset);
	    if(local->obj == NULL)
		abort();
	    TBX_GET_TICK(local->timestamp);
	}
    return local->obj;
}
#endif

