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

/*
#include "nm_so_parameters.h"

typedef intptr_t nm_so_pack_api;

typedef struct nm_so_cnx;

extern int
nm_so_pack_interface_init(struct nm_core *p_core,
			  nm_so_pack_api *p_interface);


extern int
nm_so_begin_packing(nm_so_pack_api interface,
		    uint16_t gate_id, uint8_t tag,
		    struct nm_so_cnx **cnx);

int
nm_so_pack(struct nm_so_cnx *cnx,
	   void *data, uint32_t len);

{
  return _nm_so_pack(cnx->p_gate,
                     (cnx - out_cnx), cnx->seq_number++,
                     data, len);
}

int
nm_so_end_packing(struct nm_so_cnx *cnx);

{
  return __nm_so_swait_range(p_core, cnx->p_gate,
			     cnx - out_cnx,
			     0, cnx->seq_number-1);
}


int
nm_so_begin_unpacking(nm_so_pack_api interface,
		      uint16_t gate_id, uint8_t tag,
		      struct nm_so_cnx **cnx);

int
nm_so_unpack(struct nm_so_cnx *cnx,
	     void *data, uint32_t len);

int
nm_so_end_unpacking(struct nm_so_cnx *cnx);

{
  return __nm_so_rwait_range(p_core, cnx->p_gate,
			     cnx - in_cnx,
			     0, cnx->seq_number-1);
}

*/

#endif
