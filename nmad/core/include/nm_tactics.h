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

/** Pack small data into a new packet wrapper on track #0, by copy.
 */
static inline void nm_tactic_pack_small_copy(struct nm_pack_s*p_pack, const char*data, int len, int offset,
					     struct tbx_fast_list_head*out_list)
{
  struct nm_pkt_wrap *p_pw = NULL;
  nm_so_pw_alloc(NM_PW_GLOBAL_HEADER, &p_pw);
  nm_so_pw_add_data(p_pw, p_pack, data, len, offset, NM_PW_GLOBAL_HEADER | NM_SO_DATA_USE_COPY);
  tbx_fast_list_add_tail(&p_pw->link, out_list);
}

/** Pack small data into a new packet wrapper on track #0, by iovec.
 */
static inline void nm_tactic_pack_small_ref(struct nm_pack_s*p_pack, const char*data, int len, int offset,
					    struct tbx_fast_list_head*out_list)
{
  struct nm_pkt_wrap *p_pw = NULL;
  nm_so_pw_alloc(NM_PW_GLOBAL_HEADER, &p_pw);
  nm_so_pw_add_data(p_pw, p_pack, data, len, offset, NM_PW_GLOBAL_HEADER);
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
  nm_so_init_rdv(&ctrl, p_pack, p_pack->len, 0, NM_PROTO_FLAG_LASTCHUNK);
  struct puk_receptacle_NewMad_Strategy_s*strategy = &p_pack->p_gate->strategy_receptacle;
  (*strategy->driver->pack_ctrl)(strategy->_status, p_pack->p_gate, &ctrl);
}


#endif /* NM_TACTICS_H */

