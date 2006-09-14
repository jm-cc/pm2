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
#include "nm_so_strategies.h"
#include "nm_so_pkt_wrap.h"
#include "nm_so_headers.h"
#include "nm_so_interface.h"

int
__nm_so_wait_send_range(struct nm_core *p_core,
			struct nm_gate *p_gate,
			uint8_t tag,
			uint8_t seq_inf, uint8_t seq_sup)
{
  uint8_t seq = seq_inf;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  do {
    while(!(p_so_gate->status[tag][seq] & NM_SO_STATUS_SEND_COMPLETED))

      if(p_so_gate->active_recv[0])
	/* We also need to schedule in new packets (typically ACKs) */
	nm_schedule(p_core);
      else
	/* We just need to schedule out data on this gate */
	nm_sched_out_gate(p_gate);

  } while(seq++ != seq_sup);

  return NM_ESUCCESS;
}

static int data_completion_callback(struct nm_so_pkt_wrap *p_so_pw,
				    void *ptr, uint32_t len,
				    uint8_t proto_id, uint8_t seq,
				    void *arg)
{
  struct nm_so_gate *p_so_gate = (struct nm_so_gate *)arg;

  //  printf("Send completed for chunk : %p, len = %u, tag = %d, seq = %u\n",
  //  	 ptr, len, proto_id-128, seq);

  p_so_gate->status[proto_id - 128][seq] |= NM_SO_STATUS_SEND_COMPLETED;

  return NM_SO_HEADER_MARK_READ;
}

/* process complete successful outgoing request */
int
nm_so_out_process_success_rq(struct nm_sched *p_sched,
                             struct nm_pkt_wrap	*p_pw) {
  int err;
  struct nm_so_pkt_wrap *p_so_pw = nm_pw2so(p_pw);
  struct nm_so_gate *p_so_gate = p_so_pw->pw.p_gate->sch_private;

  p_so_gate->active_send[p_pw->p_trk->id]--;

  if(p_pw->p_trk->id == 0) {
    /* Track 0 */

    nm_so_pw_iterate_over_headers(p_so_pw,
				  data_completion_callback,
				  NULL,
				  NULL,
				  p_so_gate);

  } else if(p_pw->p_trk->id == 1) {

    p_so_gate->status[p_pw->proto_id - 128][p_pw->seq] |=
      NM_SO_STATUS_SEND_COMPLETED;

    /* Free the wrapper */
    nm_so_pw_free(p_so_pw);
  }

  err = NM_ESUCCESS;
  return err;
}

/* process complete failed outgoing request */
int
nm_so_out_process_failed_rq(struct nm_sched	*p_sched,
                            struct nm_pkt_wrap	*p_pw,
                            int		 	_err)
{
    TBX_FAILURE("nm_so_out_process_failed_rq");
    return nm_so_out_process_success_rq(p_sched,p_pw);
}
