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

#ifndef _NM_SO_INTERFACE_H_
#define _NM_SO_INTERFACE_H_

#include "nm_so_parameters.h"

/* WARNING!!! There's a severe limitation here: concurrent
   begin_(un)pack/end_(un)pack session are not allowed on the same
   tag_id, even if they use different gates... */

/* The following should be hidden */
struct nm_so_cnx {
  struct nm_gate *p_gate;
  uint8_t seq_number;
};

extern struct nm_so_cnx out_cnx[], in_cnx[];

int
nm_so_interface_init(void);


int
nm_so_begin_packing(struct nm_core *p_core,
		    uint16_t gate_id, uint8_t tag,
		    struct nm_so_cnx **cnx);
extern int
(*__nm_so_pack)(struct nm_gate *p_gate,
		uint8_t tag, uint8_t seq,
		void *data, uint32_t len);

static __inline__
int
nm_so_pack(struct nm_so_cnx *cnx,
	   void *data, uint32_t len)
{
  return __nm_so_pack(cnx->p_gate,
		      (cnx - out_cnx), cnx->seq_number++,
		      data, len);
}

int
__nm_so_wait_send_range(struct nm_core *p_core,
			struct nm_gate *p_gate,
			uint8_t tag,
			uint8_t seq_inf, uint8_t seq_sup);

static __inline__
int
nm_so_end_packing(struct nm_core *p_core, struct nm_so_cnx *cnx)
{
  return __nm_so_wait_send_range(p_core, cnx->p_gate,
				 cnx - out_cnx,
				 0, cnx->seq_number-1);
}


int
nm_so_begin_unpacking(struct nm_core *p_core,
		      uint16_t gate_id, uint8_t tag,
		      struct nm_so_cnx **cnx);

int
__nm_so_unpack(struct nm_gate *p_gate,
	       uint8_t tag, uint8_t seq,
	       void *data, uint32_t len);

static __inline__
int
nm_so_unpack(struct nm_so_cnx *cnx,
	     void *data, uint32_t len)
{
  return __nm_so_unpack(cnx->p_gate,
			(cnx - in_cnx), cnx->seq_number++,
			data, len);
}

int
__nm_so_wait_recv_range(struct nm_core *p_core,
			struct nm_gate *p_gate,
			uint8_t tag,
			uint8_t seq_inf, uint8_t seq_sup);

static __inline__
int
nm_so_end_unpacking(struct nm_core *p_core, struct nm_so_cnx *cnx)
{
  return __nm_so_wait_recv_range(p_core, cnx->p_gate,
				 cnx - in_cnx,
				 0, cnx->seq_number-1);
}

#endif
