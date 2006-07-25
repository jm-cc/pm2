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

#include <nm_public.h>
#include <nm_rdv_public.h>
#include "nm_so_private.h"
#include "nm_so_optimizer.h"
#include "nm_so_pkt_wrap.h"

//#define CHRONO




static int
nm_so_schedule_init (struct nm_sched *p_sched)
{
  struct nm_core *p_core = p_sched->p_core;
  int err;
  struct nm_so_sched *p_priv = NULL;

  // Initialize optimizer
  nm_so_optimizer_init();

  p_priv = TBX_MALLOC(sizeof(struct nm_so_sched));
  if (!p_priv) {
    err = -NM_ENOMEM;
    goto out;
  }
  memset(p_priv, 0, sizeof(struct nm_so_sched));

    p_priv->trks_to_update = tbx_slist_nil();

    //// initialisation des à pré-poster
    //err = nm_so_init_pre_posted(p_core, p_priv,
    //                            NB_CREATED_PRE_POSTED);
    //
    //if(err != NM_ESUCCESS){
    //    NM_DISPF("nm_so_init_pre_posted returned err = %d\n", err);
    //    goto out;
    //}

    // allocateur rapide des entetes globales et de données
    //tbx_malloc_init(&p_priv->sched_header_key,
    //                sizeof(struct nm_so_sched_header),
    //                NB_CREATED_SCHED_HEADER, "so_sched_header");
    //
    //tbx_malloc_init(&p_priv->header_key,
    //                sizeof(struct nm_so_header),
    //                NB_CREATED_HEADER, "so_header");

    //// initialisation de pack d'agrégation à envoyer
    //err = nm_so_init_aggregation_pw(p_core, p_priv,
    //                                NB_AGGREGATION_PW);
    //if(err != NM_ESUCCESS){
    //    NM_DISPF("nm_so_init_aggregation_pw returned err = %d\n",
    //             err);
    //    goto out;
    //}

    /* Enregistrement de protocole de rdv */
    err = nm_core_proto_init(p_core, nm_rdv_load,
                             &p_priv->p_proto_rdv);
    if (err != NM_ESUCCESS) {
        NM_DISPF("nm_core_proto_init returned err = %d\n", err);
        goto out;
    }

    p_priv->unexpected_list = tbx_slist_nil();

    p_priv->acks = NULL;

    p_sched->sch_private	= p_priv;

    err	= NM_ESUCCESS;

 out:
    return err;
}

static int
nm_so_init_trks	(struct nm_sched	*p_sched,
                 struct nm_drv		*p_drv) {
    struct nm_core *p_core	= p_sched->p_core;
    struct nm_so_sched *so_sched = p_sched->sch_private;
    p_tbx_slist_t trks_to_update = so_sched->trks_to_update;
    int	err;

    /* Track 0 - piste réservée aux unexpected */
    struct nm_trk_rq trk_rq_0 = {
        .p_trk	= NULL,
        .cap	= {
            .rq_type			= nm_trk_rq_unspecified,
            .iov_type			= nm_trk_iov_unspecified,
            .max_pending_send_request	= 0,
            .max_pending_recv_request	= 0,
            .min_single_request_length	= 0,
            .max_single_request_length	= 0,
            .max_iovec_request_length	= 0,
            .max_iovec_size		= 0
        },
        .flags	= 0
    };

    err = p_core->ops.trk_alloc(p_core, p_drv, &trk_rq_0.p_trk);
    if (err != NM_ESUCCESS)
        goto out;

    err = p_drv->ops.open_trk(&trk_rq_0);
    if (err != NM_ESUCCESS) {
        goto out_free;
    }

    so_sched->so_trks[0].pending_stream_intro = tbx_false;

    if(trk_rq_0.p_trk->cap.rq_type == nm_trk_rq_stream
       || trk_rq_0.p_trk->cap.rq_type == nm_trk_rq_dgram){
        tbx_slist_append(trks_to_update, trk_rq_0.p_trk);
    }

    /* Track 1 - piste pour les longs*/
    struct nm_trk_rq trk_rq_1 = {
        .p_trk	= NULL,
        .cap	= {
            .rq_type			= nm_trk_rq_rdv,
            .iov_type			= nm_trk_iov_unspecified,
            .max_pending_send_request	= 0,
            .max_pending_recv_request	= 0,
            .min_single_request_length	= 0,
            .max_single_request_length	= 0,
            .max_iovec_request_length	= 0,
            .max_iovec_size		= 0
        },
        .flags	= 0
    };

    err = p_core->ops.trk_alloc(p_core, p_drv, &trk_rq_1.p_trk);
    if (err != NM_ESUCCESS)
        goto out_free;

    err = p_drv->ops.open_trk(&trk_rq_1);
    if (err != NM_ESUCCESS) {
        goto out_free_2;
    }

    so_sched->so_trks[1].pending_stream_intro = tbx_false;

    trk_rq_1.p_trk->cap.rq_type = nm_trk_rq_rdv;


    /* Track 2 - piste pour les longs*/
    struct nm_trk_rq trk_rq_2 = {
        .p_trk	= NULL,
        .cap	= {
            .rq_type			= nm_trk_rq_rdv,
            .iov_type			= nm_trk_iov_unspecified,
            .max_pending_send_request	= 0,
            .max_pending_recv_request	= 0,
            .min_single_request_length	= 0,
            .max_single_request_length	= 0,
            .max_iovec_request_length	= 0,
            .max_iovec_size		= 0
        },
        .flags	= 0
    };

    err = p_core->ops.trk_alloc(p_core, p_drv, &trk_rq_2.p_trk);
    if (err != NM_ESUCCESS)
        goto out_free;

    err = p_drv->ops.open_trk(&trk_rq_2);
    if (err != NM_ESUCCESS) {
        goto out_free_3;
    }

    so_sched->so_trks[2].pending_stream_intro = tbx_false;

    trk_rq_2.p_trk->cap.rq_type = nm_trk_rq_rdv;

    ///* Track 3 - piste pour les longs*/
    //struct nm_trk_rq trk_rq_3 = {
    //    .p_trk	= NULL,
    //    .cap	= {
    //        .rq_type			= nm_trk_rq_rdv,
    //        .iov_type			= nm_trk_iov_unspecified,
    //        .max_pending_send_request	= 0,
    //        .max_pending_recv_request	= 0,
    //        .min_single_request_length	= 0,
    //        .max_single_request_length	= 0,
    //        .max_iovec_request_length	= 0,
    //        .max_iovec_size		= 0
    //    },
    //    .flags	= 0
    //};
    //
    //err = p_core->ops.trk_alloc(p_core, p_drv, &trk_rq_3.p_trk);
    //if (err != NM_ESUCCESS)
    //    goto out_free;
    //
    //err = p_drv->ops.open_trk(&trk_rq_3);
    //if (err != NM_ESUCCESS) {
    //    goto out_free_4;
    //}
    //
    //so_sched->so_trks[3].pending_stream_intro = tbx_false;
    //
    //trk_rq_3.p_trk->cap.rq_type = nm_trk_rq_rdv;

    err	= NM_ESUCCESS;

 out:
    return err;

// out_free_4:
//    p_core->ops.trk_free(p_core, trk_rq_3.p_trk);

 out_free_3:
    p_core->ops.trk_free(p_core, trk_rq_2.p_trk);

 out_free_2:
    p_core->ops.trk_free(p_core, trk_rq_1.p_trk);

 out_free:
    p_core->ops.trk_free(p_core, trk_rq_0.p_trk);
    goto out;
}

static int
nm_so_init_gate	(struct nm_sched	*p_sched,
                 struct nm_gate		*p_gate) {
    struct nm_so_gate	*p_gate_priv	=	NULL;
    int	err;

    p_gate_priv	= TBX_MALLOC(sizeof(struct nm_so_gate));
    if (!p_gate_priv) {
        err = -NM_ENOMEM;
        goto out;
    }
    p_gate_priv->next_pw = NULL;

    p_gate->sch_private	= p_gate_priv;

    err	= NM_ESUCCESS;

 out:
    return err;
}

int
nm_so_load		(struct nm_sched_ops	*p_ops)
{
    p_ops->init			= nm_so_schedule_init;

    p_ops->init_trks		= nm_so_init_trks;
    p_ops->init_gate		= nm_so_init_gate;

    p_ops->out_schedule_gate	  = nm_so_out_schedule_gate;
    p_ops->out_process_success_rq = nm_so_out_process_success_rq;
    p_ops->out_process_failed_rq  = nm_so_out_process_failed_rq;

    p_ops->in_schedule		 = nm_so_in_schedule;
    p_ops->in_process_success_rq = nm_so_in_process_success_rq;
    p_ops->in_process_failed_rq	 = nm_so_in_process_failed_rq;

    return NM_ESUCCESS;
}

/******* Utilitaires **********/

void
nm_so_control_error(char *fct_name, int err){
    if(err != NM_ESUCCESS){
        NM_DISPF("%s returned %d", fct_name, err);
        TBX_FAILURE("err != NM_ESUCCESS");
    }
}
