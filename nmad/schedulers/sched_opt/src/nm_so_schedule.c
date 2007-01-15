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

#include "nm_so_private.h"
#include "nm_so_pkt_wrap.h"

#include "nm_so_strategies/nm_so_strat_default.h"
#include "nm_so_strategies/nm_so_strat_aggreg.h"
#include "nm_so_strategies/nm_so_strat_balance.h"

static int
nm_so_schedule_init (struct nm_sched *p_sched)
{
  struct nm_core *p_core = p_sched->p_core;
  int err;

  struct nm_so_sched *p_priv = NULL;

  /* Initialize "Lightning Fast" Packet Wrappers Manager */
  nm_so_pw_init(p_core);

  p_priv = TBX_MALLOC(sizeof(struct nm_so_sched));
  if (!p_priv) {
    err = -NM_ENOMEM;
    goto out;
  }

  memset(p_priv->any_src, 0, sizeof(p_priv->any_src));
  
  p_priv->next_gate_id = 0;
  p_priv->pending_any_src_unpacks = 0;

#if defined(CONFIG_MULTI_RAIL)
  p_priv->current_strategy = &nm_so_strat_balance;
#elif defined(CONFIG_STRAT_DEFAULT)
  p_priv->current_strategy = &nm_so_strat_default;
#elif defined(CONFIG_STRAT_AGGREG)
  p_priv->current_strategy = &nm_so_strat_aggreg;
#else
  /* Fall back to the default strategy */
  p_priv->current_strategy = &nm_so_strat_default;
#endif

  /* Initialize strategy */
  p_priv->current_strategy->init();

  p_sched->sch_private	= p_priv;

  err = NM_ESUCCESS;

 out:
  return err;
}

static int
nm_so_schedule_exit (struct nm_sched *p_sched)
{
  struct nm_so_sched *p_priv = p_sched->sch_private;

  /* Shutdown "Lightning Fast" Packet Wrappers Manager */
  nm_so_pw_exit();

  TBX_FREE(p_priv);
  p_sched->sch_private = NULL;
  return NM_ESUCCESS;
}

static int
nm_so_close_trks(struct nm_sched	*p_sched,
                 struct nm_drv		*p_drv) {
  struct nm_core *p_core = p_sched->p_core;
  struct nm_trk  *p_trk;
  int i, err;

  for(i=0 ; i<p_drv->nb_tracks ; i++) {
    p_trk = p_drv->p_track_array[i];
    err = nm_core_trk_free(p_core, p_trk);
    if (err != NM_ESUCCESS) {
      NM_DISPF("nm_core_trk_free returned %d", err);
      return err;
    }
  }
  return err;
}

static int
nm_so_init_trks	(struct nm_sched	*p_sched,
                 struct nm_drv		*p_drv) {
    struct nm_core *p_core	= p_sched->p_core;
    int	err;

    /* Track 0 - piste r�serv�e aux unexpected */
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

    err = nm_core_trk_alloc(p_core, p_drv, &trk_rq_0);
    if (err != NM_ESUCCESS)
        goto out;

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

static int
nm_so_close_gate(struct nm_sched	*p_sched,
                 struct nm_gate		*p_gate)
{
  struct nm_so_sched *p_so_sched = p_sched->sch_private;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  p_so_sched->current_strategy->exit_gate(p_gate);

  if(!p_so_sched->current_interface)
    TBX_FAILURE("Interface not defined");
  p_so_sched->current_interface->exit_gate(p_gate);

  TBX_FREE(p_so_gate);
  return NM_ESUCCESS;
}

static int
nm_so_init_gate	(struct nm_sched	*p_sched,
                 struct nm_gate		*p_gate)
{
  struct nm_so_sched *p_so_sched = p_sched->sch_private;
  struct nm_so_gate *p_so_gate = NULL;
  int i;
  int err;

  p_so_gate = TBX_MALLOC(sizeof(struct nm_so_gate));
  if (!p_so_gate) {
    err = -NM_ENOMEM;
    goto out;
  }

  p_so_gate->p_so_sched = p_so_sched;

  memset(p_so_gate->active_recv, 0, sizeof(p_so_gate->active_recv));
  memset(p_so_gate->active_send, 0, sizeof(p_so_gate->active_send));

  memset(p_so_gate->status, 0, sizeof(p_so_gate->status));

  memset(p_so_gate->send_seq_number, 0, sizeof(p_so_gate->send_seq_number));
  memset(p_so_gate->recv_seq_number, 0, sizeof(p_so_gate->recv_seq_number));

  memset(p_so_gate->recv, 0, sizeof(p_so_gate->recv));

  p_so_gate->pending_unpacks = 0;

  p_gate->sch_private = p_so_gate;

  for(i = 0; i < NM_SO_MAX_TAGS; i++)
    INIT_LIST_HEAD(&p_so_gate->pending_large_send[i]);

  INIT_LIST_HEAD(&p_so_gate->pending_large_recv);

  p_so_sched->current_strategy->init_gate(p_gate);

  if(!p_so_sched->current_interface)
    TBX_FAILURE("Interface not defined");
  p_so_sched->current_interface->init_gate(p_gate);

  err = NM_ESUCCESS;

 out:
  return err;
}


int
nm_so_load		(struct nm_sched_ops	*p_ops)
{
  p_ops->init			= nm_so_schedule_init;
  p_ops->exit			= nm_so_schedule_exit;

  p_ops->init_trks		= nm_so_init_trks;
  p_ops->init_gate		= nm_so_init_gate;

  p_ops->close_trks             = nm_so_close_trks;
  p_ops->close_gate             = nm_so_close_gate;

  p_ops->out_schedule_gate      = nm_so_out_schedule_gate;
  p_ops->out_process_success_rq = nm_so_out_process_success_rq;
  p_ops->out_process_failed_rq  = nm_so_out_process_failed_rq;

  p_ops->in_schedule		 = nm_so_in_schedule;
  p_ops->in_process_success_rq   = nm_so_in_process_success_rq;
  p_ops->in_process_failed_rq	 = nm_so_in_process_failed_rq;

  return NM_ESUCCESS;
}
