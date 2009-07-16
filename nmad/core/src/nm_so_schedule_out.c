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

#include <nm_private.h>


/** Process a complete successful outgoing request.
 */
int nm_so_process_complete_send(struct nm_core *p_core,
				struct nm_pkt_wrap *p_pw)
{
  struct nm_gate *p_gate = p_pw->p_gate;

  NM_TRACEF("send request complete: gate %d, drv %d, trk %d, tag %d, seq %d",
	    p_pw->p_gate->id, p_pw->p_drv->id, p_pw->trk_id,
	    p_pw->tag, p_pw->seq);
  
#ifdef PIOMAN
  piom_req_success(&p_pw->inst);
#endif
  FUT_DO_PROBE3(FUT_NMAD_NIC_OPS_SEND_PACKET, p_pw, p_pw->p_drv->id, p_pw->trk_id);
  
  p_gate->active_send[p_pw->p_drv->id][p_pw->trk_id]--;
  nm_pw_complete_contribs(p_core, p_pw);
  nm_so_pw_free(p_pw);
  nm_strat_try_and_commit(p_gate);
  
  return NM_ESUCCESS;
}

