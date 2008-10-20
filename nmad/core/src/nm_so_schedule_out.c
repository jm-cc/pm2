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

/** Schedule outbound requests.
 */
int nm_so_out_schedule_gate(struct nm_gate *p_gate)
{
  struct puk_receptacle_NewMad_Strategy_s*r = &p_gate->p_so_gate->strategy_receptacle;
  return r->driver->try_and_commit(r->_status, p_gate);
}

/** Process a data request completion.
 */
static inline void nm_so_out_data_complete(struct nm_gate*p_gate, nm_tag_t proto_id, uint8_t seq, uint32_t len)
{
  const nm_tag_t tag = proto_id - 128;
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_so_gate->tags, tag);

  p_so_tag->send[seq] -= len;

  if(p_so_tag->send[seq] <= 0)
    {
      NM_SO_TRACE("all chunks sent for msg tag=%u seq=%u len=%u!\n", tag, seq, len);
      nm_so_status_event(p_gate->p_core, NM_SO_STATUS_PACK_COMPLETED, p_gate, tag, seq, tbx_false);
    } 
  else 
    {
      NM_SO_TRACE("It is missing %d bytes to complete sending of the msg with tag %u, seq %u\n", p_so_tag->send[seq], tag, seq);
    }
 
}

static int data_completion_callback(struct nm_pkt_wrap *p_pw,
                                    void *ptr TBX_UNUSED,
                                    void *header, uint32_t len TBX_UNUSED,
                                    nm_tag_t proto_id, uint8_t seq,
                                    uint32_t chunk_offset, uint8_t is_last_chunk)
{
  struct nm_gate *p_gate = p_pw->p_gate;
  NM_SO_TRACE("completed chunk ptr=%p len=%u tag=%d seq=%u offset=%u\n", ptr, len, proto_id - 128, seq, chunk_offset);
  nm_so_out_data_complete(p_gate, proto_id, seq, len);
  return NM_SO_HEADER_MARK_READ;
}

/** Process a complete successful outgoing request.
 */
int nm_so_out_process_success_rq(struct nm_core *p_core TBX_UNUSED,
				 struct nm_pkt_wrap *p_pw)
{
  int err;
  struct nm_gate *p_gate = p_pw->p_gate;
  struct nm_so_gate *p_so_gate = p_gate->p_so_gate;

  p_so_gate->active_send[p_pw->p_drv->id][p_pw->trk_id]--;

  if(p_pw->trk_id == NM_TRK_SMALL) 
    {
      NM_SO_TRACE("completed short msg- drv=%d trk=%d\n", p_pw->p_drv->id, p_pw->trk_id);
      
      nm_so_pw_iterate_over_headers(p_pw, data_completion_callback, NULL, NULL, NULL);
    } 
  else if(p_pw->trk_id == NM_TRK_LARGE)
    {
      NM_SO_TRACE("completed large msg- drv=%d trk=%d size=%llu bytes\n",
		  p_pw->p_drv->id, p_pw->trk_id, (long long unsigned int)p_pw->length);

      nm_so_out_data_complete(p_gate, p_pw->proto_id, p_pw->seq, p_pw->length);

      if(p_pw->datatype_copied_buf)
	{
	  TBX_FREE(p_pw->v[0].iov_base);
	}

      nm_so_pw_free(p_pw);
    }
  
  nm_so_out_schedule_gate(p_gate);
  
  err = NM_ESUCCESS;
  return err;
}

/** Process a failed outgoing request.
 */
int nm_so_out_process_failed_rq(struct nm_core *p_core,
				struct nm_pkt_wrap	*p_pw,
				int		 	_err TBX_UNUSED)
{
    TBX_FAILURE("nm_so_out_process_failed_rq");
    return nm_so_out_process_success_rq(p_core, p_pw);
}