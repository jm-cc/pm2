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

#ifndef _NM_SO_SENDRECV_INTERFACE_H_
#define _NM_SO_SENDRECV_INTERFACE_H_

#include <nm_public.h>

typedef intptr_t nm_so_sr_interface;
typedef unsigned long nm_so_sr_request;

extern int
nm_so_sr_interface_init(struct nm_core *p_core,
			nm_so_sr_interface *p_interface);


extern int
nm_so_sr_isend(nm_so_sr_interface interface,
	       uint16_t gate_id, uint8_t tag,
	       void *data, uint32_t len,
	       nm_so_sr_request *p_request);

extern int
nm_so_sr_swait(nm_so_sr_interface interface,
	       nm_so_sr_request request);

extern int
nm_so_sr_irecv(nm_so_sr_interface interface,
	       uint16_t gate_id, uint8_t tag,
	       void *data, uint32_t len,
	       nm_so_sr_request *p_request);

extern int
nm_so_sr_rwait(nm_so_sr_interface interface,
	       nm_so_sr_request request);

#endif
