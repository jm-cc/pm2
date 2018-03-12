/*
 * NewMadeleine
 * Copyright (C) 2014-2018 (see AUTHORS file)
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

/** @ingroup coll_interface
 * @{
 */

/* ** group-based collectives */

extern void nm_coll_group_barrier(nm_session_t p_session, nm_group_t p_group, int self, nm_tag_t tag);

extern void nm_coll_group_bcast(nm_session_t p_session, nm_group_t p_group, int root, int self,
				void*buffer, nm_len_t len, nm_tag_t tag);

extern void nm_coll_group_scatter(nm_session_t p_session, nm_group_t p_group, int root, int self,
				  const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag);

extern void nm_coll_group_gather(nm_session_t p_session, nm_group_t p_group, int root, int self,
				 const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag);

/* ** communicator-based collectives */

extern void nm_coll_barrier(nm_comm_t comm, nm_tag_t tag);

extern void nm_coll_bcast(nm_comm_t comm, int root, void*buffer, nm_len_t len, nm_tag_t tag);

extern void nm_coll_scatter(nm_comm_t comm, int root, const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag);

extern void nm_coll_gather(nm_comm_t comm, int root, const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag);

/* ** data-based collectives */

extern void nm_coll_data_bcast(nm_comm_t comm, int root, struct nm_data_s*p_data, nm_tag_t tag);

extern void nm_coll_data_scatter(nm_comm_t p_comm, int root, struct nm_data_s p_sdata[], struct nm_data_s*p_rdata, nm_tag_t tag);

extern void nm_coll_data_gather(nm_comm_t p_comm, int root, struct nm_data_s*p_sdata, struct nm_data_s p_rdata[], nm_tag_t tag);

extern void nm_coll_group_data_bcast(nm_session_t p_session, nm_group_t p_group, int root, int self,
                                     struct nm_data_s*p_data, nm_tag_t tag);

/* ** non-blocking collectives */

extern struct nm_coll_bcast_s*nm_coll_group_data_ibcast(nm_session_t p_session, nm_group_t p_group, int root, int self,
                                                        struct nm_data_s*p_data, nm_tag_t tag);
extern void nm_coll_ibcast_wait(struct nm_coll_bcast_s*p_bcast);


/** @} */

