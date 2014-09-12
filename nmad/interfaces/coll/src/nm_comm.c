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
#include "nm_coll_private.h"
#include <nm_launcher.h>

/* ********************************************************* */

static void nm_comm_commit(nm_comm_t p_comm)
{
  /* hash gates */
  p_comm->reverse = puk_hashtable_new_ptr();
  int j;
  for(j = 0; j < nm_gate_vect_size(p_comm->group); j++)
    {
      nm_gate_t p_gate = nm_gate_vect_at(p_comm->group, j);
      const intptr_t rank_as_ptr = j + 1;
      puk_hashtable_insert(p_comm->reverse, p_gate, (void*)rank_as_ptr);
    }
}

nm_comm_t nm_comm_world(void)
{
  static nm_comm_t world = NULL;
  if(world == NULL)
    {
      nm_group_t group = nm_gate_vect_new();
      int size = -1;
      nm_launcher_get_size(&size);
      int i;
      for(i = 0; i < size; i++)
	{
	  nm_gate_t p_gate = NULL;
	  nm_launcher_get_gate(i, &p_gate);
	  nm_gate_vect_push_back(group, p_gate);
	}
      nm_session_t p_session = NULL;
      nm_launcher_get_session(&p_session);
      nm_comm_t comm = malloc(sizeof(struct nm_comm_s));
      comm->group = group;
      comm->rank = nm_group_rank(group);
      nm_session_open(&comm->p_session, "nm_comm_world");
      nm_comm_commit(comm);
      void*old = __sync_val_compare_and_swap(&world, NULL, comm);
      if(old != NULL)
	{
	  nm_group_free(group);
	  free(comm);
	}
    }
  return world;
}

nm_comm_t nm_comm_self(void)
{
  static nm_comm_t self = NULL;
  if(self == NULL)
    {
      int launcher_rank = -1;
      nm_launcher_get_rank(&launcher_rank);
      nm_gate_t p_gate = NULL;
      nm_launcher_get_gate(launcher_rank, &p_gate);
      nm_group_t group = nm_gate_vect_new();
      nm_gate_vect_push_back(group,p_gate);
      nm_comm_t comm = malloc(sizeof(struct nm_comm_s));
      comm->group = group;
      comm->rank = nm_group_rank(group);
      nm_session_open(&comm->p_session, "nm_comm_self");
      nm_comm_commit(comm);
      void*old = __sync_val_compare_and_swap(&self, NULL, comm);
      if(old != NULL)
	{
	  nm_group_free(group);
	  free(comm);
	}
    }
  return self;
}

int nm_comm_size(nm_comm_t comm)
{
  return nm_group_size(comm->group);
}

int nm_comm_rank(nm_comm_t comm)
{
  return comm->rank;
}

nm_gate_t nm_comm_get_gate(nm_comm_t p_comm, int rank)
{
  return nm_group_get_gate(p_comm->group, rank);
}

int nm_comm_get_dest(nm_comm_t p_comm, nm_gate_t p_gate)
{
  intptr_t rank_as_ptr = (intptr_t)puk_hashtable_lookup(p_comm->reverse, p_gate);
  return (rank_as_ptr - 1);
}

nm_session_t nm_comm_get_session(nm_comm_t p_comm)
{
  return p_comm->p_session;
}

nm_group_t nm_comm_group(nm_comm_t comm)
{
  return comm->group;
}

nm_comm_t nm_comm_dup(nm_comm_t comm)
{
  nm_group_t group = nm_comm_group(comm);
  nm_comm_t newcomm = nm_comm_create(comm, group);
  return newcomm;
}

nm_comm_t nm_comm_create(nm_comm_t comm, nm_group_t group)
{
  int rank = nm_group_rank(group);
  struct nm_comm_create_header_s
  {
    int commit;
    char session_name[32];
  } header = { .session_name = { '\0'} };
  const int root = 0;
  const nm_tag_t tag = NM_COLL_TAG_COMM_CREATE;
  if(rank == -1)
    {
      /* not in group, wait for others */
      do
	{
	  nm_coll_bcast(comm, root, &header, sizeof(header), tag);
	}
      while(header.commit != 1);
      return NULL;
    }
  else
    {
      nm_session_t p_session = NULL;
      nm_comm_t newcomm = malloc(sizeof(struct nm_comm_s));
      newcomm->rank = rank;
      newcomm->group = nm_group_dup(group);
      newcomm->p_session = NULL;
      while(newcomm->p_session == NULL)
	{
	  if(rank == root)
	    {
	      snprintf(&header.session_name[0], 32, "nm_comm-%08x", (unsigned)random());
	      int rc = nm_session_open(&p_session, header.session_name);
	      if(rc == NM_ESUCCESS)
		{
		  int i;
		  header.commit = 0;
		  nm_coll_bcast(comm, root, &header, sizeof(header), tag);
		  int*acks = malloc(sizeof(int) * nm_group_size(group));
		  int ack = 1;
		  nm_coll_gather(comm, root, &ack, sizeof(ack), acks, sizeof(ack), tag);
		  int total_acks = 0;
		  for(i = 0; i < nm_group_size(group); i++)
		    {
		      total_acks += acks[i];
		    }
		  free(acks);
		  if(total_acks == nm_group_size(group))
		    {
		      header.commit = 1;
		      nm_coll_bcast(comm, root, &header, sizeof(header), tag);
		      newcomm->p_session = p_session;
		    }
		}
	    }
	  else
	    {
	      nm_coll_bcast(comm, root, &header, sizeof(header), tag);
	      if(header.commit == 0)
		{
		  if(p_session != NULL)
		    {
		      /* release previous round failed session creation */
		      nm_session_destroy(p_session);
		    }
		  int rc = nm_session_open(&p_session, header.session_name);
		  int ack = (rc == NM_ESUCCESS);
		  nm_coll_gather(comm, root, &ack, sizeof(ack), NULL, 0, tag);
		}
	      else if(header.commit == 1)
		{
		  newcomm->p_session = p_session;
		}
	    }
	}
      nm_comm_commit(newcomm);
      return newcomm;
    }
}

extern void nm_comm_destroy(nm_comm_t p_comm)
{
  if(p_comm != NULL)
    {
      nm_group_free(p_comm->group);
      free(p_comm);
    }
}
