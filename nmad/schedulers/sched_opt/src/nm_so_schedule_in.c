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

//#define NM_SO_OPTIMISTIC_RECV

int
__nm_so_wait_recv_range(struct nm_core *p_core,
			struct nm_gate *p_gate,
			uint8_t tag,
			uint8_t seq_inf, uint8_t seq_sup)
{
  uint8_t seq = seq_inf;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  do {
    while(!(p_so_gate->status[tag][seq] & NM_SO_STATUS_RECV_COMPLETED))
      nm_schedule(p_core);
  } while(seq++ != seq_sup);

  return NM_ESUCCESS;
}

int
__nm_so_unpack(struct nm_gate *p_gate,
	       uint8_t tag, uint8_t seq,
	       void *data, uint32_t len)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
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
      *status |= NM_SO_STATUS_RECV_COMPLETED;

    } else {
      /* Data not yet received */

      /* Add the unpack info into the recv array */
      p_so_gate->recv[tag][seq].unpack_here.data = data;
      p_so_gate->recv[tag][seq].unpack_here.len = len;

      *status |= NM_SO_STATUS_UNPACK_HERE;
      *status &= ~NM_SO_STATUS_RECV_COMPLETED;

      p_so_gate->pending_unpacks++;

      /* Check if we should post a new recv packet */
      if(!p_so_gate->active_recv[0])
#ifdef NM_SO_OPTIMISTIC_RECV
	nm_so_post_optimistic_recv(p_gate, tag, seq, data, len);
#else
        nm_so_post_regular_recv(p_gate);
#endif
    }

  } else {
    /* Large packet */

    *status &= ~NM_SO_STATUS_RECV_COMPLETED;

    /* Check if the RdV request is already in */
    if(*status & NM_SO_STATUS_RDV_HERE) {
      /* A RdV request has already been received for this chunk */

      *status &= ~NM_SO_STATUS_RDV_HERE;

      /* Is the large data track available? */
      if(!p_so_gate->active_recv[1]) {
	/* Cool! Track 1 is available, so let's post the receive and
	   send an ACK */
	union nm_so_generic_ctrl_header ctrl;

	nm_so_post_large_recv(p_gate, tag + 128, seq, data, len);

	nm_so_init_ack(&ctrl, tag + 128, seq, 1);

	err = active_strategy->pack_ctrl(p_gate, &ctrl);

	/* We're done! */
	goto out;

      } else {
	/* Track 1 is not available : postpone the receive */
	struct nm_so_pkt_wrap *p_so_pw;

	err = nm_so_pw_alloc_and_fill_with_data(tag + 128, seq,
						data, len,
						NM_SO_DATA_DONT_USE_HEADER,
						&p_so_pw);
	if(err != NM_ESUCCESS)
	  goto out;

	list_add_tail(&p_so_pw->link, &p_so_gate->pending_large_recv);
      }

    } else {
      /* No RdV request has been received */

      p_so_gate->pending_unpacks++;

      /* Add the unpack info into the recv array */
      p_so_gate->recv[tag][seq].unpack_here.data = data;
      p_so_gate->recv[tag][seq].unpack_here.len = len;

      *status |= NM_SO_STATUS_UNPACK_HERE;

      /* Check if we should post a new recv packet
         in order to receive the rdv request */
      if(!p_so_gate->active_recv[0])
        nm_so_post_regular_recv(p_gate);

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
  uint8_t tag = proto_id - 128;
  volatile uint8_t *status = &(p_so_gate->status[tag][seq]);

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
    *status |= NM_SO_STATUS_RECV_COMPLETED;

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

    if(!p_so_gate->active_recv[1]) {
      /* Track 1 is available */
	union nm_so_generic_ctrl_header ctrl;

	/* Post the data reception */
	nm_so_post_large_recv(p_gate, tag_id, seq,
			      p_so_gate->recv[tag][seq].unpack_here.data,
			      p_so_gate->recv[tag][seq].unpack_here.len);

	nm_so_init_ack(&ctrl, tag_id, seq, 1);

	err = active_strategy->pack_ctrl(p_gate, &ctrl);

    } else {
      /* Argh! Track 1 is currently not available */
      struct nm_so_pkt_wrap *p_so_large_pw;

      err = nm_so_pw_alloc_and_fill_with_data(tag_id, seq,
                                              p_so_gate->recv[tag][seq].unpack_here.data,
                                              p_so_gate->recv[tag][seq].unpack_here.len,
                                              NM_SO_DATA_DONT_USE_HEADER,
                                              &p_so_large_pw);

      /* WARNING!! We should probably "panic" here! */
      if(err != NM_ESUCCESS)
        return err;

      /* Postpone receive */
      list_add_tail(&p_so_large_pw->link,
                    &p_so_gate->pending_large_recv);

    }

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
  struct nm_gate * p_gate = p_so_pw->pw.p_gate;
  struct nm_so_pkt_wrap *p_so_large_pw;
  uint8_t tag = tag_id - 128;

  p_so_gate->pending_unpacks--;

  assert(!list_empty(&p_so_gate->pending_large_send[tag]));

  p_so_large_pw = nm_l2so(p_so_gate->pending_large_send[tag].next);
  list_del(p_so_gate->pending_large_send[tag].next);

  assert(seq == p_so_large_pw->pw.seq);

  /* Send the data */
  _nm_so_post_send(p_gate, p_so_large_pw, track_id);

  return NM_SO_HEADER_MARK_READ;
}


/* process complete successful incoming request */
int
nm_so_in_process_success_rq(struct nm_sched	*p_sched,
                            struct nm_pkt_wrap	*p_pw)
{
  struct nm_so_pkt_wrap *p_so_pw = nm_pw2so(p_pw);
  struct nm_so_gate *p_so_gate = p_so_pw->pw.p_gate->sch_private;
  int err;

  //printf("Packet %p received completely (on track %d)!\n",
  //	 p_so_pw, p_pw->p_trk->id);

  if(p_pw->p_trk->id == 0) {
    /* Track 0 */

    p_so_gate->active_recv[0] = 0;

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

	*status &= ~NM_SO_STATUS_UNPACK_HERE;
	*status |= NM_SO_STATUS_RECV_COMPLETED;

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
      nm_so_post_regular_recv(p_so_pw->pw.p_gate);

  } else if(p_pw->p_trk->id == 1) {
    /* This is the completion of a large message. */
    volatile uint8_t *status =
      &(p_so_gate->status[p_so_pw->pw.proto_id - 128][p_so_pw->pw.seq]);

    /* Free the wrapper */
    nm_so_pw_free(p_so_pw);

    *status |= NM_SO_STATUS_RECV_COMPLETED;

    p_so_gate->active_recv[1] = 0;

    /* Check if some recv requests were postponed */
    if(!list_empty(&p_so_gate->pending_large_recv)) {
      union nm_so_generic_ctrl_header ctrl;
      struct nm_so_pkt_wrap *p_so_large_pw
        = nm_l2so(p_so_gate->pending_large_recv.next);

      list_del(p_so_gate->pending_large_recv.next);

      /* Post the data reception */
      nm_so_direct_post_large_recv(p_so_pw->pw.p_gate,
                                   p_so_large_pw);

      /* Send an ACK */
      nm_so_init_ack(&ctrl,
		     p_so_large_pw->pw.proto_id,
		     p_so_large_pw->pw.seq,
		     1);

      err = active_strategy->pack_ctrl(p_so_pw->pw.p_gate, &ctrl);
      if(err != NM_ESUCCESS)
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
