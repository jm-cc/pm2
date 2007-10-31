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
#include "nm_so_public.h"
#include "nm_so_pkt_wrap.h"
#include "nm_sched.h"
#include "nm_trk_rq.h"
#include "nm_log.h"

#include "nm_so_strategies/nm_so_strat_aggreg_autoextended.h"
#include "nm_so_strategies/nm_so_strat_aggreg_extended.h"
#include "nm_so_strategies/nm_so_strat_aggreg.h"
#include "nm_so_strategies/nm_so_strat_default.h"
#ifdef NMAD_QOS
#include "nm_so_strategies/nm_so_strat_qos.h"
#endif /* NMAD_QOS */
#include "nm_so_strategies/nm_so_strat_split_balance.h"

#include <nm_predictions.h>


#define INITIAL_CHUNK_NUM (NM_SO_MAX_TAGS * NM_SO_PENDING_PACKS_WINDOW)

p_tbx_memory_t nm_so_chunk_mem = NULL;

/** Initialize the scheduler.
 */
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
  p_priv->current_strategy = &nm_so_strat_split_balance;
#elif defined(CONFIG_STRAT_DEFAULT)
  p_priv->current_strategy = &nm_so_strat_default;
#elif defined(CONFIG_STRAT_AGGREG)
  p_priv->current_strategy = &nm_so_strat_aggreg;
#ifdef NMAD_QOS
#elif defined(CONFIG_STRAT_QOS)
  p_priv->current_strategy = &nm_so_strat_qos;
#endif /* NMAD_QOS */
#elif defined(CONFIG_STRAT_AGGREG_EXTENDED)
  p_priv->current_strategy = &nm_so_strat_aggreg_extended;
#elif defined(CONFIG_STRAT_AGGREG_AUTOEXTENDED)
  p_priv->current_strategy = &nm_so_strat_aggreg_autoextended;
#else
  /* Fall back to the default strategy */
  p_priv->current_strategy = &nm_so_strat_default;
#endif

  /* Initialize strategy */
  p_priv->current_strategy->init();

  p_sched->sch_private	= p_priv;

  tbx_malloc_init(&nm_so_chunk_mem,
                  sizeof(struct nm_so_chunk),
                  INITIAL_CHUNK_NUM, "nmad/.../sched_opt/nm_so_chunk");

  err = NM_ESUCCESS;

 out:
  return err;
}

/** Shutdown the scheduler.
 */
static int
nm_so_schedule_exit (struct nm_sched *p_sched)
{
  struct nm_so_sched *p_priv = p_sched->sch_private;

  /* Terminates strategy */
  if (p_priv->current_strategy->exit != NULL) {
    p_priv->current_strategy->exit();
  }

  while (!tbx_slist_is_nil(p_sched->post_aux_recv_req)) {
    void *pw = tbx_slist_extract(p_sched->post_aux_recv_req);
    nm_so_pw_free(pw);
  }

  while (!tbx_slist_is_nil(p_sched->post_perm_recv_req)) {
    void *pw = tbx_slist_extract(p_sched->post_perm_recv_req);
    nm_so_pw_free(pw);
  }

  while (!tbx_slist_is_nil(p_sched->pending_aux_recv_req)) {
    void *pw = tbx_slist_extract(p_sched->pending_aux_recv_req);
    nm_so_pw_free(pw);
  }

  while (!tbx_slist_is_nil(p_sched->pending_perm_recv_req)) {
    void *pw = tbx_slist_extract(p_sched->pending_perm_recv_req);
    nm_so_pw_free(pw);
  }

  /* Shutdown "Lightning Fast" Packet Wrappers Manager */
  nm_so_pw_exit();

  tbx_malloc_clean(nm_so_chunk_mem);

  TBX_FREE(p_priv);
  p_sched->sch_private = NULL;
  return NM_ESUCCESS;
}

/** Shutdown the tracks.
 */
static int
nm_so_close_trks(struct nm_sched	*p_sched,
                 struct nm_drv		*p_drv) {
  struct nm_core *p_core = p_sched->p_core;
  struct nm_trk  *p_trk;
  int i, err=NM_EAGAIN;

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

/** Initialize the tracks.
 */
static int
nm_so_init_trks	(struct nm_sched	*p_sched,
                 struct nm_drv		*p_drv) {
    struct nm_core *p_core	= p_sched->p_core;
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

/** Close a gate.
 */
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

  /* Release the last pw always in use */
  {
    p_tbx_slist_t pending_aux_slist, pending_perm_slist;
    pending_aux_slist = p_sched->pending_aux_recv_req;
    pending_perm_slist = p_sched->pending_perm_recv_req;

    while (!tbx_slist_is_nil(pending_aux_slist)) {
      struct nm_pkt_wrap *p_pw = tbx_slist_extract(pending_aux_slist);
      struct nm_so_pkt_wrap *p_so_pw =  nm_pw2so(p_pw);

      if(p_pw->p_drv->ops.release_req){
        p_pw->p_drv->ops.release_req(p_pw);
      }

      /* Free the wrapper */
      nm_so_pw_free(p_so_pw);
    }

    while (!tbx_slist_is_nil(pending_perm_slist)) {
      struct nm_pkt_wrap *p_pw = tbx_slist_extract(pending_perm_slist);
      struct nm_so_pkt_wrap *p_so_pw =  nm_pw2so(p_pw);

      if(p_pw->p_drv->ops.release_req){
        p_pw->p_drv->ops.release_req(p_pw);
      }

      /* Free the wrapper */
      nm_so_pw_free(p_so_pw);
    }
  }

  TBX_FREE(p_so_gate);
  return NM_ESUCCESS;
}

/** Open a gate.
 */
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

  for(i = 0; i < NM_SO_MAX_TAGS; i++)
    INIT_LIST_HEAD(&p_so_gate->pending_large_send[i]);

  INIT_LIST_HEAD(&p_so_gate->pending_large_recv);

  p_gate->sch_private = p_so_gate;

  p_so_sched->current_strategy->init_gate(p_gate);

  if(!p_so_sched->current_interface)
    TBX_FAILURE("Interface not defined");
  p_so_sched->current_interface->init_gate(p_gate);

  err = NM_ESUCCESS;

 out:
  return err;
}

/** Load the SchedOpt optimizing scheduler.
 */
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
