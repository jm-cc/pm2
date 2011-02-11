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

#ifndef NM_TACTICS_H
#define NM_TACTICS_H


/** Pack a generic control header as a new packet wrapper on track #0.
 */
static inline void nm_tactic_pack_ctrl(const union nm_so_generic_ctrl_header*p_ctrl,
				       struct tbx_fast_list_head*out_list)
{
  struct nm_pkt_wrap*p_pw = NULL;
  nm_so_pw_alloc(NM_PW_GLOBAL_HEADER, &p_pw);
  nm_so_pw_add_control(p_pw, p_ctrl);
  tbx_fast_list_add_tail(&p_pw->link, out_list);
}

/** Pack small data into an existing packet wrapper on track #0
 */
static inline void nm_tactic_pack_small_into_pw(struct nm_pack_s*p_pack, const char*data, int len, int offset,
						int copy_threshold, struct nm_pkt_wrap*p_pw)
{
  if(len < copy_threshold)
    nm_so_pw_add_data(p_pw, p_pack, data, len, offset, NM_PW_GLOBAL_HEADER | NM_SO_DATA_USE_COPY);
  else
    nm_so_pw_add_data(p_pw, p_pack, data, len, offset, NM_PW_GLOBAL_HEADER);
}

/** Pack small data into a new packet wrapper on track #0
 */
static inline void nm_tactic_pack_small_new_pw(struct nm_pack_s*p_pack, const char*data, int len, int offset,
					       int copy_threshold, struct tbx_fast_list_head*out_list)
{ 
  struct nm_pkt_wrap *p_pw = NULL;
  nm_so_pw_alloc(NM_PW_GLOBAL_HEADER, &p_pw);
  nm_tactic_pack_small_into_pw(p_pack, data, len, offset, copy_threshold, p_pw);
  tbx_fast_list_add_tail(&p_pw->link, out_list);
}

/** Pack large data into a new packet wrapper stored as pending large,
 * and pack a rdv for this data.
 */
static inline void nm_tactic_pack_rdv(struct nm_pack_s*p_pack, const char*data, int len, int offset)
{
  struct nm_pkt_wrap *p_pw = NULL;
  nm_so_pw_alloc(NM_PW_NOHEADER, &p_pw);
  nm_so_pw_add_data(p_pw, p_pack, data, len, offset, NM_PW_NOHEADER);
  tbx_fast_list_add_tail(&p_pw->link, &p_pack->p_gate->pending_large_send);
  union nm_so_generic_ctrl_header ctrl;
  nm_so_init_rdv(&ctrl, p_pack, len, offset, (p_pack->scheduled == p_pack->len) ? NM_PROTO_FLAG_LASTCHUNK : 0);
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_pack->p_gate->strategy_receptacle;
  (*strategy->driver->pack_ctrl)(strategy->_status, p_pack->p_gate, &ctrl);
}

/** Find in the given outlist a packet wrapper with
 * at least 'header_len' available as header and
 * 'data_len' available as data.
 * return NULL if none found.
 */
static inline struct nm_pkt_wrap*nm_tactic_try_to_aggregate(struct tbx_fast_list_head*out_list,
							    int header_len, int data_len)
{
  struct nm_pkt_wrap*p_pw = NULL;
  tbx_fast_list_for_each_entry(p_pw, out_list, link)
    {
      const uint32_t h_rlen = nm_so_pw_remaining_header_area(p_pw);
      const uint32_t d_rlen = nm_so_pw_remaining_data(p_pw);
      if(header_len < h_rlen && data_len < d_rlen)
	{
	  return p_pw;
	}
    }
  return NULL;
}

#endif /* NM_TACTICS_H */

