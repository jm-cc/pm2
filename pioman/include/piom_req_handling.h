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

#ifndef PIOM_REQ_HANDLING_H
#define PIOM_REQ_HANDLING_H

#include "pioman.h"

/* Request options */
enum {
    /* used with POLL_POLLONE */
    PIOM_OPT_REQ_IS_GROUPED = 1,	/* is the request grouped ? */
    PIOM_OPT_REQ_ITER = 2,	/* is the current polling done for every request ? */
};

/* Request priority */
typedef enum {
    PIOM_REQ_PRIORITY_LOWEST = 0,
    PIOM_REQ_PRIORITY_LOW = 1,
    PIOM_REQ_PRIORITY_NORMAL = 2,
    PIOM_REQ_PRIORITY_HIGH = 3,
    PIOM_REQ_PRIORITY_HIGHEST = 4,
} piom_req_priority_t;

/* Request state */
enum {
    PIOM_STATE_OCCURED = 1,	/* the event corresponding to the request occured */
    PIOM_STATE_GROUPED = 2,	/* the request is grouped */
    PIOM_STATE_ONE_SHOT = 4, /* Only one event has to be detected */
    PIOM_STATE_NO_WAKE_SERVER = 8, 	/* Don't wake up the threads waiting for the server (ie piom_server_wait() ) */
    PIOM_STATE_REGISTERED = 16,	/* the request is registered  */
    PIOM_STATE_EXPORTED = 32,	/* the request has been exported on another LWP */
    PIOM_STATE_DONT_POLL_FIRST = 64,	/* Don't poll when the request is posted (start polling later) */
};

/* Events attributes */
enum {
    /* Disactivate the request next time the event occurs */
    PIOM_ATTR_ONE_SHOT = 1,
    /* Don't wake up the threads waiting for the server (ie piom_server_wait() ) */
    PIOM_ATTR_NO_WAKE_SERVER = 2,
    /* Don't poll when the request is posted (start polling later) */
    PIOM_ATTR_DONT_POLL_FIRST = 4,
};

/* Request structure */
struct piom_req {
    /* Method to use for this request (syscall, polling, etc.) */
    piom_func_to_use_t func_to_use;
    /* Request priority */
    piom_req_priority_t priority;

    /* List of submitted queries */
    struct tbx_fast_list_head chain_req_registered;
    /* List of grouped queries */
    struct tbx_fast_list_head chain_req_grouped;
    /* List of grouped queries (for a blocking syscall)*/
    struct tbx_fast_list_head chain_req_block_grouped;
    /* Chaine des requêtes signalées OK par un call-back */
    struct tbx_fast_list_head chain_req_ready;
    /* List of completed queries */
    struct tbx_fast_list_head chain_req_success;
    /* List of queries to be exported */
    struct tbx_fast_list_head chain_req_to_export;
    /* List of exported queries*/
    struct tbx_fast_list_head chain_req_exported;

    /* Request state */
    unsigned state;
    /* Request's server */
    piom_server_t server;
    /* List of waiters for this request */
    struct tbx_fast_list_head list_wait;

    /* Number of polling loops before the request is exported */
    /*  -1 : don't export this request */
    /*   0 : use the server's max_poll var */
    /* not yet implemented */
    int max_poll;
    /* Number of polling loops done */
    int nb_polled;
};

/* Test a submitted request (return 1 if the request was completed, 0 otherwise) */
__tbx_inline__ static int piom_test(piom_req_t req)
{
    return req->state & PIOM_STATE_OCCURED;
}

/* Request initialization
 * (call it first if piom_wait is not used) */
int piom_req_init(piom_req_t req);

/* Add a specific attribute to a request */
int piom_req_attr_set(piom_req_t req, int attr);

/* Submit a request
 *   the server CAN start polling if it "wants" */
int piom_req_submit(piom_server_t server, piom_req_t req);

/* Stop polling a specified request */
void piom_poll_stop(piom_req_t req);

/* Resume polling a specified request */
void piom_poll_resume(piom_req_t req);

/* Cancel a request.
 *   threads waiting for it are woken up and ret_code is returned */
int piom_req_cancel(piom_req_t req, int ret_code);

/* Free a request (and may wake up corresponding threads ) */
int piom_req_free(piom_req_t req);

/* Macro used in callbacks to indicate that request received an event.
 *
 *   * the request is transmited to _piom_get_success_req() (until it
 *   is desactivated)
 *   * threads waiting for an event attached to this request are waken up 
 *   and return 0.
 *
 *   piom_req_t req : request
 */
void piom_req_success(piom_req_t req);

/* Returns a completed request without the NO_WAKE_SERVER attribute
 *  (usefull when server_wait() returns)
 * When a request fails (wait, req_cancel or ONE_SHOT), it is also
 *  removed from this list */
piom_req_t piom_get_success(piom_server_t server);

/* Register a request */
int __piom_register(piom_server_t server, piom_req_t req);

/* Unregister a request */
int __piom_unregister(piom_server_t server, piom_req_t req);

/* Remove a request from the completed list 
 * return 1 if the request is deleted or 0 overwise
 */
int __piom_del_success_req(piom_server_t server,
			   piom_req_t req);

/* Add a request to the polling list 
 * after that, the request will be polled 
 */
int __piom_register_poll(piom_server_t server, piom_req_t req);

#ifdef PIOM_BLOCKING_CALLS
/* Add a request to the list of requests a syscall is waiting for */
int 
__piom_register_block(piom_server_t server, piom_req_t req);
#endif

/* Initialize a request */
void __piom_init_req(piom_req_t req);

/* Handle events signaled as completed by callbacks
 * - remove request from grouped requests' list
 * - wake up thread that are waiting for this event or for any event on the server
 */
int __piom_manage_ready(piom_server_t server);

/* add a request to the list of completed requests
 * return 1 if the request is successfully added or 0 overwise
 */
int __piom_add_success_req(piom_server_t server,
			   piom_req_t req);

/* Remove a request from the polling list */
int __piom_unregister_poll(piom_server_t server, piom_req_t req);

#endif	/* PIOM_REQ_HANDLING_H */
