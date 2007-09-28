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

#include <tbx.h>
#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include "nm_so_private.h"
#include "nm_so_strategies/nm_so_strat_qos.h"
#include "nm_so_pkt_wrap.h"
#include "nm_so_tracks.h"
#include "nm_so_sendrecv_interface.h"
#include "nm_so_strategies/nm_so_strat_qos/nm_so_policy_fifo.h"
#include "nm_so_strategies/nm_so_strat_qos/nm_so_policy_latency.h"
#include "nm_so_strategies/nm_so_strat_qos/nm_so_policy_rate.h"
#include "nm_so_strategies/nm_so_strat_qos/nm_so_policy_priority.h"
#include "nm_so_strategies/nm_so_strat_qos/nm_so_policy_priority_latency.h"
#include "nm_so_strategies/nm_so_strat_qos/nm_so_policy_priority_rate.h"

struct nm_so_strat_qos_gate {
  /* list of raw outgoing packets */
  void *policy_priv[NM_SO_NB_POLICIES];
  uint8_t current_policy;
  uint8_t round;
  struct list_head pending_acks[NM_SO_NB_PRIORITIES];
  p_tbx_memory_t nm_so_ack_mem;
};

struct nm_so_strat_qos_ack{
  uint8_t tag;
  struct list_head link;
  uint8_t seq;
  uint8_t track_id;
};


static nm_so_policy* policies[NM_SO_NB_POLICIES];
static uint8_t policies_weights[NM_SO_NB_POLICIES];

/* Add a new control "header" to the flow of outgoing packets */
static int pack_ctrl(struct nm_gate *p_gate,
		     union nm_so_generic_ctrl_header *p_ctrl)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_strat_qos_gate *p_so_sq_gate
    = (struct nm_so_strat_qos_gate *)p_so_gate->strat_priv;
  int err;
  uint8_t tag = p_ctrl->r.tag_id - 128;
  int policy = nm_so_get_policy(tag);

  err = policies[policy]->pack_ctrl(p_so_sq_gate->policy_priv[policy],
				    p_ctrl);
  
  return err;
}

/* Handle the arrival of a new packet. The strategy may already apply
   some optimizations at this point */
static int pack(struct nm_gate *p_gate,
		uint8_t tag, uint8_t seq,
		void *data, uint32_t len)
{
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)p_gate->sch_private;
  struct nm_so_strat_qos_gate *p_so_sq_gate
    = (struct nm_so_strat_qos_gate *)p_so_gate->strat_priv;
  int err;
  int policy;
  static int i = 0;
  i++;

  policy = nm_so_get_policy(tag);

  err = policies[policy]->pack(p_gate, p_so_sq_gate->policy_priv[policy],
			       tag, seq, data, len);

  return err;
}

static int get_next_busy_policy(struct nm_so_strat_qos_gate *p_so_sq_gate)
{
  uint8_t policy, round, i;

  policy = p_so_sq_gate->current_policy;
  round = p_so_sq_gate->round;

  if(round == 0)
    {
      policy = (policy + 1) % NM_SO_NB_POLICIES;
      round = policies_weights[policy];
    }
  
  for(i = 0; i < NM_SO_NB_POLICIES; i++)
    {
      round--;
      if(policies[policy]->busy(p_so_sq_gate->policy_priv[policy]) > 0)
	{
	  p_so_sq_gate->round = round;
	  p_so_sq_gate->current_policy = policy;
	  return policy;
	}
      
      policy = (policy + 1) % NM_SO_NB_POLICIES;
      round = policies_weights[policy];
      
    }
  
  return NM_SO_POLICY_UNDEFINED;
}


/* Compute and apply the best possible packet rearrangement, then
   return next packet to send */
static int try_and_commit(struct nm_gate *p_gate)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_strat_qos_gate *p_so_sq_gate
    = (struct nm_so_strat_qos_gate *)p_so_gate->strat_priv;
  int err = NM_ESUCCESS;
  uint8_t policy;
   
  if(p_so_gate->active_send[NM_SO_DEFAULT_NET][TRK_SMALL] ==
     NM_SO_MAX_ACTIVE_SEND_PER_TRACK)
    /* We're done */
    goto out;

  if((policy = get_next_busy_policy(p_so_sq_gate)) == NM_SO_POLICY_UNDEFINED)
    /* No data available */
    goto out;
  
  err = 
    policies[policy]->try_and_commit(p_gate,
				     p_so_sq_gate->policy_priv[policy]);
 out:  
  return err;
}

/* Initialization */
static int init(void)
{
  printf("[loading strategy: <QoS>]\n");
  policies[NM_SO_POLICY_FIFO] = &nm_so_policy_fifo;
  policies_weights[NM_SO_POLICY_FIFO] = 1;
  policies[NM_SO_POLICY_LATENCY] = &nm_so_policy_latency;
  policies_weights[NM_SO_POLICY_LATENCY] = 1;
  policies[NM_SO_POLICY_RATE] = &nm_so_policy_rate;
  policies_weights[NM_SO_POLICY_RATE] = 1;  
  policies[NM_SO_POLICY_PRIORITY] = &nm_so_policy_priority;
  policies_weights[NM_SO_POLICY_PRIORITY] = 1;
  policies[NM_SO_POLICY_PRIORITY_LATENCY] = &nm_so_policy_priority_latency;
  policies_weights[NM_SO_POLICY_PRIORITY_LATENCY] = 1;
  policies[NM_SO_POLICY_PRIORITY_RATE] = &nm_so_policy_priority_rate;
  policies_weights[NM_SO_POLICY_PRIORITY_RATE] = 1;
  
  return NM_ESUCCESS;
}

/* Warning: drv_id and trk_id are IN/OUT parameters. They initially
   hold values "suggested" by the caller. */
static int rdv_accept(struct nm_gate *p_gate,
		      unsigned long *drv_id,
		      unsigned long *trk_id)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  if(p_so_gate->active_recv[*drv_id][*trk_id] == 0)
    /* Cool! The suggested track is available! */
    return NM_ESUCCESS;
  else
    /* We decide to postpone the acknowledgement. */
    return -NM_EAGAIN;
}

static int ack_callback(struct nm_so_pkt_wrap *p_so_pw,
			uint8_t tag_id, uint8_t seq,
			uint8_t track_id, uint8_t finished)
{
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_strat_qos_gate *p_so_sq_gate
    = (struct nm_so_strat_qos_gate *)p_so_gate->strat_priv;
  struct nm_so_pkt_wrap *p_so_large_pw;
  struct nm_so_strat_qos_ack *p_so_ack;
  uint8_t tag = tag_id - 128;
  uint8_t i;
  
  if(!finished)
    {
      uint8_t priority = nm_so_get_priority(tag);
      
      p_so_ack = tbx_malloc(p_so_sq_gate->nm_so_ack_mem);
     
      p_so_ack->tag = tag;
      p_so_ack->seq = seq;
      p_so_ack->track_id = track_id;
      
      list_add_tail(&p_so_ack->link, &p_so_sq_gate->pending_acks[priority]);
    }
  else
    {
      for(i = 0; i < NM_SO_NB_PRIORITIES; i++)
	{
	  list_for_each_entry(p_so_ack, &p_so_sq_gate->pending_acks[i], link)
	    {
	      tag = p_so_ack->tag;
	      seq = p_so_ack->seq;
	      track_id = p_so_ack->track_id;
	      
	      NM_SO_TRACE("ACK completed for tag = %d, seq = %u\n", tag, seq);
	     
	      p_so_gate->pending_unpacks--;
	      
	      list_for_each_entry(p_so_large_pw, &p_so_gate->pending_large_send[tag], link)
		{
		  if(p_so_large_pw->pw.seq == seq) 
		    {
		      list_del(&p_so_large_pw->link);
		      
		      /* Send the data */
		      _nm_so_post_send(p_gate, p_so_large_pw,
				       track_id % NM_SO_MAX_TRACKS,
				       track_id / NM_SO_MAX_TRACKS);
		     
		      goto next;
		    }	    
		}
	      TBX_FAILURE("PANIC!\n");
	      
	    next:
	      list_del(&p_so_ack->link);
	      tbx_free(p_so_sq_gate->nm_so_ack_mem, p_so_ack);
	    }
	}
    }
  return NM_SO_HEADER_MARK_READ;
}

static int init_gate(struct nm_gate *p_gate)
{
  uint8_t i;
  struct nm_so_strat_qos_gate *priv
    = TBX_MALLOC(sizeof(struct nm_so_strat_qos_gate));
  
  for(i = 0; i < NM_SO_NB_POLICIES; i++)
    policies[i]->init_gate(&priv->policy_priv[i]);

  priv->current_policy = 0;
  priv->round = policies_weights[0];

  for(i = 0; i < NM_SO_NB_PRIORITIES; i++)
    INIT_LIST_HEAD(&priv->pending_acks[i]);
  
  tbx_malloc_init(&priv->nm_so_ack_mem,
		  sizeof(struct nm_so_strat_qos_ack),
		  16, "nmad/.../sched_opt/nm_so_strat_qos/ack");
  
  ((struct nm_so_gate *)p_gate->sch_private)->strat_priv = priv;

  return NM_ESUCCESS;
}

static int exit_gate(struct nm_gate *p_gate)
{
  uint8_t i;
  //  char host_name[30];
  struct nm_so_strat_qos_gate *priv = ((struct nm_so_gate *)p_gate->sch_private)->strat_priv;
  
  for(i = 0; i < NM_SO_NB_POLICIES; i++)
    policies[i]->exit_gate(&priv->policy_priv[i]);
  
  tbx_malloc_clean(priv->nm_so_ack_mem);

  TBX_FREE(priv);

  ((struct nm_so_gate *)p_gate->sch_private)->strat_priv = NULL;
  return NM_ESUCCESS;
}

nm_so_strategy nm_so_strat_qos = {
  .init = init,
  .init_gate = init_gate,
  .exit_gate = exit_gate,
  .pack = pack,
  .pack_ctrl = pack_ctrl,
  .try = NULL,
  .commit = NULL,
  .try_and_commit = try_and_commit,
  .cancel = NULL,
  .rdv_accept = rdv_accept,
  .ack_callback = ack_callback,
  .priv = NULL,
};

#endif /* NMAD_QOS */
