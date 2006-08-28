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

static __inline__
void
_nm_so_post_pw(struct nm_gate *p_gate, struct nm_so_pkt_wrap *p_so_pw)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  p_so_pw->pw.p_gate = p_gate;

  /* Packet is assigned to track 0 */
  p_so_pw->pw.p_drv = (p_so_pw->pw.p_gdrv =
		       p_gate->p_gate_drv_array[0])->p_drv;
  p_so_pw->pw.p_trk = (p_so_pw->pw.p_gtrk
		       = p_so_pw->pw.p_gdrv->p_gate_trk_array[0])->p_trk;

  /* append pkt to scheduler post list */
  tbx_slist_append(p_gate->p_sched->post_aux_recv_req, &p_so_pw->pw);

  p_so_gate->active_recv[0] = 1;
}

static __inline__
int
nm_so_post_regular_pw(struct nm_gate *p_gate)
{
  int err;
  struct nm_so_pkt_wrap *p_so_pw;

  err = nm_so_pw_alloc(NM_SO_DATA_DONT_USE_HEADER |
		       NM_SO_DATA_PREPARE_RECV,
		       &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  _nm_so_post_pw(p_gate, p_so_pw);
  
  err = NM_ESUCCESS;
 out:
  return err;
}

#ifdef NM_SO_OPTIMISTIC_RECV
static __inline__
int
nm_so_post_optimistic_pw(struct nm_gate *p_gate,
			 uint8_t tag, uint8_t seq,
			 void *data, uint32_t len)
{
  int err;
  struct nm_so_pkt_wrap *p_so_pw;

  err = nm_so_pw_alloc_optimistic(tag, seq, data, len, &p_so_pw);
  if(err != NM_ESUCCESS)
    goto out;

  _nm_so_post_pw(p_gate, p_so_pw);
  
  err = NM_ESUCCESS;
 out:
  return err;
}
#endif

int
__nm_so_unpack(struct nm_gate *p_gate,
	       uint8_t tag, uint8_t seq,
	       void *data, uint32_t len)
{
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  volatile uint8_t *status = &(p_so_gate->status[tag][seq]);

  if(*status & NM_SO_STATUS_PACKET_HERE) {
    /* Wow! Data already in! */

    /* Copy data to its final destination */
    memcpy(data, p_so_gate->recv[tag][seq].pkt_here.data, len);

    /* Decrement the packet wrapper reference counter. If no other
       chunks are still in use, the pw will be destroyed. */
    nm_so_pw_dec_header_ref_count(p_so_gate->recv[tag][seq].pkt_here.p_so_pw);

    *status &= ~NM_SO_STATUS_PACKET_HERE;
    *status |= NM_SO_STATUS_RECV_COMPLETED;

  } else {
    /* Data not yet received */

    /* WARNING! We currently ONLY support SMALL messages, so only
       track 0 is used! */

    /* Add the unpack info into the recv array */
    p_so_gate->recv[tag][seq].unpack_here.data = data;
    p_so_gate->recv[tag][seq].unpack_here.len = len;

    *status |= NM_SO_STATUS_UNPACK_HERE;
    *status &= ~NM_SO_STATUS_RECV_COMPLETED;

    p_so_gate->pending_unpacks++;

    /* Check if we should post a new recv packet */
    if(!p_so_gate->active_recv[0])
#ifdef NM_SO_OPTIMISTIC_RECV
      nm_so_post_optimistic_pw(p_gate, tag, seq, data, len);
#else
      nm_so_post_regular_pw(p_gate);
#endif
  }

  return NM_ESUCCESS;
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
				  NULL,
				  NULL,
				  p_so_gate);

    if(p_so_gate->pending_unpacks)
      /* Check if we should post a new recv packet */
      nm_so_post_regular_pw(p_so_pw->pw.p_gate);

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
