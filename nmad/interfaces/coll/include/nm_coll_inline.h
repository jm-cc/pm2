/*
 * NewMadeleine
 * Copyright (C) 2014-2015 (see AUTHORS file)
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

/** @internal NM communicator internal definition */
struct nm_comm_s
{
  nm_session_t p_session;  /**< session for the communicator */
  nm_group_t group;        /**< group of nodes involved in the communicator */
  int rank;                /**< rank of local process in the group */
  puk_hashtable_t reverse; /**< reverse table: p_gate -> rank */
};

/** @internal internal nmad inter-communicator */
struct nm_intercomm_s
{
  nm_comm_t p_overlay_comm;  /**< an overlay communicator spanning all nodes */
  struct nm_comm_s*p_local_comm;
  struct nm_comm_s*p_remote_comm;
  nm_tag_t tag;
};


/* ********************************************************* */

static inline int nm_comm_size(nm_comm_t p_comm)
{
  return nm_group_size(p_comm->group);
}

static inline int nm_comm_rank(nm_comm_t p_comm)
{
  return p_comm->rank;
}

static inline nm_gate_t nm_comm_get_gate(nm_comm_t p_comm, int rank)
{
  return nm_group_get_gate(p_comm->group, rank);
}

static inline int nm_comm_get_dest(nm_comm_t p_comm, nm_gate_t p_gate)
{
  intptr_t rank_as_ptr = (intptr_t)puk_hashtable_lookup(p_comm->reverse, p_gate);
  return (rank_as_ptr - 1);
}

static inline nm_session_t nm_comm_get_session(nm_comm_t p_comm)
{
  return p_comm->p_session;
}

static inline nm_group_t nm_comm_group(nm_comm_t p_comm)
{
  return p_comm->group;
}

static inline nm_gate_t nm_intercomm_get_gate(nm_intercomm_t p_intercomm, int rank)
{
  nm_gate_t p_gate = nm_comm_get_gate(p_intercomm->p_remote_comm, rank);
  return p_gate;
}

static inline int nm_intercomm_get_dest(nm_intercomm_t p_intercomm, nm_gate_t p_gate)
{
  int rank = nm_comm_get_dest(p_intercomm->p_remote_comm, p_gate);
  return rank;
}

static inline nm_session_t nm_intercomm_get_session(nm_intercomm_t p_intercomm)
{
  return nm_comm_get_session(p_intercomm->p_overlay_comm);
}
