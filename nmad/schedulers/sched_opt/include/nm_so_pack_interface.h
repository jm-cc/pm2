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

#ifndef _NM_SO_PACK_INTERFACE_H_
#define _NM_SO_PACK_INTERFACE_H_

#include "nm_so_parameters.h"
#include "nm_so_raw_interface.h"

/* The following should be hidden */
struct nm_so_cnx {
  struct nm_gate *p_gate;
  uint8_t seq_number;
};

extern nm_so_interface nm_so_pack_interface;

extern struct nm_so_cnx out_cnx[], in_cnx[];

extern int
nm_so_pack_interface_init(struct nm_core *p_core);


extern int
nm_so_begin_packing(struct nm_core *p_core,
		    uint16_t gate_id, uint8_t tag,
		    struct nm_so_cnx **cnx);

static __inline__
int
nm_so_pack(struct nm_so_cnx *cnx,
	   void *data, uint32_t len)
{
  return _nm_so_pack(cnx->p_gate,
                     (cnx - out_cnx), cnx->seq_number++,
                     data, len);
}

static __inline__
int
nm_so_end_packing(struct nm_core *p_core, struct nm_so_cnx *cnx)
{
  return __nm_so_swait_range(p_core, cnx->p_gate,
			     cnx - out_cnx,
			     0, cnx->seq_number-1);
}


int
nm_so_begin_unpacking(struct nm_core *p_core,
		      uint16_t gate_id, uint8_t tag,
		      struct nm_so_cnx **cnx);

static __inline__
int
nm_so_unpack(struct nm_so_cnx *cnx,
	     void *data, uint32_t len)
{
  return _nm_so_unpack(cnx->p_gate,
                       (cnx - in_cnx), cnx->seq_number++,
                       data, len);
}

static __inline__
int
nm_so_end_unpacking(struct nm_core *p_core, struct nm_so_cnx *cnx)
{
  return __nm_so_rwait_range(p_core, cnx->p_gate,
			     cnx - in_cnx,
			     0, cnx->seq_number-1);
}

#endif
