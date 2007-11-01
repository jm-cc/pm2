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

#include "nm_public.h"
#include "nm_gate.h"
#include "nm_sched.h"
#include "nm_pkt_wrap.h"
#include "nm_trk_rq.h"

/* initialize the scheduler instance and memory allocators
 */
static
int
nm_null_schedule_init	(struct nm_sched	*p_sched) {
        struct nm_null_sched	*p_priv	=	NULL;
        int	err;

        p_priv	= TBX_MALLOC(sizeof(struct nm_null_sched));
        if (!p_priv) {
                err = -NM_ENOMEM;
                goto out;
        }
        p_sched->sch_private	= p_priv;

        err	= NM_ESUCCESS;

 out:
        return err;
}

/* initialize one track
 *
 * the scheduler uses a single track only for example purpose
 */
static
int
nm_null_init_trks	(struct nm_sched	*p_sched,
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
        struct nm_core		*p_core	= NULL;
        int	err;

        p_core		= p_sched->p_core;

        /* Track 0
         */
        err = nm_core_trk_alloc(p_core, p_drv, &trk_rq_0);

        return err;

}

/* handler called when a new gate is opened
 */
static
int
nm_null_init_gate	(struct nm_sched	*p_sched,
                         struct nm_gate		*p_gate) {
        struct nm_null_gate	*p_gate_priv	=	NULL;
        int	err;

        p_gate_priv	= TBX_MALLOC(sizeof(struct nm_null_gate));
        if (!p_gate_priv) {
                err = -NM_ENOMEM;
                goto out;
        }

        p_gate->sch_private	= p_gate_priv;
        err	= NM_ESUCCESS;

 out:
        return err;
}

int
nm_null_load		(struct nm_sched_ops	*p_ops) {
        p_ops->init			= nm_null_schedule_init;

        p_ops->init_trks		= nm_null_init_trks;
        p_ops->init_gate		= nm_null_init_gate;

        p_ops->out_schedule_gate	= nm_null_out_schedule_gate;
        p_ops->out_process_success_rq	= nm_null_out_process_success_rq;
        p_ops->out_process_failed_rq	= nm_null_out_process_failed_rq;

        p_ops->in_schedule		= nm_null_in_schedule;
        p_ops->in_process_success_rq	= nm_null_in_process_success_rq;
        p_ops->in_process_failed_rq	= nm_null_in_process_failed_rq;

        return NM_ESUCCESS;
}
