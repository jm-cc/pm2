/*
 * NewMadeleine
 * Copyright (C) 2014-2016 (see AUTHORS file)
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

#include <nm_public.h>
#include <nm_sendrecv_interface.h>
#include <Padico/Puk.h>

/* ** group-based collectives */

extern void nm_coll_group_barrier(nm_session_t p_session, nm_group_t p_group, nm_gate_t p_self_gate, nm_tag_t tag);

extern void nm_coll_group_bcast(nm_session_t p_session, nm_group_t p_group, nm_gate_t p_root_gate, nm_gate_t p_self_gate,
				void*buffer, nm_len_t len, nm_tag_t tag);

extern void nm_coll_group_scatter(nm_session_t p_session, nm_group_t p_group, nm_gate_t p_root_gate, nm_gate_t p_self_gate,
				  const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag);

extern void nm_coll_group_gather(nm_session_t p_session, nm_group_t p_group, nm_gate_t p_root_gate, nm_gate_t p_self_gate,
				 const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag);

/* ** communicator_based collectives */

extern void nm_coll_barrier(nm_comm_t comm, nm_tag_t tag);

extern void nm_coll_bcast(nm_comm_t comm, int root, void*buffer, nm_len_t len, nm_tag_t tag);

extern void nm_coll_scatter(nm_comm_t comm, int root, const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag);

extern void nm_coll_gather(nm_comm_t comm, int root, const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag);

