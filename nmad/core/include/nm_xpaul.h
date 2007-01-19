/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

#ifdef XPAULETTE

#ifndef NM_XPAUL_H
#define NM_XPAUL_H

#include "xpaul.h"
#include "nm_public.h"


struct nm_gate;
struct nm_sched;

int
nm_process_complete_send_rq(struct nm_gate	*p_gate,
                            struct nm_pkt_wrap	*p_pw,
                            int		 	_err);

int
nm_process_complete_recv_rq(struct nm_sched	*p_sched,
                            struct nm_pkt_wrap	*p_pw,
                            int		 	_err);

typedef enum
{
	CNX,
	PKT_WRAP
} nm_xpaul_poll_op_t;

typedef enum{
	SEND,
	RECV
} nm_xpaul_op_t;

struct nm_xpaul_data;

typedef int (nm_xpaul_complete_callback_t) (struct nm_xpaul_data *xp_data);

struct nm_xpaul_data{
	struct xpaul_req               inst;
	nm_xpaul_poll_op_t             which;
	nm_xpaul_op_t                  method;	
	struct nm_core                *p_core;
	nm_xpaul_complete_callback_t  *complete_callback;
#ifdef MARCEL
	marcel_sem_t                   sem;
#else
	int                            done;
#endif /* MARCEL */
};

int 
nm_xpaul_init(struct nm_core * p_core);

/* Use XPaulette to wait for a cnx (end_(un)packing, sr_wait, etc.) */
int 
nm_xpaul_wait(struct nm_xpaul_data     *cnx);

/* Polling functions */

/* Main polling function */
int 
nm_xpaul_poll(xpaul_server_t            server,
	      xpaul_op_t                _op,
	      xpaul_req_t               req, 
	      int                       nb_ev, 
	      int                       option);

/* Check the state of the cnx */
int 
nm_xpaul_mainpoll(struct nm_xpaul_data *cnx);

int
nm_xpaul_poll_send(struct nm_pkt_wrap  *p_pw);

int
nm_xpaul_poll_recv(struct nm_pkt_wrap  *p_pw);

/* Posting functions */
int
nm_xpaul_post_all(struct nm_xpaul_data *cnx);

int 
nm_xpaul_post_all_recv(struct nm_xpaul_data *cnx);

int 
nm_xpaul_post_all_send(struct nm_xpaul_data *cnx, 
		       struct nm_gate *p_gate);



int 
nm_xpaul_post_recv(struct nm_xpaul_data *cnx, 
		   struct nm_sched	*p_sched,
		   p_tbx_slist_t 	 post_slist,
		   p_tbx_slist_t 	 pending_slist);

int
nm_xpaul_post_send(struct nm_xpaul_data *cnx,
		   struct nm_gate       *p_gate);

#ifdef MA__LWPS
int 
nm_xpaul_block(xpaul_server_t server,
	       xpaul_op_t     _op,
	       xpaul_req_t    req, 
	       int            nb_ev, 
	       int            option);
int 
nm_xpaul_block_send(struct nm_xpaul_data *cnx,
		    struct nm_gate       *p_gate);

int 
nm_xpaul_block_recv(struct nm_xpaul_data *cnx, 
		   struct nm_sched	 *p_sched,
		   p_tbx_slist_t 	  post_slist,
		   p_tbx_slist_t 	  pending_slist);
#endif

#endif /* NM_XPAUL_H */

#endif /* XPAULETTE */
