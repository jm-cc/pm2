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
#define _GNU_SOURCE
#include <errno.h>

#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <nm_trace.h>
#include <nm_private.h>
#include <Padico/Module.h>


/* ********************************************************* */

static int  strat_decision_tree_todo(void*, struct nm_gate*);/* todo: s/nm_gate/nm_pack/ ? */
static void strat_decision_tree_pack_chunk(void*_status, struct nm_pack_s*p_pack, void*ptr, nm_len_t len, nm_len_t chunk_offset);
static int  strat_decision_tree_pack_ctrl(void*, struct nm_gate *, const union nm_so_generic_ctrl_header*);
static int  strat_decision_tree_try_and_commit(void*, struct nm_gate*);
static void strat_decision_tree_rdv_accept(void*, struct nm_gate*);

static const struct nm_strategy_iface_s nm_strat_decision_tree_driver =
  {
    .todo               = &strat_decision_tree_todo,
    .pack_chunk         = &strat_decision_tree_pack_chunk,
    .pack_ctrl          = &strat_decision_tree_pack_ctrl,
    .try_and_commit     = &strat_decision_tree_try_and_commit,
    .rdv_accept         = &strat_decision_tree_rdv_accept,
    .flush              = NULL
};

static void*strat_decision_tree_instanciate(puk_instance_t, puk_context_t);
static void strat_decision_tree_destroy(void*);

static const struct puk_adapter_driver_s nm_strat_decision_tree_adapter_driver =
  {
    .instanciate = &strat_decision_tree_instanciate,
    .destroy     = &strat_decision_tree_destroy
  };

/* ********************************************************* */

PADICO_MODULE_COMPONENT(NewMad_Strategy_decision_tree,
			puk_component_declare("NewMad_Strategy_decision_tree",
					      puk_component_provides("PadicoAdapter", "adapter", &nm_strat_decision_tree_adapter_driver),
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
  /** List of raw outgoing packets. */
  struct tbx_fast_list_head out_list;
  int nm_max_small;
  int nm_copy_on_send_threshold;
  tbx_tick_t time_orig;
  struct nm_strat_decision_tree_sample_s samples[NM_STRAT_DECISION_TREE_SAMPLES_MAX];
  int n_samples;
};

/** Initialize the gate storage for strategy.
 */
static void*strat_decision_tree_instanciate(puk_instance_t ai, puk_context_t context)
{
  struct nm_strat_decision_tree *status = TBX_MALLOC(sizeof(struct nm_strat_decision_tree));
  TBX_INIT_FAST_LIST_HEAD(&status->out_list);
  const char*nm_max_small = puk_instance_getattr(ai, "nm_max_small");
  status->nm_max_small = atoi(nm_max_small);
  const char*nm_copy_on_send_threshold = puk_instance_getattr(ai, "nm_copy_on_send_threshold");
  status->nm_copy_on_send_threshold = atoi(nm_copy_on_send_threshold);
  status->n_samples = 0;
  TBX_GET_TICK(status->time_orig);
  return (void*)status;
}

/** Cleanup the gate storage for strategy.
 */
static void strat_decision_tree_destroy(void*_status)
{
  struct nm_strat_decision_tree*status = _status;
  char template[64] = "/tmp/nmad-decision-tree-XXXXXX";
  int fd = mkstemp(template);
  FILE*f = fdopen(fd, "w");
  fprintf(stderr, "# strat_decision_tree- flushing %d samples to %s\n", status->n_samples, template);
  fprintf(f, "# ## %s\n", program_invocation_name);
  fprintf(f, "# timestamp         size  nb_pw   size_outlist\n");
  int i;
  for(i = 0; i < status->n_samples; i++)
    {
      const struct nm_strat_decision_tree_sample_s*sample = &status->samples[i];
      double t = TBX_TIMING_DELAY(status->time_orig, sample->timestamp);
      fprintf(f, "%16.2f %10ld %4d %10ld\n", t, sample->submitted_size, sample->nb_pw, sample->size_outlist);
    }
  fclose(f);
  close(fd);
  TBX_FREE(_status);
}


/** Add a new control "header" to the flow of outgoing packets.
 *
 *  @param _status the strat_decision_tree instance status.
 *  @param p_ctrl a pointer to the ctrl header.
 *  @return The NM status.
 */
static int strat_decision_tree_pack_ctrl(void*_status, struct nm_gate*p_gate, const union nm_so_generic_ctrl_header *p_ctrl)
{
  struct nm_strat_decision_tree*status = _status;
  nm_tactic_pack_ctrl(p_ctrl, &status->out_list);
  return NM_ESUCCESS;
}

static int strat_decision_tree_todo(void* _status, struct nm_gate*p_gate)
{
  struct nm_strat_decision_tree*status = _status;
  return !(tbx_fast_list_empty(&status->out_list));
}

/** push a message chunk */
static void strat_decision_tree_pack_chunk(void*_status, struct nm_pack_s*p_pack, void*ptr, nm_len_t len, nm_len_t chunk_offset)
{
  struct nm_strat_decision_tree*status = _status;

  /* ******************************************************* */
  {
    struct tbx_fast_list_head *out_list = &status->out_list;
    nm_len_t size_outlist = 0;
    int nb_pw = 0;
    int smaller_pw_size = NM_SO_MAX_UNEXPECTED;
    int max_reamaining_data_area = 0;
    struct nm_pkt_wrap *p_pw_trace = NULL;
    
    if(!tbx_fast_list_empty(out_list))
      {
	struct tbx_fast_list_head*pos;
	tbx_fast_list_for_each(pos, out_list)
	  {
	    p_pw_trace = nm_l2so(pos);	  
	    size_outlist = size_outlist + p_pw_trace->length;
	    const int aux = nm_so_pw_remaining_data(p_pw_trace);
	    if (aux > max_reamaining_data_area )
	      max_reamaining_data_area = aux;
	    if (p_pw_trace->length < smaller_pw_size)
	      smaller_pw_size = p_pw_trace->length ;
	    nb_pw++;
	  }
	p_pw_trace = nm_l2so(out_list->next);
      }
    struct nm_strat_decision_tree_sample_s sample =
      {
	.submitted_size    = len,
	.nb_pw             = nb_pw,
	.size_outlist      = size_outlist,
	.pw_length         = p_pw_trace ? p_pw_trace->length : 0,
	.pw_remaining_data = p_pw_trace ? nm_so_pw_remaining_data(p_pw_trace) : 0,
	/*	    .profile_latency   = p_drv->profile.latency,
		    .profile_bandwidth = p_drv->profile.bandwidth*/
      };
    TBX_GET_TICK(sample.timestamp);
    status->samples[status->n_samples] = sample;
    if(status->n_samples < NM_STRAT_DECISION_TREE_SAMPLES_MAX - 1)
      status->n_samples++;
  }
  /* ******************************************************* */
  
  if(len <= status->nm_max_small)
    {
      nm_tactic_pack_small_new_pw(p_pack, ptr, len, chunk_offset, 
				  status->nm_copy_on_send_threshold, &status->out_list);
    }
  else
    {
      nm_tactic_pack_rdv(p_pack, ptr, len, chunk_offset);
    }
}


/** Compute and apply the best possible packet rearrangement, then
 *  return next packet to send.
 *
 *  @param p_gate a pointer to the gate object.
 *  @return The NM status.
 */
static int strat_decision_tree_try_and_commit(void*_status, struct nm_gate *p_gate)
{
  struct nm_strat_decision_tree*status = _status;
  struct tbx_fast_list_head *out_list = &status->out_list;
  nm_drv_t p_drv = nm_drv_default(p_gate);
  struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);

#ifdef NMAD_TRACE
  int trace_co_id = p_gate->trace_connections_id;
  nm_trace_event(TOPO_CONNECTION, NM_TRACE_EVENT_TRY_COMMIT, NULL, trace_co_id);
  nm_trace_var(TOPO_CONNECTION, NM_TRACE_EVENT_VAR_CO_Outlist_Nb_Pw, nb_pw, trace_co_id);
  nm_trace_var(TOPO_CONNECTION, NM_TRACE_EVENT_VAR_CO_Outlist_Pw_Size, size_outlist, trace_co_id);
  nm_trace_var(TOPO_CONNECTION, NM_TRACE_EVENT_VAR_CO_Next_Pw_Size, p_pw_trace->length, trace_co_id);
  nm_trace_var(TOPO_CONNECTION, NM_TRACE_EVENT_VAR_CO_Next_Pw_Remaining_Data_Area, nm_so_pw_remaining_data(p_pw_trace), trace_co_id);
#endif
  
  if((p_gdrv->active_send[NM_TRK_SMALL] == 0) && !(tbx_fast_list_empty(out_list)))
    {
      struct nm_pkt_wrap *p_so_pw = nm_l2so(out_list->next);
#ifdef PROFILE_NMAD
      static long double wait_time = 0.0;
      static int count = 0, send_count = 0;
      static long long int send_size = 0;
      static tbx_tick_t t_orig;
      if(count != 0)
	{
	  tbx_tick_t t2;
	  TBX_GET_TICK(t2);
	  const long double t = TBX_TIMING_DELAY(t_orig, t2);
	  wait_time += t;
	  if(random() < 100000)
	    fprintf(stderr, "wait time = %fus (%fs) send = %d; size = %dMB; avg %d bytes/msg\n", 
		    (double)t, (double)wait_time / 1000000.0, send_count, (int)(send_size / 1000000),
		    (int)(send_size/send_count));
	}
      count = 0;
      send_count++;
      send_size += p_so_pw->length;
#endif /* PROFILE_NMAD */
      tbx_fast_list_del(out_list->next);
      /* Post packet on track 0 */
      nm_core_post_send(p_gate, p_so_pw, NM_TRK_SMALL, p_drv);

#ifdef NMAD_TRACE
      nm_trace_event(NM_TRACE_TOPO_CONNECTION, NM_TRACE_EVENT_Pw_Submited, NULL, trace_co_id);
      nm_trace_var(NM_TRACE_TOPO_CONNECTION, NM_TRACE_EVENT_VAR_CO_Pw_Submitted_Size, p_so_pw->length, trace_co_id);
      nm_trace_var(NM_TRACE_TOPO_CONNECTION, NM_TRACE_EVENT_VAR_CO_Gdrv_Profile_Latency, p_gdrv->p_drv->profile.latency, trace_co_id);
      nm_trace_var(NM_TRACE_TOPO_CONNECTION, NM_TRACE_EVENT_VAR_CO_Gdrv_Profile_Bandwidth, p_gdrv->p_drv->profile.bandwidth, trace_co_id);
#endif /* NMAD_TRACE */
    }
  else if((p_gdrv->active_send[NM_TRK_SMALL] != 0) && !(tbx_fast_list_empty(out_list)))
    {
#ifdef PROFILE_NMAD
      if(count == 0)
	{
	  TBX_GET_TICK(t_orig);
	}
      count++;
#endif /* PROFILE_NMAD */
    }
  return NM_ESUCCESS;
}

/** Emit RTR for received RDV requests
 */
static void strat_decision_tree_rdv_accept(void*_status, struct nm_gate*p_gate)
{
  if(!tbx_fast_list_empty(&p_gate->pending_large_recv))
    {
      struct nm_pkt_wrap*p_pw = nm_l2so(p_gate->pending_large_recv.next);
      nm_drv_t p_drv = nm_drv_default(p_gate);
      struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, p_drv);
      if(p_gdrv->active_recv[NM_TRK_LARGE] == 0)
	{
	  /* The large-packet track is available- post recv and RTR */
	  struct nm_rdv_chunk chunk = 
	    { .len = p_pw->length, .p_drv = p_drv, .trk_id = NM_TRK_LARGE };
	  tbx_fast_list_del(p_gate->pending_large_recv.next);
	  nm_tactic_rtr_pack(p_pw, 1, &chunk);
	}
    }
}
