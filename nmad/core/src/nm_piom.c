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
#ifdef PIOMAN

#include "nm_piom.h"
#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <tbx.h>

#include "nm_private.h"
#include "nm_pkt_wrap.h"
#include "nm_drv.h"
#include "nm_gate.h"
#include "nm_core.h"
#include "nm_sched.h"
#include "nm_trk.h"
#include "nm_log.h"
/* Ugly */
#include "nm_so_pkt_wrap.h"

int nm_piom_poll(piom_server_t            server,
		 piom_op_t                _op,
		 piom_req_t               req,
		 int                       nb_ev,
		 int                       option) {

	nmad_lock();
	struct nm_pkt_wrap * pkt= struct_up(req, struct nm_pkt_wrap, inst);
	int err;

	if (pkt->which == SEND)
		err = nm_piom_poll_send(pkt);
	else if(pkt->which == RECV)
		err = nm_piom_poll_recv(pkt);
#ifdef PIO_OFFLOAD
	else
		err = nm_piom_post_all(pkt->p_drv->p_core);
#endif
	nmad_unlock();
	PROF_EVENT(piom_polling_done);
	return err;
}

int nm_piom_poll_any(piom_server_t            server,
		     piom_op_t                _op,
		     piom_req_t               req,
		     int                       nb_ev,
		     int                       option) {

	nmad_lock();
	int err;
	struct nm_drv *p_drv = struct_up(server, struct nm_drv, server);
	int i;
	int j=0;

	struct nm_pkt_wrap *p_pw;
	err = p_drv->driver->poll_send_any_iov(NULL,
					       &p_pw);
	/* process poll command status				*/
	if (err == -NM_EAGAIN) {
		err = p_drv->driver->poll_recv_any_iov(NULL,
						       &p_pw);
		if(err == -NM_EAGAIN) {
			/* not complete, try again later
			   - leave the request in the list and go to next
			*/
			nmad_unlock();
			return err;
		}
	}

	if (p_pw->which == SEND) {
		if (err != -NM_EAGAIN){
			const int req_nb = p_pw->p_gate->out_req_nb;

			struct nm_pkt_wrap *pkt2;
			p_pw->err = err;
			/* met a jour la requete */
			piom_req_success(&p_pw->inst);
			nm_process_complete_send_rq(p_pw->p_gate, p_pw, p_pw->err);

			for (i=0;i<req_nb;i++) { /* supprime la requete de la liste */
				pkt2 = p_pw->p_gate->out_req_list[i];
				if (pkt2->err == -NM_EAGAIN) {
					pkt2->p_gate->out_req_list[j] = pkt2->p_gate->out_req_list[i];
					j++;
				}
			}
		}
	} else {
		if (err != -NM_EAGAIN){
			p_pw->err = err;
			piom_req_success(&p_pw->inst);
			/* supprimer la req de la liste des requetes pending */
			tbx_slist_search_and_extract(p_pw->slist,NULL,p_pw);
			/* process complete request */
			p_pw->err = nm_process_complete_recv_rq(p_pw->p_gate->p_core->p_sched, p_pw, p_pw->err);

			if (p_pw->err < 0) { // possible ?
				NM_DISPF("nm_process_complete_rq returned %d", p_pw->err);
			}

			err = p_pw->err;
		}
	}
	nm_piom_post_all(p_pw->p_gate->p_core);
	nmad_unlock();
	return err;
}

int
nm_piom_poll_send(struct nm_pkt_wrap  *p_pw) {
        const int req_nb = p_pw->p_gate->out_req_nb;
	int i;
	int j=0;

	struct nm_gate_drv*p_gdrv = p_pw->p_gate->p_gate_drv_array[p_pw->p_drv->id];
	p_pw->err = p_gdrv->receptacle.driver->poll_send_iov(p_gdrv->receptacle._status,
							     p_pw);

	if (p_pw->err != -NM_EAGAIN){

		struct nm_pkt_wrap *pkt;
		/* met a jour la requete */
		piom_req_success(&p_pw->inst);
		nm_process_complete_send_rq(p_pw->p_gate, p_pw, p_pw->err);

		for (i=0;i<req_nb;i++) { /* supprime la requete de la liste */
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
nm_piom_poll_recv(struct nm_pkt_wrap  *p_pw) {
	NM_TRACEF("polling inbound request: gate %d, drv %d, trk %d, proto %d, seq %d",
		  p_pw->p_gate?p_pw->p_gate->id:-1,
		  p_pw->p_drv->id,
		  p_pw->p_trk->id,
		  p_pw->proto_id,
		  p_pw->seq);

	struct nm_gate_drv*p_gdrv = p_pw->p_gate->p_gate_drv_array[p_pw->p_drv->id];
	p_pw->err = p_gdrv->receptacle.driver->poll_recv_iov(p_gdrv->receptacle._status,
							     p_pw);

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
	piom_req_success(&p_pw->inst);
	/* supprimer la req de la liste des requetes pending */
	tbx_slist_search_and_extract(p_pw-> slist,NULL,p_pw);
	/* process complete request */
	p_pw->err = nm_process_complete_recv_rq(p_pw->p_gate->p_core->p_sched, p_pw, p_pw->err);

	if (p_pw->err < 0) { // possible ?
		NM_DISPF("nm_process_complete_rq returned %d", p_pw->err);
	}

        return p_pw->err;
}


/* Posting functions */
int
nm_piom_post_all(struct nm_core	 *p_core){
	int err;
	int g;
        static int post_pending = 0;

	if (post_pending) {
		err = -NM_EAGAIN;
		goto out;
	}
	post_pending = 1;

	/* send */
	for (g = 0; g < p_core->nb_gates; g++) {
		err= nm_piom_post_all_send(p_core->gate_array + g);

		if (err < 0) {
			NM_DISPF("nm_piom_post_all_send(%p, %d) returned %d",
				 p_core, g, err);
		}
	}

	/* Receive */
	err = nm_piom_post_all_recv(p_core);
	if (err<0) {
		NM_DISPF("nm_piom_post_all_recv(%p) returned %d",
			 p_core, err);
	}

	post_pending = 0;
 out:
	return err;
}


int
nm_piom_post_all_recv(struct nm_core	 *p_core){

        struct nm_sched *p_sched = p_core->p_sched;
        int err;

        /* schedule new reception requests				*/
        err	= p_core->p_sched->ops.in_schedule(p_sched);
        if (err < 0) {
                NM_DISPF("sched.in_schedule returned %d", err);
        }

        /* post new requests 						*/

        /* aux requests */
	while (!tbx_slist_is_nil(p_sched->post_aux_recv_req)) {
                NM_TRACEF("posting inbound aux requests");
                err = nm_piom_post_recv(p_sched,
					p_sched->post_aux_recv_req,
					p_sched->pending_aux_recv_req);
		if (err == -NM_EAGAIN)
			break;
                if (err < 0 && err != -NM_EAGAIN) {
                        NM_DISPF("nm_piom_post_recv (aux) returned %d", err);
                }
        }

        /* permissive requests */
        while (!tbx_slist_is_nil(p_sched->post_perm_recv_req)) {
                NM_TRACEF("posting inbound perm requests");
                err = nm_piom_post_recv(p_sched,
					p_sched->post_perm_recv_req,
					p_sched->pending_perm_recv_req);
		if (err == -NM_EAGAIN)
			break;
                if (err < 0 && err != -NM_EAGAIN) {
                        NM_DISPF("nm_piom_post_recv (perm) returned %d", err);
                }
        }

        err = NM_ESUCCESS;

        return err;
}

int
nm_piom_post_all_send(struct nm_gate *p_gate) {
	int	err;

        /* schedule new requests	*/
	err	= p_gate->p_sched->ops.out_schedule_gate(p_gate);
	if (err < 0) {
		NM_DISPF("sched.schedule_out returned %d", err);
	}

        /* post new requests	*/
        while (!tbx_slist_is_nil(p_gate->post_sched_out_list)) {
		NM_TRACEF("posting outbound requests");
		err	= nm_piom_post_send(p_gate);

		if (err == -NM_EAGAIN)
			break;
		if (err < 0 && err != -NM_EAGAIN) {
			NM_DISPF("nm_piom_post_send returned %d", err);
		}
		err	= p_gate->p_sched->ops.out_schedule_gate(p_gate);
		if (err < 0) {
			NM_DISPF("sched.schedule_out returned %d", err);
		}

        }
	return NM_ESUCCESS;
}

int
nm_piom_post_recv(struct nm_sched	*p_sched,
		  p_tbx_slist_t 	 post_slist,
		  p_tbx_slist_t 	 pending_slist) {
        int err;

	struct nm_pkt_wrap	*p_pw;
        tbx_slist_ref_to_head(post_slist);
        do {
	again:
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
		piom_req_init(&p_pw->inst);
		p_pw->inst.server=&p_pw->p_drv->server;
		p_pw->which=RECV;
                struct nm_gate_drv*p_gdrv = p_pw->p_gate->p_gate_drv_array[p_pw->p_drv->id];
		err = p_gdrv->receptacle.driver->post_recv_iov(p_gdrv->receptacle._status,
							       p_pw);
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
			p_pw->slist=pending_slist;

			/* Submit the pkt_wrapper to Pioman */

			p_pw->err = -NM_EAGAIN;
			p_pw->inst.state|=PIOM_STATE_DONT_POLL_FIRST|PIOM_STATE_ONE_SHOT;
			p_pw->which=RECV;
			/* TODO : implementer les syscall */
#ifdef PIOM_BLOCKING_CALLS
			/* Disable for now because of a compilation problem */
//			if (! ((p_pw->p_drv->driver->get_capabilities(p_pw->p_drv))->is_exportable))
//				p_pw->inst.func_to_use=PIOM_FUNC_POLLING;
#endif
			piom_req_submit(&p_pw->p_drv->server, &p_pw->inst);

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

		}

                /* remove request from post list */
                if (tbx_slist_ref_extract_and_forward(post_slist, NULL))
			goto again;
                else
                        break;

	next:
		;
        } while (tbx_slist_ref_forward(post_slist));

        err = NM_ESUCCESS;
        return err;
}



int
nm_piom_post_send(struct nm_gate       *p_gate) {

        p_tbx_slist_t 	 const post_slist	= p_gate->post_sched_out_list;
        int err;

        tbx_slist_ref_to_head(post_slist);
        do {
                struct nm_pkt_wrap	*p_pw	= NULL;
	again:
		p_pw = tbx_slist_ref_get(post_slist);

                /* check track availability				*/
                /* TODO */
                if (0/* track always available for now */)
                        goto next;

                if (p_gate->out_req_nb == 256)
                        goto next;

                /* ready to send					*/
                NM_TRACEF("posting new send request: gate %d, drv %d, trk %d, proto %d, seq %d",
			  p_pw->p_gate->id,
			  p_pw->p_drv->id,
			  p_pw->p_trk->id,
			  p_pw->proto_id,
			  p_pw->seq);

		piom_req_init(&p_pw->inst);
		p_pw->inst.server=&p_pw->p_drv->server;
		p_pw->which=SEND;

#ifdef PIO_OFFLOAD
		struct nm_so_pkt_wrap *p_so_pw=NULL;
		p_so_pw=nm_pw2so(p_pw);

                nm_so_pw_offloaded_finalize(p_so_pw);
#endif
                /* post request */
		struct nm_gate_drv*p_gdrv = p_pw->p_gate->p_gate_drv_array[p_pw->p_drv->id];
		err = p_gdrv->receptacle.driver->post_send_iov(p_gdrv->receptacle._status,
							       p_pw);

                /* process post command status				*/

                /* driver immediately succeeded in sending the packet? */
                if (err == -NM_EAGAIN) {
                        /* No, put the request in the list */

			/* Submit  the pkt_wrapper to Pioman */
			piom_req_init(&p_pw->inst);
			p_pw->err = -NM_EAGAIN;
			/* TODO : implementer les syscalls */
#ifdef PIOM_BLOCKING_CALLS
			/* Disable for now because of a compilation problem */
//			if (! ((p_pw->p_drv->driver->get_capabilities(p_pw->p_drv))->is_exportable))
//				p_pw->inst.func_to_use=PIOM_FUNC_POLLING;
#endif
			p_pw->inst.state|=PIOM_STATE_DONT_POLL_FIRST|PIOM_STATE_ONE_SHOT;
			p_pw->which=SEND;
			piom_req_submit(&p_pw->p_drv->server, &p_pw->inst);

			p_gate->out_req_nb++;
			p_pw->p_gdrv->out_req_nb++;
			p_pw->p_drv->out_req_nb++;
			p_pw->p_trk->out_req_nb++;

                        p_gate->out_req_list[p_gate->out_req_nb-1]	= p_pw;
                        NM_TRACEF("new request %d pending: gate %d, drv %d, trk %d, proto %d, seq %d",
				  p_gate->out_req_nb - 1,
				  p_pw->p_gate->id,
				  p_pw->p_drv->id,
				  p_pw->p_trk->id,
				  p_pw->proto_id,
				  p_pw->seq);

                } else {
			p_gate->out_req_nb++;
			p_pw->p_gdrv->out_req_nb++;
			p_pw->p_drv->out_req_nb++;
			p_pw->p_trk->out_req_nb++;

                        /* Yes, request complete, process it */
                        NM_TRACEF("request completed immediately");

                        if (err != NM_ESUCCESS) {
                                NM_DISPF("drv->post_send returned %d", err);
                        }

                        err = nm_process_complete_send_rq(p_gate, p_pw, err);
                        if (err < 0) {
                                NM_DISPF("nm_process_complete send_rq returned %d", err);
                        }

                }

                /* remove request from post list */
                if (tbx_slist_ref_extract_and_forward(post_slist, NULL))
			goto again;
                else
                        break;

	next:
		;
        } while (tbx_slist_ref_forward(post_slist));

        err = NM_ESUCCESS;

        return err;
}


#ifdef PIOM_BLOCKING_CALLS

int
nm_piom_block(piom_server_t server,
	      piom_op_t     _op,
	      piom_req_t    req,
	      int            nb_ev,
	      int            option) {
	nmad_lock();
	struct nm_pkt_wrap * pkt= struct_up(req, struct nm_pkt_wrap, inst);
	int err;
	if (pkt->which == SEND)
		err = nm_piom_block_send(pkt);
	else
		err = nm_piom_block_recv(pkt);

	nmad_unlock();
	return err;

}

int
nm_piom_block_send(struct nm_pkt_wrap  *p_pw) {
        const int req_nb = p_pw->p_gate->out_req_nb;
	int i;
	int j=0;

	nmad_unlock();
	struct nm_gate_drv*p_gdrv = p_pw->p_gate->p_gate_drv_array[p_pw->p_drv->id];
	p_pw->err = p_gdrv->receptacle.driver->wait_send_iov(p_gdrv->receptacle._status,
							     p_pw);
	nmad_lock();

	if (p_pw->err != -NM_EAGAIN) {

		struct nm_pkt_wrap *pkt;
		/* met a jour la requete */
		piom_req_success(&p_pw->inst);
		nm_process_complete_send_rq(p_pw->p_gate, p_pw, p_pw->err);

		for (i=0;i<req_nb;i++) { /* supprime la requete de la liste */
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
nm_piom_block_recv(struct nm_pkt_wrap  *p_pw) {
	NM_TRACEF("waiting inbound request: gate %d, drv %d, trk %d, proto %d, seq %d",
		  p_pw->p_gate?p_pw->p_gate->id:-1,
		  p_pw->p_drv->id,
		  p_pw->p_trk->id,
		  p_pw->proto_id,
		  p_pw->seq);

	nmad_unlock();
	struct nm_gate_drv*p_gdrv = p_pw->p_gate->p_gate_drv_array[p_pw->p_drv->id];
	p_pw->err = p_gdrv->receptacle.driver->wait_recv_iov(p_gdrv->receptacle._status,
							     p_pw);
	nmad_lock();

	/* process poll command status				*/
	if (p_pw->err == -NM_EAGAIN) {
		/* not complete, try again later
		   - leave the request in the list and go to next
		*/
		return p_pw->err;
	}

	if (p_pw->err != NM_ESUCCESS) {
		NM_DISPF("drv->wait_recv returned %d", p_pw->err);
	}
	piom_req_success(&p_pw->inst);
	/* process complete request */
	p_pw->err = nm_process_complete_recv_rq(p_pw->p_gate->p_core->p_sched, p_pw, p_pw->err);

	if (p_pw->err < 0) { // possible ?
		NM_DISPF("nm_process_complete_rq returned %d", p_pw->err);
	}

        return p_pw->err;
}

int nm_piom_block_any(piom_server_t            server,
		     piom_op_t                _op,
		     piom_req_t               req,
		     int                       nb_ev,
		     int                       option) {

	nmad_lock();
	int err;
	struct nm_drv *p_drv = struct_up(server, struct nm_drv, server);
	int i;
	int j=0;

	struct nm_pkt_wrap *p_pw;
	err = p_drv->driver->wait_send_any_iov(NULL,
					       &p_pw);
	/* process poll command status				*/
	if (err == -NM_EAGAIN) {
		err = p_drv->driver->wait_recv_any_iov(NULL,
						       &p_pw);
		if( err == -NM_EAGAIN) {
			/* not complete, try again later
			   - leave the request in the list and go to next
			*/
			nmad_unlock();
			return err;
		}
	}

	if (p_pw->which == SEND) {
		if (err != -NM_EAGAIN){
			const int req_nb = p_pw->p_gate->out_req_nb;

			struct nm_pkt_wrap *pkt2;
			p_pw->err = err;
			/* met a jour la requete */
			piom_req_success(&p_pw->inst);
			nm_process_complete_send_rq(p_pw->p_gate, p_pw, p_pw->err);

			for (i=0;i<req_nb;i++) { /* supprime la requete de la liste */
				pkt2 = p_pw->p_gate->out_req_list[i];
				if (pkt2->err == -NM_EAGAIN) {
					pkt2->p_gate->out_req_list[j] = pkt2->p_gate->out_req_list[i];
					j++;
				}
			}
		}
	} else {
		if (err != -NM_EAGAIN){
			p_pw->err = err;
			piom_req_success(&p_pw->inst);
			/* supprimer la req de la liste des requetes pending */
			tbx_slist_search_and_extract(p_pw->slist,NULL,p_pw);
			/* process complete request */
			p_pw->err = nm_process_complete_recv_rq(p_pw->p_gate->p_core->p_sched, p_pw, p_pw->err);

			if (p_pw->err < 0) { // possible ?
				NM_DISPF("nm_process_complete_rq returned %d", p_pw->err);
			}

			err = p_pw->err;
		}
	}
	nmad_unlock();
	return err;
}

#endif

#endif /* PIOMAN */
