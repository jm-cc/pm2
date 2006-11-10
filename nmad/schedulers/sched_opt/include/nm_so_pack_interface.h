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

typedef intptr_t nm_so_pack_interface;

struct nm_so_cnx {
  intptr_t interface;
  intptr_t gate;
  intptr_t tag;
  intptr_t first_seq_number;
};

extern int
nm_so_pack_interface_init(struct nm_core *p_core,
			  nm_so_pack_interface *p_interface);


extern int
nm_so_begin_packing(nm_so_pack_interface interface,
		    uint16_t gate_id, uint8_t tag,
		    struct nm_so_cnx *cnx);

extern int
nm_so_pack(struct nm_so_cnx *cnx,
	   void *data, uint32_t len);

extern int
nm_so_end_packing(struct nm_so_cnx *cnx);


int
nm_so_begin_unpacking(nm_so_pack_interface interface,
		      uint16_t gate_id, uint8_t tag,
		      struct nm_so_cnx *cnx);

int
nm_so_begin_unpacking_any_src(nm_so_pack_interface interface,
                              uint8_t tag,
                              struct nm_so_cnx *cnx);


int
nm_so_unpack(struct nm_so_cnx *cnx,
	     void *data, uint32_t len);

int
nm_so_end_unpacking(struct nm_so_cnx *cnx);


#endif
