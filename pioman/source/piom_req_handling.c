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

/****************************************************************
 * Handling of successfull requests
 */

/* Signals a request is completed
 */
void 
piom_req_success(piom_req_t req)
{
#ifdef MARCEL
    _piom_spin_lock_softirq(&req->server->req_ready_lock); 
#endif	/* MARCEL */
    PIOM_LOGF("Req %p succeded !\n",req);
    if(req->priority >  req->server->max_priority)
	req->server->max_priority = req->priority;
    tbx_fast_list_move(&(req)->chain_req_ready, &(req)->server->list_req_ready); 
#ifdef MARCEL
    _piom_spin_unlock_softirq(&req->server->req_ready_lock); 
#endif	/* MARCEL */
}

/* Get a completed request (or NULL) */
piom_req_t 
piom_get_success(piom_server_t server)
{
    LOG_IN();
    piom_req_t req = NULL;
    _piom_spin_lock_softirq(&server->req_success_lock);

    if (!tbx_fast_list_empty(&server->list_req_success)) {
	req = tbx_fast_list_entry(server->list_req_success.next,
			 struct piom_req, chain_req_success);
	tbx_fast_list_del_init(&req->chain_req_success);
	PIOM_LOGF("Getting success req %p pour [%s]\n", req, server->name);
    }

    _piom_spin_unlock_softirq(&server->req_success_lock);
    LOG_RETURN(req);
}

/* Remove a request from the completed list 
 * return 1 if the request is deleted or 0 overwise
 */
int 
__piom_del_success_req(piom_server_t server,
		       piom_req_t req)
{
    LOG_IN();
    if (tbx_fast_list_empty(&req->chain_req_success)) {
	/* the request is not yet completed */
	LOG_RETURN(0);
    }
    _piom_spin_lock(&server->req_success_lock);
    tbx_fast_list_del_init(&req->chain_req_success);
    PIOM_LOGF("Removing success ev %p pour [%s]\n", req, server->name);
    _piom_spin_unlock(&server->req_success_lock);

    LOG_RETURN(1);
}

/* add a request to the list of completed requests
 * return 1 if the request is successfully added or 0 overwise
 */
int 
__piom_add_success_req(piom_server_t server,
		       piom_req_t req)
{
    LOG_IN();
    if (!tbx_fast_list_empty(&req->chain_req_success)) {
	/* the request is already completed */
	LOG_RETURN(0);
    }
    _piom_spin_lock(&server->req_success_lock);

    PIOM_LOGF("Adding success req %p pour [%s]\n", req, server->name);
    tbx_fast_list_add_tail(&req->chain_req_success, &server->list_req_success);
    _piom_spin_unlock(&server->req_success_lock);

    LOG_RETURN(0);
}

/* Add server-specific attributes to a request */
int 
piom_req_attr_set(piom_req_t req, int attr)
{
    LOG_IN();
    if (attr & PIOM_ATTR_ONE_SHOT) {
	req->state |= PIOM_STATE_ONE_SHOT;
    }
    if (attr & PIOM_ATTR_NO_WAKE_SERVER) {
	req->state |= PIOM_STATE_NO_WAKE_SERVER;
    }
    if (attr & PIOM_ATTR_DONT_POLL_FIRST) {
	req->state |= PIOM_STATE_DONT_POLL_FIRST;
    }
    if (attr &
	(~
	 (PIOM_ATTR_ONE_SHOT | PIOM_ATTR_NO_WAKE_SERVER))) {
	PIOM_EXCEPTION_RAISE(PIOM_CONSTRAINT_ERROR);
    }
    LOG_RETURN(0);
}

/* Initialize a request */
int 
piom_req_init(piom_req_t req)
{
    LOG_IN();
    __piom_init_req(req);
    LOG_RETURN(0);
}


#ifdef PIOM_BLOCKING_CALLS
/* Add a request to the list of requests a syscall is waiting for */
int 
__piom_register_block(piom_server_t server, piom_req_t req)
{
    LOG_IN();
    PIOM_LOGF("Grouping Block request %p for [%s]\n", req, server->name);

    /* remove the request from the polling list */
    __piom_unregister_poll(server, req);

    /* add it to the list of blocking requests */
    tbx_fast_list_add(&req->chain_req_to_export, &server->list_req_to_export);			
    tbx_fast_list_add(&req->chain_req_block_grouped, &server->list_req_block_grouped);			

    server->req_block_grouped_nb++;	
    req->state |= PIOM_STATE_GROUPED;// | PIOM_STATE_EXPORTED;

    __piom_block_group(server, req);
		
    LOG_RETURN(0);
}
#endif	/* PIOM_BLOCKING_CALLS */


/* Handle events signaled as completed by callbacks
 * - remove request from grouped requests' list
 * - wake up thread that are waiting for this event or for any event on the server
 */
int 
__piom_manage_ready(piom_server_t server)
{
    int nb_grouped_req_removed = 0;
    int nb_req_ask_wake_server = 0;
    piom_req_t req, tmp, bak;

    if (tbx_fast_list_empty(&server->list_req_ready)) {
	/* No completed request */
	return 0;
    }
    bak = NULL;
    _piom_spin_lock_softirq(&server->req_ready_lock); 
    /* Iterate over the list of completed requests */
    tbx_fast_list_for_each_entry_safe(req, tmp, &server->list_req_ready,
			     chain_req_ready) {
	if (req == bak) {
	    break;
	}
	PIOM_LOGF("Poll succeed with req %p\n", req);
	req->state |= PIOM_STATE_OCCURED;

	/* Wake up the threads that wait for this event */
	__piom_wake_req_waiters(server, req, 0);
	if (!(req->state & PIOM_STATE_NO_WAKE_SERVER)) {
	    __piom_add_success_req(server, req);
	    nb_req_ask_wake_server++;
	}

	/* Remove the request from the list of completed requests */
	tbx_fast_list_del_init(&req->chain_req_ready);
	if (req->state & PIOM_STATE_ONE_SHOT) {
	    nb_grouped_req_removed +=__piom_unregister_poll(server, req);
	    __piom_unregister(server, req);
	}
	bak = req;
    }

    PIOM_BUG_ON(!tbx_fast_list_empty(&server->list_req_ready));
    _piom_spin_unlock_softirq(&server->req_ready_lock); 
    if (nb_grouped_req_removed) {
	PIOM_LOGF("Nb grouped task set to %i\n",
		  server->req_poll_grouped_nb);
    }

    if (nb_req_ask_wake_server) {
	/* Wake up the threads that wait for any event on the server */
	__piom_wake_id_waiters(server, nb_req_ask_wake_server);
    }
	
    return 0;
}

/* Handle completed requests */
void 
piom_manage_from_tasklet(unsigned long hid)
{
    piom_server_t server = (piom_server_t) hid;
    PIOM_LOGF("Tasklet function for [%s]\n", server->name);
    __piom_manage_ready(server);
    return;
}

/* Stop polling a specified query */
void 
piom_poll_stop(piom_req_t req)
{
    if(req->state & PIOM_STATE_GROUPED)
	__piom_unregister_poll(req->server,req);
}

/* Resume polling a specified query */
void 
piom_poll_resume(piom_req_t req)
{
    if(!(req->state & PIOM_STATE_GROUPED))
	__piom_register_poll(req->server,req);
}


/* Submit a new request to a server */
int 
piom_req_submit(piom_server_t server, piom_req_t req)
{
    LOG_IN();
#ifdef MARCEL
    marcel_task_t *lock;
    lock=piom_ensure_trylock_from_tasklet(server);
#endif	/* MARCEL */

    piom_verify_server_state(server);
    PIOM_BUG_ON(req->state & PIOM_STATE_REGISTERED);

    __piom_register(server, req);

    PIOM_BUG_ON(!(req->state & PIOM_STATE_REGISTERED));
#ifdef PIOM_BLOCKING_CALLS
    req->state &= ~PIOM_STATE_OCCURED;

    if (__piom_need_export(server, req, NULL, NULL) )
	{
	    piom_wakeup_lwp(server, req);
	}
    else
#endif /* PIOM_BLOCKING_CALLS */

	if(! (req->state & PIOM_STATE_DONT_POLL_FIRST)) {
	    piom_poll_req(req, 0);
	    req->state&= ~PIOM_STATE_DONT_POLL_FIRST;
	    if(req->chain_req_ready.next!= &(req->chain_req_ready) && (req->state= PIOM_STATE_ONE_SHOT)) {
		/* ie. state occured */
		//piom_req_cancel(req, 0);
	    } else
		__piom_register_poll(server, req);
	} else {
	    server->registered_req_not_yet_polled++;
	    __piom_register_poll(server, req);
	}
#ifdef MARCEL
    piom_restore_trylocked_from_tasklet(server, lock);
#endif	/* MARCEL */
    LOG_RETURN(0);
}

/* Cancel a request and wake up threads waiting for this request */
int 
piom_req_cancel(piom_req_t req, int ret_code)
{
#ifdef MARCEL
    marcel_task_t *lock;
#endif	/* MARCEL */
    piom_server_t server;
    LOG_IN();

    PIOM_BUG_ON(!(req->state & PIOM_STATE_REGISTERED));
    server = req->server;
#ifdef MARCEL
    lock = piom_ensure_lock_server(server);
#endif	/* MARCEL */
    piom_verify_server_state(server);

    /* Wake up the threads with the return code */
    __piom_wake_req_waiters(server, req, ret_code);

    /* Unregister the request */
    if (__piom_unregister_poll(server, req)) {
	if (__piom_need_poll(server)) {
	    __piom_poll_group(server, NULL);
	} else {
	    __piom_poll_stop(server);
	}
    }
    __piom_unregister(server, req);

#ifdef MARCEL
    piom_restore_lock_server_locked(server, lock);
#endif	/* MARCEL */

    LOG_RETURN(0);
}

/* Free a request (and may wake up corresponding threads ) */
int 
piom_req_free(piom_req_t req)
{
    piom_server_t server=req->server;
    if(server) {
#ifdef MARCEL
	marcel_task_t *lock;
	lock=piom_ensure_trylock_from_tasklet(server);
#endif	/* MARCEL */
	int nb_req_ask_wake_server=0;
	_piom_spin_lock_softirq(&server->req_ready_lock);

	if(! tbx_fast_list_empty(&req->chain_req_ready)) {
	    /* Successful request. */
	    __piom_wake_req_waiters(server, req, 0);
	    if (!(req->state & PIOM_STATE_NO_WAKE_SERVER)) {
		__piom_add_success_req(server, req);
		nb_req_ask_wake_server++;
	    }
	    tbx_fast_list_del_init(&req->chain_req_ready);
	}
	PIOM_BUG_ON(!tbx_fast_list_empty(&req->chain_req_ready));
	__piom_unregister_poll(server, req);
	__piom_unregister(server, req);
	_piom_spin_unlock_softirq(&server->req_ready_lock);
	if (nb_req_ask_wake_server) {
	    __piom_wake_id_waiters(server, nb_req_ask_wake_server);
	}
#ifdef MARCEL
	piom_restore_trylocked_from_tasklet(server, lock);
#endif	/* MARCEL */

	if (!__piom_need_poll(server)) {
	    __piom_poll_stop(server);
	    return 1;
	} else {
	    __piom_poll_group(server, NULL);
	}
    }

    return 0;
}

/* Register a request */
int 
__piom_register(piom_server_t server, piom_req_t req)
{

    LOG_IN();
    PIOM_LOGF("Register req %p for [%s]\n", req, server->name);

    tbx_fast_list_add(&req->chain_req_registered, &server->list_req_registered);
    req->state |= PIOM_STATE_REGISTERED;
    req->server = server;
    LOG_RETURN(0);
}


/* Initialize a request */
void 
__piom_init_req(piom_req_t req)
{
    PIOM_LOGF("Clearing Grouping request %p\n", req);
    TBX_INIT_FAST_LIST_HEAD(&req->chain_req_registered);
    TBX_INIT_FAST_LIST_HEAD(&req->chain_req_grouped);
#ifdef PIOM_BLOCKING_CALLS
    TBX_INIT_FAST_LIST_HEAD(&req->chain_req_block_grouped);
    TBX_INIT_FAST_LIST_HEAD(&req->chain_req_to_export);
#endif /* PIOM_BLOCKING_CALLS */
    TBX_INIT_FAST_LIST_HEAD(&req->chain_req_ready);
    TBX_INIT_FAST_LIST_HEAD(&req->chain_req_success);
	
    req->state = 0;
    req->server = NULL;
    req->max_poll = 0;
    req->nb_polled = 0;

    req->func_to_use=PIOM_FUNC_AUTO;
    req->priority=PIOM_REQ_PRIORITY_NORMAL;

    TBX_INIT_FAST_LIST_HEAD(&req->list_wait);
}

/* Unregister a request */
int 
__piom_unregister(piom_server_t server, piom_req_t req)
{
    LOG_IN();
    PIOM_LOGF("Unregister request %p for [%s]\n", req, server->name);
    __piom_del_success_req(server, req);
    tbx_fast_list_del_init(&req->chain_req_registered);
    tbx_fast_list_del_init(&req->chain_req_ready);
    req->state &= ~PIOM_STATE_REGISTERED;
    LOG_RETURN(0);
}

/* Add a request to the polling list 
 * after that, the request will be polled 
 */
int 
__piom_register_poll(piom_server_t server, piom_req_t req)
{
    LOG_IN();
    /* Add the request to the list */
    PIOM_LOGF("Grouping Poll request %p for [%s]\n", req, server->name);
    PIOM_BUG_ON(! (req->state & PIOM_STATE_REGISTERED));
    tbx_fast_list_add(&req->chain_req_grouped, &server->list_req_poll_grouped);

    server->req_poll_grouped_nb++;
    req->state |= PIOM_STATE_GROUPED;
    /* Try to group the request */
    __piom_poll_group(server, req);

    PIOM_BUG_ON(&req->chain_req_grouped == req->chain_req_grouped.next);
    PIOM_BUG_ON(&req->chain_req_grouped == req->chain_req_grouped.prev);

    /* if the server isn't running, add it to the list of running servers*/
    if (server->req_poll_grouped_nb == 1) {
	__piom_poll_start(server);
    }
    LOG_RETURN(0);
}

/* Remove a request from the polling list */
int 
__piom_unregister_poll(piom_server_t server, piom_req_t req)
{
    LOG_IN();
    if (req->state & PIOM_STATE_GROUPED) {
	PIOM_LOGF("Ungrouping Poll request %p for [%s]\n", req,
		  server->name);
#ifdef PIOM_BLOCKING_CALLS
	if(req->state & PIOM_STATE_EXPORTED){
	    tbx_fast_list_del_init(&req->chain_req_block_grouped);
	    tbx_fast_list_del_init(&req->chain_req_to_export);
	    server->req_block_grouped_nb--;
	} else 
#endif /* PIOM_BLOCKING_CALLS */
	    {
		tbx_fast_list_del_init(&req->chain_req_grouped);
		server->req_poll_grouped_nb--;
	    }
	req->state &= ~PIOM_STATE_GROUPED;
	PIOM_BUG_ON(server->list_req_poll_grouped.next == &req->chain_req_grouped);
	PIOM_BUG_ON(server->list_req_poll_grouped.prev == &req->chain_req_grouped);
	LOG_RETURN(1);
    }
#ifdef PIOM_BLOCKING_CALLS
    else if(req->state & PIOM_STATE_EXPORTED){
	tbx_fast_list_del_init(&req->chain_req_to_export);
    }
#endif /* PIOM_BLOCKING_CALLS */
    LOG_RETURN(0);
}
