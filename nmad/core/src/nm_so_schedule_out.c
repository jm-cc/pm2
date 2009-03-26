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

/** Process a data request completion.
 */
static inline void nm_so_out_data_complete(struct nm_gate*p_gate, nm_tag_t proto_id, uint8_t seq, uint32_t len)
{
  const nm_tag_t tag = proto_id - 128;
  struct nm_so_tag_s*p_so_tag = nm_so_tag_get(&p_gate->tags, tag);

  p_so_tag->send[seq] -= len;

  if(p_so_tag->send[seq] == 0)
    {
      NM_SO_TRACE("all chunks sent for msg tag=%u seq=%u len=%u!\n", tag, seq, len);
      const struct nm_so_event_s event =
	{
	  .status = NM_SO_STATUS_PACK_COMPLETED,
	  .p_gate = p_gate,
	  .tag = tag,
	  .seq = seq,
	  .any_src = tbx_false
	};
      nm_so_status_event(p_gate->p_core, &event);
    } 
  else if(p_so_tag->send[seq] > 0)
     {
      NM_SO_TRACE("It is missing %d bytes to complete sending of the msg with tag %u, seq %u\n", p_so_tag->send[seq], tag, seq);
    }
  else
    {
      TBX_FAILUREF("more bytes sent than posted on tag %d! (diff = %d; just sent = %d)\n",
		   tag, p_so_tag->send[seq], len);
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
int nm_so_process_complete_send(struct nm_core *p_core TBX_UNUSED,
				struct nm_pkt_wrap *p_pw)
{
  struct nm_gate *p_gate = p_pw->p_gate;

  NM_TRACEF("send request complete: gate %d, drv %d, trk %d, proto %d, seq %d",
	    p_pw->p_gate->id,
	    p_pw->p_drv->id,
	    p_pw->trk_id,
	    p_pw->proto_id,
	    p_pw->seq);
  
#ifdef PIOMAN
  piom_req_success(&p_pw->inst);
#endif
  FUT_DO_PROBE3(FUT_NMAD_NIC_OPS_SEND_PACKET, p_pw, p_pw->p_drv->id, p_pw->trk_id);
  
  p_gate->active_send[p_pw->p_drv->id][p_pw->trk_id]--;

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
  
  return NM_ESUCCESS;
}

