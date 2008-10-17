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

#ifndef PIOM_SERVER_H
#define PIOM_SERVER_H

#include "pioman.h"
#include "tbx_compiler.h"

/* State of the events server */
enum {
    /* Not yet initialized */
    PIOM_SERVER_STATE_NONE,
    /* Being initialized */
    PIOM_SERVER_STATE_INIT = 1,
    /* Running */
    PIOM_SERVER_STATE_LAUNCHED = 2,
    /* Stopped */
    PIOM_SERVER_STATE_HALTED = 3,
};

/* Events server structure */
struct piom_server {
#ifdef MARCEL
    /* Spinlock used to access this structure */
    /* TODO : useless without marcel ? */
    ma_spinlock_t lock;
    /* Thread that owns the lock */
    /* TODO useless without marcel ? */
    marcel_task_t *lock_owner;
#else
    /* foo */
    void *lock;
    void *lock_owner;
#endif	/* MARCEL */

	/* List of submitted queries */
    struct list_head list_req_registered;
    /* List of ready queries */
    struct list_head list_req_ready;
    /* Liste des requêtes pour ceux en attente dans la liste précédente */
    struct list_head list_req_success;
    /* List of waiters */
    struct list_head list_id_waiters;

#ifdef PIOM_BLOCKING_CALLS
    /* Liste des requêtes à exporter */
    struct list_head list_req_to_export;
    /* Liste des requêtes exportées */
    struct list_head list_req_exported;

    /* spinlock to modify the lists of lwp */
    ma_spinlock_t lwp_lock;
    /* List of ready LWPs */
    struct list_head list_lwp_ready;
    /* List of working LWPs */
    struct list_head list_lwp_working;
#endif /* PIOM_BLOCKING_CALLS */

#ifdef MARCEL
    /* Spinlock used to access list_req_success */
    ma_spinlock_t req_success_lock;
    ma_spinlock_t req_ready_lock;
#else
    /* foo */
    void *req_success_lock;
#endif	/* MARCEL */
	/* Polling events registered but not yet polled */
    int registered_req_not_yet_polled;

    /* List of grouped queries for polling (or being grouped) */
    struct list_head list_req_poll_grouped;
    /* List of grouped queries for syscall (or being grouped) */
    struct list_head list_req_block_grouped;
    /* Amount of queries in list_req_poll_grouped */
    int req_poll_grouped_nb;
    int req_block_grouped_nb;

    /* Callbacks */
    piom_pcallback_t funcs[PIOM_FUNCTYPE_SIZE];

    /* Polling points and polling period */
    unsigned poll_points;
    unsigned period;	/* polls every 'period' timeslices */
    int stopable; 	       
    /* TODO: ? piom_chooser_t chooser; // a callback used to decide wether to export or not */
    /* Amount of polling loop before exporting a syscall 
     * ( -1 : never export a syscall) */
    int max_poll;

    /* List of servers currently doing some polling
       (state == 2 and tasks waiting for polling */
    struct list_head chain_poll;

#ifdef MARCEL
    /* Tasklet and timer used for polling */
    /* TODO : useless without marcel ?  */
    struct ma_tasklet_struct poll_tasklet;
    //	struct ma_tasklet_struct manage_tasklet;
    int need_manage;
    struct ma_timer_list poll_timer;
#endif	/* MARCEL */
        /* Max priority for successfull requests */
	/* ie : don't poll if you have a lower priority ! */
    piom_req_priority_t max_priority;
    /* server state */
    int state;
    /* server's name (for debuging) */
    char *name;
};

/* Static initialisation
 * var : the constant variable piom_server_t
 * name: used to identify this server in the debug messages
 */
#define PIOM_SERVER_DEFINE(var, name)					\
    struct piom_server ma_s_#var = PIOM_SERVER_INIT(ma_s_#var, name);	\
    const PIOM_SERVER_DECLARE(var) = &ma_s_#var

#define PIOM_SERVER_DECLARE(var)		\
    const piom_server_t var

#ifdef MARCEL
#ifdef PIOM_BLOCKING_CALLS
#define PIOM_SERVER_INIT(var, sname)					             \
    {									             \
	.lock=MA_SPIN_LOCK_UNLOCKED,					             \
	    .lock_owner=NULL,						             \
	    .list_req_registered=LIST_HEAD_INIT((var).list_req_registered),          \
	    .list_req_ready=LIST_HEAD_INIT((var).list_req_ready),	             \
	    .list_req_success=LIST_HEAD_INIT((var).list_req_success),	             \
	    .list_id_waiters=LIST_HEAD_INIT((var).list_id_waiters),	             \
	    .req_success_lock=MA_SPIN_LOCK_UNLOCKED,			             \
	    .req_ready_lock=MA_SPIN_LOCK_UNLOCKED,			             \
	    .registered_req_not_yet_polled=0,				             \
	    .list_req_poll_grouped=LIST_HEAD_INIT((var).list_req_poll_grouped),      \
	    .list_req_block_grouped=LIST_HEAD_INIT((var).list_req_block_grouped),    \
	    .list_req_to_export=LIST_HEAD_INIT((var).list_req_to_export),            \
	    .list_req_exported=LIST_HEAD_INIT((var).list_req_exported),              \
            .lwp_lock=MA_SPIN_LOCK_UNLOCKED,                                         \
	    .list_lwp_ready=LIST_HEAD_INIT((var).list_lwp_ready),	             \
	    .list_lwp_working=LIST_HEAD_INIT((var).list_lwp_working),	             \
	    .req_poll_grouped_nb=0,					             \
	    .poll_points=0,						             \
	    .period=0,							             \
	    .stopable=0,						             \
	    .max_poll=-1,						             \
	    .chain_poll=LIST_HEAD_INIT((var).chain_poll),		             \
	    .poll_tasklet= MA_TASKLET_INIT((var).poll_tasklet,		             \
					   &piom_poll_from_tasklet,	             \
					   (unsigned long)(piom_server_t)&(var),1 ), \
	    .need_manage=0,						             \
	    .poll_timer= MA_TIMER_INITIALIZER(piom_poll_timer, 0,	             \
					      (unsigned long)(piom_server_t)&(var)), \
	    .state=PIOM_SERVER_STATE_INIT,				             \
	    .name=sname,						             \
	    }
#else  /* MA__LWPS */

#define PIOM_SERVER_INIT(var, sname)					             \
    {									             \
	.lock=MA_SPIN_LOCK_UNLOCKED,					             \
	    .lock_owner=NULL,						             \
	    .list_req_registered=LIST_HEAD_INIT((var).list_req_registered),          \
	    .list_req_ready=LIST_HEAD_INIT((var).list_req_ready),	             \
	    .list_req_success=LIST_HEAD_INIT((var).list_req_success),	             \
	    .list_id_waiters=LIST_HEAD_INIT((var).list_id_waiters),	             \
	    .req_success_lock=MA_SPIN_LOCK_UNLOCKED,			             \
	    .req_ready_lock=MA_SPIN_LOCK_UNLOCKED,			             \
	    .registered_req_not_yet_polled=0,				             \
	    .list_req_poll_grouped=LIST_HEAD_INIT((var).list_req_poll_grouped),      \
	    .req_poll_grouped_nb=0,					             \
	    .poll_points=0,						             \
	    .period=0,							             \
	    .stopable=0,						             \
	    .max_poll=-1,						             \
	    .chain_poll=LIST_HEAD_INIT((var).chain_poll),		             \
	    .poll_tasklet= MA_TASKLET_INIT((var).poll_tasklet,		             \
					   &piom_poll_from_tasklet,	             \
					   (unsigned long)(piom_server_t)&(var),1 ), \
	    .need_manage=0,						             \
	    .poll_timer= MA_TIMER_INITIALIZER(piom_poll_timer, 0,	             \
					      (unsigned long)(piom_server_t)&(var)), \
	    .state=PIOM_SERVER_STATE_INIT,				             \
	    .name=sname,						             \
	    }
#endif /* MA__LWPS */
#else  /* MARCEL */

#define PIOM_SERVER_INIT(var, sname)					        \
    {									        \
	.list_req_registered=LIST_HEAD_INIT((var).list_req_registered),         \
	    .list_req_ready=LIST_HEAD_INIT((var).list_req_ready),	        \
	    .list_req_success=LIST_HEAD_INIT((var).list_req_success),	        \
	    .list_id_waiters=LIST_HEAD_INIT((var).list_id_waiters),	        \
	    .registered_req_not_yet_polled=0,				        \
	    .list_req_poll_grouped=LIST_HEAD_INIT((var).list_req_poll_grouped), \
	    .req_poll_grouped_nb=0,					        \
	    .poll_points=0,						        \
	    .period=0,							        \
	    .stopable=0,						        \
	    .max_poll=-1,						        \
	    .chain_poll=LIST_HEAD_INIT((var).chain_poll),		        \
	    .state=PIOM_SERVER_STATE_INIT,				        \
	    .name=sname,						        \
	    }
#endif	/* MARCEL */

/* Callback registration */
__tbx_inline__ static int piom_server_add_callback(piom_server_t server,
						   piom_op_t op,
						   piom_pcallback_t func)
{
#ifdef PIOM_FILE_DEBUG
    /* This function must be called between initialization and 
     * events server startup*/
    PIOM_BUG_ON(server->state != PIOM_SERVER_STATE_INIT);
    /* Verifies the index is ok */
    PIOM_BUG_ON(op >= PIOM_FUNCTYPE_SIZE || op < 0);
    /* Verifies the function isn't yet registered */
    PIOM_BUG_ON(server->funcs[op].func != NULL);
#endif
    server->funcs[op] = func;
    return 0;
}

/* Check wether the server is launched */
__tbx_inline__ 
static void 
piom_verify_server_state(piom_server_t server)
{
    PIOM_BUG_ON(server->state != PIOM_SERVER_STATE_LAUNCHED);
}

/* Static initialization
 * var : the constant var piom_server_t
 * name: for debug messages
 */
void piom_server_init(piom_server_t server, char *name);

void piom_server_kill(piom_server_t server);

/* Polling setup */
int piom_server_set_poll_settings(piom_server_t server,
				  unsigned poll_points,
				  unsigned poll_period, int max_poll);

/* Start the server
 * It is then possible to wait for events
 */
int piom_server_start(piom_server_t server);

/* Stop the server
 * Reinitialize it before restarting it
 */
int piom_server_stop(piom_server_t server);

/* No more request to poll. we have to
 * + stop the timer
 * + remove the server from the list of running servers
 */
void __piom_poll_stop(piom_server_t server);

/* Start the polling (including the timer) by adding the server to the list
 */
void __piom_poll_start(piom_server_t server);

#endif	/* PIOM_SERVER_H */
