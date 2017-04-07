/*
 * NewMadeleine
 * Copyright (C) 2006-2016 (see AUTHORS file)
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

#include <nm_private.h>

#include <Padico/Module.h>

static int nm_strat_split_balance_load(void);

PADICO_MODULE_BUILTIN(NewMad_Strategy_split_balance, &nm_strat_split_balance_load, NULL, NULL);

/* Components structures:
 */

static void strat_split_balance_pack_chunk(void*_status, struct nm_req_s*p_pack, void*ptr, nm_len_t len, nm_len_t chunk_offset);
static void strat_split_balance_pack_ctrl(void*, nm_gate_t , const union nm_header_ctrl_generic_s*);
static int  strat_split_balance_try_and_commit(void*, nm_gate_t );
static void strat_split_balance_rdv_accept(void*, nm_gate_t );

static const struct nm_strategy_iface_s nm_strat_split_balance_driver =
  {
    .pack_chunk         = &strat_split_balance_pack_chunk,
    .pack_ctrl          = &strat_split_balance_pack_ctrl,
    .try_and_commit     = &strat_split_balance_try_and_commit,
    .rdv_accept         = &strat_split_balance_rdv_accept,
    .flush              = NULL,
};

static void*strat_split_balance_instantiate(puk_instance_t, puk_context_t);
static void strat_split_balance_destroy(void*);

static const struct puk_component_driver_s nm_strat_split_balance_component_driver =
  {
    .instantiate = &strat_split_balance_instantiate,
    .destroy     = &strat_split_balance_destroy
  };

struct nm_strat_split_balance
{
  unsigned nb_packets;
  int nm_max_small;
  int nm_copy_on_send_threshold;
};

/** Component declaration */
static int nm_strat_split_balance_load(void)
{
  puk_component_declare("NewMad_Strategy_split_balance",
			puk_component_provides("PadicoComponent", "component", &nm_strat_split_balance_component_driver),
			puk_component_provides("NewMad_Strategy", "strat", &nm_strat_split_balance_driver),
			puk_component_attr("nm_max_small", "16342"),
			puk_component_attr("nm_copy_on_send_threshold", "4096"));
  return NM_ESUCCESS;
}


/** Initialize the gate storage for split_balance strategy.
 */
static void*strat_split_balance_instantiate(puk_instance_t ai, puk_context_t context)
{
  struct nm_strat_split_balance *status = TBX_MALLOC(sizeof(struct nm_strat_split_balance));

  status->nb_packets = 0;

  const char*nm_max_small = puk_instance_getattr(ai, "nm_max_small");
  status->nm_max_small = atoi(nm_max_small);

  const char*nm_copy_on_send_threshold = puk_instance_getattr(ai, "nm_copy_on_send_threshold");
  status->nm_copy_on_send_threshold = atoi(nm_copy_on_send_threshold);

  return (void*)status;
}

/** Cleanup the gate storage for split_balance strategy.
 */
static void strat_split_balance_destroy(void*status)
{
  TBX_FREE(status);
}

/* Add a new control "header" to the flow of outgoing packets */
static void strat_split_balance_pack_ctrl(void *_status, nm_gate_t p_gate, const union nm_header_ctrl_generic_s *p_ctrl)
{
  struct nm_pkt_wrap_s*p_pw = NULL;
  struct nm_strat_split_balance*status = _status;
  if(!nm_pkt_wrap_list_empty(&p_gate->out_list))
    {
      /* Inspect only the head of the list */
      p_pw = nm_pkt_wrap_list_begin(&p_gate->out_list);
      /* If the paquet is reasonably small, we can form an aggregate */
      if(NM_HEADER_CTRL_SIZE <= nm_pw_remaining_buf(p_pw))
	{
	  nm_pw_add_control(p_pw, p_ctrl);
	  return;
	}
    }
  /* Otherwise, simply form a new packet wrapper */
  p_pw = nm_pw_alloc_global_header();
  nm_pw_add_control(p_pw, p_ctrl);

  /* Add the control packet to the BEGINING of out_list */
  nm_pkt_wrap_list_push_front(&p_gate->out_list, p_pw);
  status->nb_packets++;

}


static void strat_split_balance_launch_large_chunk(void *_status, struct nm_req_s*p_pack, const void *data, nm_len_t len, nm_len_t chunk_offset, uint8_t is_last_chunk)
{
  struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_noheader();
  nm_pw_add_data_chunk(p_pw, p_pack, data, len, chunk_offset, 0);
  p_pw->chunk_offset = chunk_offset;
  nm_pkt_wrap_list_push_back(&p_pack->p_gate->pending_large_send, p_pw);
  union nm_header_ctrl_generic_s ctrl;
  nm_header_init_rdv(&ctrl, p_pack, len, chunk_offset, is_last_chunk ? NM_PROTO_FLAG_LASTCHUNK : 0);
  strat_split_balance_pack_ctrl(_status, p_pack->p_gate, &ctrl);
}

static void 
strat_split_balance_try_to_agregate_small(void *_status, struct nm_req_s*p_pack,
					  const void *data, nm_len_t len, nm_len_t chunk_offset, uint8_t is_last_chunk)
{
  struct nm_strat_split_balance*status = _status;
  struct nm_pkt_wrap_s*p_pw;

  /* We aggregate ONLY if data are very small OR if there are
     already two ready packets */

  if(p_pack->pack.len <= 512 || status->nb_packets >= 2)
    {
      /* We first try to find an existing packet to form an aggregate */
      puk_list_foreach(p_pw, &p_pack->p_gate->out_list)
	{
	  const nm_len_t h_rlen = nm_pw_remaining_buf(p_pw);
	  const nm_len_t size = NM_HEADER_DATA_SIZE + nm_aligned(len);
	  if(size <= h_rlen)
	    {
	      /* We can copy data into the header zone */
	      nm_pw_add_data_chunk(p_pw, p_pack, data, len, chunk_offset, NM_PW_DATA_USE_COPY);
	      return;
	    }
	}
    }

  /* We didn't have a chance to form an aggregate, so simply form a
     new packet wrapper and add it to the out_list */
  p_pw = nm_pw_alloc_global_header();
  nm_pw_add_data_chunk(p_pw, p_pack, data, len, chunk_offset, NM_PW_DATA_USE_COPY);
  p_pw->chunk_offset = chunk_offset;
  nm_pkt_wrap_list_push_back(&p_pack->p_gate->out_list, p_pw);
  status->nb_packets++;
}

/** push message chunk */
static void strat_split_balance_pack_chunk(void*_status, struct nm_req_s*p_pack, void*ptr, nm_len_t len, nm_len_t chunk_offset)
{
  struct nm_strat_split_balance*status = _status;
  tbx_bool_t is_last_chunk = (chunk_offset + len >= p_pack->pack.len);
  if(len <= status->nm_max_small)
    {
      /* Small packet */
      strat_split_balance_try_to_agregate_small(status, p_pack, ptr, len, chunk_offset, is_last_chunk);
    }
  else
    {
      /* Large packets are split in 2 chunks. */
      strat_split_balance_launch_large_chunk(status, p_pack, ptr, len, chunk_offset, is_last_chunk);
    }
}


/* Compute and apply the best possible packet rearrangement, then
   return next packet to send */
static int strat_split_balance_try_and_commit(void *_status, nm_gate_t p_gate)
{
  struct nm_strat_split_balance*status = _status;
  int nb_drivers = p_gate->p_core->nb_drivers;
  int n = 0;
  nm_drv_t const*p_drvs = NULL;
  nm_ns_inc_lats(p_gate->p_core, &p_drvs, &nb_drivers);
  assert(nb_drivers > 0);
  while(n < nb_drivers && !nm_pkt_wrap_list_empty(&p_gate->out_list))
    {
      nm_drv_t p_drv = p_drvs[n];
      struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
      if(p_gdrv != NULL &&
	 p_gdrv->active_send[NM_TRK_SMALL] == 0 &&
	 p_gdrv->active_send[NM_TRK_LARGE] == 0)
	{
	  /* We found an idle NIC
	   * Take the packet at the head of the list and post it on trk #0 */
	  struct nm_pkt_wrap_s *p_pw = nm_pkt_wrap_list_pop_front(&p_gate->out_list);
	  status->nb_packets--;
	  nm_core_post_send(p_gate, p_pw, NM_TRK_SMALL, p_drv);
	}
      n++;
    }
  return NM_ESUCCESS;
}

/** Emit RTR for received RDV requests
 */
static void strat_split_balance_rdv_accept(void*_status, nm_gate_t p_gate)
{
  struct nm_pkt_wrap_s*p_pw = nm_pkt_wrap_list_begin(&p_gate->pending_large_recv);
  if(p_pw != NULL)
    {
      int nb_drivers = p_gate->p_core->nb_drivers;
      int chunk_index = 0;
      struct nm_rdv_chunk chunks[nb_drivers];
      const nm_trk_id_t trk_id = NM_TRK_LARGE;
      nm_drv_t const*ordered_drv_id_by_bw = NULL;
      nm_ns_dec_bws(p_gate->p_core, &ordered_drv_id_by_bw, &nb_drivers);
      int i;
      for(i = 0; i < nb_drivers; i++)
	{
	  nm_drv_t p_drv = ordered_drv_id_by_bw[i];
	  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
	  if(p_gdrv != NULL && p_gdrv->active_recv[trk_id] == 0)
	    {
	      chunks[chunk_index].p_drv = p_drv;
	      chunks[chunk_index].trk_id = NM_TRK_LARGE;
	      chunk_index++;
	    }
	}
      int nb_chunks = chunk_index;
      if(nb_chunks == 1)
	{
	  chunks[0].len = p_pw->length;
	}
      else if(nb_chunks > 1)
	{
	  nm_ns_multiple_split_ratio(p_pw->length, p_gate->p_core, &nb_chunks, chunks);
	}      
      if(nb_chunks > 0)
	{
	  nm_pkt_wrap_list_erase(&p_gate->pending_large_recv, p_pw);
	  nm_tactic_rtr_pack(p_pw, nb_chunks, chunks);
	}
    }
}


