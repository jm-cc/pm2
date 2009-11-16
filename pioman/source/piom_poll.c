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

static int job_scheduled = 0;

/* Try to group requests (polling version) */
int 
__piom_poll_group(piom_server_t server,
		  piom_req_t req)
{
    PIOM_BUG_ON(!server->req_poll_grouped_nb);
	
    if (server->funcs[PIOM_FUNCTYPE_POLL_GROUP].func &&
	(server->req_poll_grouped_nb > 1
	 || !server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func)) {
	PIOM_LOGF
	    ("Factorizing %i polling(s) with POLL_GROUP for [%s]\n",
	     server->req_poll_grouped_nb, server->name);
	(*server->funcs[PIOM_FUNCTYPE_POLL_GROUP].func)
	    (server, PIOM_FUNCTYPE_POLL_POLLONE,
	     req, server->req_poll_grouped_nb, 0);
	return 1;
    }
    PIOM_LOGF("No need to group %i polling(s) for [%s]\n",
	      server->req_poll_grouped_nb, server->name);
    return 0;
}

/* Are there requests to poll on this server ? */
int 
__piom_need_poll(piom_server_t server)
{
    return server->req_poll_grouped_nb;
}

/* polling is done, reset the timer */
__tbx_inline__ 
static void
__piom_update_timer(piom_server_t server)
{
    if (server->poll_points & PIOM_POLL_AT_TIMER_SIG) {
	PIOM_LOGF("Update timer polling for [%s] at period %i\n",
		  server->name, server->period);
#ifdef MARCEL
	ma_mod_timer(&server->poll_timer,
		     ma_jiffies + server->period);
#else
	/* TODO: timer support without Marcel */
#endif	/* MARCEL */
    }
}

/* Checks to see if some polling jobs should be done.
 */
/* TODO: check if we should use a blocking syscall
 */
void 
piom_check_polling_for(piom_server_t server)
{
    
    if(server->state != PIOM_SERVER_STATE_LAUNCHED){
	return;
    }
    
    int nb = __piom_need_poll(server);
    piom_req_t req  ;	
#ifdef PIOM__DEBUG
    static int count = 0;

    PIOM_LOGF("Check polling for [%s]\n", server->name);

    if (!count--) {
	PIOM_LOGF("Check polling 50000 for [%s]\n", server->name);
	count = 50000;
    }
#endif	/* PIOM__DEBUG */
    if (!nb)
	return;

    server->max_priority = PIOM_REQ_PRIORITY_LOWEST;
    server->registered_req_not_yet_polled = 0;
#ifdef PIOM_THREAD_ENABLED
    piom_thread_t owner = piom_ensure_lock_server(server);
#endif
    nb = __piom_need_poll(server);
    if (!nb){
	piom_restore_lock_server_locked(server, owner);	
	return;
    }

    if (nb == 1 && server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func){
	req=tbx_fast_list_entry(server->list_req_poll_grouped.next,
		       struct piom_req, chain_req_grouped);
	/* Only one request, use the fast_poll method */
#ifdef PIOM_BLOCKING_CALLS
	if(tbx_fast_list_entry(server->list_req_poll_grouped.next,
		      struct piom_req, chain_req_grouped)->func_to_use != PIOM_FUNC_SYSCALL ) 
#endif	/* PIOM_BLOCKING_CALLS */
	    {
		(*server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func)
		    (server, PIOM_FUNCTYPE_POLL_POLLONE,
		     req, nb, PIOM_OPT_REQ_IS_GROUPED);
	    }
    } else if (server->funcs[PIOM_FUNCTYPE_POLL_POLLANY].func) {
	/* poll all the requests with the pollanny callback */
#ifdef PIOM_THREAD_ENABLED
	__piom_unlock_server(server);	
#endif		
	(*server->funcs[PIOM_FUNCTYPE_POLL_POLLANY].func)
	    (server, PIOM_FUNCTYPE_POLL_POLLANY, NULL, nb, 0);
#ifdef PIOM_THREAD_ENABLED
	__piom_lock_server(server, PIOM_SELF);
#endif

    } else if (server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func) {
	piom_req_t req, tmp;
	/* for each requests, call the fast_poll callback */

       FOREACH_REQ_POLL_BASE_SAFE(req, tmp, server) {
            if(! (req->state & PIOM_STATE_OCCURED))
                /* If a high priority request is completed, do not poll low priority
                   requests for now */
                if(req->priority >= server->max_priority) {
                    if(req->func_to_use != PIOM_FUNC_SYSCALL){
#ifdef PIOM_THREAD_ENABLED
			__piom_unlock_server(server);	
#endif
                        (*server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func)
                            (server, PIOM_FUNCTYPE_POLL_POLLONE,
                             req, nb,
                             PIOM_OPT_REQ_IS_GROUPED
                             | PIOM_OPT_REQ_ITER);
#ifdef PIOM_THREAD_ENABLED
			__piom_lock_server(server, PIOM_SELF);
#endif

                    }
                }
       }

    } else {
	/* no polling method available ! */
	PIOM_EXCEPTION_RAISE(PIOM_PROGRAM_ERROR);
    }
 out:
    /* Handle completed requests */
    __piom_manage_ready(server);
    if (nb != __piom_need_poll(server)) {
	/* The number of polling requests have change, do we need to poll again ? */
	if (!__piom_need_poll(server)) {
	    __piom_poll_stop(server);
#ifdef PIOM_THREAD_ENABLED
	    piom_restore_lock_server_locked(server, owner);
#endif
	    return;
	} else {
	    __piom_poll_group(server, NULL);
	}
    }
#ifdef PIOM_THREAD_ENABLED
    piom_restore_lock_server_locked(server, owner);
#endif
    __piom_update_timer(server);
    return;
}

/* Function called by a tasklet to poll
 */
void 
piom_poll_from_tasklet(unsigned long hid)
{
    piom_server_t server = (piom_server_t) hid;
    PIOM_LOGF("Tasklet function for [%s]\n", server->name);

    if(server->state == PIOM_SERVER_STATE_HALTED || server->state == PIOM_SERVER_STATE_NONE) {
	PIOM_LOGF("polling on a dead server\n");
	return;
    }

    piom_check_polling_for(server);
#ifdef MARCEL
    job_scheduled=0;
#endif	/* MARCEL */
    return;
}

/* This function is called by the timer interrupt, just schedule a tasklet */
void 
piom_poll_timer(unsigned long hid)
{
    piom_server_t server = (piom_server_t) hid;

    if(server->state == PIOM_SERVER_STATE_HALTED)
	return;
    PIOM_LOGF("Timer function for [%s]\n", server->name);
#ifdef PIOM_USE_TASKLETS
    ma_tasklet_schedule(&server->poll_tasklet);
#else
    piom_check_polling_for(server);
#endif	/* PIOM_USE_TASKLETS */
    return;
}

//#ifdef MARCEL_REMOTE_TASKLETS
#if 1

/* Schedules tasklets for each server
 * this function is called on some triggers (mainly cpu idleness and timer interrupt) by Marcel
 */
void 
__piom_check_polling(unsigned polling_point)
{
#ifdef PIOM_ENABLE_SHM
    if(piom_shs_polling_is_required())
	piom_shs_poll(); 
#endif

#ifdef PIOM_ENABLE_LTASKS
	piom_ltask_schedule();
#endif
    if( tbx_fast_list_empty(&piom_list_poll))
	return;	

    PROF_IN();
    piom_server_t server, bak=NULL;

    /* TODO: if the vpset does not match idle core, remote schedule the tasklet 
       (or schedule the tasklet somewhere else) */
    tbx_fast_list_for_each_entry_safe(server, bak, &piom_list_poll, chain_poll) {		
#ifdef MARCEL_REMOTE_TASKLETS
	if( marcel_vpset_isset(&(server->poll_tasklet.vp_set),
			       marcel_current_vp())) 
#endif
	    {
		if (server->poll_points & polling_point) {
		    /* disable bh to avoid piom_poll_timer to be invoked 
		       while scheduling the tasklet */
		    ma_local_bh_disable();
#ifdef PIOM_USE_TASKLETS
		    ma_tasklet_schedule(&server->poll_tasklet);
#else
		    piom_check_polling_for(server);
#endif /* PIOM_USE_TASKLETS */
		    ma_local_bh_enable();
		}
	    }
    }
    PROF_OUT();
}

#else

/* Schedules tasklets for each server
 * this function is called on some triggers (mainly cpu idleness and timer interrupt) by Marcel
 */
void 
__piom_check_polling(unsigned polling_point)
{
#ifdef MARCEL
    if( job_scheduled || tbx_fast_list_empty(&piom_list_poll))
	return;
    int scheduled=0;
#endif	/* MARCEL */

    PROF_IN();
    piom_server_t server, bak=NULL;

    tbx_fast_list_for_each_entry(server, &piom_list_poll, chain_poll) {
	if(bak==server){
	    break;	
	}
	if (server->poll_points & polling_point) {		
#ifdef MARCEL		
	    ma_local_bh_disable();
	    job_scheduled=1;
	    scheduled++;
	    ma_tasklet_schedule(&server->poll_tasklet);
	    ma_local_bh_enable();
#else
	    piom_check_polling_for(server);
#endif /* MARCEL */
	}
    }

#ifdef MARCEL
    if(!scheduled)
	{
	    tbx_fast_list_for_each_entry(server, &piom_list_poll, chain_poll) {
		ma_local_bh_disable();
		job_scheduled=1;
		ma_tasklet_schedule(&server->poll_tasklet);
		ma_local_bh_enable();
	    }
	}
#endif /* MARCEL */
    PROF_OUT();
}
#endif /* MARCEL_REMOTE_TASKLETS */

/* Poll 'req' for 'usec' micro secondes*/
void 
piom_poll_req(piom_req_t req, unsigned usec)
{
    if(!req->server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func) 
	/* TODO : allow polling with other functions (pollany, etc.) */
	return; 
#ifdef MARCEL
    marcel_task_t *lock;
    lock=piom_ensure_trylock_from_tasklet(req->server);
#endif	/* MARCEL */

    tbx_tick_t t1, t2;
    TBX_GET_TICK(t1);
    int i=0;
    do {
	(*req->server->funcs[PIOM_FUNCTYPE_POLL_POLLONE].func)
	    (req->server, PIOM_FUNCTYPE_POLL_POLLONE,
	     req, 1, PIOM_OPT_REQ_ITER);
	
	_piom_spin_lock_softirq(&req->server->req_ready_lock); 
	if(! tbx_fast_list_empty(&req->chain_req_ready)) {
	    /* Request succeeded */
	    TBX_GET_TICK(t2);
	    req->state |= PIOM_STATE_OCCURED;
	    __piom_wake_req_waiters(req->server, req, 0);
	    if (!(req->state & PIOM_STATE_NO_WAKE_SERVER))
		__piom_add_success_req(req->server, req);
			
	    tbx_fast_list_del_init(&req->chain_req_ready);
	    if (req->state & PIOM_STATE_ONE_SHOT) {
		__piom_unregister_poll(req->server, req);
		__piom_unregister(req->server, req);
	    }
	    _piom_spin_unlock_softirq(&req->server->req_ready_lock); 
	    break;
	}
	_piom_spin_unlock_softirq(&req->server->req_ready_lock); 
	TBX_GET_TICK(t2);
	i++;
    } while(TBX_TIMING_DELAY(t1, t2) < usec);
#ifdef MARCEL
    piom_restore_trylocked_from_tasklet(req->server, lock);
#endif	/* MARCEL */
}

/* Force polling for a specified server */
void 
piom_poll_force(piom_server_t server)
{
    LOG_IN();
    PIOM_LOGF("Poll forced for [%s]\n", server->name);
    if (__piom_need_poll(server)) {
#ifdef PIOM_USE_TASKLETS
	ma_tasklet_schedule(&server->poll_tasklet);
#else
	piom_check_polling_for(server);
#endif	/* PIOM_USE_TASKLETS */
    }
    LOG_OUT();
}

/* Force synchronous polling for a specified server 
 * (ie: not polling from a tasklet)
 */
void 
piom_poll_force_sync(piom_server_t server)
{
    LOG_IN();
    PIOM_LOGF("Sync poll forced for [%s]\n", server->name);
#ifdef MARCEL
    marcel_task_t *lock;
    lock = piom_ensure_lock_server(server);
#endif	/* MARCEL */
    piom_check_polling_for(server);

#ifdef MARCEL
    piom_restore_lock_server_locked(server, lock);
#endif	/* MARCEL */
    LOG_OUT();
}
