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

/* Communications specialized LWP */
struct piom_comm_lwp {
	int vp_nb; 		/* LWP number */
	int fds[2];		/* file descriptor used to unblock the lwp  */
	struct list_head chain_lwp_ready;
	struct list_head chain_lwp_working;
	piom_server_t server;
	marcel_t pid;
};

/* Wake up a LWP to export a request and perform a blocking call */
int 
piom_wakeup_lwp(piom_server_t server, piom_req_t req);
#endif

/* Create a pool of nb_lwps LWPs */
void piom_server_start_lwp(piom_server_t server, unsigned nb_lwps);

#endif	/* PIOM_BLOCK_H */
