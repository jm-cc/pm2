/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001-2017 "the PM2 team" (see AUTHORS file)
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

#include "piom_private.h"

#define PIOM_PTHREAD_MAX_THREADS 64

#ifdef PIOMAN_PTHREAD
static struct
{
    /** number of spare LWPs running */
    int lwps_num;
    /** number of available LWPs */
    int lwps_avail;
    /** signal new tasks to LWPs */
    sem_t lwps_ready;
    /** ltasks queue to feed LWPs */
    struct piom_ltask_lfqueue_s lwps_queue;
    /** array of all created threads */
    pthread_t all_threads[PIOM_PTHREAD_MAX_THREADS];
    /** size of above array */
    int n_threads;
#ifdef DEBUG
    /** thread local debug information */
    pthread_key_t threadstate_key;
#endif /* DEBUG */
} __piom_pthread =
    {
	.lwps_num  = 0,
	.n_threads = 0
    };

struct piom_pthread_threadstate_s
{
    int dispatching; /**< whether the pthread is currently dispatching an ltask */
};

#endif /* PIOMAN_PTHREAD */

    
#ifdef PIOMAN_PTHREAD
static void __piom_pthread_setname(const char*name)
{
#if defined(__GLIBC__) && defined(__GLIBC_MINOR__) && ((__GLIBC__ == 2 && __GLIBC_MINOR__ >= 12) || (__GLIBC__ > 2))
    pthread_setname_np(pthread_self(), name);
#else
#warning pthread_setname_np not available
#endif
}
static pthread_t*__piom_pthread_create(const pthread_attr_t*attr, void*(*start_routine)(void*), void *arg)
{
    pthread_t*thread = &__piom_pthread.all_threads[__piom_pthread.n_threads];
    __piom_pthread.n_threads++;
    assert(__piom_pthread.n_threads < PIOM_PTHREAD_MAX_THREADS);
    int err = pthread_create(thread, attr, start_routine, arg);
    if(err)
	{
	    PIOM_FATAL("cannot create threads- error %d (%s)\n", err, strerror(err));
	}
    return thread;
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
	    PIOM_WARN("timer thread could not get priority %d (err=%d, %s).\n", max_prio, rc, strerror(rc));
	}
    piom_ltask_queue_t*global_queue = piom_topo_get_queue(piom_topo_current_obj());
    while(global_queue->state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	{
	    struct timespec t = { .tv_sec = 0, .tv_nsec = timeslice_nsec };
	    clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL);
	    piom_trace_local_event(PIOM_TRACE_EVENT_TIMER_POLL, NULL);
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
    const int granularity0 = piom_parameters.idle_granularity;
    const int coef = (piom_parameters.idle_coef > 0) ? piom_parameters.idle_coef : 1;
    if(rc != 0)
	{
	    PIOM_WARN("idle thread could not get priority %d (err=%d, %s).\n", min_prio, rc, strerror(rc));
	}
    usleep(1000); /* give an opportunity to Linux scheduler to move this thread away */
    while(queue->state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	{
	    tbx_tick_t s1, s2;
	    const int poll_level = piom_ltask_poll_level_get();
	    const int granularity = (poll_level && !piom_ltask_lfqueue_empty(&queue->ltask_queue)) ?
		granularity0 : coef * (granularity0 + 1);
	    TBX_GET_TICK(s1);
	    piom_trace_local_event(PIOM_TRACE_EVENT_IDLE_POLL, NULL);
	    if(prio_enable)
		pthread_setschedprio(pthread_self(), old_prio);
	    piom_ltask_schedule(PIOM_POLL_POINT_IDLE);
	    if(prio_enable)
		pthread_setschedprio(pthread_self(), min_prio);
	    if(granularity > 0 && granularity < 55)
		{
		    double d = 0.0;
		    do
			{
			    sched_yield();
#ifdef PIOMAN_X86INTRIN
			    /* TODO: we should check for 'contant_tsc' cpuid flag before using tsc */
			    uint64_t tsc0 = __rdtsc();
			    uint64_t tsc1 = 0;
			    do
				{
				    tsc1 = __rdtsc();
				}
			    while(tsc1 - tsc0 < 100 * granularity);
#endif /* PIOMAN_X86INTRIN */
			    const int poll_level2 = piom_ltask_poll_level_get();
			    if(poll_level2 > poll_level)
				break;
			    TBX_GET_TICK(s2);
			    d = TBX_TIMING_DELAY(s1, s2);
			}
		    while(d < granularity);
		}
	    else if(granularity > 0)
		{
		    struct timespec t = { .tv_sec = 0, .tv_nsec = granularity * 1000 };
		    clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL);
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
	    sem_wait(&__piom_pthread.lwps_ready);
	    struct piom_ltask*task = piom_ltask_lfqueue_dequeue(&__piom_pthread.lwps_queue);
	    const int options = task->options;
	    assert(task != NULL);
	    piom_ltask_state_set(task, PIOM_LTASK_STATE_BLOCKED);
	    (*task->blocking_func)(task->data_ptr);
	    if(!(options & PIOM_LTASK_OPTION_DESTROY))
		piom_ltask_state_unset(task, PIOM_LTASK_STATE_BLOCKED);
	    __sync_fetch_and_add(&__piom_pthread.lwps_avail, 1);
	}
    return NULL;
}

void piom_pthread_preinvoke(void)
{
#ifdef DEBUG
    struct piom_pthread_threadstate_s*p_threadstate = pthread_getspecific(__piom_pthread.threadstate_key);
    if(p_threadstate == NULL)
	{
	    p_threadstate = malloc(sizeof(struct piom_pthread_threadstate_s));
	    p_threadstate->dispatching = 0;
	    pthread_setspecific(__piom_pthread.threadstate_key, p_threadstate);
	}
    assert(p_threadstate->dispatching == 0);
    p_threadstate->dispatching++;
#endif
}
void piom_pthread_postinvoke(void)
{
#ifdef DEBUG
    struct piom_pthread_threadstate_s*p_threadstate = pthread_getspecific(__piom_pthread.threadstate_key);
    assert(p_threadstate != NULL);
    assert(p_threadstate->dispatching == 1);
    p_threadstate->dispatching--;
#endif
}
void piom_pthread_check_wait(void)
{
#ifdef DEBUG
    struct piom_pthread_threadstate_s*p_threadstate = pthread_getspecific(__piom_pthread.threadstate_key);
    if(p_threadstate != NULL && p_threadstate->dispatching)
	{
	    fprintf(stderr, "pioman: deadlock detected- cannot wait while dispatching ltask.\n");
	    abort();
	}
#endif /* DEBUG */
}
#ifdef DEBUG
static void __piom_pthread_state_destructor(void*_p_threadstate)
{
    struct piom_pthread_threadstate_s*p_threadstate = _p_threadstate;
    assert(p_threadstate != NULL);
    assert(p_threadstate->dispatching == 0);
    free(p_threadstate);
}
#endif /* DEBUG */
void piom_pthread_init_ltasks(void)
{
#ifdef DEBUG
    pthread_key_create(&__piom_pthread.threadstate_key, &__piom_pthread_state_destructor);
#endif
    /* ** timer-based polling */
    if(piom_parameters.timer_period > 0)
	{
	    PIOM_DISP("starting timer worker (period = %d usec.)\n", piom_parameters.timer_period);
	    __piom_pthread_create(NULL, &__piom_ltask_timer_worker, NULL);
	}
    /* ** idle polling */
    if(piom_parameters.idle_granularity >= 0)
	{
#if defined(PIOMAN_TOPOLOGY_NONE)
	    PIOM_WARN("no hwloc, using global queue. Running in degraded mode.\n");
	    piom_ltask_queue_t*queue = piom_topo_get_queue(piom_topo_full);
	    pthread_t*idle_thread = __piom_pthread_create(NULL, &__piom_ltask_idle_worker, queue);
	    pthread_setschedprio(*idle_thread, sched_get_priority_min(SCHED_OTHER));
#elif defined(PIOMAN_TOPOLOGY_HWLOC)
	    hwloc_topology_t topo = piom_topo_get();
	    const hwloc_obj_type_t level = piom_parameters.binding_level;
	    hwloc_obj_t o = NULL;
	    int i = 0;
	    do
		{
		    o = hwloc_get_obj_by_type(topo, level, i);
		    if(o == NULL)
			break;
		    if( ((piom_parameters.idle_distrib == PIOM_BIND_DISTRIB_FIRST) && (o->logical_index == 1)) ||
			((piom_parameters.idle_distrib == PIOM_BIND_DISTRIB_ODD)   && (o->logical_index % 2 == 1)) ||
			((piom_parameters.idle_distrib == PIOM_BIND_DISTRIB_EVEN)  && (o->logical_index % 2 == 0)) ||
			( piom_parameters.idle_distrib == PIOM_BIND_DISTRIB_ALL)
			)
			{
			    char string[128];
			    hwloc_obj_snprintf(string, sizeof(string), topo, o, "#", 0);
			    PIOM_DISP("idle worker #%d on %s (granularity = %d usec.)\n", i, string, piom_parameters.idle_granularity);
			    piom_ltask_queue_t*queue = piom_topo_get_queue(o);
			    pthread_attr_t attr;
			    pthread_attr_init(&attr);
#ifdef SCHED_IDLE
			    if(!piom_parameters.mckernel)
				{
				    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
				    pthread_attr_setschedpolicy(&attr, SCHED_IDLE);
				}
			    else
				{
				    PIOM_WARN("idle thread not set as SCHED_IDLE on mckernel.\n");
				}
#endif /* SCHED_IDLE */
			    pthread_t*idle_thread = __piom_pthread_create(&attr, &__piom_ltask_idle_worker, queue);
			    int rc = hwloc_set_thread_cpubind(topo, *idle_thread, o->cpuset, HWLOC_CPUBIND_THREAD);
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
    __piom_pthread.lwps_avail = 0;
    __piom_pthread.lwps_num = piom_parameters.spare_lwp;
    if(piom_parameters.spare_lwp)
	{
	    PIOM_DISP("starting %d spare LWPs.\n", piom_parameters.spare_lwp);
	    sem_init(&__piom_pthread.lwps_ready, 0, 0);
	    piom_ltask_lfqueue_init(&__piom_pthread.lwps_queue);
	    int i;
	    for(i = 0; i < __piom_pthread.lwps_num; i++)
		{
		    pthread_t*tid = __piom_pthread_create(NULL, &__piom_ltask_lwp_worker, NULL);
		    if(tid == NULL)
			{
			    PIOM_FATAL("cannot create spare LWP #%d\n", i);
			}
		    __piom_pthread.lwps_avail++;
		}
	}

}

void piom_pthread_exit_ltasks(void)
{
    int i;
    for(i = 0; i < __piom_pthread.n_threads; i++)
	{
	    pthread_join(__piom_pthread.all_threads[i], NULL);
	}
}

#endif /* PIOMAN_PTHREAD */

int piom_ltask_submit_in_lwp(struct piom_ltask*task)
{
#ifdef PIOMAN_PTHREAD
    if(__sync_fetch_and_sub(&__piom_pthread.lwps_avail, 1) > 0)
	{
	    piom_ltask_lfqueue_enqueue(&__piom_pthread.lwps_queue, task);
	    sem_post(&__piom_pthread.lwps_ready);
	    return 0;
	}
    else
	{
	    /* rollback */
	    __sync_fetch_and_add(&__piom_pthread.lwps_avail, 1);
	}
#endif /* PIOMAN_PTHREAD */
    return -1;
}
