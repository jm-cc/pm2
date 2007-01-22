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

#include "nm_null_private.h"

/* schedule and post new outgoing buffers */
int
nm_null_out_schedule_gate(struct nm_gate *p_gate) {
        p_tbx_slist_t 		 pre		= NULL;
        p_tbx_slist_t		 post		= NULL;
        struct nm_pkt_wrap	*p_pw		= NULL;
        struct nm_null_gate	*p_null_gate	= NULL;

        struct nm_gate_drv	**const p_gdrv_a	=
                p_gate->p_gate_drv_array;
        struct nm_gate_drv	 *const p_gdrv	= p_gdrv_a[0];
        struct nm_gate_trk	 *const p_gtrk	= p_gdrv->p_gate_trk_array[0];

        int	 err;

        pre	= p_gate->pre_sched_out_list;
        post	= p_gate->post_sched_out_list;

        /* already one pending request */
        if (p_gate->out_req_nb)
                goto out;

        p_null_gate	= p_gate->sch_private;

        /* nothing to send, exit */
        if (tbx_slist_is_nil(pre))
                goto out;

        p_pw	= tbx_slist_extract(pre);
        p_pw->p_gate		= p_gate;
        p_pw->p_drv		= p_gdrv->p_drv;
        p_pw->p_trk		= p_gtrk->p_trk;
        p_pw->p_gdrv		= p_gdrv;
        p_pw->p_gtrk		= p_gtrk;


        /* append pkt to scheduler post list
         */
        tbx_slist_append(post, p_pw);

 out:
        err	= NM_ESUCCESS;

        return err;
}


/* process complete successful outgoing request */
int
nm_null_out_process_success_rq(struct nm_sched		*p_sched,
                               struct nm_pkt_wrap	*p_pw) {
        struct nm_core		*p_core		= NULL;
        struct nm_proto		*p_proto	= NULL;
        int	err;

        p_core		= p_sched->p_core;

        p_proto	= p_core->p_proto_array[p_pw->proto_id];
        if (!p_proto) {
                NM_TRACEF("unregistered protocol: %d", p_pw->proto_id);
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

        err	= NM_ESUCCESS;

        return err;
}

/* process complete failed outgoing request */
int
nm_null_out_process_failed_rq(struct nm_sched		*p_sched,
                              struct nm_pkt_wrap	*p_pw,
                              int		 	_err) {
        struct nm_core		*p_core		= NULL;
        struct nm_proto		*p_proto	= NULL;
        int	err;

        p_core		= p_sched->p_core;

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

        err	= NM_ESUCCESS;

        return err;
}

