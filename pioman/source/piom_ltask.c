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

#include <pioman.h>
#include "piom_lfqueue.h"

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
    piom_vpset_t                      vpset;
} piom_ltask_queue_t;

static void __piom_ltask_submit_in_queue(struct piom_ltask *task, piom_ltask_queue_t *queue);

/** refcounter on piom_ltask */
static volatile int piom_ltask_initialized = 0;
#ifdef PIOMAN_LTASK_GLOBAL_QUEUE
static struct piom_ltask_queue global_queue;
#endif

#ifdef PIOMAN_PTHREAD
static timer_t timer_id;
static pthread_t idle_thread;
volatile int __piom_ltask_handler_masked = 0;
static inline void piom_tasklet_mask(void)
{
    __sync_fetch_and_add(&__piom_ltask_handler_masked, 1);
}
static inline void piom_tasklet_unmask(void)
{
    __sync_fetch_and_sub(&__piom_ltask_handler_masked, 1);
}
#elif defined(PIOMAN_MARCEL)
static inline void piom_tasklet_mask(void)
{
    marcel_tasklet_disable();
}
static inline void piom_tasklet_unmask(void)
{
    marcel_tasklet_enable();
}
#else /* PIOMAN_MARCEL */
static inline void piom_tasklet_mask(void)
{ }
static inline void piom_tasklet_unmask(void)
{ }
#endif /* PIOMAN_PTHREAD/MARCEL */


/* Get the queue that matches a vpset:
 * - a local queue if only one vp is set in the vpset
 * - the global queue otherwise
 */
static inline piom_ltask_queue_t*__piom_get_queue(piom_vpset_t vpset)
{
#ifdef PIOMAN_LTASK_GLOBAL_QUEUE
    return &global_queue;
#else /* PIOMAN_LTASK_GLOBAL_QUEUE */
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
#endif	/* PIOMAN_LTASK_GLOBAL_QUEUE */
}

/* Initialize a queue */
static inline void __piom_init_queue(piom_ltask_queue_t*queue)
{
    queue->state = PIOM_LTASK_QUEUE_STATE_NONE;
    piom_ltask_lfqueue_init(&queue->ltask_queue);
    queue->processing = 0;
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
		    task->state |= PIOM_LTASK_STATE_SCHEDULED;
		    piom_tasklet_mask();
		    const int options = task->options;
		    (*task->func_ptr)(task->data_ptr);
		    if(options & PIOM_LTASK_OPTION_DESTROY)
			{
			    /* ltask has been destroyed */
			    piom_tasklet_unmask();
			}
		    else
			{
			    task->state ^= PIOM_LTASK_STATE_SCHEDULED;
			    if ((task->options & PIOM_LTASK_OPTION_REPEAT)
				&& !(task->state & PIOM_LTASK_STATE_DONE))
				{
				    piom_tasklet_unmask();
				    /* If another thread is currently stopping the queue don't repost the task */
				    if(queue->state == PIOM_LTASK_QUEUE_STATE_RUNNING)
					{
					    __piom_ltask_submit_in_queue(task, queue);
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
				    piom_tasklet_unmask();
				}
			}
		} 
	    else if(task->state & PIOM_LTASK_STATE_DONE) 
		{
		    task->state |= PIOM_LTASK_STATE_TERMINATED;
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
    return task;
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
static void __piom_ltask_sighandler(int num)
{
    if(!__piom_ltask_handler_masked)
	piom_check_polling(PIOM_POLL_AT_TIMER_SIG);
}
static void*__piom_ltask_idle(void*_dummy)
{
    int num_skip = 0;
    while(global_queue.state != PIOM_LTASK_QUEUE_STATE_STOPPED)
	{
	    if(!__piom_ltask_handler_masked)
		{
		    piom_check_polling(PIOM_POLL_AT_IDLE);
		    num_skip = 0;
		}
	    else
		{
		    num_skip++;
#ifdef DEBUG
		    if((num_skip > 10000) && (num_skip % 1000 == 0))
			fprintf(stderr, "PIOMan: WARNING- idle thread cannot acquire lock (count = %d); suspecting deadlock.\n", num_skip);
#endif
		    usleep(10);
		}
	    usleep(1);
	}
    return NULL;
}
#endif /* PIOMAN_PTHREAD */

void piom_init_ltasks(void)
{
    if (!piom_ltask_initialized)
	{
#ifdef PIOMAN_MARCEL
	    /* register a callback to Marcel */
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_TIMER);
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_YIELD);
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_IDLE);
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_LIB_ENTRY);
	    marcel_register_scheduling_hook(piom_check_polling, MARCEL_SCHEDULING_POINT_CTX_SWITCH);
#endif /* PIOMAN_MARCEL */

#ifdef PIOMAN_PTHREAD 
	    const char*env_piom_enable_progression = getenv("PIOM_ENABLE_PROGRESSION");
	    const char*env_piom_enable_idle_thread = getenv("PIOM_ENABLE_IDLE_THREAD");
	    const char*env_piom_enable_sighandler  = getenv("PIOM_ENABLE_SIGHANDLER");
	    int piom_enable_progression = env_piom_enable_progression ? atoi(env_piom_enable_progression) : 1;
	    int piom_enable_idle_thread = env_piom_enable_idle_thread ? atoi(env_piom_enable_idle_thread) : piom_enable_progression;
	    int piom_enable_sighandler  = env_piom_enable_sighandler  ? atoi(env_piom_enable_sighandler) : piom_enable_progression;
	    if(piom_enable_sighandler)
		{
		    const int signal_number = SIGRTMIN + 1;
		    const long timeslice_nsec = 4 * 1000 * 1000; /* 4 msec. */
		    sigset_t set;
		    sigemptyset(&set);
		    const struct sigaction action =
			{
			    .sa_handler = &__piom_ltask_sighandler,
			    .sa_mask    = set,
			    .sa_flags   = SA_RESTART
			};
		    sigaction(signal_number, &action, NULL);
		    struct sigevent sev = 
			{
			    .sigev_notify = SIGEV_SIGNAL,
			    .sigev_signo  = signal_number
			};
		    timer_create(CLOCK_MONOTONIC, &sev, &timer_id);
		    const struct itimerspec spec =
			{
			    .it_interval.tv_sec  = 0,
			    .it_interval.tv_nsec = timeslice_nsec,
			    .it_value.tv_sec     = 0,
			    .it_value.tv_nsec    = timeslice_nsec
			};
		    timer_settime(timer_id, 0, &spec, NULL);
		}
	    if(piom_enable_idle_thread)
		{
		    pthread_create(&idle_thread, NULL, &__piom_ltask_idle, NULL);
		    pthread_setschedprio(idle_thread, sched_get_priority_min(SCHED_OTHER)); 
		}
#endif /* PIOMAN_PTHREAD */

#ifdef PIOMAN_LTASK_GLOBAL_QUEUE
	    __piom_init_queue (&global_queue);
	    global_queue.vpset= ~0;
#else /* PIOMAN_LTASK_GLOBAL_QUEUE */
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
#endif	/* PIOMAN_LTASK_GLOBAL_QUEUE */
	}
    piom_ltask_initialized++;
}

void piom_exit_ltasks(void)
{
    piom_ltask_initialized--;
    if(piom_ltask_initialized == 0)
	{
#ifdef PIOMAN_LTASK_GLOBAL_QUEUE
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
	}

}

int piom_ltask_test_activity(void)
{
    return (piom_ltask_initialized != 0);
}

void __piom_ltask_submit_in_queue(struct piom_ltask *task, piom_ltask_queue_t *queue)
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
	    rc = piom_ltask_lfqueue_enqueue(&queue->ltask_queue, task);
	}
}

void piom_ltask_submit(struct piom_ltask *task)
{
    piom_ltask_queue_t *queue;
    assert(task->state == PIOM_LTASK_STATE_NONE || tast->state == PIOM_LTASK_STATE_DONE);
    task->state = PIOM_LTASK_STATE_NONE;
#ifdef PIOMAN_LTASK_GLOBAL_QUEUE
    queue = &global_queue;
#else /* PIOMAN_LTASK_GLOBAL_QUEUE */
    queue = __piom_get_queue (task->vp_mask);
#endif /* PIOMAN_LTASK_GLOBAL_QUEUE */
    assert(task != NULL);
    assert(queue != NULL);
    __piom_ltask_submit_in_queue(task, queue);
}

void*piom_ltask_schedule(void)
{
    struct piom_ltask*ltask = NULL;
    if(piom_ltask_initialized)
	{
#ifdef PIOMAN_LTASK_GLOBAL_QUEUE
	    ltask = __piom_ltask_queue_schedule(&global_queue);
#else /* PIOMAN_LTASK_GLOBAL_QUEUE */
	    const int vp = piom_ltask_current_vp();
	    const struct marcel_topo_level *l;
	    for(l = &marcel_topo_levels[marcel_topo_nblevels - 1][vp];
		l;			/* do this as long as there's a topo level */
		l=l->father)
		{
		    piom_ltask_queue_t*queue = (piom_ltask_queue_t*)(l->ltask_data);
		    ltask = __piom_ltask_queue_schedule(queue);
		    if(ltask)
			break;
		}
#endif /* PIOMAN_LTASK_GLOBAL_QUEUE */
	}
    return ltask;
}

void piom_ltask_wait_success(struct piom_ltask *task)
{
    assert(ltask->state != PIOM_LTASK_STATE_NONE);
    assert(!(task->options & PIOM_LTASK_OPTION_DESTROY));
    piom_cond_wait(&task->done, PIOM_LTASK_STATE_DONE);
}

void piom_ltask_wait(struct piom_ltask *task)
{
    assert(ltask->state != PIOM_LTASK_STATE_NONE);
    assert(!(task->options & PIOM_LTASK_OPTION_DESTROY));
    piom_cond_wait(&task->done, PIOM_LTASK_STATE_DONE);
    while (!(task->state & PIOM_LTASK_STATE_TERMINATED))
	{
	    piom_ltask_schedule();
	}
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
    while(!found)
	{
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
	}
    ltask->state = PIOM_LTASK_STATE_TERMINATED;
    if(!(options & PIOM_LTASK_OPTION_DESTROY))
	{
	    piom_ltask_completed(ltask);
	}
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

