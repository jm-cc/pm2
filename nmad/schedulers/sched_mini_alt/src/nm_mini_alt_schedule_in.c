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


#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <tbx.h>

#include "nm_mini_alt_private.h"

/* if true, incoming packet requests are posted only when user code
   expects at least one packet
 */
#undef NM_MA_NO_ANTICIPATION

#ifdef NM_MA_NO_ANTICIPATION
#warning anticipated incoming ctrl pkt post disabled
#endif

/** Allocate and fill a ctrl pkt.
 */
static
__inline__
int
nm_mini_alt_make_ctrl_pkt(struct nm_sched		 *p_sched,
                      struct nm_pkt_wrap	**pp_pw) {
        struct nm_core			*p_core		= NULL;
        struct nm_mini_alt_sched	*p_mini_alt_sched	= NULL;
        struct nm_pkt_wrap		*p_pw		= NULL;
        struct nm_mini_alt_ctrl		*p_mini_alt_ctrl	= NULL;

        int err;

        p_core		= p_sched->p_core;
        p_mini_alt_sched	= p_sched->sch_private;

        /* allocate packet wrapper
           - ignore seq num since seq nums are meaningless when posting
           permissive requests
         */
        err	= nm_pkt_wrap_alloc(p_core, &p_pw,
                                        nm_pi_sched,
                                        0);
        if (err != NM_ESUCCESS)
                goto out;

        /* allocate iov
         */
        err	= nm_iov_alloc(p_core, p_pw, 1);
        if (err != NM_ESUCCESS) {
                nm_pkt_wrap_free(p_core, p_pw);
                goto out;
        }

        /* allocate control pkt buffer
         */
        p_mini_alt_ctrl	= tbx_malloc(p_mini_alt_sched->ctrl_mem);
        if (!p_mini_alt_ctrl) {
                nm_iov_free(p_core, p_pw);
                nm_pkt_wrap_free(p_core, p_pw);
                err	= -NM_ENOMEM;
                goto out;
        }

        /* initialize control pkt
         */
        p_mini_alt_ctrl->proto_id	= 0;
        p_mini_alt_ctrl->seq	= 0;

        /* append control pkt buffer to iovec
         */
        err	= nm_iov_append_buf(p_core, p_pw,
                                        p_mini_alt_ctrl, sizeof(struct nm_mini_alt_ctrl));
        if (err != NM_ESUCCESS) {
                NM_DISPF("nm_iov_append_buf returned %d", err);
        }

        p_pw->data	= p_mini_alt_ctrl;

        /* return new pkt
         */
        *pp_pw	= p_pw;

        err	= NM_ESUCCESS;

 out:
        return err;
}

/** Schedule the reception of a new ctrl pkt on the post list.
 */
static
__inline__
int
nm_mini_alt_in_schedule_ctrl(struct nm_sched		*p_sched,
                             struct nm_mini_alt_sched	*p_mini_alt_sched,
                             p_tbx_slist_t		 post) {
        struct nm_pkt_wrap	 *p_pw	= NULL;
        int	err;

        if (p_mini_alt_sched->state)
                goto out;

        err	= nm_mini_alt_make_ctrl_pkt(p_sched, &p_pw);
        if (err != NM_ESUCCESS) {
                NM_DISPF("nm_iov_append_buf returned %d", err);
        }

        p_pw->p_drv	= p_sched->p_core->driver_array + 0;
        p_pw->p_trk	= p_pw->p_drv->p_track_array[0];

        NM_TRACE_PTR("adding a ctrl pkt to inbound post", p_pw);
        tbx_slist_append(post, p_pw);
        p_mini_alt_sched->state	= 1;

 out:
        err = NM_ESUCCESS;

        return err;
}


/** Schedule the selective reception of a new data pkt on the post list.
 */
static
__inline__
int
nm_mini_alt_in_schedule_data_aux(struct nm_sched		*p_sched,
                                 struct nm_mini_alt_sched	*p_mini_alt_sched,
                                 p_tbx_slist_t		 pre,
                                 p_tbx_slist_t		 post) {
        int	err;

        if (tbx_slist_is_nil(pre))
                goto early_out;

        /* loop on the new submitted pkts
         */
        do {
                struct nm_pkt_wrap		*p_pw		= NULL;
                struct nm_mini_alt_ctrl		*p_ctrl		= NULL;
                struct nm_mini_alt_sched_gate	*p_mini_alt_sg	= NULL;
                p_tbx_slist_t			 ctrl_list	= NULL;
                p_tbx_slist_t			 pkt_list	= NULL;
                uint8_t				 proto_id;
                uint8_t				 seq;

                /* fetch a pkt
                 */
                p_pw	= tbx_slist_extract(pre);
                proto_id	= p_pw->proto_id;
                seq		= p_pw->seq;

                NM_TRACEF("examining pkt - proto %d, seq %d", proto_id, seq);

                /* fetch sched_gate struct for this pkt
                 */
                p_mini_alt_sg	= p_mini_alt_sched->sched_gates + p_pw->p_gate->id;
                pkt_list	= p_mini_alt_sg->pkt_list;

                /* if no token for this protocol, skip the loop
                 */
                if (!p_mini_alt_sched->token_per_proto[proto_id])
                        goto skip_match;

                ctrl_list	= p_mini_alt_sg->ctrl_list;

                /* if no token to match, skip the loop
                 */
                if (tbx_slist_is_nil(ctrl_list))
                        goto skip_match;

                /* loop on already arrived tokens
                 */
                tbx_slist_ref_to_head(ctrl_list);
                do {

                        p_ctrl	= tbx_slist_ref_get(ctrl_list);
                        NM_TRACEF("matching token - proto %d, seq %d", p_ctrl->proto_id, p_ctrl->seq);

                        if (proto_id != p_ctrl->proto_id  ||  seq != p_ctrl->seq)
                                goto next_token;

                        /* matching token found */
                        NM_TRACEF("match");

                        tbx_slist_ref_extract_and_forward(ctrl_list, NULL);

                        {
                                struct nm_gate_drv	**const p_gdrv_a	=
                                        p_pw->p_gate->p_gate_drv_array;
                                struct nm_gate_drv	 *const p_gdrv	= p_gdrv_a[0];
                                struct nm_gate_trk	 *const p_gtrk	= p_gdrv->p_gate_trk_array[1];
                                p_pw->p_drv		= p_gdrv->p_drv;
                                p_pw->p_trk		= p_gtrk->p_trk;
                                p_pw->p_gdrv		= p_gdrv;
                                p_pw->p_gtrk		= p_gtrk;
                        }
                        tbx_free(p_mini_alt_sched->ctrl_mem, p_ctrl);

                        NM_TRACE_PTR("adding a data pkt to inbound post", p_pw);
                        tbx_slist_append(post, p_pw);

                        p_mini_alt_sched->token_per_proto[proto_id]--;
                        p_mini_alt_sg->token_per_proto[proto_id]--;
                        goto next_pkt;

                next_token:
                                ;
                                NM_TRACEF("no match");
                } while (tbx_slist_ref_forward(ctrl_list));

        skip_match:
                ;

                NM_TRACE_PTR("adding a data pkt to expected pkt list", p_pw);
                tbx_slist_append(pkt_list, p_pw);

        next_pkt:
                ;
        } while (!tbx_slist_is_nil(pre));

 early_out:
        err	= NM_ESUCCESS;

        return err;
}

/** Schedule the permissive reception of a new data pkt on the post list.
 */
static
__inline__
int
nm_mini_alt_in_schedule_data_perm(struct nm_sched		*p_sched,
                                  struct nm_mini_alt_sched	*p_mini_alt_sched,
                                  p_tbx_slist_t			 pre,
                                  p_tbx_slist_t			 post) {
        p_tbx_slist_t	pkt_list	= NULL;
        int	err;

        if (tbx_slist_is_nil(pre))
                goto early_out;

        pkt_list	= p_mini_alt_sched->pkt_list;

        /* loop on the new submitted pkts
         */
        do {
                struct nm_pkt_wrap		*p_pw		= NULL;
                struct nm_mini_alt_ctrl		*p_ctrl		= NULL;
                struct nm_mini_alt_sched_gate	*p_mini_alt_sg	= NULL;
                p_tbx_slist_t			 ctrl_list	= NULL;
                uint8_t				 proto_id;
                uint8_t				 seq;
                gate_id_t			 gate_id;

                /* fetch a pkt
                 */
                p_pw	= tbx_slist_extract(pre);
                proto_id	= p_pw->proto_id;
                seq		= p_pw->seq;

                NM_TRACEF("examining pkt - proto %d, seq %d", p_pw->proto_id, p_pw->seq);

                if (!p_mini_alt_sched->token_per_proto[proto_id])
                        goto skip_match;

                /* loop on gates
                 */
                for (gate_id = 0; gate_id < 255; gate_id++) {

                        /* fetch sched_gate struct for this pkt
                         */
                        p_mini_alt_sg	= p_mini_alt_sched->sched_gates + gate_id;
                        if (!p_mini_alt_sg->token_per_proto[proto_id])
                                goto skip_gate;

                        ctrl_list	= p_mini_alt_sg->ctrl_list;

                        /* if no token to match, skip the loop
                         */
                        if (tbx_slist_is_nil(ctrl_list))
                                goto skip_gate;

                        /* loop on already arrived tokens
                         */
                        tbx_slist_ref_to_head(ctrl_list);
                        do {

                                p_ctrl	= tbx_slist_ref_get(ctrl_list);
                                NM_TRACEF("matching token - proto %d, seq %d", p_ctrl->proto_id, p_ctrl->seq);

                                if (proto_id != p_ctrl->proto_id  ||  seq != p_ctrl->seq)
                                        goto next_token;

                                /* matching token found */
                                NM_TRACEF("match");

                                tbx_slist_ref_extract_and_forward(ctrl_list, NULL);

                                p_pw->p_gate = p_sched->p_core->gate_array + gate_id;

                                {
                                        struct nm_gate_drv	**const p_gdrv_a	=
                                                p_pw->p_gate->p_gate_drv_array;
                                        struct nm_gate_drv	 *const p_gdrv	= p_gdrv_a[0];
                                        struct nm_gate_trk	 *const p_gtrk	= p_gdrv->p_gate_trk_array[1];
                                        p_pw->p_drv		= p_gdrv->p_drv;
                                        p_pw->p_trk		= p_gtrk->p_trk;
                                        p_pw->p_gdrv		= p_gdrv;
                                        p_pw->p_gtrk		= p_gtrk;
                                }
                                tbx_free(p_mini_alt_sched->ctrl_mem, p_ctrl);

                                NM_TRACE_PTR("adding a data pkt to inbound post", p_pw);
                                tbx_slist_append(post, p_pw);

                                p_mini_alt_sched->token_per_proto[proto_id]--;
                                p_mini_alt_sg->token_per_proto[proto_id]--;
                                goto next_pkt;

                        next_token:
                                ;
                                NM_TRACEF("no match");
                        } while (tbx_slist_ref_forward(ctrl_list));

                skip_gate:
                        ;
                        //
                }

                skip_match:
                        ;
                tbx_slist_append(pkt_list, p_pw);

        next_pkt:
                ;
        } while (!tbx_slist_is_nil(pre));

 early_out:
        err	= NM_ESUCCESS;

        return err;
}

/** Schedule and post new incoming buffers.
 */
int
nm_mini_alt_in_schedule(struct nm_sched *p_sched) {
        struct nm_mini_alt_sched	*p_mini_alt_sched	= NULL;
        p_tbx_slist_t 			 pre			= NULL;
        p_tbx_slist_t			 post			= NULL;
        int	err;

#ifdef NM_MA_NO_ANTICIPATION
        if (tbx_slist_is_nil(p_sched->submit_perm_recv_req)
            &&
            tbx_slist_is_nil(p_sched->submit_aux_recv_req))
                goto early_out;
#endif /* NM_MA_NO_ANTICIPATION */

        post	= p_sched->post_perm_recv_req;
        p_mini_alt_sched	= p_sched->sch_private;
        err = nm_mini_alt_in_schedule_ctrl(p_sched, p_mini_alt_sched, post);
        if (err != NM_ESUCCESS) {
                NM_DISPF("schedule_ctrl returned %d", err);
        }

        pre	= p_sched->submit_perm_recv_req;
        post	= p_sched->post_aux_recv_req;
        err = nm_mini_alt_in_schedule_data_perm(p_sched, p_mini_alt_sched, pre, post);
        if (err != NM_ESUCCESS) {
                NM_DISPF("schedule data (perm) returned %d", err);
        }

        pre	= p_sched->submit_aux_recv_req;
        err = nm_mini_alt_in_schedule_data_aux(p_sched, p_mini_alt_sched, pre, post);
        if (err != NM_ESUCCESS) {
                NM_DISPF("schedule data (aux) returned %d", err);
        }

#ifdef NM_MA_NO_ANTICIPATION
 early_out:
#endif /* NM_MA_NO_ANTICIPATION */
        err	= NM_ESUCCESS;

        return err;
}

/** Handle a successful receive request for a ctrl pkt.
 */
static
__inline__
int
nm_mini_alt_in_process_success_ctrl(struct nm_sched	*p_sched,
                                    struct nm_pkt_wrap	*p_pw) {
        struct nm_gate			*p_gate			= NULL;
        struct nm_core			*p_core			= NULL;
        struct nm_mini_alt_sched	*p_mini_alt_sched	= NULL;
        struct nm_mini_alt_sched_gate	*p_mini_alt_sg		= NULL;
        p_tbx_slist_t			 ctrl_list		= NULL;
        p_tbx_slist_t			 post			= NULL;
        p_tbx_slist_t			 pkt_list		= NULL;
        struct nm_mini_alt_ctrl		*p_ctrl			= NULL;
        int	err;

        p_gate			= p_pw->p_gate;
        p_core			= p_sched->p_core;
        p_mini_alt_sched	= p_sched->sch_private;
        p_mini_alt_sg		= p_mini_alt_sched->sched_gates + p_gate->id;
        ctrl_list		= p_mini_alt_sg->ctrl_list;
        post			= p_sched->post_aux_recv_req;

        /* control pkt */
        p_ctrl	= p_pw->data;
        NM_TRACEF("control pkt - proto %d, seq %d", p_ctrl->proto_id, p_ctrl->seq);


        /* First check permissive pkts
         */
        pkt_list	= p_mini_alt_sched->pkt_list;

        /* any pkt waiting for this token?
         */
        if (tbx_slist_is_nil(pkt_list))
                goto gate_list;

        NM_TRACEF("looking for a matching pkt in permissive list");
        tbx_slist_ref_to_head(pkt_list);
        do {
                struct nm_pkt_wrap	*p_data_pw	= NULL;

                p_data_pw	= tbx_slist_ref_get(pkt_list);
                NM_TRACEF("examining pkt - proto %d, seq %d", p_data_pw->proto_id, p_data_pw->seq);
                if (p_data_pw->proto_id != p_ctrl->proto_id
                    || p_data_pw->seq != p_ctrl->seq)
                        continue;

                tbx_slist_ref_extract_and_forward(pkt_list, NULL);

                {
                        struct nm_gate_drv	**const p_gdrv_a	=
                                p_gate->p_gate_drv_array;
                        struct nm_gate_drv	 *const p_gdrv	= p_gdrv_a[0];
                        struct nm_gate_trk	 *const p_gtrk	= p_gdrv->p_gate_trk_array[1];
                        /* permissive pkt: fill the p_gate field
                         */
                        p_data_pw->p_gate	= p_pw->p_gate;

                        p_data_pw->p_drv	= p_gdrv->p_drv;
                        p_data_pw->p_trk	= p_gtrk->p_trk;
                        p_data_pw->p_gdrv	= p_gdrv;
                        p_data_pw->p_gtrk	= p_gtrk;
                }

                tbx_slist_append(post, p_data_pw);
                /* free ctrl pkt
                 */
                tbx_free(p_mini_alt_sched->ctrl_mem, p_pw->v[0].iov_base);
                goto free_iovec;
        } while (tbx_slist_ref_forward(pkt_list));
        NM_TRACEF("no match");


 gate_list:
        /* Then check selective pkts
         */
        pkt_list	= p_mini_alt_sg->pkt_list;

        /* any pkt waiting for this token?
         */
        if (tbx_slist_is_nil(pkt_list))
                goto new_token;

        NM_TRACEF("looking for a matching pkt in selective list");
        tbx_slist_ref_to_head(pkt_list);
        do {
                struct nm_pkt_wrap	*p_data_pw	= NULL;

                p_data_pw	= tbx_slist_ref_get(pkt_list);
                NM_TRACEF("examining pkt - proto %d, seq %d", p_data_pw->proto_id, p_data_pw->seq);
                if (p_data_pw->proto_id != p_ctrl->proto_id
                    || p_data_pw->seq != p_ctrl->seq)
                        continue;

                tbx_slist_ref_extract_and_forward(pkt_list, NULL);

                {
                        struct nm_gate_drv	**const p_gdrv_a	=
                                p_gate->p_gate_drv_array;
                        struct nm_gate_drv	 *const p_gdrv	= p_gdrv_a[0];
                        struct nm_gate_trk	 *const p_gtrk	= p_gdrv->p_gate_trk_array[1];

                        p_data_pw->p_drv		= p_gdrv->p_drv;
                        p_data_pw->p_trk		= p_gtrk->p_trk;
                        p_data_pw->p_gdrv		= p_gdrv;
                        p_data_pw->p_gtrk		= p_gtrk;
                }

                tbx_slist_append(post, p_data_pw);
                /* free ctrl pkt
                 */
                tbx_free(p_mini_alt_sched->ctrl_mem, p_pw->v[0].iov_base);
                goto free_iovec;
        } while (tbx_slist_ref_forward(pkt_list));
        NM_TRACEF("no match");


 new_token:
        NM_TRACEF("mini_alt_schedule_in: new token - proto %d, seq %d", p_ctrl->proto_id, p_ctrl->seq);
        p_mini_alt_sg->token_per_proto[p_ctrl->proto_id]++;
        p_mini_alt_sched->token_per_proto[p_ctrl->proto_id]++;
        tbx_slist_append(ctrl_list, p_ctrl);

 free_iovec:

        /* free iovec
         */
        nm_iov_free(p_core, p_pw);

        /* free pkt_wrap
         */
        nm_pkt_wrap_free(p_core, p_pw);

        p_mini_alt_sched->state	= 0;

        err	= NM_ESUCCESS;

        return err;
}

/** Handle a successful receive request for a data pkt.
 */
static
__inline__
int
nm_mini_alt_in_process_success_data(struct nm_sched	*p_sched,
                                  struct nm_pkt_wrap	*p_pw) {
        struct nm_core		*p_core		= NULL;
        struct nm_proto		*p_proto	= NULL;
        int	err;

        p_core		= p_sched->p_core;

        /* data pkt */


        p_proto	= p_core->p_proto_array[p_pw->proto_id];
        if (!p_proto) {
                NM_TRACEF("unregistered protocol: %d", p_proto->id);
                goto discard;
        }

        if (!p_proto->ops.in_success)
                goto discard;

        err	= p_proto->ops.in_success(p_proto, p_pw);
        if (err != NM_ESUCCESS) {
                NM_DISPF("proto.ops.in_success returned: %d", err);
        }

 discard:
        /* free iovec
         */
        nm_iov_free(p_core, p_pw);

        /* free pkt_wrap
         */
        nm_pkt_wrap_free(p_core, p_pw);

        err	= NM_ESUCCESS;

        return err;
}

/** Handle a successful incoming request.
 */
int
nm_mini_alt_in_process_success_rq(struct nm_sched	*p_sched,
                                  struct nm_pkt_wrap	*p_pw) {
        int	err;

        if (p_pw->proto_id == nm_pi_sched) {
                err	= nm_mini_alt_in_process_success_ctrl(p_sched, p_pw);
        } else {
                err	= nm_mini_alt_in_process_success_data(p_sched, p_pw);
        }

        return err;
}


/** Handle a failed incoming request.
 */
int
nm_mini_alt_in_process_failed_rq(struct nm_sched	*p_sched,
                             struct nm_pkt_wrap	*p_pw,
                             int		_err) {
        struct nm_core		*p_core		= NULL;
        struct nm_mini_alt_sched	*p_mini_alt_sched	= NULL;
        int	err;

        p_core		= p_sched->p_core;
        p_mini_alt_sched	= p_sched->sch_private;

        if (p_pw->proto_id == nm_pi_sched) {
                /* free buffer
                 */
                tbx_free(p_mini_alt_sched->ctrl_mem, p_pw->v[0].iov_base);

                /* free iovec
                 */
                nm_iov_free(p_core, p_pw);

                /* free pkt_wrap
                 */
                nm_pkt_wrap_free(p_core, p_pw);

                p_mini_alt_sched->state	= 0;
        } else {
                /* data pkt */

                struct nm_proto		*p_proto	= NULL;

                p_proto	= p_core->p_proto_array[p_pw->proto_id];
                if (!p_proto) {
                        NM_DISPF("unregistered protocol: %d", p_proto->id);
                        goto discard;
                }

                if (!p_proto->ops.in_failed)
                        goto discard;

                err	= p_proto->ops.in_failed(p_proto, p_pw, _err);
                if (err != NM_ESUCCESS) {
                        NM_DISPF("proto.ops.in_failed returned: %d", err);
                }

         discard:
               /* free iovec
                 */
                nm_iov_free(p_core, p_pw);

                /* free pkt_wrap
                 */
                nm_pkt_wrap_free(p_core, p_pw);
        }

        err	= NM_ESUCCESS;

        return err;
}

