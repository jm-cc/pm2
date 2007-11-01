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

#include "nm_core_inline.h"
#include "nm_sched.h"
#include "nm_proto_id.h"
#include "nm_proto.h"
#include "nm_gate.h"
#include "nm_trk.h"
#include "nm_errno.h"

/** Build and post a new send request of a ctrl pkt.
 */
static
int
nm_mini_alt_make_ctrl_pkt(struct nm_sched		 *p_sched,
                      struct nm_mini_alt_gate	 *p_mini_alt_gate,
                      struct nm_pkt_wrap	 *p_src_pw,
                      struct nm_pkt_wrap	**pp_dst_pw) {
        struct nm_core		*p_core		= NULL;
        struct nm_mini_alt_sched	*p_mini_alt_sched	= NULL;
        struct nm_pkt_wrap	*p_dst_pw	= NULL;
        struct nm_mini_alt_ctrl	*p_mini_alt_ctrl	= NULL;

        int err;

        p_core		= p_sched->p_core;
        p_mini_alt_sched	= p_sched->sch_private;

        /* allocate packet wrapper
         */
        err	= nm_pkt_wrap_alloc(p_core, &p_dst_pw,
                                        nm_pi_sched,
                                        p_mini_alt_gate->seq++);
        if (err != NM_ESUCCESS)
                goto out;

        /* allocate iov
         */
        err	= nm_iov_alloc(p_core, p_dst_pw, 1);
        if (err != NM_ESUCCESS) {
                nm_pkt_wrap_free(p_core, p_dst_pw);
                goto out;
        }

        /* allocate control pkt buffer
         */
        p_mini_alt_ctrl	= tbx_malloc(p_mini_alt_sched->ctrl_mem);
        if (!p_mini_alt_ctrl) {
                nm_iov_free(p_core, p_dst_pw);
                nm_pkt_wrap_free(p_core, p_dst_pw);
                goto out;
        }

        /* initialize control pkt
         */
        p_mini_alt_ctrl->proto_id	= p_src_pw->proto_id;
        p_mini_alt_ctrl->seq	= p_src_pw->seq;

        /* append control pkt buffer to iovec
         */
        err	= nm_iov_append_buf(p_core, p_dst_pw,
                                        p_mini_alt_ctrl, sizeof(struct nm_mini_alt_ctrl));
        if (err != NM_ESUCCESS) {
                NM_DISPF("nm_iov_append_buf returned %d", err);
        }

        /* return new pkt
         */
        *pp_dst_pw	= p_dst_pw;
        err	= NM_ESUCCESS;

 out:
        return err;
}

/** Check if track is available.
 */
static
int
nm_check_track(struct nm_gate_drv	 *const p_gdrv,
               struct nm_gate_trk	 *const p_gtrk) {
        int	err = -NM_EAGAIN;

        /* all the track sending slots are currently in use */
        if (p_gtrk->p_trk->cap.max_pending_send_request &&
            p_gtrk->p_trk->out_req_nb >= p_gtrk->p_trk->cap.max_pending_send_request)
                goto out;

        err = NM_ESUCCESS;

 out:
        return err;
}

/** Schedule and post new outgoing buffers.
 */
int
nm_mini_alt_out_schedule_gate(struct nm_gate *p_gate) {
        p_tbx_slist_t 		 pre		= NULL;
        p_tbx_slist_t		 post		= NULL;
        struct nm_pkt_wrap	*p_pw		= NULL;
        struct nm_mini_alt_gate	*p_mini_alt_gate	= NULL;

        struct nm_gate_drv	**const p_gdrv_a	=
                p_gate->p_gate_drv_array;

        int	_err;
        int	 err;

        pre	= p_gate->pre_sched_out_list;
        post	= p_gate->post_sched_out_list;

        /* already one pending request */
        if (p_gate->out_req_nb)
                goto early_out;

        p_mini_alt_gate	= p_gate->sch_private;

        /* nothing to send, exit */
        if (!p_mini_alt_gate->state
            &&
            !p_mini_alt_gate->p_next_pw
            &&
            tbx_slist_is_nil(pre))
                goto early_out;


        /* check scheduler state */
        if (p_mini_alt_gate->state == 0) {
                /* time to send a control pkt */
                struct nm_gate_drv	 *const p_gdrv	= p_gdrv_a[0];
                struct nm_gate_trk	 *const p_gtrk	= p_gdrv->p_gate_trk_array[0];

                /* peek first element from list */
                if (!p_mini_alt_gate->p_next_pw)
                        p_mini_alt_gate->p_next_pw	= tbx_slist_extract(pre);

                /* see if driver is ready
                   - note: it should be since we send one request at a time,
                   check is done for debugging purpose
                */
                _err	= nm_check_track(p_gdrv, p_gtrk);

                if (_err != NM_ESUCCESS) {
                        err = -NM_EAGAIN;
                        goto out;
                }

                /* build control pkt to describe the next data pkt
                 */
                err = nm_mini_alt_make_ctrl_pkt(p_gate->p_sched, p_mini_alt_gate,
                                            p_mini_alt_gate->p_next_pw, &p_pw);
                if (err != NM_ESUCCESS) {
                        NM_DISPF("mini_alt_make_ctrl_pkt returned %d", err);
                }

                p_pw->p_gate		= p_gate;
                p_pw->p_drv		= p_gdrv->p_drv;
                p_pw->p_trk		= p_gtrk->p_trk;
                p_pw->p_gdrv		= p_gdrv;
                p_pw->p_gtrk		= p_gtrk;
                p_mini_alt_gate->state	= 1;

                NM_TRACE_PTR("adding a ctrl pkt to outbound post", p_pw);
        } else {
                /* time to send a data pkt
                   - note: send on track 1 for now
                 */
                struct nm_gate_drv	 *const p_gdrv	= p_gdrv_a[0];
                struct nm_gate_trk	 *const p_gtrk	= p_gdrv->p_gate_trk_array[1];

                p_pw	= p_mini_alt_gate->p_next_pw;
                p_pw->p_drv		= p_gdrv->p_drv;
                p_pw->p_trk		= p_gtrk->p_trk;
                p_pw->p_gdrv		= p_gdrv;
                p_pw->p_gtrk		= p_gtrk;
                p_mini_alt_gate->p_next_pw	= NULL;
                p_mini_alt_gate->state	= 0;

                NM_TRACE_PTR("adding a data pkt to outbound post", p_pw);
       }

        /* append pkt to scheduler post list
         */
        tbx_slist_append(post, p_pw);

 early_out:
        err	= NM_ESUCCESS;

 out:
        return err;
}


/** Handle a complete successful outgoing request.
 */
int
nm_mini_alt_out_process_success_rq(struct nm_sched		*p_sched,
                               struct nm_pkt_wrap	*p_pw) {
        struct nm_gate		*p_gate		= NULL;
        struct nm_mini_alt_sched	*p_mini_alt_sched	= NULL;
        struct nm_core		*p_core		= NULL;
        int	err;

        p_gate		= p_pw->p_gate;
        p_mini_alt_sched	= p_sched->sch_private;
        p_core		= p_sched->p_core;

        if (p_pw->proto_id == nm_pi_sched) {
                /* control pkt */

                /* free buffer
                 */
                tbx_free(p_mini_alt_sched->ctrl_mem, p_pw->v[0].iov_base);

                /* free iovec
                 */
                nm_iov_free(p_core, p_pw);

                /* free pkt_wrap
                 */
                nm_pkt_wrap_free(p_core, p_pw);

        } else {
                /* data pkt */

                struct nm_proto		*p_proto	= NULL;

                p_proto	= p_core->p_proto_array[p_pw->proto_id];
                if (!p_proto) {
                        NM_TRACEF("unregistered protocol: %d", p_proto->id);
                        goto discard;
                }

                if (!p_proto->ops.out_success)
                        goto discard;

                err	= p_proto->ops.out_success(p_proto, p_pw);
                if (err != NM_ESUCCESS) {
                        NM_DISPF("proto.ops.out_success returned: %d", err);
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

/** Handle a failed outgoing request.
 */
int
nm_mini_alt_out_process_failed_rq(struct nm_sched		*p_sched,
                              struct nm_pkt_wrap	*p_pw,
                              int		 	_err) {
        struct nm_gate		*p_gate		= NULL;
        struct nm_mini_alt_sched	*p_mini_alt_sched	= NULL;
        struct nm_core		*p_core		= NULL;
        int	err;

        p_gate		= p_pw->p_gate;
        p_mini_alt_sched	= p_sched->sch_private;
        p_core		= p_sched->p_core;

        if (p_pw->proto_id == nm_pi_sched) {
                /* control pkt */

                /* free buffer
                 */
                tbx_free(p_mini_alt_sched->ctrl_mem, p_pw->v[0].iov_base);

                /* free iovec
                 */
                nm_iov_free(p_core, p_pw);

                /* free pkt_wrap
                 */
                nm_pkt_wrap_free(p_core, p_pw);

        } else {
                /* data pkt */

                struct nm_proto		*p_proto	= NULL;

                p_proto	= p_core->p_proto_array[p_pw->proto_id];
                if (!p_proto) {
                        NM_TRACEF("unregistered protocol: %d", p_proto->id);
                        goto discard;
                }

                if (!p_proto->ops.out_failed)
                        goto discard;

                err	= p_proto->ops.out_failed(p_proto, p_pw, _err);
                if (err != NM_ESUCCESS) {
                        NM_DISPF("proto.ops.out_failed returned: %d", err);
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

