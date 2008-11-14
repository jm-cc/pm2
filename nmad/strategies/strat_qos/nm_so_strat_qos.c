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

#ifdef NMAD_QOS

#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <nm_private.h>

#include "nm_so_policy_fifo.h"
#include "nm_so_policy_latency.h"
#include "nm_so_policy_rate.h"
#include "nm_so_policy_priority.h"
#include "nm_so_policy_priority_latency.h"
#include "nm_so_policy_priority_rate.h"
#include "nm_so_strat_qos.h"

/* Components structures:
*/

static int strat_qos_pack(void*, struct nm_gate*, nm_tag_t, uint8_t, void*, uint32_t);
static int strat_qos_pack_ctrl(void*, struct nm_gate *, union nm_so_generic_ctrl_header*);
static int strat_qos_try_and_commit(void*, struct nm_gate*);
static int strat_qos_rdv_accept(void*, struct nm_gate*, nm_drv_id_t*, uint8_t*);
static int strat_qos_ack_callback(void *, struct nm_pkt_wrap *, nm_tag_t, uint8_t, nm_trk_id_t, uint8_t);

static const struct nm_strategy_iface_s nm_so_strat_qos_driver =
  {
    .pack           = &strat_qos_pack,
    .pack_ctrl      = &strat_qos_pack_ctrl,
    .try_and_commit = &strat_qos_try_and_commit,
    .ack_callback   = &strat_qos_ack_callback,
    .rdv_accept     = &strat_qos_rdv_accept
  };

static void*strat_qos_instanciate(puk_instance_t, puk_context_t);
static void strat_qos_destroy(void*);

static const struct puk_adapter_driver_s nm_so_strat_qos_adapter_driver =
  {
    .instanciate = &strat_qos_instanciate,
    .destroy     = &strat_qos_destroy
  };

/** Component declaration */
static int nm_strat_qos_load(void)
{
  nm_sr_qos_table_init(&qos);
  puk_component_declare("NewMad_Strategy_qos",
			puk_component_provides("PadicoAdapter", "adapter", &nm_so_strat_qos_adapter_driver),
			puk_component_provides("NewMad_Strategy", "strat", &nm_so_strat_qos_driver));

  return NM_ESUCCESS;
}

struct nm_so_strat_qos_ack {
  nm_tag_t tag;
  struct list_head link;
  uint8_t seq;
  nm_trk_id_t track_id;
};

/** Per-gate status for strat qos instances
 */
struct nm_so_strat_qos {
  /** List of raw outgoing packets. */
  struct list_head out_list;
  nm_so_policy* policies[NM_SO_NB_POLICIES];
  uint8_t policies_weights[NM_SO_NB_POLICIES];

  void *policy_priv[NM_SO_NB_POLICIES];
  uint8_t current_policy;
  uint8_t round;
  struct list_head pending_acks[NM_SO_NB_PRIORITIES];
  p_tbx_memory_t nm_so_ack_mem;

  struct nm_so_strat_qos_ack *p_ack;
};

struct nm_sr_qos_s
{
  /** priority value for the given tag */
  uint8_t priority;
  
  /** policy linked with corresponding tag */
  uint8_t policy;
};
static inline void nm_sr_qos_ctor(struct nm_sr_qos_s*p_sr_qos, nm_tag_t tag)
{
  p_sr_qos->priority = (tag <= 4) ? 0 : NM_SO_NB_PRIORITIES - 1;
  p_sr_qos->policy = NM_SO_POLICY_PRIORITY;
}
static inline void nm_sr_qos_dtor(struct nm_sr_qos_s*p_sr_qos)
{
}
NM_TAG_TABLE_TYPE(nm_sr_qos,
		  struct nm_sr_qos_s,
		  nm_sr_qos_ctor,
		  nm_sr_qos_dtor);
static struct nm_sr_qos_table_s qos;

/* ** Public functions ************************************* */

void nm_so_set_priority(nm_tag_t tag, uint8_t priority)
{
  if(tag < NM_SO_MAX_TAGS && priority < NM_SO_NB_PRIORITIES)
    {
      struct nm_sr_qos_s*p_sr_qos = nm_sr_qos_get(&qos, tag);
      p_sr_qos->priority = priority;
    }
}

uint8_t nm_so_get_priority(nm_tag_t tag)
{
  if (tag < NM_SO_MAX_TAGS)
    {
      struct nm_sr_qos_s*p_sr_qos = nm_sr_qos_get(&qos, tag);
      return p_sr_qos->priority;
    }

  /* Undefined priority */
  return NM_SO_NB_PRIORITIES - 1;
}

void nm_so_set_policy(nm_tag_t tag, nm_qos_policy_t policy)
{
  if(tag < NM_SO_MAX_TAGS && policy < NM_SO_NB_POLICIES)
    {
      struct nm_sr_qos_s*p_sr_qos = nm_sr_qos_get(&qos, tag);
      p_sr_qos->policy = policy;
    }
}

nm_qos_policy_t nm_so_get_policy(nm_tag_t tag)
{
  if (tag < NM_SO_MAX_TAGS)
    {
      struct nm_sr_qos_s*p_sr_qos = nm_sr_qos_get(&qos, tag);
      return p_sr_qos->policy;
    }

  /* Undefined policy */
  return NM_SO_POLICY_FIFO;
}


/* ** Component methods ************************************ */

/** Initialize the gate storage for qos strategy.
 */
static void*strat_qos_instanciate(puk_instance_t ai, puk_context_t context)
{
  struct nm_so_strat_qos*status = TBX_MALLOC(sizeof(struct nm_so_strat_qos));
  int i;

  INIT_LIST_HEAD(&status->out_list);

  NM_LOGF("[loading strategy: <qos>]");

  status->policies[NM_SO_POLICY_FIFO] = &nm_so_policy_fifo;
  status->policies_weights[NM_SO_POLICY_FIFO] = 1;
  status->policies[NM_SO_POLICY_LATENCY] = &nm_so_policy_latency;
  status->policies_weights[NM_SO_POLICY_LATENCY] = 1;
  status->policies[NM_SO_POLICY_RATE] = &nm_so_policy_rate;
  status->policies_weights[NM_SO_POLICY_RATE] = 1;
  status->policies[NM_SO_POLICY_PRIORITY] = &nm_so_policy_priority;
  status->policies_weights[NM_SO_POLICY_PRIORITY] = 1;
  status->policies[NM_SO_POLICY_PRIORITY_LATENCY] = &nm_so_policy_priority_latency;
  status->policies_weights[NM_SO_POLICY_PRIORITY_LATENCY] = 1;
  status->policies[NM_SO_POLICY_PRIORITY_RATE] = &nm_so_policy_priority_rate;
  status->policies_weights[NM_SO_POLICY_PRIORITY_RATE] = 1;

  for(i = 0; i < NM_SO_NB_POLICIES; i++)
    status->policies[i]->init_gate(&status->policy_priv[i]);

  status->current_policy = 0;
  status->round = status->policies_weights[0];

  for(i = 0; i < NM_SO_NB_PRIORITIES; i++)
    INIT_LIST_HEAD(&status->pending_acks[i]);

  tbx_malloc_init(&status->nm_so_ack_mem,
		  sizeof(struct nm_so_strat_qos_ack),
		  16, "nmad/.../sched_opt/nm_so_strat_qos/ack");

  return (void*)status;
}

/** Cleanup the gate storage for aggreg strategy.
 */
static void strat_qos_destroy(void *_status)
{
  struct nm_so_strat_qos *status = _status;
  uint8_t i;

  for(i = 0; i < NM_SO_NB_POLICIES; i++)
    status->policies[i]->exit_gate(&status->policy_priv[i]);

  tbx_malloc_clean(status->nm_so_ack_mem);

  TBX_FREE(status);
}

/* Add a new control "header" to the flow of outgoing packets */
static int strat_qos_pack_ctrl(void*_status,
                               struct nm_gate *p_gate,
                               union nm_so_generic_ctrl_header *p_ctrl)
{
  struct nm_so_strat_qos *status = _status;
  nm_tag_t tag = p_ctrl->r.tag_id - 128;
  int policy = nm_so_get_policy(tag);
  int err;

  err = status->policies[policy]->pack_ctrl(status->policy_priv[policy],
                                            p_ctrl);

  return err;
}

/* Handle the arrival of a new packet. The strategy may already apply
   some optimizations at this point */
static int strat_qos_pack(void*_status,
                          struct nm_gate *p_gate,
                          nm_tag_t tag, uint8_t seq,
                          void *data, uint32_t len)
{
  struct nm_so_strat_qos *status = _status;
  int err;
  int policy;
  static int i = 0;
  i++;

  policy = nm_so_get_policy(tag);

  err = status->policies[policy]->pack(p_gate, status->policy_priv[policy],
                                       tag, seq, data, len);

  return err;
}

static int get_next_busy_policy(struct nm_so_strat_qos *status)
{
  uint8_t policy, round, i;

  policy = status->current_policy;
  round = status->round;

  if(round == 0)
    {
      policy = (policy + 1) % NM_SO_NB_POLICIES;
      round = status->policies_weights[policy];
    }

  for(i = 0; i < NM_SO_NB_POLICIES; i++)
    {
      round--;
      if(status->policies[policy]->busy(status->policy_priv[policy]) > 0)
	{
	  status->round = round;
	  status->current_policy = policy;
	  return policy;
	}

      policy = (policy + 1) % NM_SO_NB_POLICIES;
      round = status->policies_weights[policy];

    }

  return NM_SO_POLICY_UNDEFINED;
}


/* Compute and apply the best possible packet rearrangement, then
   return next packet to send */
static int strat_qos_try_and_commit(void*_status,
                                    struct nm_gate *p_gate)
{
  struct nm_so_strat_qos *status = _status;
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  int err = NM_ESUCCESS;
  uint8_t policy;

  if(p_so_gate->active_send[NM_SO_DEFAULT_NET][NM_TRK_SMALL] ==
     NM_SO_MAX_ACTIVE_SEND_PER_TRACK)
    /* We're done */
    goto out;

  if((policy = get_next_busy_policy(status)) == NM_SO_POLICY_UNDEFINED)
    /* No data available */
    goto out;

  err = status->policies[policy]->try_and_commit(p_gate,
                                                 status->policy_priv[policy]);
 out:
  return err;
}

/* Warning: drv_id and trk_id are IN/OUT parameters. They initially
   hold values "suggested" by the caller. */
static int strat_qos_rdv_accept(void *_status,
                                struct nm_gate *p_gate,
				nm_drv_id_t *drv_id,
				nm_trk_id_t *trk_id)
{
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;

  if(p_so_gate->active_recv[*drv_id][*trk_id] == 0)
    /* Cool! The suggested track is available! */
    return NM_ESUCCESS;
  else
    /* We decide to postpone the acknowledgement. */
    return -NM_EAGAIN;
}

static int strat_qos_ack_callback(void *_status,
                                  struct nm_pkt_wrap *p_so_pw,
                                  nm_tag_t tag_id, uint8_t seq,
                                  nm_trk_id_t track_id, uint8_t finished)
{
  struct nm_so_strat_qos *status = _status;
  struct nm_gate *p_gate = p_so_pw->p_gate;
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  struct nm_pkt_wrap *p_so_large_pw;
  nm_tag_t tag = tag_id - 128;
  uint8_t i;

  if(!finished)
    {
      uint8_t priority = nm_so_get_priority(tag);

      status->p_ack = tbx_malloc(status->nm_so_ack_mem);

      status->p_ack->tag = tag;
      status->p_ack->seq = seq;
      status->p_ack->track_id = track_id;

      list_add_tail(&status->p_ack->link, &status->pending_acks[priority]);
    }
  else
    {
      for(i = 0; i < NM_SO_NB_PRIORITIES; i++)
	{
	  list_for_each_entry(status->p_ack, &status->pending_acks[i], link)
	    {
	      tag = status->p_ack->tag;
	      seq = status->p_ack->seq;
	      track_id = status->p_ack->track_id;

	      NM_SO_TRACE("ACK completed for tag = %d, seq = %u\n", tag, seq);

	      p_so_gate->pending_unpacks--;

	      list_for_each_entry(p_so_large_pw, &nm_so_tag_get(&p_so_gate->tags, tag)->pending_large_send, link)
		{
		  if(p_so_large_pw->seq == seq)
		    {
		      list_del(&p_so_large_pw->link);

		      /* Send the data */
		      nm_core_post_send(p_gate, p_so_large_pw,
				       track_id % NM_SO_MAX_TRACKS,
				       track_id / NM_SO_MAX_TRACKS);

		      goto next;
		    }
		}
	      TBX_FAILURE("PANIC!\n");

	    next:
	      list_del(&status->p_ack->link);
	      tbx_free(status->nm_so_ack_mem, status->p_ack);
	    }
	}
    }
  return NM_SO_HEADER_MARK_READ;
}

#endif /* NMAD_QOS */
