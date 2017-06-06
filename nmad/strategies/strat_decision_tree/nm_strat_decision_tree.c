/*
 * NewMadeleine
 * Copyright (C) 2015 (see AUTHORS file)
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

/* import program_invocation_name */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>

#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <nm_trace.h>
#include <nm_private.h>
#include <Padico/Module.h>


/* ********************************************************* */

static void strat_decision_tree_try_and_commit(void*, nm_gate_t );
static void strat_decision_tree_rdv_accept(void*, nm_gate_t );

static const struct nm_strategy_iface_s nm_strat_decision_tree_driver =
  {
    .pack_data          = NULL,
    .pack_ctrl          = NULL,
    .try_and_commit     = &strat_decision_tree_try_and_commit,
    .rdv_accept         = &strat_decision_tree_rdv_accept
};

static void*strat_decision_tree_instantiate(puk_instance_t, puk_context_t);
static void strat_decision_tree_destroy(void*);

static const struct puk_component_driver_s nm_strat_decision_tree_component_driver =
  {
    .instantiate = &strat_decision_tree_instantiate,
    .destroy     = &strat_decision_tree_destroy
  };

/* ********************************************************* */

PADICO_MODULE_COMPONENT(NewMad_Strategy_decision_tree,
			puk_component_declare("NewMad_Strategy_decision_tree",
					      puk_component_provides("PadicoComponent", "component", &nm_strat_decision_tree_component_driver),
					      puk_component_provides("NewMad_Strategy", "strat", &nm_strat_decision_tree_driver),
					      puk_component_attr("nm_max_small", "16342"),
					      puk_component_attr("nm_copy_on_send_threshold", "4096"))
			);

/* ********************************************************* */

/** one sample of state */
struct nm_strat_decision_tree_sample_s
{
  tbx_tick_t timestamp;
  nm_len_t submitted_size;    /**< size of submitted data */
  int nb_pw;                  /**< number of pw in the outlist */
  nm_len_t size_outlist;      /**< total size of data in outlist, in bytes */
  nm_len_t pw_length;         /**< length of the first pw in outlist */
  nm_len_t pw_remaining_data; /**< remaining data space in first pw */

  int profile_latency;        /**< driver latency */
  int profile_bandwidth;      /**< driver bw */
};

#define NM_STRAT_DECISION_TREE_SAMPLES_MAX 1000000

/** Per-gate status for strat instances
 */
struct nm_strat_decision_tree
{
  int nm_max_small;
  int nm_copy_on_send_threshold;
  tbx_tick_t time_orig;
  struct nm_strat_decision_tree_sample_s samples[NM_STRAT_DECISION_TREE_SAMPLES_MAX];
  int n_samples;
};

/** Initialize the gate storage for strategy.
 */
static void*strat_decision_tree_instantiate(puk_instance_t ai, puk_context_t context)
{
  struct nm_strat_decision_tree*p_status = TBX_MALLOC(sizeof(struct nm_strat_decision_tree));
  const char*nm_max_small = puk_instance_getattr(ai, "nm_max_small");
  p_status->nm_max_small = atoi(nm_max_small);
  const char*nm_copy_on_send_threshold = puk_instance_getattr(ai, "nm_copy_on_send_threshold");
  p_status->nm_copy_on_send_threshold = atoi(nm_copy_on_send_threshold);
  p_status->n_samples = 0;
  TBX_GET_TICK(p_status->time_orig);
  return (void*)p_status;
}

/** Cleanup the gate storage for strategy.
 */
static void strat_decision_tree_destroy(void*_status)
{
  struct nm_strat_decision_tree*p_status = _status;
  char template[64] = "/tmp/nmad-decision-tree-XXXXXX";
  int fd = mkstemp(template);
  FILE*f = fdopen(fd, "w");
  fprintf(stderr, "# strat_decision_tree- flushing %d samples to %s\n", p_status->n_samples, template);
  fprintf(f, "# ## %s\n", program_invocation_name);
  fprintf(f, "# timestamp         size  nb_pw   size_outlist\n");
  int i;
  for(i = 0; i < p_status->n_samples; i++)
    {
      const struct nm_strat_decision_tree_sample_s*sample = &p_status->samples[i];
      const double t = TBX_TIMING_DELAY(p_status->time_orig, sample->timestamp);
      const double t_end = (sample->profile_bandwidth > 0) ? t + sample->profile_latency + (sample->submitted_size / sample->profile_bandwidth) : t;
      fprintf(f, "%16.2f %16.2f %10ld %4d %10ld\n", t, t_end, sample->submitted_size, sample->nb_pw, sample->size_outlist);
    }
  fclose(f);
  close(fd);
  TBX_FREE(p_status);
}


/** Compute and apply the best possible packet rearrangement, then
 *  return next packet to send.
 *
 *  @param p_gate a pointer to the gate object.
 */
static void strat_decision_tree_try_and_commit(void*_status, nm_gate_t p_gate)
{
  struct nm_strat_decision_tree*p_status = _status;
  nm_drv_t p_drv = nm_drv_default(p_gate);
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
  struct nm_core*p_core = p_gate->p_core;

  /* ******************************************************* */
#if 0
  /* old tracing code- disabled for now, since value traced here 
   * do not make sense anymore in the new strategy model.
   */
  {
    nm_len_t size_outlist = 0;
    int nb_pw = 0;
    int smaller_pw_size = NM_SO_MAX_UNEXPECTED;
    int max_reamaining_data_area = 0;
    struct nm_pkt_wrap_s *p_pw_trace = NULL;
    
    if(!nm_pkt_wrap_list_empty(&p_gate->out_list))
      {
	puk_list_foreach(p_pw_trace, &p_gate->out_list)
	  {
	    size_outlist = size_outlist + p_pw_trace->length;
	    const int aux = nm_pw_remaining_buf(p_pw_trace);
	    if (aux > max_reamaining_data_area )
	      max_reamaining_data_area = aux;
	    if (p_pw_trace->length < smaller_pw_size)
	      smaller_pw_size = p_pw_trace->length ;
	    nb_pw++;
	  }
	p_pw_trace = nm_pkt_wrap_list_begin(&p_gate->out_list);
      }
    struct nm_strat_decision_tree_sample_s sample =
      {
	.submitted_size    = chunk_len,
	.nb_pw             = nb_pw,
	.size_outlist      = size_outlist,
	.pw_length         = p_pw_trace ? p_pw_trace->length : 0,
	.pw_remaining_data = p_pw_trace ? nm_pw_remaining_buf(p_pw_trace) : 0,
	.profile_latency   = p_drv->profile.latency,
	.profile_bandwidth = p_drv->profile.bandwidth
      };
    TBX_GET_TICK(sample.timestamp);
    p_status->samples[p_status->n_samples] = sample;
    if(p_status->n_samples < NM_STRAT_DECISION_TREE_SAMPLES_MAX - 1)
      p_status->n_samples++;
  }
#endif
  /* ******************************************************* */

  if(p_gdrv->p_pw_send[NM_TRK_SMALL] == NULL)
    {
      if(!nm_ctrl_chunk_list_empty(&p_gate->ctrl_chunk_list))
	{
	  /* post ctrl on trk #0 */
	  struct nm_ctrl_chunk_s*p_ctrl_chunk = nm_ctrl_chunk_list_pop_front(&p_gate->ctrl_chunk_list);
	  struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_global_header();
	  nm_pw_add_control(p_pw, &p_ctrl_chunk->ctrl);
	  nm_core_post_send(p_gate, p_pw, NM_TRK_SMALL, p_drv);
	  nm_ctrl_chunk_free(p_core->ctrl_chunk_allocator, p_ctrl_chunk);
	}
      else if(!nm_req_chunk_list_empty(&p_gate->req_chunk_list))
	{
	  struct nm_req_chunk_s*p_req_chunk = nm_req_chunk_list_pop_front(&p_gate->req_chunk_list);
	  struct nm_req_s*p_pack = p_req_chunk->p_req;
	  const struct nm_data_properties_s*p_props = nm_data_properties_get(&p_pack->data);
	  const nm_len_t max_header_len = NM_HEADER_DATA_SIZE + p_props->blocks * sizeof(struct nm_header_pkt_data_chunk_s);
	  if(p_req_chunk->chunk_len + max_header_len <= p_status->nm_max_small)
	    {
	      /* post short data on trk #0 */
	      struct nm_pkt_wrap_s*p_pw = nm_pw_alloc_global_header();
	      nm_pw_add_req_chunk(p_pw, p_req_chunk, NM_REQ_FLAG_NONE);
	      assert(p_pw->length <= NM_SO_MAX_UNEXPECTED);
	      nm_core_post_send(p_gate, p_pw, NM_TRK_SMALL, p_drv);
	    }
	  else
	    {
	      /* post RDV for large data */
	      const int is_lastchunk = (p_req_chunk->chunk_offset + p_req_chunk->chunk_len == p_pack->pack.len);
	      struct nm_pkt_wrap_s*p_large_pw = nm_pw_alloc_noheader();
	      nm_pw_add_req_chunk(p_large_pw, p_req_chunk, NM_REQ_FLAG_NONE);
	      nm_pkt_wrap_list_push_back(&p_gate->pending_large_send, p_large_pw);
	      union nm_header_ctrl_generic_s rdv;
	      nm_header_init_rdv(&rdv, p_pack, p_req_chunk->chunk_len, p_req_chunk->chunk_offset,
				 is_lastchunk ? NM_PROTO_FLAG_LASTCHUNK : 0);
	      struct nm_pkt_wrap_s*p_rdv_pw = nm_pw_alloc_global_header();
	      nm_pw_add_control(p_rdv_pw, &rdv);
	      nm_core_post_send(p_gate, p_rdv_pw, NM_TRK_SMALL, p_drv);
	    }
	}
    }
}

/** Emit RTR for received RDV requests
 */
static void strat_decision_tree_rdv_accept(void*_status, nm_gate_t p_gate)
{
  struct nm_pkt_wrap_s*p_pw = nm_pkt_wrap_list_begin(&p_gate->pending_large_recv);
  if(p_pw != NULL)
    {
      nm_drv_t p_drv = nm_drv_default(p_gate);
      struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
      if(p_gdrv->p_pw_recv[NM_TRK_LARGE] == NULL)
	{
	  /* The large-packet track is available- post recv and RTR */
	  struct nm_rdv_chunk chunk = 
	    { .len = p_pw->length, .p_drv = p_drv, .trk_id = NM_TRK_LARGE };
	  nm_pkt_wrap_list_erase(&p_gate->pending_large_recv, p_pw);
	  nm_tactic_rtr_pack(p_pw, 1, &chunk);
	}
    }
}
