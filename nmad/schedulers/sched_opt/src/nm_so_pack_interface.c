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

#include <nm_so_private.h>

#include "nm_so_pack_interface.h"
#include "nm_so_raw_interface.h"
#include "nm_so_raw_interface_private.h"
#include "nm_so_tracks.h"

#define NM_SO_RECV_ANY_SRC  -1

struct __nm_so_cnx {
  struct nm_so_interface *p_interface;
  unsigned long gate_id;
  unsigned long tag;
  unsigned long first_seq_number;
};


int
nm_so_pack_interface_init(struct nm_core *p_core,
			  nm_so_pack_interface *p_interface)
{
  return nm_so_ri_init(p_core,
                       (struct nm_so_interface **)p_interface);
}


int
nm_so_begin_packing(nm_so_pack_interface interface,
		    uint16_t gate_id, uint8_t tag,
		    struct nm_so_cnx *cnx)
{
  struct __nm_so_cnx *_cnx = (struct __nm_so_cnx *)cnx;

  _cnx->p_interface = (struct nm_so_interface *)interface;
  _cnx->gate_id = gate_id;
  _cnx->tag = tag;
  _cnx->first_seq_number = nm_so_ri_get_current_send_seq(_cnx->p_interface,
							 gate_id, tag);

  return NM_ESUCCESS;
}

int
nm_so_pack(struct nm_so_cnx *cnx,
	   void *data, uint32_t len)
{
  struct __nm_so_cnx *_cnx = (struct __nm_so_cnx *)cnx;

  return nm_so_ri_isend(_cnx->p_interface, _cnx->gate_id, _cnx->tag,
			data, len, NULL);
}

int
nm_so_end_packing(struct nm_so_cnx *cnx)
{
  struct __nm_so_cnx *_cnx = (struct __nm_so_cnx *)cnx;
  unsigned long seq = nm_so_ri_get_current_send_seq(_cnx->p_interface,
						    _cnx->gate_id, _cnx->tag);

  return nm_so_ri_swait_range(_cnx->p_interface, _cnx->gate_id, _cnx->tag,
			      cnx->first_seq_number, seq-1);
}


int
nm_so_begin_unpacking(nm_so_pack_interface interface,
		      uint16_t gate_id, uint8_t tag,
		      struct nm_so_cnx *cnx)
{
  struct __nm_so_cnx *_cnx = (struct __nm_so_cnx *)cnx;

  _cnx->p_interface = (struct nm_so_interface *)interface;
  _cnx->gate_id = gate_id;
  _cnx->tag = tag;
  _cnx->first_seq_number = nm_so_ri_get_current_recv_seq(_cnx->p_interface,
							 gate_id, tag);

  return NM_ESUCCESS;
}


int
nm_so_begin_unpacking_any_src(nm_so_pack_interface interface,
                              uint8_t tag,
                              struct nm_so_cnx *cnx){
  struct __nm_so_cnx *_cnx = (struct __nm_so_cnx *)cnx;

  _cnx->p_interface = (struct nm_so_interface *)interface;
  _cnx->tag = tag;
  _cnx->gate_id = NM_SO_RECV_ANY_SRC;
  _cnx->first_seq_number = -1;

  return NM_ESUCCESS;
}



int
nm_so_unpack(struct nm_so_cnx *cnx,
	     void *data, uint32_t len)
{
  struct __nm_so_cnx *_cnx = (struct __nm_so_cnx *)cnx;

  return nm_so_ri_irecv(_cnx->p_interface, _cnx->gate_id, _cnx->tag,
			data, len, NULL);
}

int
nm_so_end_unpacking(struct nm_so_cnx *cnx)
{
  struct __nm_so_cnx *_cnx = (struct __nm_so_cnx *)cnx;

  unsigned long seq = nm_so_ri_get_current_recv_seq(_cnx->p_interface,
						    _cnx->gate_id, _cnx->tag);

  return nm_so_ri_rwait_range(_cnx->p_interface, _cnx->gate_id, _cnx->tag,
			      cnx->first_seq_number, seq-1);
}

int
nm_so_rwait(struct nm_so_cnx *cnx)
{
  struct __nm_so_cnx *_cnx = (struct __nm_so_cnx *)cnx;

  unsigned long seq = nm_so_ri_get_current_recv_seq(_cnx->p_interface,
						    _cnx->gate_id, _cnx->tag);

  return nm_so_ri_rwait_range(_cnx->p_interface, _cnx->gate_id, _cnx->tag, cnx->first_seq_number, seq-1);
}

