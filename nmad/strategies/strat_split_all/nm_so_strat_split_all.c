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

#include <pm2_common.h>
#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>

#include <nm_private.h>

#define DATATYPE_DENSITY 2*1024


#define HEADER_SIZE (NM_SO_DATA_HEADER_SIZE > NM_SO_CTRL_HEADER_SIZE ?  NM_SO_DATA_HEADER_SIZE : NM_SO_CTRL_HEADER_SIZE)

struct nm_ssa_data_req {
  void *header;
  int header_offset; //utile uniquement pour les extended_ctrls
  void *data;
  struct tbx_fast_list_head link;
};

struct nm_ssa_pw{
  uint32_t cumulated_len;
  struct tbx_fast_list_head reqs;
  struct tbx_fast_list_head link;
};

static p_tbx_memory_t nm_ssa_data_req_mem = NULL;
static p_tbx_memory_t nm_ssa_header_mem = NULL;
static p_tbx_memory_t nm_ssa_pw_mem = NULL;

/* Components structures:
 */

static int strat_split_all_todo(void*, struct nm_gate*);
static int strat_split_all_pack(void*_status, struct nm_pack_s*p_pack);
static int strat_split_all_pack_ctrl(void*, struct nm_gate *, const union nm_so_generic_ctrl_header*);
static int strat_split_all_pack_ctrl_chunk(void*, struct nm_pkt_wrap *, const union nm_so_generic_ctrl_header *);
static int strat_split_all_pack_extended_ctrl(void*, struct nm_gate *, uint32_t, const union nm_so_generic_ctrl_header *, struct nm_pkt_wrap **);
static int strat_split_all_pack_extended_ctrl_end(void*,
                                                struct nm_gate *p_gate,
                                                struct nm_pkt_wrap *p_so_pw);
static int strat_split_all_try_and_commit(void*, struct nm_gate*);
static int strat_split_all_rdv_accept(void*, struct nm_gate*, nm_drv_id_t*, nm_trk_id_t*);
static int strat_split_all_extended_rdv_accept(void*, struct nm_gate *, uint32_t, int *,
						   nm_drv_id_t *, uint32_t *);
static int strat_split_all_split_small(void *_status, struct nm_gate *p_gate, void *link, int *nb_cores, struct nm_pkt_wrap **out_pw);

static const struct nm_strategy_iface_s nm_so_strat_split_all_driver =
  {
    .pack               = &strat_split_all_pack,
    .pack_ctrl          = &strat_split_all_pack_ctrl,
    .try_and_commit     = &strat_split_all_try_and_commit,
#ifdef NMAD_QOS
    .ack_callback    = NULL,
#endif /* NMAD_QOS */
    .rdv_accept          = &strat_split_all_rdv_accept,
//    .extended_rdv_accept = &strat_split_all_extended_rdv_accept,
    .flush               = NULL,
    .todo                = &strat_split_all_todo,
#ifdef CONFIG_STRAT_SPLIT_ALL
    .split_small         = &strat_split_all_split_small
#endif
};

static void*strat_split_all_instanciate(puk_instance_t, puk_context_t);
static void strat_split_all_destroy(void*);

static const struct puk_adapter_driver_s nm_so_strat_split_all_adapter_driver =
  {
    .instanciate = &strat_split_all_instanciate,
    .destroy     = &strat_split_all_destroy
  };

struct nm_so_strat_split_all {
  /* list of raw outgoing packets */
  struct tbx_fast_list_head out_list;
  int nm_so_max_small;
};

/* Initialization */
extern int nm_strat_split_all_init(void)
{
  puk_component_declare("NewMad_Strategy_split_all",
			puk_component_provides("PadicoAdapter", "adapter", &nm_so_strat_split_all_adapter_driver),
			puk_component_provides("NewMad_Strategy", "strat", &nm_so_strat_split_all_driver),
			puk_component_attr("nm_so_max_small", NULL),
			puk_component_attr("nm_so_copy_on_send_threshold", NULL));

  return NM_ESUCCESS;
}
PADICO_MODULE_BUILTIN(NewMad_Strategy_split_all, &nm_strat_split_all_init, NULL, NULL);


/** Initialize the gate storage for split_all strategy.
 */
static void*strat_split_all_instanciate(puk_instance_t ai, puk_context_t context)
{
  struct nm_so_strat_split_all *status = TBX_MALLOC(sizeof(struct nm_so_strat_split_all));

  TBX_INIT_FAST_LIST_HEAD(&status->out_list);

  tbx_malloc_init(&nm_ssa_data_req_mem,
		  sizeof(struct nm_ssa_data_req), 10, "nmad/.../sched_opt/nm_so_strat_split_all/data_req");
  tbx_malloc_init(&nm_ssa_header_mem,
		  HEADER_SIZE, 10, "nmad/.../sched_opt/nm_so_strat_split_all/header");

  tbx_malloc_init(&nm_ssa_pw_mem,
                  sizeof(struct nm_ssa_pw), 5, "strat_ssa/pw");

  const char*nm_so_max_small = puk_context_getattr(context, "nm_so_max_small");
  status->nm_so_max_small = atoi(nm_so_max_small);

  NM_LOGF("[NM_SO_MAX_SMALL=%i]", status->nm_so_max_small);
  NM_LOGF("[loading strategy: <split_all>]");

  return (void*)status;
}

/** Cleanup the gate storage for split_all strategy.
 */
static void strat_split_all_destroy(void*status)
{
  TBX_FREE(status);
}

static int 
strat_split_all_pack_extended_ctrl(void *_status,
				       struct nm_gate *p_gate,
				       uint32_t cumulated_header_len,
				       const union nm_so_generic_ctrl_header *p_ctrl,
				       struct nm_pkt_wrap **pp_so_pw){
  return NM_ESUCCESS;
}

static int
strat_split_all_pack_ctrl_chunk(void *_status,
				struct nm_pkt_wrap *p_so_pw,
				const union nm_so_generic_ctrl_header *p_ctrl){
  return NM_ESUCCESS;
}

static int
strat_split_all_pack_extended_ctrl_end(void *_status,
					   struct nm_gate *p_gate,
					   struct nm_pkt_wrap *p_so_pw){
	return NM_ESUCCESS;
}

/* Add a new control "header" to the flow of outgoing packets */
static int
strat_split_all_pack_ctrl(void *_status,
			      struct nm_gate *p_gate,
			      const union nm_so_generic_ctrl_header *p_ctrl){
  struct nm_so_strat_split_all *status = _status;
  struct nm_ssa_pw *p_ssa_pw = NULL;
  struct nm_ssa_data_req *p_req = tbx_malloc(nm_ssa_data_req_mem);
  union nm_so_generic_ctrl_header *header = tbx_malloc(nm_ssa_header_mem);
  uint32_t remaining_len = 0;
  int err;

  *header = *p_ctrl;
  p_req->header = header;

  /* We first try to find an existing packet to form an aggregate */
  tbx_fast_list_for_each_entry(p_ssa_pw, &status->out_list, link) {

    // free space in the header line : all data will be copied in contiguous here
    remaining_len = NM_SO_PREALLOC_IOV_LEN - p_ssa_pw->cumulated_len;

    if(NM_SO_CTRL_HEADER_SIZE <= remaining_len){
      tbx_fast_list_add_tail(&p_req->link, &p_ssa_pw->reqs);
      p_ssa_pw->cumulated_len += NM_SO_CTRL_HEADER_SIZE;
      goto out;
    }
  }

  /* We didn't have a chance to form an aggregate, so simply form a
     new packet wrapper and add it to the out_list */
  p_ssa_pw = tbx_malloc(nm_ssa_pw_mem);
  TBX_INIT_FAST_LIST_HEAD(&p_ssa_pw->reqs);

  tbx_fast_list_add(&p_req->link, &p_ssa_pw->reqs);
  p_ssa_pw->cumulated_len += NM_SO_CTRL_HEADER_SIZE;
  tbx_fast_list_add_tail(&p_ssa_pw->link, &status->out_list);
   
  err = NM_ESUCCESS;
 out:
  return err;

}


//static int
//strat_split_all_process_large(void *_status,
//                              struct nm_gate *p_gate,
//                              uint8_t tag, uint8_t seq,
//                              void *data, uint32_t len,
//                              uint32_t chunk_offset, uint8_t is_last_chunk){
//  struct nm_so_strat_split_all*status = _status;
//  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)p_gate->p_so_gate;
//  struct nm_pkt_wrap *p_so_pw = NULL;
//  int err;
//
//  /* First allocate a packet wrapper */
//  err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq,
//                                          data, len, chunk_offset, is_last_chunk,
//                                          NM_SO_DATA_DONT_USE_HEADER,
//                                          &p_so_pw);
//  if(err != NM_ESUCCESS)
//    goto out;
//
//
//  FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_CREATE_PACKET, p_so_pw, tag, seq, len);
//  FUT_DO_PROBE3(FUT_NMAD_GATE_OPS_INSERT_PACKET, p_gate->id, 0, p_so_pw);
//  FUT_DO_PROBE4(FUT_NMAD_GATE_OPS_IN_TO_OUT, p_gate->id, 0, 1, p_so_pw);
//
//  /* Then place it into the appropriate list of large pending "sends". */
//  tbx_fast_list_add(&p_so_pw->link, &status->out_list);
//
//  /* Signal we're waiting for an ACK */
//  p_so_gate->pending_unpacks++;
//
//  /* Finally, generate a RdV request */
//  {
//    union nm_so_generic_ctrl_header ctrl;
//
//    nm_so_init_rdv(&ctrl, tag + 128, seq, len, chunk_offset, is_last_chunk);
//
//    NM_SO_TRACE("RDV pack_ctrl\n");
//    err = strat_split_all_pack_ctrl(_status, p_gate, &ctrl);
//    if(err != NM_ESUCCESS)
//      goto out;
//  }
//
//  /* Check if we should post a new recv packet: we're waiting for an ACK! */
//  nm_so_refill_regular_recv(p_gate);
//
//  err = NM_ESUCCESS;
// out:
//  return err;
//}


static int strat_split_all_todo(void*_status,
				struct nm_gate *p_gate)
{
  struct nm_so_strat_split_all *status = _status;
  struct tbx_fast_list_head *out_list = &(status)->out_list;
  return !(tbx_fast_list_empty(out_list));
}

/* Handle the arrival of a new packet. The strategy may already apply
   some optimizations at this point */
static int
strat_split_all_pack(void *_status, struct nm_pack_s*p_pack)
{
#warning FIXME: strat_split_all->pack is not implemented !
 #if 0
 struct nm_so_strat_split_all *status = _status;
 
  struct tbx_fast_list_head *out_list = &status->out_list;
  struct nm_ssa_pw *p_ssa_pw = NULL;
 
  struct nm_ssa_data_req *p_req = NULL;
  struct nm_so_data_header *header = NULL;
  
  uint32_t remaining_len = 0;
  uint32_t size = 0;
  int err;
  int flags=0; 

  nm_so_tag_get(&p_gate->tags, tag)->send[seq] = len;

  p_req = tbx_malloc(nm_ssa_data_req_mem);
  header = tbx_malloc(nm_ssa_header_mem);

  nm_so_init_data_header(header, tag+128, seq, len, 0, 1);
  p_req->header = header;
  p_req->data = data;
  
  /* We first try to find an existing packet to form an aggregate */
  tbx_fast_list_for_each_entry(p_ssa_pw, out_list, link) {
    
    // free space in the header line : all data will be copied in contiguous here
    remaining_len = NM_SO_PREALLOC_IOV_LEN - p_ssa_pw->cumulated_len;
    // space required to add the current data
    size = NM_SO_DATA_HEADER_SIZE + nm_so_aligned(len);
    
    if(size <= remaining_len){
      tbx_fast_list_add_tail(&p_req->link, &p_ssa_pw->reqs);
      p_ssa_pw->cumulated_len += NM_SO_DATA_HEADER_SIZE + nm_so_aligned(len);
      goto out;
    }
  }
  
   /* We didn't have a chance to form an aggregate, so simply form a
      new packet wrapper and add it to the out_list */
  p_ssa_pw = tbx_malloc(nm_ssa_pw_mem);
  TBX_INIT_FAST_LIST_HEAD(&p_ssa_pw->reqs);
  tbx_fast_list_add(&p_req->link, &p_ssa_pw->reqs);
  p_ssa_pw->cumulated_len = NM_SO_GLOBAL_HEADER_SIZE + NM_SO_DATA_HEADER_SIZE + nm_so_aligned(len);
  
  tbx_fast_list_add_tail(&p_ssa_pw->link, &status->out_list);
  
  err = NM_ESUCCESS;
 out:
#endif	/* 0 */
  return 0;
}

static int build_wrapper(struct nm_so_strat_split_all *status,
                         struct nm_gate *p_gate,
                         struct nm_ssa_pw *p_ssa_pw,
                         uint32_t threshold,
                         uint32_t offset,
                         tbx_bool_t need_split,
                         int drv_id,
                         struct nm_pkt_wrap **pp_so_pw){
#if 0

  struct nm_ssa_data_req *p_req = NULL, *p_req_next;
  struct nm_pkt_wrap *p_so_pw = NULL;
  struct tbx_fast_list_head* reqs_list = &p_ssa_pw->reqs;
  uint32_t proto_id = 0;
  struct nm_so_data_header *h = NULL;
  int flags = 0;
  int err;

  err = nm_so_pw_alloc(flags, &p_so_pw);
  tbx_fast_list_for_each_entry_safe(p_req, p_req_next, reqs_list, link) {
    h = p_req->header;
    proto_id = *(uint8_t *)p_req->header;

    if(threshold > status->nm_so_max_small) {
      /* Large message, prepare the rdv */
	    
      int len_to_send=threshold;
      uint8_t last=(need_split?0:1);
      /* Build the data pw */
      if(! need_split)
        len_to_send=h->len;
      else{
	len_to_send=threshold;
	h->len-=threshold;
      }
      struct nm_pkt_wrap *p_data_pw = NULL;
      nm_so_pw_alloc_and_fill_with_data(h->proto_id, 
					h->seq,
					p_req->data+offset, 
					len_to_send, 
					offset, 
					last,
					NM_SO_DATA_DONT_USE_HEADER,
					&p_data_pw);
      
      tbx_fast_list_add_tail(&p_data_pw->link,
		    &(nm_so_tag_get(&p_gate->tags, proto_id-128)->pending_large_send));

      /* Build the rdv pw */
      union nm_so_generic_ctrl_header ctrl;
      nm_so_init_rdv(&ctrl, 
		     h->proto_id , 
		     h->seq, 
		     len_to_send, 
		     offset, 
		     last);
      
      err = nm_so_pw_add_control(p_so_pw, &ctrl);
      
      nm_so_pw_finalize(p_so_pw);
      
      if(!need_split)
	tbx_fast_list_del(&p_req->link);
      goto out;	    
    } else {
    /* Small message */
      if(need_split){
        /* Check if it is the last request we will put in the wrapper */
	if(proto_id >= NM_SO_PROTO_DATA_FIRST) {
	  if(h->len <= NM_SO_COPY_ON_SEND_THRESHOLD)
  	    flags = NM_SO_DATA_USE_COPY;
	  
	  if(p_so_pw->length + NM_SO_DATA_HEADER_SIZE + 16 >= threshold){
	    // il faut que le header + un minimum de données (16o pour le moment) soient contigus
	    goto out;
	  }
	  
	  if(p_so_pw->length + NM_SO_DATA_HEADER_SIZE + h->len > threshold){
#warning verifier que le seuil de rdv nest pas atteint
	    int padding = nm_so_aligned(threshold - p_so_pw->length - NM_SO_DATA_HEADER_SIZE);
	    p_ssa_pw->cumulated_len -= padding;

	    /* split the message! */
	    nm_so_pw_add_data(p_so_pw,
			      h->proto_id, h->seq,
			      p_req->data+h->chunk_offset,
			      padding, // remaining space
			      h->chunk_offset,
			      flags);

	    h->chunk_offset += padding;
	    h->len -= padding;
	    goto out;
	  }
	  
	} else {
	  if(p_so_pw->length + NM_SO_CTRL_HEADER_SIZE >= threshold){
	    goto out;
	  }
	}
      }
    }

    /* Classic case */
    tbx_fast_list_del(&p_req->link);

    if(proto_id >= NM_SO_PROTO_DATA_FIRST){
      /* data header */
#warning  propre au drv normalement
      if(h->len <= NM_SO_COPY_ON_SEND_THRESHOLD)
        flags = NM_SO_DATA_USE_COPY;

      h->is_last_chunk = tbx_true;
      nm_so_pw_add_data(p_so_pw, proto_id, h->seq, p_req->data+h->chunk_offset, h->len, h->chunk_offset, flags);

      struct iovec *vec;
      void *ptr;
      unsigned long remaining_len;
      vec = p_so_pw->v;
      ptr = vec->iov_base;
      remaining_len = ((struct nm_so_global_header *)ptr)->len - NM_SO_GLOBAL_HEADER_SIZE;
      
      p_ssa_pw->cumulated_len -= NM_SO_DATA_HEADER_SIZE + h->len;
    } else { 
      /* ctrl header */
      uint8_t proto_id = *(uint8_t *)p_req->header;
      if(proto_id  == NM_SO_PROTO_MULTI_ACK) {
        union nm_so_generic_ctrl_header *header = (union nm_so_generic_ctrl_header *)p_req->header;
	int nb_chunks = header->ma.nb_chunks;
	int i;
	
	for(i = 0; i < nb_chunks+1 /* do not forget the multiack header */; i++){
		nm_so_pw_add_control(p_so_pw, p_req->header + (i * NM_SO_CTRL_HEADER_SIZE));
		p_ssa_pw->cumulated_len -= NM_SO_CTRL_HEADER_SIZE;
	}
	
      } else {
        nm_so_pw_add_control(p_so_pw, p_req->header);
	p_ssa_pw->cumulated_len -= NM_SO_CTRL_HEADER_SIZE;
      }
      
    }
#warning marche pas si on a un pack extended
    tbx_free(nm_ssa_header_mem, p_req->header);
    tbx_free(nm_ssa_data_req_mem, p_req);
  }

 out:
  nm_so_pw_finalize(p_so_pw);

  p_so_pw->p_gate = p_gate;

  /* Packet is assigned to given track */
  p_so_pw->p_drv = (p_so_pw->p_gdrv =
		    p_gate->p_gate_drv_array[drv_id])->p_drv;

  *pp_so_pw = p_so_pw;

#endif
  return NM_ESUCCESS;
}

#define ssa_pw2pw(l) \
        ((struct nm_ssa_pw *)((char *)(l) -\
         (unsigned long)(&((struct nm_ssa_pw *)0)->link)))

/* Compute and apply the best possible packet rearrangement, then
   return next packet to send */
static int
strat_split_all_try_and_commit(void *_status,
                               struct nm_gate *p_gate){
  struct nm_so_strat_split_all *status = _status;
  struct tbx_fast_list_head *out_list = &status->out_list;
  struct nm_ssa_pw *p_ssa_pw = NULL;
  int nb_drivers = p_gate->p_core->nb_drivers;
  uint8_t *drv_ids = NULL;
  int drv_id, n = 0;

  if(tbx_fast_list_empty(out_list))
    /* We're done */
    goto out;
  /* Try to find a idle NIC, from the fastest to the slowest */
  /* todo: fixme */
//  nm_ns_inc_lats(p_gate->p_core, &drv_ids);
  while(n < nb_drivers){
    struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, drv_ids[n]);
    if (p_gdrv->active_send[NM_TRK_SMALL] +
        p_gdrv->active_send[NM_TRK_LARGE] == 0){
      /* We found an idle NIC */
      drv_id = drv_ids[n];
      n++;
      goto next;
    }
    n++;
  }

  /* We didn't found any idle NIC, so we're done*/
  goto out;

 next:
  /* Simply take the head of the list */
  p_ssa_pw = ssa_pw2pw(out_list->next);
  tbx_fast_list_del(out_list->next);
  int idle_cores=3;
  struct nm_pkt_wrap *pws[8];
  int i;

  int err = strat_split_all_split_small(p_gate->strategy_receptacle._status,
					p_gate,
					p_ssa_pw,
					&idle_cores,
					pws);

  /* Post packet on NM_TRACK_SMALL */
  for(i = 0;i<idle_cores;i++){
    nm_core_post_send(p_gate, pws[i], NM_TRK_SMALL, i);
  }
 out:
  return NM_ESUCCESS;
}


static int
strat_split_all_split_small(void *_status,
                            struct nm_gate *p_gate,
                            void *link,
                            int *nb_cores,
                            struct nm_pkt_wrap **out_pw){
  /* only split in 2 chunks for the moment */
  struct nm_so_strat_split_all *status = _status;
  struct nm_ssa_pw *p_ssa_pw = link;
  int drv_id = -1;
  uint32_t split_ratio = 0;
  struct nm_pkt_wrap *p_so_pw1 = NULL, *p_so_pw2 = NULL;
  int cumulated_len = p_ssa_pw->cumulated_len;
  tbx_tick_t t1,t2;

  TBX_GET_TICK(t1);
  /* TODO: fixme */
  /* int err = nm_ns_split_ratio(cumulated_len, p_gate->p_core, 0, 1, &split_ratio); */
  int err = 0;
  if(err == NM_EAGAIN)
    return err;
  TBX_GET_TICK(t2);

  if(*nb_cores == 1)
    goto one_wrapper;

  // construction du/des wrappers et envoi
  if(split_ratio == 0 || split_ratio == cumulated_len){
    drv_id = (split_ratio == 0 ? 1 : 0);
  one_wrapper:
    /* don't split, build only one pw */
    build_wrapper(status, p_gate, p_ssa_pw, cumulated_len, 0, tbx_false, drv_id, &p_so_pw1);
    *nb_cores = 1;
    out_pw[0] = p_so_pw1;
  } else {
    TBX_GET_TICK(t1);
    /* create 2 pw: send 'split_ratio' bytes through drv0 and the remaining part through drv1 */
    build_wrapper(status, p_gate, p_ssa_pw, split_ratio, 0, tbx_true, 0, &p_so_pw1);
    build_wrapper(status, p_gate, p_ssa_pw, cumulated_len - split_ratio, split_ratio, tbx_false, 1, &p_so_pw2);
    TBX_GET_TICK(t2);
  
    *nb_cores = 2;
    out_pw[0] = p_so_pw1;
    out_pw[1] = p_so_pw2;
  }

  return NM_ESUCCESS;
}

/* Warning: drv_id and trk_id are IN/OUT parameters. They initially
   hold values "suggested" by the caller. */
static int
strat_split_all_rdv_accept(void *_status,
			   struct nm_gate *p_gate,
			   nm_drv_id_t *drv_id,
			   nm_trk_id_t *trk_id){
  int nb_drivers = p_gate->p_core->nb_drivers;
  int n;

  /* Is there any large data track available? */
  for(n = 0; n < nb_drivers; n++) {

    /* We explore the drivers from the fastest to the slowest. */
/* todo: fixme */
	  //*drv_id = nm_ns_get_bws(p_gate->p_core, n);

    struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, *drv_id);
    if(p_gdrv->active_recv[*trk_id] == 0) {
      /* Cool! The suggested track is available! */
      return NM_ESUCCESS;
    }
  }

  /* We're forced to postpone the acknowledgement. */
  return -NM_EAGAIN;
}


//#warning ajouter le n° de la track a utiliser pour chaque driver
static int
strat_split_all_extended_rdv_accept(void *_status,
					struct nm_gate *p_gate,
					uint32_t len_to_send,
					int * nb_drv,
					nm_drv_id_t *drv_ids,
					uint32_t *chunk_lens){
  int nb_drivers = p_gate->p_core->nb_drivers;
  uint8_t *ordered_drv_id_by_bw = NULL;
  int cur_drv_idx = 0;
  uint8_t trk_id = NM_TRK_LARGE;
  int err;
  int i;
  /* todo: fixme */
//  nm_ns_dec_bws(p_gate->p_core, &ordered_drv_id_by_bw);

  for(i = 0; i < nb_drivers; i++){
    struct nm_gate_drv*p_gdrv = nm_gate_drv_get(p_gate, ordered_drv_id_by_bw[i]);
    if(p_gdrv->active_recv[trk_id] == 0) {
      drv_ids[cur_drv_idx] = i;
      cur_drv_idx++;
    }
  }

  *nb_drv = cur_drv_idx;

  if(cur_drv_idx == 0){
    /* We're forced to postpone the acknowledgement. */
    err = -NM_EAGAIN;
  } else if(cur_drv_idx == 1){
    err = NM_ESUCCESS;
  } else {
    int final_nb_drv = 0;

#if 0
    /* todo: virer tout la fonction ? */
    nm_ns_multiple_split_ratio(len_to_send,
                               p_gate->p_core,
                               cur_drv_idx, drv_ids,
                               chunk_lens,
                               &final_nb_drv);
#endif
    *nb_drv = final_nb_drv;

    err = NM_ESUCCESS;
  }

  return err;
}

