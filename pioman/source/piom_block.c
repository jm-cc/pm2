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

//#define EXPORT_THREADS 1
#include "pioman.h"

/* Check if it is interesting to use a blocking syscall 
 * return 0 if we should use polling
 * 1 owerwise
 */
int 
__piom_need_export(piom_server_t server, piom_req_t req,
		   piom_wait_t wait, piom_time_t timeout)
{
    /*
     *  TODO: this should be taken into account:
     *   - the polling function's speed
     *   - the behavior of previous requests
     *   - the priority of the request/thread
     */
#ifndef PIOM_BLOCKING_CALLS
    return 0;
#else
    /* the request is already completed */
    if (req->state & PIOM_STATE_OCCURED)
	return 0;
    /* there is no blocking function */
    if (!server->funcs[PIOM_FUNCTYPE_BLOCK_WAITONE].func)
	return 0;
    /* the application forces the polling */
    if (req->func_to_use == PIOM_FUNC_POLLING)
	return 0;
    
    /* the application forces the syscall */
    if(req->func_to_use==PIOM_FUNC_SYSCALL)
	return 1;

    /* check wether a core is idle */
    int i;
    for(i=0;i<marcel_nbvps();i++) {
	if(marcel_idle_lwp(ma_get_lwp_by_vpnum(i)))
	    return 0;
    }
#endif	/* MARCEL */
    return 1;
}


/* Try to group requests (syscall version) */
int 
__piom_block_group(piom_server_t server,
		   piom_req_t req)
{
    PIOM_BUG_ON(!server->req_block_grouped_nb);
	
    if (server->funcs[PIOM_FUNCTYPE_BLOCK_GROUP].func &&
	(server->req_block_grouped_nb > 1
	 || !server->funcs[PIOM_FUNCTYPE_BLOCK_WAITONE].func)) {
	PIOM_LOGF
	    ("Factorizing %i blocking(s) with BLOCK_GROUP for [%s]\n",
	     server->req_block_grouped_nb, server->name);
	(*server->funcs[PIOM_FUNCTYPE_BLOCK_GROUP].func)
	    (server, PIOM_FUNCTYPE_BLOCK_WAITONE,
	     req, server->req_block_grouped_nb, 0);
		
	//		req->state |= PIOM_STATE_GROUPED;
	return 1;
    }
    PIOM_LOGF("No need to group %i polling(s) for [%s]\n",
	      server->req_block_grouped_nb, server->name);
    return 0;
}

#ifdef PIOM_BLOCKING_CALLS
/* Initialize a lwp */
__tbx_inline__ 
static void 
__piom_init_lwp(piom_comm_lwp_t lwp)
{
    INIT_LIST_HEAD(&lwp->chain_lwp_ready);
    INIT_LIST_HEAD(&lwp->chain_lwp_working);
    lwp->vp_nb = -1;
    lwp->fds[0] = lwp->fds[1] = -1;
    lwp->server = NULL;
    lwp->pid = NULL;
}
#endif /* PIOM_BLOCKING_CALLS */

#ifdef PIOM_BLOCKING_CALLS
/* Wake up a LWP to export a request and perform a blocking call */
int 
piom_wakeup_lwp(piom_server_t server, piom_req_t req) 
{
    piom_comm_lwp_t lwp=NULL;	
    int foo=42;

    __piom_register_block(server, req);

    /* Pick up a LWP from the pool */
    _piom_spin_lock(&server->lwp_lock);
    do {
	if(!list_empty(server->list_lwp_ready.next))
	    lwp = list_entry(server->list_lwp_ready.next, struct piom_comm_lwp, chain_lwp_ready);
	else if( server->stopable && !list_empty(server->list_lwp_working.next))
	    lwp = list_entry(server->list_lwp_working.next, struct piom_comm_lwp, chain_lwp_working);
	else {
	    /* Create another LWP (warning: this is *very* expensive !) */
	    _piom_spin_unlock(&server->lwp_lock);	
	    piom_server_start_lwp(server, 1);
	    _piom_spin_lock(&server->lwp_lock);
	}
    }while(! lwp);
    _piom_spin_unlock(&server->lwp_lock);

    /* wake up the LWP */
    PIOM_LOGF("Waiking up comm LWP #%d\n",lwp->vp_nb);
    PROF_EVENT(writing_tube);
    write(lwp->fds[1], &foo, 1);
    PROF_EVENT(writing_tube_done);

#ifdef EXPORT_THREADS
    /* In that case, the LWP we just woke up is in charge of rescuing the 
       threads so that we can perform the blocking call here  */
    marcel_task_t *lock;	
    piom_req_t prev, tmp, req2=NULL;
    if(server->funcs[PIOM_FUNCTYPE_BLOCK_WAITANY].func && server->req_block_grouped_nb>1)
	{
	    __piom_tryunlock_server(server);
	    /* Perform one blocking call for all the requests */
	    (*server->funcs[PIOM_FUNCTYPE_BLOCK_WAITANY].
	     func) (server, PIOM_FUNCTYPE_BLOCK_WAITANY, req2,
		    server->req_block_grouped_nb, lwp->fds[0]);
		
	    lock = piom_ensure_trylock_server(server);
	}
    else{
	if(server->funcs[PIOM_FUNCTYPE_BLOCK_WAITONE].func)
	    {
		prev=NULL;
		FOREACH_REQ_TO_EXPORT_BASE_SAFE(req2, tmp, server) {
		    if(req2==prev)
			break;
		    PIOM_WARN_ON(req2->state & PIOM_STATE_OCCURED);
		    /* Set the request as 'exported' */
		    list_del_init(&req2->chain_req_to_export);
		    list_add(&req2->chain_req_exported, &server->list_req_exported);
		    req2->state|=PIOM_STATE_EXPORTED;
				
		    __piom_tryunlock_server(server);
					
		    /* call the blocking callback */		
		    (*server->funcs[PIOM_FUNCTYPE_BLOCK_WAITONE].
		     func) (server, PIOM_FUNCTYPE_BLOCK_WAITONE, req2,
			    server->req_poll_grouped_nb, lwp->fds[0]);
				
		    list_del_init(&req2->chain_req_exported);
				
		    lock = piom_ensure_trylock_server(server);
				
		    prev=req;
		    break;
		}
	    }
	else 
	    PIOM_EXCEPTION_RAISE(PIOM_CALLBACK_ERROR);
		
    }
    /* handle completed requests */
    __piom_manage_ready(server);
    lock = piom_ensure_trylock_server(server);
	
#endif	/* EXPORT_THREADS */
    return 0;
}

/* TODO: Specify the number of LWP at the server's initialization */
/* Wait for a command (using a pipe) and call blocking callbacks */
void *
__piom_syscall_loop(void * param)
{
    piom_comm_lwp_t lwp=(piom_comm_lwp_t) param;
    piom_server_t server=lwp->server;
    piom_req_t prev, tmp, req=NULL;
    int foo=0;
    marcel_task_t *lock;

    lwp->state=PIOM_LWP_STATE_LAUNCHED;
#ifdef EXPORT_THREADS

    /* On met la priorite au minimum */
    /* Set priority to minimum to avoid the LWP to be woken up as soon
       as it is unblocked */
    struct sched_param pthread_param;
    if(sched_getparam(0, &pthread_param)) {
	fprintf(stderr, "couldn't get my priority !\n");
	switch(errno){
	case EINVAL: 
	    fprintf(stderr, "invalid param\n"); 
	    break;
	case EPERM:
	    fprintf(stderr, " The calling process does not have appropriate privileges \n");
	    break;
	case ESRCH:
	    fprintf(stderr, "The process whose ID is 0 is unknown\n");
	    break;
	}
    }
    if(nice(10) == -1){
	fprintf(stderr, "couldn't lower my priority\n");
    }
#endif	/* EXPORT_THREADS */
    
    /* Main loop: wait for commands until the server is halted */
    do {
	_piom_spin_lock(&server->lwp_lock);
	list_del_init(&lwp->chain_lwp_working);
	list_add_tail(&lwp->chain_lwp_ready, &server->list_lwp_ready);
	lwp->state=PIOM_LWP_STATE_READY;
	_piom_spin_unlock(&server->lwp_lock);

	/* Wait for a command */		
	PROF_EVENT2(syscall_waiting, lwp->vp_nb, lwp->fds[0]);
#ifdef EXPORT_THREADS
	do {
	    read(lwp->fds[0], &foo, 1); 
	} while(1); 
#else
	read(lwp->fds[0], &foo, 1); 
#endif	/* EXPORT_THREADS */

    process_blocking_call:
	PROF_EVENT2(syscall_read_done, lwp->vp_nb, lwp->fds[0]);
	PIOM_LOGF("Comm LWP #%d is working\n",lwp->vp_nb);

	_piom_spin_lock(&server->lwp_lock);
	/* Check the server's state */
	if( lwp->server->state == PIOM_SERVER_STATE_HALTED ){
	    list_del_init(&lwp->chain_lwp_ready);
	    _piom_spin_unlock(&server->lwp_lock);
	    return NULL;
	}
		
	/* Remove the LWP from the ready list and add it to the working list */
	lwp->state=PIOM_LWP_STATE_WORKING;
	list_del_init(&lwp->chain_lwp_ready);
	list_add(&lwp->chain_lwp_working, &server->list_lwp_working);

	_piom_spin_unlock(&server->lwp_lock);
	lock = piom_ensure_trylock_server(server);

	/* Get the requests to be posted */
	if(server->funcs[PIOM_FUNCTYPE_BLOCK_WAITANY].func && server->req_block_grouped_nb>1)
	    {
		__piom_tryunlock_server(server);
		/* Wait for a group of requests */
		(*server->funcs[PIOM_FUNCTYPE_BLOCK_WAITANY].
		 func) (server, PIOM_FUNCTYPE_BLOCK_WAITANY, req,
			server->req_block_grouped_nb, lwp->fds[0]);

		lock = piom_ensure_trylock_server(server);
	    }
	else{
	    if(server->funcs[PIOM_FUNCTYPE_BLOCK_WAITONE].func)
		{
		    prev=NULL;
		    FOREACH_REQ_TO_EXPORT_BASE_SAFE(req, tmp, server) {
			if(req==prev)
			    break;
			PIOM_WARN_ON(req->state &PIOM_STATE_OCCURED);
			/* Set the request as exported */
			list_del_init(&req->chain_req_to_export);
			list_add(&req->chain_req_exported, &server->list_req_exported);
			req->state|=PIOM_STATE_EXPORTED;
					
			__piom_tryunlock_server(server);
					
			/* call the blocking callback function */		
			(*server->funcs[PIOM_FUNCTYPE_BLOCK_WAITONE].
			 func) (server, PIOM_FUNCTYPE_BLOCK_WAITONE, req,
				server->req_poll_grouped_nb, lwp->fds[0]);
					
			list_del_init(&req->chain_req_exported);
					
			lock = piom_ensure_trylock_server(server);

			prev=req;
			break;
		    }
		}
	    else 
		PIOM_EXCEPTION_RAISE(PIOM_CALLBACK_ERROR);

	}
	/* handle completed requests */
	__piom_manage_ready(server);
	__piom_tryunlock_server(server);

    } while(lwp->server->state!=PIOM_SERVER_STATE_HALTED);
    return NULL;
}


/* Create nb_lwps new LWP */
void 
piom_server_start_lwp(piom_server_t server, unsigned nb_lwps)
{
    int i;
    marcel_attr_t attr;
	
    marcel_attr_init(&attr);
    marcel_attr_setdetachstate(&attr, tbx_true);
    marcel_attr_setname(&attr, "piom_receiver");

    /* TODO: what happens if this function is called several times ? (are the first LWPs lost ?) */
    INIT_LIST_HEAD(&server->list_lwp_working);
    INIT_LIST_HEAD(&server->list_lwp_ready);
    for(i=0;i<nb_lwps;i++)
	{
	    PIOM_LOGF("Creating a lwp for server %p\n", server);
	    piom_comm_lwp_t lwp=(piom_comm_lwp_t) malloc(sizeof(struct piom_comm_lwp));
	    __piom_init_lwp(lwp);
	    lwp->state=PIOM_LWP_STATE_NONE;
	    lwp->server=server;

	    /* The LWP is set as Working during initialization */
	    _piom_spin_lock(&server->lwp_lock);
	    list_add(&lwp->chain_lwp_working, &server->list_lwp_working);
	    _piom_spin_unlock(&server->lwp_lock);

	    lwp->vp_nb = marcel_add_lwp();
	    marcel_attr_setvpset(&attr, MARCEL_VPSET_VP(lwp->vp_nb));

	    if (pipe(lwp->fds) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	    }
	    lwp->state=PIOM_LWP_STATE_INITIALIZED;
	    /* Create a (user-level) thread on top of this LWP */
	    marcel_create(&lwp->pid, &attr, (void*) &__piom_syscall_loop, lwp);
	    /* Wait for the lwp to be ready before continuing */
	    while(lwp->state!=PIOM_LWP_STATE_READY);
	}

}
#else
/* Create nb_lwps new LWP */
void 
piom_server_start_lwp(piom_server_t server, unsigned nb_lwps)
{

}
#endif /* PIOM_BLOCKING_CALLS */
