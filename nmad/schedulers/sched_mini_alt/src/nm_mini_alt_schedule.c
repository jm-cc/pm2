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

#include "nm_public.h"
#include "nm_sched.h"
#include "nm_trk_rq.h"
#include "nm_gate.h"

#define INITIAL_CTRL_NUM		16
#define INITIAL_TOKEN_NUM		16

/** Initialize the scheduler.
 */
static
int
nm_mini_alt_schedule_init	(struct nm_sched	*p_sched) {
        struct nm_mini_alt_sched	*p_priv	=	NULL;
        int	err;

        p_priv	= TBX_MALLOC(sizeof(struct nm_mini_alt_sched));
        if (!p_priv) {
                err = -NM_ENOMEM;
                goto out;
        }

        memset(p_priv, 0, sizeof(struct nm_mini_alt_sched));

        tbx_malloc_init(&(p_priv->ctrl_mem),   sizeof(struct nm_mini_alt_ctrl),
                        INITIAL_CTRL_NUM,   "nmad/sched_mini_alt/ctrl");
        p_priv->state		=    0;
        p_priv->pkt_list	= tbx_slist_nil();
        p_sched->sch_private	= p_priv;

        err	= NM_ESUCCESS;

 out:
        return err;
}

/** Initialize the tracks.
 */
static
int
nm_mini_alt_init_trks	(struct nm_sched	*p_sched,
                         struct nm_drv		*p_drv) {
        struct nm_trk_rq	 trk_rq_0 = {
                .p_trk	= NULL,
                .cap	= {
                        .rq_type			= nm_trk_rq_unspecified,
                        .iov_type			= nm_trk_iov_unspecified,
                        .max_pending_send_request	= 0,
                        .max_pending_recv_request	= 0,
                        .min_single_request_length	= 0,
                        .max_single_request_length	= 0,
                        .max_iovec_request_length	= 0,
                        .max_iovec_size			= 0
                },
                .flags	= 0
        };
        struct nm_trk_rq	 trk_rq_1 = {
                .p_trk	= NULL,
                .cap	= {
                        .rq_type			= nm_trk_rq_unspecified,
                        .iov_type			= nm_trk_iov_unspecified,
                        .max_pending_send_request	= 0,
                        .max_pending_recv_request	= 0,
                        .min_single_request_length	= 0,
                        .max_single_request_length	= 0,
                        .max_iovec_request_length	= 0,
                        .max_iovec_size			= 0
                },
                .flags	= 0
        };
        struct nm_core		*p_core	= NULL;
        int	err;

        p_core		= p_sched->p_core;

        /* Track 0
         */
        err = nm_core_trk_alloc(p_core, p_drv, &trk_rq_0);
        if (err != NM_ESUCCESS)
                goto out;

        /* Track 1
         */
        err = nm_core_trk_alloc(p_core, p_drv, &trk_rq_1);
        if (err != NM_ESUCCESS)
                goto out_free;

        err	= NM_ESUCCESS;

 out:
        return err;

 out_free:
        nm_core_trk_free(p_core, trk_rq_0.p_trk);
	goto out;
}

/** Open a new gate.
 */
static
int
nm_mini_alt_init_gate	(struct nm_sched	*p_sched,
                         struct nm_gate		*p_gate) {
        struct nm_mini_alt_gate	*p_gate_priv	=	NULL;
        struct nm_mini_alt_sched	*p_sched_priv	=	NULL;
        int	err;

        p_gate_priv	= TBX_MALLOC(sizeof(struct nm_mini_alt_gate));
        if (!p_gate_priv) {
                err = -NM_ENOMEM;
                goto out;
        }

        p_gate_priv->state		=    0;
        p_gate_priv->seq		=    0;
        p_gate_priv->p_next_pw	= NULL;

        p_gate->sch_private	= p_gate_priv;

        p_sched_priv	= p_sched->sch_private;
        p_sched_priv->sched_gates[p_gate->id].pkt_list		= tbx_slist_nil();
        p_sched_priv->sched_gates[p_gate->id].ctrl_list		= tbx_slist_nil();

        err	= NM_ESUCCESS;

 out:
        return err;
}

/** Load the scheduler.
 */
int
nm_mini_alt_load		(struct nm_sched_ops	*p_ops) {
        p_ops->init			= nm_mini_alt_schedule_init;

        p_ops->init_trks		= nm_mini_alt_init_trks;
        p_ops->init_gate		= nm_mini_alt_init_gate;

        p_ops->out_schedule_gate	= nm_mini_alt_out_schedule_gate;
        p_ops->out_process_success_rq	= nm_mini_alt_out_process_success_rq;
        p_ops->out_process_failed_rq	= nm_mini_alt_out_process_failed_rq;

        p_ops->in_schedule		= nm_mini_alt_in_schedule;
        p_ops->in_process_success_rq	= nm_mini_alt_in_process_success_rq;
        p_ops->in_process_failed_rq	= nm_mini_alt_in_process_failed_rq;

        return NM_ESUCCESS;
}
