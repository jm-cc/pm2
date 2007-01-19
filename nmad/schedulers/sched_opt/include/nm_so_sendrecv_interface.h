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

#ifndef NM_SO_ANY_SRC
#define NM_SO_ANY_SRC  ((long)-1)
#endif

struct nm_so_interface;
#ifdef XPAULETTE
#include "nm_xpaul.h"
#endif

typedef struct {
#ifdef XPAULETTE
#warning XPAUL_TODO
	struct nm_xpaul_data nm_xp_data;	
	int err;
	p_tbx_slist_t *list;
	struct nm_xpaul_data *cnx; // "Master query's" xpaul_data	
#endif
	volatile intptr_t request;	
} nm_so_request, *nm_so_request_t;

#include "nm_so_private.h"

extern int
nm_so_sr_init(struct nm_core *p_core,
	      struct nm_so_interface **p_so_interface);

int
nm_so_sr_exit(struct nm_so_interface *p_so_interface);


extern int
nm_so_sr_isend(struct nm_so_interface *p_so_interface,
	       uint16_t gate_id, uint8_t tag,
	       void *data, uint32_t len,
	       nm_so_request_t *p_request);

extern int
nm_so_sr_stest(struct nm_so_interface *p_so_interface,
	       nm_so_request_t request);

extern int
nm_so_sr_stest_range(struct nm_so_interface *p_so_interface,
		     uint16_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long nb);
extern int
nm_so_sr_swait(struct nm_so_interface *p_so_interface,
	       nm_so_request_t request);

extern int
nm_so_sr_swait_range(struct nm_so_interface *p_so_interface,
		     uint16_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long nb);


extern int
nm_so_sr_irecv(struct nm_so_interface *p_so_interface,
	       long gate_id, uint8_t tag,
	       void *data, uint32_t len,
	       nm_so_request_t *p_request);

extern int
nm_so_sr_rtest(struct nm_so_interface *p_so_interface,
	       nm_so_request_t request);

extern int
nm_so_sr_rtest_range(struct nm_so_interface *p_so_interface,
		     uint16_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long nb);

extern int
nm_so_sr_rwait(struct nm_so_interface *p_so_interface,
	       nm_so_request_t request);

extern int
nm_so_sr_rwait_range(struct nm_so_interface *p_so_interface,
		     uint16_t gate_id, uint8_t tag,
		     unsigned long seq_inf, unsigned long nb);

int
nm_so_sr_recv_source(struct nm_so_interface *p_so_interface,
                     nm_so_request_t request, long *gate_id);

extern int
nm_so_sr_probe(struct nm_so_interface *p_so_interface,
               long gate_id, uint8_t tag);

extern unsigned long
nm_so_sr_get_current_send_seq(struct nm_so_interface *p_so_interface,
			      uint16_t gate_id, uint8_t tag);

extern unsigned long
nm_so_sr_get_current_recv_seq(struct nm_so_interface *p_so_interface,
			      uint16_t gate_id, uint8_t tag);


#endif
