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

#ifndef PIOM_BLOCK_H
#define PIOM_BLOCK_H

#ifdef PIOM_BLOCKING_CALLS
/* LWP used to perform blocking calls without blocking over user-lveel threads */
typedef struct piom_comm_lwp *piom_comm_lwp_t;

/* Available kinds of callbacks */
typedef enum {
       PIOM_LWP_STATE_NONE,    /* LWP not initialized */
       PIOM_LWP_STATE_INITIALIZED,/* LWP initialized but not running*/
       PIOM_LWP_STATE_LAUNCHED, /* LWP is running, but not yet ready */
       PIOM_LWP_STATE_WORKING, /* LWP is running but performing a blocling call*/
       PIOM_LWP_STATE_READY    /* LWP is running and waiting for work */
} piom_lwp_state_t;

/* Communications specialized LWP */
struct piom_comm_lwp {
	int vp_nb; 		/* LWP number */
	int fds[2];		/* file descriptor used to unblock the lwp  */
	struct tbx_fast_list_head chain_lwp_ready;
	struct tbx_fast_list_head chain_lwp_working;
	piom_server_t server;
	marcel_t pid;
        volatile piom_lwp_state_t state;    
};

int __piom_lower_my_priority();
void __piom_higher_my_priority(int prio);

/* Wake up a LWP to export a request and perform a blocking call */
int piom_wakeup_lwp(piom_server_t server, piom_req_t req);
#endif

/* Create a pool of nb_lwps LWPs */
void piom_server_start_lwp(piom_server_t server, unsigned nb_lwps);


/* Check if it is interesting to use a blocking syscall 
 * return 0 if we should use polling
 * 1 owerwise
 */
int 
__piom_need_export(piom_server_t server, piom_req_t req,
		   piom_wait_t wait, piom_time_t timeout);

int 
__piom_block_group(piom_server_t server,
		   piom_req_t req);


#endif	/* PIOM_BLOCK_H */
