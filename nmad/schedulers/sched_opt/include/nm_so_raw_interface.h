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

#ifndef _NM_SO_RAW_INTERFACE_H_
#define _NM_SO_RAW_INTERFACE_H_

struct nm_so_interface;

typedef intptr_t nm_so_request;

extern int
nm_so_ri_init(struct nm_core *p_core,
	      struct nm_so_interface **p_so_interface);


extern int
nm_so_ri_isend(struct nm_so_interface *p_so_interface,
	       uint16_t gate_id, uint8_t tag,
	       void *data, uint32_t len,
	       nm_so_request *p_request);

extern int
nm_so_ri_stest(struct nm_so_interface *p_so_interface,
	       nm_so_request request);

extern int
nm_so_ri_swait(struct nm_so_interface *p_so_interface,
	       nm_so_request request);

extern int
nm_so_ri_swait_range(struct nm_so_interface *p_so_interface,
		     uint16_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long seq_sup);


extern int
nm_so_ri_irecv(struct nm_so_interface *p_so_interface,
	       long gate_id, uint8_t tag,
	       void *data, uint32_t len,
	       nm_so_request *p_request);

extern int
nm_so_ri_rtest(struct nm_so_interface *p_so_interface,
	       nm_so_request request);

extern int
nm_so_ri_rwait(struct nm_so_interface *p_so_interface,
	       nm_so_request request);

extern int
nm_so_ri_rwait_range(struct nm_so_interface *p_so_interface,
		     uint16_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long seq_sup);


extern unsigned long
nm_so_ri_get_current_send_seq(struct nm_so_interface *p_so_interface,
			      uint16_t gate_id, uint8_t tag);

extern unsigned long
nm_so_ri_get_current_recv_seq(struct nm_so_interface *p_so_interface,
			      uint16_t gate_id, uint8_t tag);


#endif
