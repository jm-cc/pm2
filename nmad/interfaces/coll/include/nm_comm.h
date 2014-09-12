/*
 * NewMadeleine
 * Copyright (C) 2014 (see AUTHORS file)
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

typedef struct nm_comm_s*nm_comm_t;

extern nm_comm_t nm_comm_world(void);

extern nm_comm_t nm_comm_self(void);

extern int nm_comm_size(nm_comm_t comm);

extern int nm_comm_rank(nm_comm_t comm);

extern nm_gate_t nm_comm_get_gate(nm_comm_t p_comm, int rank);

extern int nm_comm_get_dest(nm_comm_t p_comm, nm_gate_t p_gate);

extern nm_session_t nm_comm_get_session(nm_comm_t p_comm);

extern nm_comm_t nm_comm_create(nm_comm_t comm, nm_group_t group);

extern void nm_comm_destroy(nm_comm_t p_comm);

extern nm_group_t nm_comm_group(nm_comm_t comm);

extern nm_comm_t nm_comm_dup(nm_comm_t comm);
