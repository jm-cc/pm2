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

#include "nm_mini_private.h"

/* TODO: merge with version in mini_schedule_out
 */
static
int
nm_mini_make_ctrl_pkt(struct nm_sched		 *p_sched,
                      struct nm_pkt_wrap	**pp_pw) {
        struct nm_core		*p_core		= NULL;
        struct nm_mini_sched	*p_mini_sched	= NULL;
        struct nm_pkt_wrap	*p_pw		= NULL;
        struct nm_mini_ctrl	*p_mini_ctrl	= NULL;

        int err;

        p_core		= p_sched->p_core;
        p_mini_sched	= p_sched->sch_private;

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
        p_mini_ctrl	= tbx_malloc(p_mini_sched->ctrl_mem);
        if (!p_mini_ctrl) {
                nm_iov_free(p_core, p_pw);
                nm_pkt_wrap_free(p_core, p_pw);
                goto out;
        }

        /* initialize control pkt
         */
        p_mini_ctrl->proto_id	= 0;
        p_mini_ctrl->seq	= 0;

        /* append control pkt buffer to iovec
         */
        err	= nm_iov_append_buf(p_core, p_pw,
                                        p_mini_ctrl, sizeof(struct nm_mini_ctrl));
        if (err != NM_ESUCCESS) {
                NM_DISPF("nm_iov_append_buf returned %d", err);
        }

        p_pw->data	= p_mini_ctrl;

        /* return new pkt
         */
        *pp_pw	= p_pw;

        err	= NM_ESUCCESS;

 out:
        return err;
}

/* schedule and post new incoming buffers */
int
nm_mini_in_schedule(struct nm_sched *p_sched) {
        p_tbx_slist_t 		 pre		= NULL;
        p_tbx_slist_t		 post		= NULL;
        struct nm_mini_sched	*p_mini_sched	= NULL;
        int	err;

        p_mini_sched	= p_sched->sch_private;

        /* post permissive requests for control pkts
         */
        post	= p_sched->post_perm_recv_req;

        if (!p_mini_sched->state) {
                struct nm_pkt_wrap	 *p_pw	= NULL;

                err	= nm_mini_make_ctrl_pkt(p_sched, &p_pw);
                if (err != NM_ESUCCESS) {
                        NM_DISPF("nm_iov_append_buf returned %d", err);
                }

                p_pw->p_drv		= p_sched->p_core->driver_array + 0;
                p_pw->p_trk		= p_pw->p_drv->p_track_array[0];

                NM_TRACE_PTR("adding a ctrl pkt to inbound post", p_pw);
                tbx_slist_append(post, p_pw);
                p_mini_sched->state	= 1;
        }

        post	= p_sched->post_aux_recv_req;
        pre	= p_sched->submit_aux_recv_req;
        if (tbx_slist_is_nil(pre))
                goto early_out;

        /* TODO: take driver capabilities into account in deciding whether to
           post a receive request or not
         */
#warning TODO

        /* TODO: if driver does not support selective receives, implement a
           RDV algorithm
         */
#warning TODO

        /* loop on the new submitted pkts
         */
        do {
                struct nm_pkt_wrap		*p_pw		= NULL;
                struct nm_mini_token		*p_token	= NULL;
                struct nm_mini_sched_gate	*p_mini_sg	= NULL;
                p_tbx_slist_t			 token_list	= NULL;
                p_tbx_slist_t			 pkt_list	= NULL;
                int				 more_tokens;

                /* fetch a pkt
                 */
                p_pw	= tbx_slist_extract(pre);

                /* mini sched does not support application level permissive
                   requests --> use alternate mini sched instead
                 */
                assert(p_pw->p_gate);

                NM_TRACEF("mini_schedule_in: examining pkt - proto %d, seq %d", p_pw->proto_id, p_pw->seq);

                /* fetch sched_gate struct for this pkt
                 */
                p_mini_sg	= p_mini_sched->sched_gates + p_pw->p_gate->id;
                token_list	= p_mini_sg->token_list;
                pkt_list	= p_mini_sg->pkt_list;

                /* see if there is any token to match
                 */
                more_tokens	= !tbx_slist_is_nil(token_list);

                /* if no token to match, skip the loop
                 */
                if (!more_tokens)
                        goto skip_match;

                /* loop on already arrived tokens
                 */
                tbx_slist_ref_to_head(token_list);
                do {

                        p_token	= tbx_slist_ref_get(token_list);
                        NM_TRACEF("mini_schedule_in: matching token - proto %d, seq %d", p_token->proto_id, p_token->seq);

                        if (p_pw->proto_id != p_token->proto_id
                            || p_pw->seq != p_token->seq)
                                goto next_token;

                        /* matching token found */
                        NM_TRACEF("match");

                        if (!tbx_slist_ref_extract_and_forward(token_list, NULL)) {
                                more_tokens	= tbx_slist_is_nil(token_list);
                        }

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
                        tbx_free(p_mini_sched->token_mem, p_token);

                        NM_TRACE_PTR("adding a data pkt to inbound post", p_pw);
                        tbx_slist_append(post, p_pw);
                        goto next_pkt;

                next_token:
                                ;
                                NM_TRACEF("no match");
                } while (tbx_slist_ref_forward(token_list));

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


/* process complete successful incoming request */
int
nm_mini_in_process_success_rq(struct nm_sched	*p_sched,
                             struct nm_pkt_wrap	*p_pw) {
        struct nm_gate			*p_gate		= NULL;
        struct nm_core			*p_core		= NULL;
        struct nm_mini_sched		*p_mini_sched	= NULL;
        struct nm_mini_sched_gate	*p_mini_sg	= NULL;
        int	err;

        p_gate		= p_pw->p_gate;
        p_mini_sched	= p_sched->sch_private;
        p_mini_sg	= p_mini_sched->sched_gates + p_pw->p_gate->id;
        p_core		= p_sched->p_core;

        if (p_pw->proto_id == nm_pi_sched) {
                p_tbx_slist_t		 token_list	= NULL;
                p_tbx_slist_t		 post		= NULL;
                p_tbx_slist_t		 pkt_list	= NULL;
                struct nm_mini_token	*p_token	= NULL;
                struct nm_mini_ctrl	*p_mini_ctrl	= NULL;

                token_list	= p_mini_sg->token_list;
                pkt_list	= p_mini_sg->pkt_list;
                post		= p_sched->post_aux_recv_req;

                /* control pkt */
                p_mini_ctrl	= p_pw->data;

                /* any pkt waiting for this token?
                 */
                if (tbx_slist_is_nil(pkt_list))
                        goto new_token;

                tbx_slist_ref_to_head(pkt_list);
                do {
                        struct nm_pkt_wrap	*p_data_pw	= NULL;

                        p_data_pw	= tbx_slist_ref_get(pkt_list);
                        if (p_data_pw->proto_id != p_mini_ctrl->proto_id
                            || p_data_pw->seq != p_mini_ctrl->seq)
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
                        goto free_ctrl_pkt;
                } while (tbx_slist_ref_forward(pkt_list));


        new_token:
                /* no pkt waiting, put the token into the token list
                 */
                p_token	= tbx_malloc(p_mini_sched->token_mem);
                p_token->proto_id	= p_mini_ctrl->proto_id;
                p_token->seq		= p_mini_ctrl->seq;

                NM_TRACEF("mini_schedule_in: new token - proto %d, seq %d", p_token->proto_id, p_token->seq);
                tbx_slist_append(token_list, p_token);

        free_ctrl_pkt:
                /* free buffer
                 */
                tbx_free(p_mini_sched->ctrl_mem, p_pw->v[0].iov_base);

                /* free iovec
                 */
                nm_iov_free(p_core, p_pw);

                /* free pkt_wrap
                 */
                nm_pkt_wrap_free(p_core, p_pw);

                p_mini_sched->state	= 0;
        } else {
                /* data pkt */

                struct nm_proto		*p_proto	= NULL;

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

                /* TODO: move iov_free and pkt_wrap_free to user/proto code
                 */
#warning TODO


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


/* process complete failed incoming request */
int
nm_mini_in_process_failed_rq(struct nm_sched	*p_sched,
                             struct nm_pkt_wrap	*p_pw,
                             int		_err) {
        struct nm_core		*p_core		= NULL;
        struct nm_mini_sched	*p_mini_sched	= NULL;
        int	err;

        p_core		= p_sched->p_core;
        p_mini_sched	= p_sched->sch_private;

        if (p_pw->proto_id == nm_pi_sched) {
                /* free buffer
                 */
                tbx_free(p_mini_sched->ctrl_mem, p_pw->v[0].iov_base);

                /* free iovec
                 */
                nm_iov_free(p_core, p_pw);

                /* free pkt_wrap
                 */
                nm_pkt_wrap_free(p_core, p_pw);

                p_mini_sched->state	= 0;
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

                /* TODO: move iov_free and pkt_wrap_free to user/proto code
                 */
#warning TODO


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

