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

#ifdef MARCEL
#include "marcel.h"
#include "linux_spinlock.h"
#include "linux_interrupt.h"
#endif	/* MARCEL */

#include <errno.h>

/*
 * See piom_io.c for an implementation example
 */

/*
 * Compatibility stuff
 * TODO: is this still used somewhere ?
 */
#define MAX_POLL_IDS    16

/* TODO: dupliquer les fonctions de lock, unlock pour support sans Marcel */
#ifdef MARCEL
typedef struct per_lwp_polling_s {
    int *data;
    int value_to_match;
    void (*func) (void *);
    void *arg;
    marcel_t task;
    volatile tbx_bool_t blocked;
    struct piom_per_lwp_polling_s *next;
} piom_per_lwp_polling_t;
#endif	/* MARCEL */

/* End of compatibility stuff */

/* piom_list_poll is the list that contains the polling servers that 
 * have something to poll
 */
TBX_FAST_LIST_HEAD(piom_list_poll);
/* lock used to access piom_list_poll */
piom_spinlock_t piom_poll_lock = PIOM_SPIN_LOCK_INITIALIZER;


/* Wake up threads that are waiting for a request */
int 
__piom_wake_req_waiters(piom_server_t server,
			piom_req_t req, int code)
{
    piom_wait_t wait, tmp;
    LOG_IN();
    FOREACH_WAIT_BASE_SAFE(wait, tmp, req) {
#ifdef PIOM__DEBUG
	switch (code) {
	case 0:
	    PIOM_LOGF("Poll succeed with task %p\n", wait->task);
	    break;
	default:
	    PIOM_LOGF("Poll awake with task %p on code %i\n",
		      wait->task, code);
	}
#endif	/* PIOM__DEBUG */
	wait->ret = code;
	tbx_fast_list_del_init(&wait->chain_wait);
#ifdef PIOM_THREAD_ENABLED
	piom_sem_V(&wait->sem);
#endif	/* PIOM_THREAD_ENABLED */
    }
    LOG_RETURN(0);
}

/* Wake up threads that are waiting for a server's event */
int 
__piom_wake_id_waiters(piom_server_t server, int code)
{
    piom_wait_t wait, tmp;
    LOG_IN();
    tbx_fast_list_for_each_entry_safe(wait, tmp, &server->list_id_waiters,
			     chain_wait) {
#ifdef PIOM__DEBUG
	switch (code) {
	case 0:
	    PIOM_LOGF("Poll succeed with global task %p\n",
		      wait->task);
	    break;
	default:
	    PIOM_LOGF
		("Poll awake with global task %p on code %i\n",
		 wait->task, code);
	}
#endif	/* PIOM__DEBUG */
	wait->ret = code;
	tbx_fast_list_del_init(&wait->chain_wait);
#ifdef PIOM_THREAD_ENABLED
	piom_sem_V(&wait->sem);
#endif	/* PIOM_THREAD_ENABLED */
    }
    LOG_RETURN(0);
}


/* Wait for a request's completion (internal use only) */
int 
__piom_wait_req(piom_server_t server, piom_req_t req,
		piom_wait_t wait, piom_time_t timeout)
{
    LOG_IN();
    if (timeout) {
	PIOM_EXCEPTION_RAISE(PIOM_NOT_IMPLEMENTED);
    }
    TBX_INIT_FAST_LIST_HEAD(&wait->chain_wait);
    TBX_INIT_FAST_LIST_HEAD(&req->list_wait);
    tbx_fast_list_add(&wait->chain_wait, &req->list_wait);
#ifdef PIOM_THREAD_ENABLED
    piom_sem_init(&wait->sem, 0);
    wait->task = PIOM_SELF;
#endif /* PIOM_THREAD_ENABLED */
    wait->ret = 0;

#ifdef PIOM_BLOCKING_CALLS
    if (__piom_need_export(server, req, wait, timeout) )
	{
	    piom_wakeup_lwp(server, req);
	}
#endif /* PIOM_BLOCKING_CALLS */

    piom_server_unlock(server);

#ifdef PIOM_THREAD_ENABLED
    /* TODO: use pmarcel_sem_P that can return -1 and set errno to EINT (posix compliant) */
    piom_sem_P(&wait->sem);	
#else
    int waken_up = 0;

    /* Let's poll this server until the request is completed */
    do {
	req->state |=
	    PIOM_STATE_ONE_SHOT | PIOM_STATE_NO_WAKE_SERVER;
	piom_check_polling_for(server);

	if (req->state & PIOM_STATE_OCCURED) {
	    if (waken_up) {
		/* The number of request changed */
		if (__piom_need_poll(server)) {
		    __piom_poll_group(server, NULL);
		} else {
		    __piom_poll_stop(server);
		}
	    }
	    __piom_unregister_poll(server, req);
	    __piom_unregister(server, req);

	    LOG_RETURN(0);
	}
    } while (!(req->state & PIOM_STATE_OCCURED));

#endif	/* PIOM_THREAD_ENABLED */
    LOG_RETURN(wait->ret);
}


/* Wait for a request's completion
 * The request has to be already registered
 */
int 
piom_req_wait(piom_req_t req, piom_wait_t wait,
	      piom_time_t timeout)
{
    if(req->state&PIOM_STATE_OCCURED)
	return 0;
    piom_thread_t lock;
    int ret = 0;
    piom_server_t server;
    LOG_IN();

    PIOM_BUG_ON(!(req->state & PIOM_STATE_REGISTERED));
    server = req->server;
    lock = piom_server_lock_reentrant(server);
    piom_verify_server_state(server);

    req->state &= ~PIOM_STATE_OCCURED;

    /* TODO: only check *this* request (instead of *all* the requests) */
    if (!(req->state & PIOM_STATE_DONT_POLL_FIRST))
	/* Before waiting, try to poll the request */
	piom_check_polling_for(server);
    else
	req->state &= ~PIOM_STATE_DONT_POLL_FIRST;

    if (!(req->state & PIOM_STATE_OCCURED)) {
	/* Wait for the request */
	ret = __piom_wait_req(server, req, wait, timeout);
	piom_server_relock_reentrant(server, lock);
    } else {
	piom_server_unlock_reentrant(server, lock);
    }

    LOG_RETURN(ret);
}

/* TODO: support without Marcel */
/* Wait for any event on a server */
int 
piom_server_wait(piom_server_t server, piom_time_t timeout)
{
    LOG_IN();
    struct piom_wait wait;
    piom_thread_t lock;
    lock = piom_server_lock_reentrant(server);

    piom_verify_server_state(server);

    if (timeout) {
	PIOM_EXCEPTION_RAISE(PIOM_NOT_IMPLEMENTED);
    }

    tbx_fast_list_add(&wait.chain_wait, &server->list_id_waiters);
#ifdef PIOM_THREAD_ENABLED
    piom_sem_init(&wait.sem, 0);
#endif	/* PIOM_THREAD_ENABLED */
    wait.ret = 0;
#ifdef PIOM__DEBUG
    wait.task = PIOM_SELF;
#endif	/* PIOM__DEBUG */
	/* TODO: only register if the polling fails */
    piom_check_polling_for(server);
    piom_server_unlock(server);
#ifdef PIOM_THREAD_ENABLED
    /* TODO: use pmarcel_sem_P (posix compliant) */
    piom_sem_P(&wait.sem);
#endif	/* PIOM_THREAD_ENABLED */
    piom_server_relock_reentrant(server, lock);

    LOG_RETURN(wait.ret);
}

/* Register, wait and unregister a request */
int 
piom_wait(piom_server_t server, piom_req_t req,
	  piom_wait_t wait, piom_time_t timeout)
{
    LOG_IN();
    int checked = 0;
    int waken_up = 0;

    piom_thread_t lock_owner;
    lock_owner = piom_server_lock_reentrant(server);

    piom_verify_server_state(server);

    /* Initialize and register the request */
    __piom_init_req(req);
    __piom_register(server, req);
#ifdef MARCEL
    PIOM_LOGF("Marcel_poll (thread %p)...\n", marcel_self());
#endif	/* MARCEL */
    PIOM_LOGF("using pollid [%s]\n", server->name);
    req->state |= PIOM_STATE_ONE_SHOT | PIOM_STATE_NO_WAKE_SERVER;

    /* First, poll th request */
    if (server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func
	&& req->func_to_use != PIOM_FUNC_SYSCALL && !checked) {
	PIOM_LOGF("Using Immediate POLL_ONE\n");
	(*server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func)
	    (server, PIOM_FUNCTYPE_POLL_POLLONE,
	     req, server->req_poll_grouped_nb, 0);
	waken_up = __piom_need_poll(server);
	__piom_manage_ready(server);
	waken_up -= __piom_need_poll(server);
	checked = 1;
    }

    if (req->state & PIOM_STATE_OCCURED) {
	if (waken_up) {
	    /* The number of request changed */
	    if (__piom_need_poll(server)) {
		__piom_poll_group(server, NULL);
	    } else {
		__piom_poll_stop(server);
	    }
	}

	piom_server_unlock_reentrant(server, lock_owner);
	LOG_RETURN(0);
    }

    /* Add the request to the polling list */
    __piom_register_poll(server, req);
    if (!checked && req->func_to_use != PIOM_FUNC_SYSCALL) {
	piom_check_polling_for(server);
    }

    /* Wait for the request's completion */
    if (!(req->state & PIOM_STATE_OCCURED)) {
	__piom_wait_req(server, req, wait, timeout);
    lock_owner = piom_server_lock_reentrant(server);
    }
    /* Unregister the request */
    __piom_unregister(server, req);
    piom_server_unlock_reentrant(server, lock_owner);
    LOG_RETURN(wait->ret);
}


