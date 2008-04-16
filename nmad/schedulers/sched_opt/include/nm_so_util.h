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

#ifndef _NM_SO_UTIL_H_
#define _NM_SO_UTIL_H_

#include <nm_public.h>
#include <nm_so_sendrecv_interface.h>
#include <nm_so_pack_interface.h>

/** Initializes everything. */
int nm_so_init(int *argc, char	**argv);

/** Cleans session. Returns NM_ESUCCESS or EXIT_FAILURE. */
int nm_so_exit(void);

/** Returns process rank */
int nm_so_get_rank(int *);

/** Returns the number of nodes */
int nm_so_get_size(int *);

/** Returns the send/receive interface */
int nm_so_get_sr_if(struct nm_so_interface **sr_if);

/** Returns the pack/unpack interface */
int nm_so_get_pack_if(nm_so_pack_interface *pack_if);

/** Returns the out gate id of the process dest */
int nm_so_get_gate_out_id(int dest, nm_gate_id_t *gate_id);

/** Returns the in gate id of the process dest */
int nm_so_get_gate_in_id(int dest, nm_gate_id_t *gate_id);

#endif /* _NM_SO_UTIL_H_ */
