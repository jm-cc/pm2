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

#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <tbx.h>

#include "nm_private.h"

#include "xpaul.h"
#include "nm_xpaul.h"


int 
nm_xpaul_init(struct nm_core * p_core){
	/* Initialisation du serveur */
	LOG_IN();
	p_core->xpaul_server=(xpaul_server_t) malloc(sizeof(struct xpaul_server));
	xpaul_server_init(p_core->xpaul_server, "NMad IO Server");

#ifdef MA__LWPS
	/* TODO : lancer plus de LWP (un par requete maitre + les autres...) */
	xpaul_server_start_lwp(p_core->xpaul_server, 1);
#endif /* MA__LWPS */

	/* TODO : possibilité d'interrompre le LWP ? */
	p_core->xpaul_server->stopable=0;

	xpaul_server_set_poll_settings(p_core->xpaul_server,
				       XPAUL_POLL_AT_TIMER_SIG
				       | XPAUL_POLL_AT_IDLE, 1, -1);

	/* Définition des callbacks */
	xpaul_server_add_callback(p_core->xpaul_server,
				  XPAUL_FUNCTYPE_POLL_POLLONE,
				  (xpaul_pcallback_t) {
		.func = &nm_xpaul_poll,
			 .speed = XPAUL_CALLBACK_NORMAL_SPEED});

	/* TODO : définition des callbacks bloquants */
	xpaul_server_start(p_core->xpaul_server);
	return 0;
}

/* Use XPaulette to wait for a cnx (end_(un)packing, sr_wait, etc.) */
int 
nm_xpaul_wait(struct nm_xpaul_data     *cnx) {
	/* TODO : creer la requete 'maitre' avant de soumettre ?  */
#ifdef MA__LWPS
	cnx.inst.func_to_use=XPAUL_FUNC_SYSCALL;
#endif
#ifdef MARCEL
	marcel_sem_init(&cnx->sem,0);
#else
	cnx->done=0;
#endif

	/* Poste les requetes */

	int err;
	int g;
	/* send */
	for (g = 0; g < cnx->p_core->nb_gates; g++) {
		err= nm_xpaul_post_all_send(cnx, cnx->p_core->gate_array + g);
		
                if (err < 0) {
                        NM_DISPF("nm_post_all_send(%d) returned %d",
				 g, err);
                }
        }

	/* Receive */
	err = nm_xpaul_post_all_recv(cnx);
	if(err<0) {
                NM_DISPF("nm_post_all_recv(%p) returned %d",
			 cnx, err);
        }

	/* XPaulette fait le reste ! */
	struct xpaul_wait wait;	
	xpaul_wait(cnx->p_core->xpaul_server, 
		   &cnx->inst, 
		   &wait, 
		   0);

	return NM_ESUCCESS;
}

/* Polling functions */

/* Main polling function */
int 
nm_xpaul_poll(xpaul_server_t            server,
	      xpaul_op_t                _op,
	      xpaul_req_t               req, 
	      int                       nb_ev, 
	      int                       option) {
	
	struct nm_xpaul_data * xp_data=struct_up(req, struct nm_xpaul_data, inst);
	struct nm_pkt_wrap * pkt= struct_up(xp_data, struct nm_pkt_wrap, nm_xp_data);

	if(pkt->nm_xp_data.which == CNX) {
		/* Requete 'Maitre' */
		struct nm_xpaul_data *cnx=struct_up(req, struct nm_xpaul_data, inst);
		return nm_xpaul_mainpoll(cnx);

	} else {
		/* Requete 'normale' */
		int err;
		if(pkt->nm_xp_data.method == SEND)
			err = nm_xpaul_poll_send(pkt);
		else
			err = nm_xpaul_poll_recv(pkt);

		/* poste les nouvelles requetes  */
		nm_xpaul_post_all(pkt->cnx);

		if((*pkt->cnx->complete_callback)(pkt->cnx)){
#ifdef MA__LWPS
			marcel_sem_V(&pkt->cnx->sem);
#else
			pkt->cnx->done=1;
#endif
		}

		return err;
	}
}


/* Check the state of the cnx */
int 
nm_xpaul_mainpoll(struct nm_xpaul_data *cnx) {

	if(!cnx->done){ // il reste des requetes
			/* Poste les éventuelles nouvelles requetes */
		return nm_xpaul_post_all(cnx);
	} else {
		/* cnx fini, on retourne */
		xpaul_req_success(&cnx->inst);
		return -NM_ESUCCESS;
	}
}

int
nm_xpaul_poll_send(struct nm_pkt_wrap  *p_pw) {

        const int req_nb = p_pw->p_gate->out_req_nb;
	int i;
	int j=0;
	
	p_pw->err = p_pw->p_drv->ops.poll_send_iov(p_pw);
	
	if (p_pw->err != -NM_EAGAIN){
		
		struct nm_pkt_wrap *pkt;
		/* met a jour la requete */
		xpaul_req_success(&p_pw->nm_xp_data.inst);
		nm_process_complete_send_rq(p_pw->p_gate, p_pw, p_pw->err);

		for(i=0;i<req_nb;i++) { /* supprime la requete de la liste */
			pkt = p_pw->p_gate->out_req_list[i];
			if (pkt->err == -NM_EAGAIN) {
				pkt->p_gate->out_req_list[j] = pkt->p_gate->out_req_list[i];
				j++;
			}
		}
	}
	return p_pw->err;
}

int 
nm_xpaul_poll_recv(struct nm_pkt_wrap  *p_pw) {
	
	NM_TRACEF("polling inbound request: gate %d, drv %d, trk %d, proto %d, seq %d",
                     p_pw->p_gate?p_pw->p_gate->id:-1,
                     p_pw->p_drv->id,
                     p_pw->p_trk->id,
                     p_pw->proto_id,
                     p_pw->seq);

	p_pw->err = p_pw->p_drv->ops.poll_recv_iov(p_pw);
	
	/* process poll command status				*/
	if (p_pw->err == -NM_EAGAIN) {
		/* not complete, try again later
		   - leave the request in the list and go to next
		*/
		return p_pw->err;
	}

	if (p_pw->err != NM_ESUCCESS) {
		NM_DISPF("drv->poll_recv returned %d", p_pw->err);
	}
	xpaul_req_success(&p_pw->nm_xp_data.inst);
	/* process complete request */
	p_pw->err = nm_process_complete_recv_rq(p_pw->p_drv->p_core->p_sched, p_pw, p_pw->err);
	
	if (p_pw->err < 0) { // possible ?
		NM_DISPF("nm_process_complete_rq returned %d", p_pw->err);
	}

        return p_pw->err;
}


/* Posting functions */
int
nm_xpaul_post_all(struct nm_xpaul_data *cnx){
	int err;
	int g;
	/* send */
	for (g = 0; g < cnx->p_core->nb_gates; g++) {
		err= nm_xpaul_post_all_send(cnx, cnx->p_core->gate_array + g);

		if (err < 0) {
			NM_DISPF("nm_xpaul_post_all_send(%p, %d) returned %d",
				 cnx, g, err);
		}
	}

	/* Receive */
	err = nm_xpaul_post_all_recv(cnx);
	if(err<0) {
		NM_DISPF("nm_xpaul_post_all_recv(%p, %p) returned %d",
			 cnx, cnx->p_core, err);
	}
	return err;
}


int 
nm_xpaul_post_all_recv(struct nm_xpaul_data *cnx){

        struct nm_sched *p_sched = cnx->p_core->p_sched;
        int err;

        /* schedule new reception requests				*/
        err	= cnx->p_core->p_sched->ops.in_schedule(p_sched);
        if (err < 0) {
                NM_DISPF("sched.in_schedule returned %d", err);
        }

        /* post new requests 						*/

        /* aux requests */
        if (!tbx_slist_is_nil(p_sched->post_aux_recv_req)) {
                NM_TRACEF("posting inbound aux requests");
                err = nm_xpaul_post_recv(cnx, 
					 p_sched,
					 p_sched->post_aux_recv_req,
					 p_sched->pending_aux_recv_req);
                if (err < 0 && err != -NM_EAGAIN) {
                        NM_DISPF("nm_xpaul_post_recv (aux) returned %d", err);
                }
        }

        /* permissive requests */
        if (!tbx_slist_is_nil(p_sched->post_perm_recv_req)) {
                NM_TRACEF("posting inbound perm requests");
                err = nm_xpaul_post_recv(cnx, 
					 p_sched,
					 p_sched->post_perm_recv_req,
					 p_sched->pending_perm_recv_req);
                if (err < 0 && err != -NM_EAGAIN) {
                        NM_DISPF("nm_xpaul_post_recv (perm) returned %d", err);
                }
        }

        err = NM_ESUCCESS;

        return err;
}

int 
nm_xpaul_post_all_send(struct nm_xpaul_data *cnx, 
		       struct nm_gate *p_gate) {
	int	err;
 
        /* schedule new requests	*/
	err	= p_gate->p_sched->ops.out_schedule_gate(p_gate);
	if (err < 0) {
		NM_DISPF("sched.schedule_out returned %d", err);
	}

        /* post new requests	*/
        if (!tbx_slist_is_nil(p_gate->post_sched_out_list)) {
		NM_TRACEF("posting outbound requests");
		err	= nm_xpaul_post_send(cnx, p_gate);
		
		if (err < 0 && err != -NM_EAGAIN) {
			NM_DISPF("nm_xpaul_post_send returned %d", err);
		}
        }
	return NM_ESUCCESS;
}


int 
nm_xpaul_post_recv(struct nm_xpaul_data *cnx, 
		   struct nm_sched	*p_sched,
		   p_tbx_slist_t 	 post_slist,
		   p_tbx_slist_t 	 pending_slist) {

        int err;
	struct nm_pkt_wrap	*p_pw;
        tbx_slist_ref_to_head(post_slist);
        do {
		p_pw = tbx_slist_ref_get(post_slist);
		

                /* check track availability				*/
                /* a recv is already pending on this gate/track */
                if (p_pw->p_gate && p_pw->p_gtrk->p_in_rq)
                        goto next;

                /* ready to post request				*/

                /* update incoming request field in gate track if
                   the request specifies a gate */
                if (p_pw->p_gate) {
                        p_pw->p_gtrk->p_in_rq	= p_pw;
                }

                NM_TRACEF("posting new recv request: gate %d, drv %d, trk %d, proto %d, seq %d",
                     p_pw->p_gate?p_pw->p_gate->id:-1,
                     p_pw->p_drv->id,
                     p_pw->p_trk->id,
                     p_pw->proto_id,
                     p_pw->seq);

                /* post request */
                err = p_pw->p_drv->ops.post_recv_iov(p_pw);

                /* process post command status				*/

                /* driver immediately succeeded in receiving the packet? */
                if (err == -NM_EAGAIN) {
                        NM_TRACEF("new recv request pending: gate %d, drv %d, trk %d, proto %d, seq %d",
                             p_pw->p_gate?p_pw->p_gate->id:-1,
                             p_pw->p_drv->id,
                             p_pw->p_trk->id,
                             p_pw->proto_id,
                             p_pw->seq);

                        /* No, put it in pending list */
                        tbx_slist_append(pending_slist, p_pw);
			
			/* Submit the pkt_wrapper to XPaulette */
			xpaul_req_init(&p_pw->nm_xp_data.inst);
			
			p_pw->nm_xp_data.which = PKT_WRAP;
			p_pw->nm_xp_data.p_core=cnx->p_core;
			p_pw->nm_xp_data.method = RECV;
			p_pw->err = -NM_EAGAIN;
			p_pw->list=&pending_slist;
			p_pw->cnx=cnx;

			p_pw->nm_xp_data.inst.state|=XPAUL_STATE_DONT_POLL_FIRST|XPAUL_STATE_ONE_SHOT;
			/* TODO : implementer les syscall */
#warning XPAUL_TODO : implementer les syscalls
//		if(! (p_pw->p_drv->cap->has_syscall))
			p_pw->nm_xp_data.inst.func_to_use=XPAUL_FUNC_POLLING;
			xpaul_req_submit(p_pw->p_gate->p_core->xpaul_server, &p_pw->nm_xp_data.inst);

                } else {
                        NM_TRACEF("request completed immediately");

                        /* Yes, request complete, process it */
                        if (err != NM_ESUCCESS) {
                                NM_DISPF("drv->post_recv returned %d", err);
                        }

                        err = nm_process_complete_recv_rq(p_sched, p_pw, err);
                        if (err < 0) {
                                NM_DISPF("nm_process_complete_rq returned %d", err);
                        }

			if((*cnx->complete_callback)(cnx)){ // pour l'instant
#ifdef MA__LWPS
				marcel_sem_V(&cnx->sem);
#else
				cnx->done=1;
#endif
			}


                }

                /* remove request from post list */
                if (tbx_slist_ref_extract_and_forward(post_slist, NULL))
                        continue;
                else
                        break;

                next:
                        ;
        } while (tbx_slist_ref_forward(post_slist));

        err = NM_ESUCCESS;

        return err;
}



int
nm_xpaul_post_send(struct nm_xpaul_data *cnx,
		   struct nm_gate       *p_gate) {
	
        p_tbx_slist_t 	 const post_slist	= p_gate->post_sched_out_list;
        int err;

        tbx_slist_ref_to_head(post_slist);
        do {
                struct nm_pkt_wrap	*p_pw	= tbx_slist_ref_get(post_slist);

                /* check track availability				*/
                /* TODO */
                if (0/* track always available for now */)
                        goto next;

                if (p_gate->out_req_nb == 256)
                        goto next;

                p_gate->out_req_nb++;
                p_pw->p_gdrv->out_req_nb++;
                p_pw->p_drv->out_req_nb++;
                p_pw->p_trk->out_req_nb++;

                /* ready to send					*/
                NM_TRACEF("posting new send request: gate %d, drv %d, trk %d, proto %d, seq %d",
                     p_pw->p_gate->id,
                     p_pw->p_drv->id,
                     p_pw->p_trk->id,
                     p_pw->proto_id,
                     p_pw->seq);

                /* post request */

                err = p_pw->p_gdrv->p_drv->ops.post_send_iov(p_pw);

                /* process post command status				*/

                /* driver immediately succeeded in sending the packet? */
                if (err == -NM_EAGAIN) {
                        /* No, put the request in the list */

			/* Submit  the pkt_wrapper to XPaulette */
			xpaul_req_init(&p_pw->nm_xp_data.inst);
			p_pw->nm_xp_data.which = PKT_WRAP;
			p_pw->nm_xp_data.p_core=p_gate->p_core;
			p_pw->nm_xp_data.method = SEND;
			p_pw->err = -NM_EAGAIN;
			p_pw->cnx=cnx;
			/* TODO : implementer les syscalls */
#warning XPAUL_TODO : implementer les syscalls
//		if(! (p_pw->p_drv->cap->has_syscall))
			p_pw->nm_xp_data.inst.func_to_use=XPAUL_FUNC_POLLING;
			p_pw->nm_xp_data.inst.state|=XPAUL_STATE_DONT_POLL_FIRST|XPAUL_STATE_ONE_SHOT;
			
			xpaul_req_submit(p_gate->p_core->xpaul_server, &p_pw->nm_xp_data.inst);
			
                        p_gate->out_req_list[p_gate->out_req_nb-1]	= p_pw;
                        NM_TRACEF("new request %d pending: gate %d, drv %d, trk %d, proto %d, seq %d",
                             p_gate->out_req_nb - 1,
                             p_pw->p_gate->id,
                             p_pw->p_drv->id,
                             p_pw->p_trk->id,
                             p_pw->proto_id,
                             p_pw->seq);
 
                } else {
                        /* Yes, request complete, process it */
                        NM_TRACEF("request completed immediately");

                        if (err != NM_ESUCCESS) {
                                NM_DISPF("drv->post_send returned %d", err);
                        }

                        err = nm_process_complete_send_rq(p_gate, p_pw, err);
                        if (err < 0) {
                                NM_DISPF("nm_process_complete send_rq returned %d", err);
                        }
			
			if((*cnx->complete_callback)(cnx)){ // pour l'instant
#ifdef MARCEL
				marcel_sem_V(&cnx->sem);
#else
				cnx->done=1;
#endif
			}

                }

                /* remove request from post list */
                if (tbx_slist_ref_extract_and_forward(post_slist, NULL))
                        continue;
                else
                        break;

                next:
                        ;
        } while (tbx_slist_ref_forward(post_slist));

        err = NM_ESUCCESS;

        return err;
}


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
		   struct nm_core        *p_core, 
		   struct nm_sched	 *p_sched,
		   p_tbx_slist_t 	  post_slist,
		   p_tbx_slist_t 	  pending_slist);
#endif

#endif /* XPAULETTE */
