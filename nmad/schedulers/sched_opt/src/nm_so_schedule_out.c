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
#include "nm_pkt_wrap.h"
#include "nm_trk.h"
#include "nm_so_private.h"
#include "nm_so_strategies.h"
#include "nm_so_pkt_wrap.h"
#include "nm_so_headers.h"
#include "nm_so_debug.h"
#include "nm_log.h"

/** Schedule outbound requests.
 */
int
nm_so_out_schedule_gate(struct nm_gate *p_gate)
{
  struct nm_so_gate *p_so_gate   = p_gate->sch_private;
  struct nm_so_sched *p_so_sched = p_so_gate->p_so_sched;

  return p_so_sched->current_strategy->try_and_commit(p_gate);
}

static int data_completion_callback(struct nm_so_pkt_wrap *p_so_pw,
                                    void *ptr TBX_UNUSED,
                                    void *header, uint32_t len TBX_UNUSED,
                                    uint8_t proto_id, uint8_t seq,
                                    uint32_t chunk_offset, uint8_t is_last_chunk)
{
  struct nm_gate *p_gate = p_so_pw->pw.p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;
  struct nm_so_interface_ops *interface = p_so_gate->p_so_sched->current_interface;

  /** Process a complete data request.
   */
  NM_SO_TRACE("Send completed for chunk : %p, len = %u, tag = %d, seq = %u, offset = %u\n", ptr, len, proto_id-128, seq, chunk_offset);

  p_so_gate->send[proto_id-128][seq] -= len;

  if(p_so_gate->send[proto_id-128][seq] <= 0){
    NM_SO_TRACE("All chunks are sent for the message with tag %u, seq %u and len %u!\n", proto_id -128, seq, len);
    interface->pack_success(p_gate, proto_id - 128, seq);

  } else {
    NM_SO_TRACE("It is missing %d bytes to complete sending of the msg with tag %u, seq %u\n", p_so_gate->send[proto_id-128][seq], proto_id - 128, seq);
  }

  return NM_SO_HEADER_MARK_READ;
}

/** Process a complete successful outgoing request.
 */
int
nm_so_out_process_success_rq(struct nm_sched *p_sched TBX_UNUSED,
                             struct nm_pkt_wrap	*p_pw) {
  int err;
  struct nm_so_pkt_wrap *p_so_pw = nm_pw2so(p_pw);
  struct nm_gate *p_gate = p_pw->p_gate;
  struct nm_so_gate *p_so_gate = p_gate->sch_private;

  p_so_gate->active_send[p_pw->p_drv->id][p_pw->p_trk->id]--;

  if(p_pw->p_trk->id == 0) {
    NM_SO_TRACE("********** sent of a short one on drv %d and trk %d***********\n", p_pw->p_drv->id, p_pw->p_trk->id);

    /* Track 0 */
    nm_so_pw_iterate_over_headers(p_so_pw, data_completion_callback, NULL, NULL, NULL);

  } else if(p_pw->p_trk->id == 1) {
    NM_SO_TRACE("********** sent of a large one on drv %d and trk %d of %llu octets ***********\n", p_pw->p_drv->id, p_pw->p_trk->id, p_pw->length);

    struct nm_so_interface_ops *interface=p_so_gate->p_so_sched->current_interface;
    uint8_t  proto_id = p_pw->proto_id - 128;
    uint8_t  seq = p_pw->seq;

    p_so_gate->send[proto_id][seq] -= p_pw->length;

    if(p_so_pw->datatype_copied_buf){
      //liberation du tampon ou l'on a copie en contigu les donnees parce qu'on avait pas assez d'entrees dans l'iovec
      TBX_FREE(p_so_pw->pw.v[0].iov_base);
    }

    if(p_so_gate->send[proto_id][seq] <= 0){
      NM_SO_TRACE("---> All chunks are sended!\n");
      interface->pack_success(p_gate, proto_id, seq);

    } else {
      NM_SO_TRACE("It is missing %d bytes to complete sending of the msg with tag %u, seq %u\n", p_so_gate->send[proto_id][seq], proto_id, seq);
    }

    /* Free the wrapper */
    nm_so_pw_free(p_so_pw);
  }

  err = NM_ESUCCESS;
  return err;
}

/** Process a failed outgoing request.
 */
int
nm_so_out_process_failed_rq(struct nm_sched	*p_sched,
                            struct nm_pkt_wrap	*p_pw,
                            int		 	_err TBX_UNUSED)
{
    TBX_FAILURE("nm_so_out_process_failed_rq");
    return nm_so_out_process_success_rq(p_sched, p_pw);
}
