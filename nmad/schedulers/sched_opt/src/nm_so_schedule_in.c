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
#include "nm_so_headers.h"
#include "nm_so_tracks.h"
#include "nm_so_raw_interface.h"
#include "nm_so_interfaces.h"

//#define NM_SO_OPTIMISTIC_RECV

static int rdv_success(struct nm_gate *p_gate,
		       uint8_t tag, uint8_t seq,
		       void *data, uint32_t len)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;
  nm_so_strategy *cur_strat = p_so_sched->current_strategy;
  struct nm_so_pkt_wrap *p_so_pw;
  int err;
  unsigned long drv_id = NM_SO_DEFAULT_NET;
  unsigned long trk_id = TRK_LARGE;

  /* Can we find an available track to prepare the receive? */
  err = cur_strat->rdv_accept(p_gate, &drv_id, &trk_id);

  if(err == NM_ESUCCESS) {
    /* Let's acknowledge the Rendez-Vous request! */
    union nm_so_generic_ctrl_header ctrl;

    nm_so_post_large_recv(p_gate, drv_id, tag + 128, seq, data, len);

    nm_so_init_ack(&ctrl, tag + 128, seq,
		   drv_id * NM_SO_MAX_TRACKS + trk_id);

    err = cur_strat->pack_ctrl(p_gate, &ctrl);

  } else {
    /* We are forced to postpone the receive */
    err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq,
					  data, len,
					  NM_SO_DATA_DONT_USE_HEADER,
					  &p_so_pw);
    if(err != NM_ESUCCESS)
      goto out;

    list_add_tail(&p_so_pw->link, &p_so_gate->pending_large_recv);
  }

 out:
  return err;
}

int
__nm_so_unpack_any_src(struct nm_core *p_core,
                       uint8_t tag, void *data, uint32_t len){

  struct nm_so_sched *p_so_sched = p_core->p_sched->sch_private;
  uint8_t next_gate_id = p_so_sched->next_gate_id;
  uint8_t nb_gates = p_core->nb_gates;
  struct nm_so_interface_ops *interface = p_so_sched->current_interface;
  struct nm_gate *p_gate= NULL;
  struct nm_so_gate *p_so_gate = NULL;
  volatile uint8_t *status = NULL;
  int seq;

  int i, j;

  for(j = 0, i = next_gate_id; j < nb_gates; j++, i = (i +1) % p_core->nb_gates){
    p_gate = &p_core->gate_array[i];
    p_so_gate = p_gate->sch_private;
    seq = p_so_gate->recv_seq_number[tag];

    status = &(p_so_gate->status[tag][seq]);

    if(*status & NM_SO_STATUS_PACKET_HERE
       || *status & NM_SO_STATUS_RDV_HERE){

      __nm_so_unpack(p_gate,
                     tag, seq,
                     data, len);
      p_so_gate->recv_seq_number[tag]++;

      interface->unpack_success(p_gate, tag, seq, tbx_true);

      goto out;
    }
  }


  p_so_sched->any_src[tag].unpack_here |= NM_SO_STATUS_UNPACK_HERE;
  p_so_sched->any_src[tag].data = data;
  p_so_sched->any_src[tag].len = len;

  for(i = 0; i < p_core->nb_gates; i++){
    nm_so_refill_regular_recv(&p_core->gate_array[i]);
  }

 out:
  return NM_ESUCCESS;
}





int
__nm_so_unpack(struct nm_gate *p_gate,
	       uint8_t tag, uint8_t seq,
	       void *data, uint32_t len)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_interface_ops *interface = p_so_gate->p_so_sched->current_interface;
  volatile uint8_t *status = &(p_so_gate->status[tag][seq]);
  int err;

  if(len <= NM_SO_MAX_SMALL) {
    /* Small packet */

    if(*status & NM_SO_STATUS_PACKET_HERE) {
      /* Wow! Data already in! */

      if(len)
	/* Copy data to its final destination */
	memcpy(data, p_so_gate->recv[tag][seq].pkt_here.data, len);

      /* Decrement the packet wrapper reference counter. If no other
	 chunks are still in use, the pw will be destroyed. */
      nm_so_pw_dec_header_ref_count(p_so_gate->recv[tag][seq].pkt_here.p_so_pw);

      *status &= ~NM_SO_STATUS_PACKET_HERE;
      interface->unpack_success(p_gate, tag, seq, tbx_false);

    } else {
      /* Data not yet received */

      /* Add the unpack info into the recv array */
      p_so_gate->recv[tag][seq].unpack_here.data = data;
      p_so_gate->recv[tag][seq].unpack_here.len = len;

      *status |= NM_SO_STATUS_UNPACK_HERE;

      p_so_gate->pending_unpacks++;

      /* Check if we should post a new recv packet */
#ifdef NM_SO_OPTIMISTIC_RECV
      if(!p_so_gate->active_recv[NM_SO_DEFAULT_NET][TRK_SMALL])
	nm_so_post_optimistic_recv(p_gate, tag, seq, data, len);
#else
      nm_so_refill_regular_recv(p_gate);
#endif
    }

  } else {
    /* Large packet */

    /* Check if the RdV request is already in */
    if(*status & NM_SO_STATUS_RDV_HERE) {
      /* A RdV request has already been received for this chunk */

      *status &= ~NM_SO_STATUS_RDV_HERE;

      err = rdv_success(p_gate, tag, seq, data, len);

      goto out;

    } else {
      /* No RdV request has been received */

      p_so_gate->pending_unpacks++;

      /* Add the unpack info into the recv array */
      p_so_gate->recv[tag][seq].unpack_here.data = data;
      p_so_gate->recv[tag][seq].unpack_here.len = len;

      *status |= NM_SO_STATUS_UNPACK_HERE;

      /* Check if we should post a new recv packet
         in order to receive the rdv request */
      nm_so_refill_regular_recv(p_gate);

    }

  }

  err = NM_ESUCCESS;

 out:
  return err;
}

/* schedule and post new incoming buffers */
int
nm_so_in_schedule(struct nm_sched *p_sched)
{
  return NM_ESUCCESS;
}

static int data_completion_callback(struct nm_so_pkt_wrap *p_so_pw,
				    void *ptr, uint32_t len,
				    uint8_t proto_id, uint8_t seq,
				    void *arg)
{
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)arg;
  struct nm_so_sched *p_so_sched =  p_so_gate->p_so_sched;
  struct nm_so_interface_ops *interface = p_so_sched->current_interface;
  uint8_t tag = proto_id - 128;
  volatile uint8_t *status = &(p_so_gate->status[tag][seq]);
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;

  //    printf("Recv completed for chunk : %p, len = %u, tag = %d, seq = %u\n",
  //  	 ptr, len, tag, seq);

  if(*status & NM_SO_STATUS_UNPACK_HERE) {
    /* Cool! We already have a waiting unpack for this packet */

    if(len)
      /* Copy data to its final destination */
      memcpy(p_so_gate->recv[tag][seq].unpack_here.data,
	     ptr,
	     p_so_gate->recv[tag][seq].unpack_here.len);

    p_so_gate->pending_unpacks--;

    *status &= ~NM_SO_STATUS_UNPACK_HERE;
    interface->unpack_success(p_gate, tag, seq, tbx_false);

    return NM_SO_HEADER_MARK_READ;

  } else if (seq == p_so_gate->recv_seq_number[tag]
             && p_so_sched->any_src[tag].unpack_here) {

    /* we look for the any source expected message */
    memcpy(p_so_sched->any_src[tag].data,
           ptr,
           p_so_sched->any_src[tag].len);

    p_so_gate->pending_unpacks--;

    interface->unpack_success(p_gate, tag, seq, tbx_true);

    return NM_SO_HEADER_MARK_READ;
  } else {

    /* Receiver process is not ready, so store the information in the
       recv array and keep the p_so_pw packet alive */

    p_so_gate->recv[tag][seq].pkt_here.data = ptr;
    p_so_gate->recv[tag][seq].pkt_here.p_so_pw = p_so_pw;

    *status |= NM_SO_STATUS_PACKET_HERE;

    return NM_SO_HEADER_MARK_UNREAD;
  }

}

static int rdv_callback(struct nm_so_pkt_wrap *p_so_pw,
                        uint8_t tag_id, uint8_t seq,
                        void *arg)
{
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)arg;
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  uint8_t tag = tag_id - 128;
  volatile uint8_t *status = &(p_so_gate->status[tag][seq]);
  int err;

  if(*status & NM_SO_STATUS_UNPACK_HERE) {
    /* Application is already ready! */

    p_so_gate->pending_unpacks--;

    *status &= ~NM_SO_STATUS_UNPACK_HERE;

    err = rdv_success(p_gate, tag, seq,
		      p_so_gate->recv[tag][seq].unpack_here.data,
		      p_so_gate->recv[tag][seq].unpack_here.len);

    if(err != NM_ESUCCESS)
      TBX_FAILURE("PANIC!\n");

  } else {
    /* Store rdv request */
    p_so_gate->status[tag][seq] |= NM_SO_STATUS_RDV_HERE;
  }

  return NM_SO_HEADER_MARK_READ;
}

static int ack_callback(struct nm_so_pkt_wrap *p_so_pw,
                        uint8_t tag_id, uint8_t seq,
                        uint8_t track_id,
                        void *arg)
{
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)arg;
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_pkt_wrap *p_so_large_pw;
  uint8_t tag = tag_id - 128;

  p_so_gate->pending_unpacks--;

  list_for_each_entry(p_so_large_pw, &p_so_gate->pending_large_send[tag], link) {
    if(p_so_large_pw->pw.seq == seq) {
      list_del(&p_so_large_pw->link);

      /* Send the data */
      _nm_so_post_send(p_gate, p_so_large_pw,
		       track_id % NM_SO_MAX_TRACKS,
		       track_id / NM_SO_MAX_TRACKS);

      return NM_SO_HEADER_MARK_READ;
    }

  }

  TBX_FAILURE("PANIC!\n");
}


/* process complete successful incoming request */
int
nm_so_in_process_success_rq(struct nm_sched	*p_sched,
                            struct nm_pkt_wrap	*p_pw)
{
  struct nm_so_pkt_wrap *p_so_pw = nm_pw2so(p_pw);
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  int err;

  //  printf("Packet %p received completely (on track %d)!\n",
  //  	 p_so_pw, p_pw->p_trk->id);

  if(p_pw->p_trk->id == TRK_SMALL) {
    /* Track 0 */
    p_so_gate->active_recv[p_pw->p_drv->id][TRK_SMALL] = 0;

#ifdef NM_SO_OPTIMISTIC_RECV
    {
      int res;

      err = nm_so_pw_check_optimistic(p_so_pw, &res);
      if(err != NM_ESUCCESS)
	goto out;

      if(res == NM_SO_OPTIMISTIC_SUCCESS) {
	/* The optimistic recv operation succeeded! We must mark the
	   corresponding unpack 'completed'. */
	volatile uint8_t *status =
	  &(p_so_gate->status[p_so_pw->pw.proto_id - 128][p_so_pw->pw.seq]);
        struct nm_so_interface_ops *interface = p_so_gate->p_so_sched->current_interface;

	*status &= ~NM_SO_STATUS_UNPACK_HERE;
        interface->unpack_success(p_gate, tag, seq);

	p_so_gate->pending_unpacks--;
      }
    }
#endif

    nm_so_pw_iterate_over_headers(p_so_pw,
				  data_completion_callback,
				  rdv_callback,
				  ack_callback,
				  p_so_gate);

    if(p_so_gate->pending_unpacks)
      /* Check if we should post a new recv packet */
      nm_so_refill_regular_recv(p_gate);

  } else if(p_pw->p_trk->id == TRK_LARGE) {

    int drv_id = p_pw->p_drv->id;

    /* This is the completion of a large message. */
    struct nm_so_interface_ops *interface = p_so_gate->p_so_sched->current_interface;

    interface->unpack_success(p_gate,
                              p_so_pw->pw.proto_id - 128,
                              p_so_pw->pw.seq,
                              tbx_false);

    //    printf("Large received (%d bytes) on drv %d\n", p_pw->length, drv_id);

    p_so_gate->active_recv[drv_id][TRK_LARGE] = 0;

    /* Free the wrapper */
    nm_so_pw_free(p_so_pw);

    /* Check if some recv requests were postponed */
    if(!list_empty(&p_so_gate->pending_large_recv)) {
      union nm_so_generic_ctrl_header ctrl;
      struct nm_so_pkt_wrap *p_so_large_pw
        = nm_l2so(p_so_gate->pending_large_recv.next);
      struct nm_so_sched *p_so_sched = p_sched->sch_private;

      list_del(p_so_gate->pending_large_recv.next);

      /* Note: we could call current_strategy->rdv_accept(...) to let
	 the current strategy choose the appropriate drv/trk
	 combination. However, there's currently only ONE available
	 drv/trk couple, so the alternate solution would be to
	 postpone the receive... Such a strategy can be implemented on
	 the sending side, right? */

      /* Post the data reception */
      nm_so_direct_post_large_recv(p_gate, drv_id,
                                   p_so_large_pw);

      /* Send an ACK */
      nm_so_init_ack(&ctrl,
		     p_so_large_pw->pw.proto_id,
		     p_so_large_pw->pw.seq,
		     drv_id * NM_SO_MAX_TRACKS + TRK_LARGE);

      err = p_so_sched->current_strategy->pack_ctrl(p_gate, &ctrl);
      goto out;

    }

  }
  /* Hum... Well... We're done guys! */

  err = NM_ESUCCESS;

 out:
  return err;
}


/* process complete failed incoming request */
int
nm_so_in_process_failed_rq(struct nm_sched	*p_sched,
                           struct nm_pkt_wrap	*p_pw,
                           int		_err) {
  TBX_FAILURE("nm_so_in_process_failed_rq");
  return nm_so_in_process_success_rq(p_sched, p_pw);
}
