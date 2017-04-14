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

/** @ingroup coll_interface
 * @{
 */

/** NewMadeleine communicator opaque type */
typedef struct nm_comm_s*nm_comm_t;

/** get a new global communicator */
extern nm_comm_t nm_comm_world(const char*label);

/** get a new communicator containing only self */
extern nm_comm_t nm_comm_self(const char*label);

/** create a sub-communicator containing nodes in group- collective on parent communicator */
extern nm_comm_t nm_comm_create(nm_comm_t comm, nm_group_t group);

/** create a new communicator, with parent p_comm, new communicator group is p_newcomm_group, collectively called from group p_bcast_group */
extern nm_comm_t nm_comm_create_group(nm_comm_t p_comm, nm_group_t p_newcomm_group, nm_group_t p_bcast_group);

/** destroy a communicator- no synchronization is done */
extern void      nm_comm_destroy(nm_comm_t p_comm);

/** get a duplicate of the given communicator- collective on parent communicator */
extern nm_comm_t nm_comm_dup(nm_comm_t comm);

/** get the number of nodes in the communicator */
static int          nm_comm_size(nm_comm_t comm);

/** get the rank of self in the given communicator */
static int          nm_comm_rank(nm_comm_t comm);

/** get the gate for the given rank in communicator numbering */
static nm_gate_t    nm_comm_get_gate(nm_comm_t p_comm, int rank);

/** get the gate for self in the given communicator */
static nm_gate_t    nm_comm_gate_self(nm_comm_t p_comm);

/** get the rank for the given gate, in communicator numbering */
static int          nm_comm_get_dest(nm_comm_t p_comm, nm_gate_t p_gate);

/** get the session attached to the communicator */
static nm_session_t nm_comm_get_session(nm_comm_t p_comm);

/** get the group of nodes in the communicator- real group, not a duplicate */
static nm_group_t   nm_comm_group(nm_comm_t comm);

/** @} */
