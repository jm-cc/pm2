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
#include <errno.h>

/* TODO: support without Marcel */
extern unsigned long volatile ma_jiffies;
volatile int *__piom_core_status = NULL;
int __piom_nb_core = 0;

/* Start the polling (including the timer) by adding the server to the list
 */
void 
__piom_poll_start(piom_server_t server)
{
    _piom_spin_lock_softirq(&piom_poll_lock);
    PIOM_LOGF("Starting polling for %s (%p)\n", server->name, server);
    tbx_fast_list_add(&server->chain_poll, &piom_list_poll);
    if (server->poll_points & PIOM_POLL_AT_TIMER_SIG) {
	PIOM_LOGF("Starting timer polling for [%s] at period %i\n",
		  server->name, server->period);
#ifdef MARCEL
	ma_mod_timer(&server->poll_timer,
		     ma_jiffies + server->period);
#else
	/* TODO: timer without Marcel */
#endif	/* MARCEL */
    }
    _piom_spin_unlock_softirq(&piom_poll_lock);
}

/* Remove every polling request from the server's list 
 * warning: you probably should use piom_server_stop to do this!
 */
void 
piom_server_kill(piom_server_t server)
{
    _piom_spin_lock_softirq(&piom_poll_lock);
    PIOM_LOGF("Killing polling for %p (%d)\n", server, server->state);
    tbx_fast_list_del_init(&server->chain_poll);
    _piom_spin_unlock_softirq(&piom_poll_lock);
}

/* No more request to poll. we have to
 * + stop the timer
 * + remove the server from the list of running servers
 */
void 
__piom_poll_stop(piom_server_t server)
{
    if(server->state != PIOM_SERVER_STATE_LAUNCHED){
	return;
    }
    _piom_spin_lock_softirq(&piom_poll_lock);

#ifdef MARCEL
    int cur_vp = marcel_current_vp();
#else
    int cur_vp = 0;
#endif
    int i;
    for(i=0; i<__piom_nb_core; i++)
	{
	    /* Notify all the core that we need to modify the list */
	    __piom_core_status[i] |= PIOM_CORE_STATUS_BUSY;
	}
    _piom_spin_unlock_softirq(&piom_poll_lock);

    for(i=0; i<__piom_nb_core; i++)
	{
	    /* we have to add an exception here since the current core may be 'polling' */
	    if(i!=cur_vp)
	    /* wait until all the core have finished  */
		while (__piom_core_status[i] & PIOM_CORE_STATUS_POLLING);
	}

    _piom_spin_lock_softirq(&piom_poll_lock);    
    PIOM_LOGF("Stopping polling for [%s]\n", server->name);
    tbx_fast_list_del_init(&server->chain_poll);
    if (server->poll_points & PIOM_POLL_AT_TIMER_SIG) {
	PIOM_LOGF("Stopping timer polling for [%s]\n", server->name);
#ifdef MARCEL
	ma_del_timer(&server->poll_timer);
#else
	/* TODO: timer without Marcel */
#endif		/* MARCEL */
    }

    for(i=0; i<__piom_nb_core; i++)
	{
	    /* Notify all the core that we need to modify the list */
	    __piom_core_status[i] &= ~PIOM_CORE_STATUS_BUSY;
	}

    _piom_spin_unlock_softirq(&piom_poll_lock);
}



/****************************************************************
 * Initialization/exit functions
 */

static tbx_flag_t piom_activity = tbx_flag_clear;

int
piom_test_activity(void)
{
    tbx_bool_t result = tbx_false;

    LOG_IN();
    result = tbx_flag_test(&piom_activity);
    LOG_OUT();

    return result;
}


/* initialize a server */
void 
piom_server_init(piom_server_t server, char *name)
{
    *server = (struct piom_server) PIOM_SERVER_INIT(*server, name);
    PIOM_LOGF( "New server %s : %p\n",name, server);
#ifdef MARCEL
    //	ma_open_softirq(MA_PIOM_SOFTIRQ, __piom_manage_ready, NULL);
#endif /* MARCEL */
}

#ifndef MA_JIFFIES_PER_TIMER_TICK
#define MA_JIFFIES_PER_TIMER_TICK 1
#endif

/* Change a server's polling settings
 * this function has to be called between piom_server_init and piom_server_start
 */
int 
piom_server_set_poll_settings(piom_server_t server,
			      unsigned poll_points,
			      unsigned period, int max_poll)
{
    PIOM_BUG_ON(server->state != PIOM_SERVER_STATE_INIT);

    server->max_poll = max_poll;
    server->poll_points = poll_points;
    if (poll_points & PIOM_POLL_AT_TIMER_SIG) {
	period = period ? : 1;
	server->period = period * MA_JIFFIES_PER_TIMER_TICK;
    }
    return 0;
}

/* Start a server. Right after this function, PIOMan can poll for submitted requests */
int 
piom_server_start(piom_server_t server)
{
    PIOM_BUG_ON(server->state != PIOM_SERVER_STATE_INIT);
    
    if(! piom_test_activity()) {
#ifdef PIOM_THREAD_ENABLED
	__piom_nb_core = marcel_nbvps();
#else
	__piom_nb_core = 1;
#endif
	__piom_core_status = malloc(sizeof(int)*__piom_nb_core);
	int i;
	for(i=0;i<__piom_nb_core; i++)
	    __piom_core_status[i] = PIOM_CORE_STATUS_IDLE;
    }	

    if (!server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func
	&& !server->funcs[PIOM_FUNCTYPE_POLL_POLLANY].func) {
	PIOM_LOGF("One poll function needed for [%s]\n",
		  server->name);
	PIOM_EXCEPTION_RAISE(PIOM_PROGRAM_ERROR);
    }
    piom_activity = tbx_flag_set;
    server->state = PIOM_SERVER_STATE_LAUNCHED;
    return 0;
}

/* Stops a server */
int 
piom_server_stop(piom_server_t server)
{
    LOG_IN();
    piom_req_t req, tmp;
    if(server->state != PIOM_SERVER_STATE_LAUNCHED) {
	PIOM_LOGF("server %p not started (%d)\n", server, server->state);
	return 0;
    }

#ifdef PIOM_THREAD_ENABLED
    piom_thread_t lock;

    lock = piom_server_lock_reentrant(server);
    __piom_poll_stop(server);
    server->state = PIOM_SERVER_STATE_HALTED;
    piom_server_unlock_reentrant(server, lock);
    
#ifdef MARCEL
    ma_tasklet_kill(&server->poll_tasklet);
#endif
    
    lock = piom_server_lock_reentrant(server);
#ifdef MARCEL
    ma_tasklet_disable(&server->poll_tasklet);
#endif
    
#endif	/* PIOM_THREAD_ENABLED */

    int i;
    _piom_spin_lock_softirq(&piom_poll_lock);
#ifdef MARCEL
    int cur_vp = marcel_current_vp();
#else
    int cur_vp = 0;
#endif
    __piom_core_status[cur_vp] = 0;
    for(i=0; i<__piom_nb_core; i++)
	{
	    /* Notify all the core that we need to modify the list */
	    __piom_core_status[i] |= PIOM_CORE_STATUS_BUSY;
	}
    _piom_spin_unlock_softirq(&piom_poll_lock);

    for(i=0; i<__piom_nb_core; i++)
	{
	    /* wait until all the core have finished  */
	    while (__piom_core_status[i] & PIOM_CORE_STATUS_POLLING);
	}

    PIOM_LOGF("server %p is stopped\n", server);

#ifdef PIOM_BLOCKING_CALLS
    /* Stop LWPs */
    piom_comm_lwp_t lwp;

    while(!tbx_fast_list_empty(server->list_lwp_ready.next)){
	char foo=43;
	lwp = tbx_fast_list_entry(server->list_lwp_ready.next, struct piom_comm_lwp, chain_lwp_ready);
	write(lwp->fds[1], &foo, 1);
	sched_yield();
    }
    while(!tbx_fast_list_empty(server->list_lwp_working.next)){
	char foo=44;
	lwp = tbx_fast_list_entry(server->list_lwp_working.next, struct piom_comm_lwp, chain_lwp_working);
	write(lwp->fds[1], &foo, 1);
	sched_yield(); 
    }
#endif	/* PIOM_BLOCKING_CALLS */
#ifndef ECANCELED
#define ECANCELED EIO
#endif
    /* Cancel any pending request */
    tbx_fast_list_for_each_entry_safe(req, tmp,
			     &server->list_req_registered,
			     chain_req_registered) {
	PIOM_LOGF("stopping req %p\n", req);
	__piom_wake_req_waiters(server, req, -ECANCELED);
	__piom_unregister_poll(server, req);
	__piom_unregister(server, req);
    }
    __piom_wake_id_waiters(server, -ECANCELED);

    for(i=0; i<__piom_nb_core; i++)
	{
	    /* Notify all the core that we need to modify the list */
	    __piom_core_status[i] &= ~PIOM_CORE_STATUS_BUSY;
	}

#ifdef PIOM_THREAD_ENABLED
#ifdef MARCEL
    ma_tasklet_enable(&server->poll_tasklet);
#endif	/* MARCEL */
      piom_server_unlock_reentrant(server, lock);
#endif /* PIOM_THREAD_ENABLED MARCEL */

    LOG_RETURN(0);

    return 0;
}
