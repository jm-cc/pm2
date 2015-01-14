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

/** Maximum number of ltasks in a queue. 
 * Should be large enough to avoid overflow, but small enough to fit the cache.
 */
#define PIOM_MAX_LTASK 256

PIOM_LFQUEUE_TYPE(piom_ltask, struct piom_ltask*, NULL, PIOM_MAX_LTASK);

enum piom_trace_event_e {
    PIOM_TRACE_EVENT_TIMER_POLL, PIOM_TRACE_EVENT_IDLE_POLL,
    PIOM_TRACE_EVENT_SUBMIT, PIOM_TRACE_EVENT_SUCCESS,
    PIOM_TRACE_STATE_INIT, PIOM_TRACE_STATE_POLL, PIOM_TRACE_STATE_NONE,
    PIOM_TRACE_VAR_LTASKS
} event;
#ifdef PIOMAN_TRACE
#define PIOMAN_TRACE_MAX (1024*1024*8)
struct piom_trace_queue_s
{
    enum piom_topo_level_e level;
    int rank;
};
struct piom_trace_entry_s
{
    tbx_tick_t tick;
    enum piom_trace_event_e event;
    struct piom_trace_queue_s queue;
    void*value;
};
static struct
{
    tbx_tick_t orig;
    int last;
    struct piom_trace_entry_s entries[PIOMAN_TRACE_MAX];
} __piom_trace = { .last = 0 };
#endif /* PIOMAN_TRACE */

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
    struct piom_ltask_lfqueue_s       submit_queue;
    volatile int                      processing;
    volatile piom_ltask_queue_state_t state;
    piom_topo_obj_t                   binding;
    struct piom_ltask_queue          *parent;
#ifdef PIOMAN_TRACE
    const char                       *cont_type;
    const char                       *cont_name;
    struct piom_trace_queue_s         trace_info;
#endif /* PIOMAN_TRACE */
    int skip;
} piom_ltask_queue_t;

static void __piom_ltask_submit_in_queue(struct piom_ltask *task, piom_ltask_queue_t *queue);
static int __piom_ltask_submit_in_lwp(struct piom_ltask*task);

#ifdef PIOMAN_TRACE
static void pioman_trace_init(void) __attribute__((constructor,used));
static void pioman_trace_exit(void) __attribute__((destructor,used));
static void pioman_trace_init(void)
{
    char trace_name[256]; 
    char hostname[256];
    gethostname(hostname, 256);
    sprintf(trace_name, "piom_%s", hostname);
    fprintf(stderr, "# pioman trace init: %s\n", trace_name);
    setTraceType(PAJE);
    initTrace(trace_name, 0, GTG_FLAG_NONE);

    TBX_GET_TICK(__piom_trace.orig);

    /* container types */
    addContType("Container_Machine", "0",                "Machine");
    addContType("Container_Node",   "Container_Machine", "Node");
    addContType("Container_Socket", "Container_Node",    "Socket");
    addContType("Container_Core",   "Container_Socket",  "Core");
    
    addStateType ("State_Machine", "Container_Machine", "Machine state");
    addStateType ("State_Node",    "Container_Node",    "Node state");
    addStateType ("State_Socket",  "Container_Socket",  "Socket state");
    addStateType ("State_Core",    "Container_Core",    "Core state");

    addEntityValue ("State_Core_none", "State_Core", "State_Core_none", GTG_BLACK);
    addEntityValue ("State_Core_init", "State_Core", "State_Core_init", GTG_BLUE);
    addEntityValue ("State_Core_poll", "State_Core", "State_Core_poll", GTG_PINK);

    addEntityValue ("State_Socket_none", "State_Socket", "State_Socket_none", GTG_BLACK);
    addEntityValue ("State_Socket_init", "State_Socket", "State_Socket_init", GTG_BLUE);
    addEntityValue ("State_Socket_poll", "State_Socket", "State_Socket_poll", GTG_PINK);

    addEntityValue ("State_Node_none", "State_Node", "State_Node_none", GTG_BLACK);
    addEntityValue ("State_Node_init", "State_Node", "State_Node_init", GTG_BLUE);
    addEntityValue ("State_Node_poll", "State_Node", "State_Node_poll", GTG_PINK);

    addEntityValue ("State_Machine_none", "State_Machine", "State_Machine_none", GTG_BLACK);
    addEntityValue ("State_Machine_init", "State_Machine", "State_Machine_init", GTG_BLUE);
    addEntityValue ("State_Machine_poll", "State_Machine", "State_Machine_poll", GTG_PINK);
 
    addEventType ("Event_Machine_submit", "Container_Machine", "Event: Machine ltask submit");
    addEventType ("Event_Node_submit",    "Container_Node",    "Event: Node ltask submit");
    addEventType ("Event_Socket_submit",  "Container_Socket",  "Event: Socket ltask submit");
    addEventType ("Event_Core_submit",    "Container_Core",    "Event: Core ltask submit");

    addEventType ("Event_Machine_success", "Container_Machine", "Event: Machine ltask success");
    addEventType ("Event_Node_success",    "Container_Node",    "Event: Node ltask success");
    addEventType ("Event_Socket_success",  "Container_Socket",  "Event: Socket ltask success");
    addEventType ("Event_Core_success",    "Container_Core",    "Event: Core ltask success");

    addVarType("Tasks_Machine", "ltasks in Machine queue", "Container_Machine");
    addVarType("Tasks_Node",    "ltasks in Node queue",    "Container_Node");
    addVarType("Tasks_Socket",  "ltasks in Socket queue",  "Container_Socket");
    addVarType("Tasks_Core",    "ltasks in Core queue",    "Container_Core");

    addContType("Container_Global", "0", "Global");
    addContainer(0.00000, "Timer_Events", "Container_Global", "0", "Timer_Events", "0");
    addContainer(0.00000, "Idle_Events", "Container_Global", "0", "Idle_Events", "0");
    addEventType ("Event_Timer_Poll", "Container_Global", "Event: timer poll");
    addEventType ("Event_Idle_Poll", "Container_Global", "Event: idle poll");

}
static inline struct piom_trace_entry_s*piom_trace_get_entry(void)
{
    const int i = __sync_fetch_and_add(&__piom_trace.last, 1);
    if(i >= PIOMAN_TRACE_MAX)
	return NULL;
    return &__piom_trace.entries[i];
}
static inline void piom_trace_queue_event(piom_ltask_queue_t*queue, enum piom_trace_event_e _event, void*_value)
{
    struct piom_trace_entry_s*entry = piom_trace_get_entry();
    if(entry)
	{
	    TBX_GET_TICK(entry->tick);
	    entry->event = _event;
	    entry->queue = queue ? queue->trace_info : (struct piom_trace_queue_s){ 0, 0};
	    entry->value = _value;
	}
}
static inline void piom_trace_queue_state(piom_ltask_queue_t*queue, enum piom_trace_event_e _event)
{
    piom_trace_queue_event(queue, _event, NULL);
}
static inline void piom_trace_queue_var(piom_ltask_queue_t*queue, enum piom_trace_event_e _event, int _value)
{
    void*value = (void*)((uintptr_t)_value);
    piom_trace_queue_event(queue, _event, value);
}
static void piom_trace_flush(void)
{
    static int flushed = 0;
    if(flushed)
	return;
    fprintf(stderr, "# pioman: flush traces (%d entries)\n", __piom_trace.last);
    flushed = 1;
    int i;
    for(i = 0; i < ((__piom_trace.last > PIOMAN_TRACE_MAX) ? PIOMAN_TRACE_MAX : __piom_trace.last) ; i++)
	{
	    const struct piom_trace_entry_s*e = &__piom_trace.entries[i];
	    const double d = TBX_TIMING_DELAY(__piom_trace.orig, e->tick) / 1000000.0;
	    const char*level_label = NULL;
	    switch(e->queue.level)
		{
		case PIOM_TOPO_MACHINE:
		    level_label = "Machine";
		    break;
		case PIOM_TOPO_NODE:
		    level_label = "Node";
		    break;
		case PIOM_TOPO_SOCKET:
		    level_label = "Socket";
		    break;
		case PIOM_TOPO_CORE:
		    level_label = "Core";
		    break;
		default:
		    level_label = "(unkown)";
		    break;
		}
	    char cont_name[64];
	    char state_type[128];
	    char event_type[128];
	    char var_type[128];
	    char value[256];
	    sprintf(cont_name, "%s_%d", level_label, e->queue.rank);
	    sprintf(state_type, "State_%s", level_label);
	    switch(e->event)
		{
		case PIOM_TRACE_EVENT_TIMER_POLL:
		    addEvent(d, "Event_Timer_Poll", "Timer_Events", NULL);
		    break;
		case PIOM_TRACE_EVENT_IDLE_POLL:
		    addEvent(d, "Event_Idle_Poll", "Idle_Events", NULL);
		    break;
		case PIOM_TRACE_EVENT_SUBMIT:
		    sprintf(event_type, "Event_%s_submit", level_label);
		    sprintf(value, "ltask = %p", e->value);
		    addEvent(d, event_type, cont_name, value);
		    break;
		case PIOM_TRACE_EVENT_SUCCESS:
		    sprintf(event_type, "Event_%s_success", level_label);
		    sprintf(value, "ltask = %p", e->value);
		    addEvent(d, event_type, cont_name, value);
		    break;
		case PIOM_TRACE_STATE_INIT:
		    sprintf(value, "%s_init", state_type);
		    setState(d, state_type, cont_name, value);
		    break;
		case PIOM_TRACE_STATE_POLL:
		    sprintf(value, "%s_poll", state_type);
		    setState(d, state_type, cont_name, value);
		    break;
		case PIOM_TRACE_STATE_NONE:
		    sprintf(value, "%s_none", state_type);
		    setState(d, state_type, cont_name, value);
		    break;
		case PIOM_TRACE_VAR_LTASKS:
		    {
			int _var = (uintptr_t)e->value;
			sprintf(var_type, "Tasks_%s", level_label);
			setVar(d, var_type, cont_name, _var);
		    }
		    break;
		default:
		    break;
		}
	}
    endTrace();
    fprintf(stderr, "# pioman: traces flushed.\n");
}
static void pioman_trace_exit(void)
{
    piom_trace_flush();
}
#else /* PIOMAN_TRACE */
static inline void piom_trace_queue_event(piom_ltask_queue_t*q, enum piom_trace_event_e _event, void*_value)
{ /* empty */ }
static inline void piom_trace_queue_state(piom_ltask_queue_t*queue, enum piom_trace_event_e _event)
{ /* empty */ }
static inline void piom_trace_queue_var(piom_ltask_queue_t*queue, enum piom_trace_event_e _event, int _value)
{ /* empty */ }
#endif /* PIOMAN_TRACE */


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
#if defined(PIOMAN_TOPOLOGY_HWLOC)
    assert(obj != NULL);
    while(obj != NULL && obj->userdata == NULL)
	{
	    obj = obj->parent;
	}
    return (obj != NULL) ? obj->userdata : NULL;
#elif defined(PIOMAN_TOPOLOGY_MARCEL)
    assert(obj != NULL);
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
    piom_ltask_lfqueue_init(&queue->submit_queue);
    queue->processing = 0;
    queue->parent = NULL;
    queue->state = PIOM_LTASK_QUEUE_STATE_RUNNING;
}


/* Try to schedule a task from a given queue
 * Returns the task that have been scheduled (or NULL if no task)
 * If it returns a task, this task have been freed.
 */
static inline void __piom_ltask_queue_schedule(piom_ltask_queue_t*queue, int count)
{
    if((queue->state != PIOM_LTASK_QUEUE_STATE_RUNNING)
       && (queue->state != PIOM_LTASK_QUEUE_STATE_STOPPING)) 
	{
	    return;
	}
#ifdef PIOMAN_MULTITHREAD
    if(__sync_fetch_and_add(&queue->processing, 1) != 0)
	{
	    __sync_fetch_and_sub(&queue->processing, 1);
	    return;
	}
#endif /* PIOMAN_MULTITHREAD */
    int i;
    for(i = 0; i < count; i++)
	{
	    int success = 0, again = 0;
	    struct piom_ltask*task = piom_ltask_lfqueue_dequeue_single_reader(&queue->submit_queue);
	    if(task == NULL)
		task = piom_ltask_lfqueue_dequeue_single_reader(&queue->ltask_queue);
	    piom_trace_queue_var(queue, PIOM_TRACE_VAR_LTASKS, (PIOM_MAX_LTASK + queue->ltask_queue._head - queue->ltask_queue._tail) % PIOM_MAX_LTASK);
	    if(task)
		{
		    piom_trace_queue_state(queue, PIOM_TRACE_STATE_INIT);
		    if(task->masked)
			{
			    /* re-submit to poll later when task will be unmasked  */
			    again = 1;
			    assert(task->state != PIOM_LTASK_STATE_NONE);
			    assert(task->state != PIOM_LTASK_STATE_DESTROYED);
			}
		    else if(task->state == PIOM_LTASK_STATE_READY)
			{
			    /* wait is pending: poll */
			    piom_ltask_state_set(task, PIOM_LTASK_STATE_SCHEDULED);
			    piom_trace_queue_state(queue, PIOM_TRACE_STATE_POLL);
			    piom_tasklet_mask();
			    const int options = task->options;
			    (*task->func_ptr)(task->data_ptr);
			    const int state = task->state;
			    if(options & PIOM_LTASK_OPTION_DESTROY)
				{
				    /* ltask was destroyed by handler */
				    task = NULL;
				    piom_tasklet_unmask();
				    success = 1;
				}
			    else if((options & PIOM_LTASK_OPTION_REPEAT) && !(state & PIOM_LTASK_STATE_SUCCESS))
				{
				    piom_ltask_state_unset(task, PIOM_LTASK_STATE_SCHEDULED);
				    piom_tasklet_unmask();
				    /* If another thread is currently stopping the queue don't repost the task */
				    if(queue->state == PIOM_LTASK_QUEUE_STATE_RUNNING)
					{
					    again = 1;
					    assert(task->state != PIOM_LTASK_STATE_NONE);
					    assert(task->state != PIOM_LTASK_STATE_DESTROYED);
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
					    fprintf(stderr, "PIOMan: WARNING- task %p is left uncompleted\n", task);
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
		    else if(piom_ltask_state_test(task, PIOM_LTASK_STATE_SUCCESS) ||
			    piom_ltask_state_test(task, PIOM_LTASK_STATE_CANCELLED))
			{
			    success = 1;
			    piom_ltask_state_set(task, PIOM_LTASK_STATE_TERMINATED);
			}
		    else
			{
			    fprintf(stderr, "PIOMan: FATAL- wrong state %x for scheduled ltask.\n", task->state);
			    abort();
			}
		    piom_trace_queue_state(queue, PIOM_TRACE_STATE_NONE);
		    if(again)
			{
			    int rc = -1;
			    while(rc != 0)
				{
				    assert(task->state != PIOM_LTASK_STATE_NONE);
				    assert(task->state != PIOM_LTASK_STATE_DESTROYED);
				    rc = piom_ltask_lfqueue_enqueue_single_writer(&queue->ltask_queue, task);
				}
			}
		}
	    /* no more task to run, set the queue as stopped */
	    if( (task == NULL) && 
		(queue->state == PIOM_LTASK_QUEUE_STATE_STOPPING) )
		{
		    queue->state = PIOM_LTASK_QUEUE_STATE_STOPPED;
		    break;
		}
	    if(success)
		{
		    piom_trace_queue_event(queue, PIOM_TRACE_EVENT_SUCCESS, task);
		}
	}
#ifdef PIOMAN_MULTITHREAD
    __sync_fetch_and_sub(&queue->processing, 1);
#endif /* PIOMAN_MULTITHREAD */
}


static inline void __piom_exit_queue(piom_ltask_queue_t *queue)
{
    queue->state = PIOM_LTASK_QUEUE_STATE_STOPPING;

    /* empty the list of tasks */
    while(queue->state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	{
	    __piom_ltask_queue_schedule(queue, 1);
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
	    fprintf(stderr, "# pioman: WARNING- timer thread could not get priority %d.\n", max_prio);
	}
    piom_ltask_queue_t*global_queue = __piom_get_queue(piom_topo_full);
    while(global_queue->state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	{
	    struct timespec t = { .tv_sec = 0, .tv_nsec = timeslice_nsec };
	    clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL);
	    piom_trace_queue_event(NULL, PIOM_TRACE_EVENT_TIMER_POLL, NULL);
	    if(prio_enable)
		pthread_setschedprio(pthread_self(), old_prio);
	    piom_check_polling(PIOM_POLL_AT_TIMER_SIG);
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
	    fprintf(stderr, "# pioman: WARNING- idle thread could not get priority %d.\n", min_prio);
	}
    while(queue->state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	{
	    tbx_tick_t s1, s2;
	    TBX_GET_TICK(s1);
	    piom_trace_queue_event(NULL, PIOM_TRACE_EVENT_IDLE_POLL, NULL);
	    if(prio_enable)
		pthread_setschedprio(pthread_self(), old_prio);
	    piom_check_polling(PIOM_POLL_AT_IDLE);
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
				    sprintf(cont_name, "%s_%d", level_label, i);
				    sprintf(cont_type, "Container_%s", level_label);
				    queue->cont_name = strdup(cont_name);
				    queue->trace_info.level = o->type;
				    queue->trace_info.rank = i;
				    addContainer(0.00000, queue->cont_name, cont_type, 
						 queue->parent ? queue->parent->cont_name:"0", cont_name, "0");
				    piom_trace_queue_state(queue, PIOM_TRACE_STATE_NONE);
#endif /* PIOMAN_TRACE */

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
			    fprintf(stderr, "# pioman: idle #%d on %s\n", i, string);
#endif /* DEBUG */
			    piom_ltask_queue_t*queue = __piom_get_queue(o);
			    pthread_t idle_thread;
			    pthread_attr_t attr;
			    pthread_attr_init(&attr);
#ifdef SCHED_IDLE
			    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
			    pthread_attr_setschedpolicy(&attr, SCHED_IDLE);
#endif /* SCHED_IDLE */
			    pthread_create(&idle_thread, &attr, &__piom_ltask_idle_worker, queue);
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
			    piom_trace_queue_state(queue, PIOM_TRACE_STATE_NONE);
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
    task->state = PIOM_LTASK_STATE_READY;
    task->queue = queue;
    /* wait until a task is removed from the list */
    int rc = -1;
    while(rc != 0)
	{
	    assert(task->state != PIOM_LTASK_STATE_NONE); /* XXX */
	    rc = piom_ltask_lfqueue_enqueue(&queue->submit_queue, task);
	}
    piom_trace_queue_var(queue, PIOM_TRACE_VAR_LTASKS, (PIOM_MAX_LTASK + queue->ltask_queue._head - queue->ltask_queue._tail) % PIOM_MAX_LTASK);
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
    assert(task->state == PIOM_LTASK_STATE_NONE || (task->state & PIOM_LTASK_STATE_TERMINATED) );
    task->state = PIOM_LTASK_STATE_NONE;
    queue = __piom_get_queue(task->binding);
    assert(task != NULL);
    assert(queue != NULL);
    piom_trace_queue_event(queue, PIOM_TRACE_EVENT_SUBMIT, task);
    __piom_ltask_submit_in_queue(task, queue);
}

void piom_ltask_schedule(void)
{
    if(piom_ltask_initialized)
	{
	    piom_ltask_queue_t*queue = __piom_get_queue(piom_ltask_current_obj());
	    while(queue != NULL)
		{
		    int hint1 = (PIOM_MAX_LTASK + queue->ltask_queue._head - queue->ltask_queue._tail) % PIOM_MAX_LTASK;
		    int hint2 = (PIOM_MAX_LTASK + queue->submit_queue._head - queue->submit_queue._tail) % PIOM_MAX_LTASK;
		    const int hint = (hint1 > 0)?hint1:1 + (hint2 > 0)?hint2:1;
		    __piom_ltask_queue_schedule(queue, hint);
#ifdef PIOMAN_TOPOLOGY_HWLOC
		    if(queue->binding->type == HWLOC_OBJ_SOCKET)
			{
			    queue->skip ++;
			    if(queue->skip % 32 != 0) queue = NULL; else queue = queue->parent;
			}
		    else
#endif
			queue = queue->parent;
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
    while(piom_ltask_state_test(task, PIOM_LTASK_STATE_SCHEDULED))
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
    if(ltask->state == PIOM_LTASK_STATE_NONE)
	return;
    __sync_fetch_and_add(&ltask->masked, 1);
    piom_ltask_state_set(ltask, PIOM_LTASK_STATE_CANCELLED);
    piom_ltask_state_unset(ltask, PIOM_LTASK_STATE_READY);
    struct piom_ltask_queue*queue = ltask->queue;
    if(queue)
	{
	    __sync_fetch_and_sub(&ltask->masked, 1);
	    while(!piom_ltask_state_test(ltask, PIOM_LTASK_STATE_TERMINATED))
		{
		    __piom_ltask_queue_schedule(queue, 1);
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
		    if(delay > 10000)
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

