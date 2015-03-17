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
      nm_comm_t p_comm = malloc(sizeof(struct nm_comm_s));
      p_comm->group = group;
      p_comm->rank = nm_group_rank(group);
      nm_session_open(&p_comm->p_session, "nm_comm_world");
      nm_comm_commit(p_comm);
      void*old = __sync_val_compare_and_swap(&world, NULL, p_comm);
      if(old != NULL)
	{
	  nm_comm_destroy(p_comm);
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
      nm_gate_vect_push_back(group, p_gate);
      nm_comm_t p_comm = malloc(sizeof(struct nm_comm_s));
      p_comm->group = group;
      p_comm->rank = nm_group_rank(group);
      nm_session_open(&p_comm->p_session, "nm_comm_self");
      nm_comm_commit(p_comm);
      void*old = __sync_val_compare_and_swap(&self, NULL, p_comm);
      if(old != NULL)
	{
	  nm_comm_destroy(p_comm);
	}
    }
  return self;
}


nm_comm_t nm_comm_dup(nm_comm_t p_comm)
{
  nm_group_t group = nm_comm_group(p_comm);
  nm_comm_t p_newcomm = nm_comm_create(p_comm, group);
  return p_newcomm;
}

nm_comm_t nm_comm_create(nm_comm_t p_comm, nm_group_t group)
{
  const int newrank = nm_group_rank(group);
  const int newroot = 0;
  struct nm_comm_create_header_s
  {
    int commit;
    char session_name[32];
  } header = { .session_name = { '\0'} };
  const int root = nm_comm_get_dest(p_comm, nm_gate_vect_at(group, newroot));
  const nm_tag_t tag = NM_COLL_TAG_COMM_CREATE;
  if(newrank == -1)
    {
      /* not in group, wait for others */
      do
	{
	  nm_coll_bcast(p_comm, root, &header, sizeof(header), tag);
	  if(header.commit == 0)
	    {
	      int ack = 1;
	      nm_coll_gather(p_comm, root, &ack, sizeof(ack), NULL, 0, tag);
	    }
	}
      while(header.commit != 1);
      return NULL;
    }
  else
    {
      nm_session_t p_session = NULL;
      nm_comm_t newcomm = malloc(sizeof(struct nm_comm_s));
      newcomm->rank = newrank;
      newcomm->group = nm_group_dup(group);
      newcomm->p_session = NULL;
      while(newcomm->p_session == NULL)
	{
	  if(newrank == newroot)
	    {
	      snprintf(&header.session_name[0], 32, "nm_comm-%08x", (unsigned)random());
	      int rc = nm_session_open(&p_session, header.session_name);
	      if(rc == NM_ESUCCESS)
		{
		  int i;
		  header.commit = 0;
		  nm_coll_bcast(p_comm, root, &header, sizeof(header), tag);
		  int*acks = malloc(sizeof(int) * nm_comm_size(p_comm));
		  int ack = 1;
		  nm_coll_gather(p_comm, root, &ack, sizeof(ack), acks, sizeof(ack), tag);
		  int total_acks = 0;
		  for(i = 0; i < nm_group_size(group); i++)
		    {
		      total_acks += acks[i];
		    }
		  free(acks);
		  if(total_acks == nm_group_size(group))
		    {
		      header.commit = 1;
		      nm_coll_bcast(p_comm, root, &header, sizeof(header), tag);
		      newcomm->p_session = p_session;
		    }
		}
	    }
	  else
	    {
	      nm_coll_bcast(p_comm, root, &header, sizeof(header), tag);
	      if(header.commit == 0)
		{
		  if(p_session != NULL)
		    {
		      /* release previous round failed session creation */
		      nm_session_destroy(p_session);
		    }
		  int rc = nm_session_open(&p_session, header.session_name);
		  int ack = (rc == NM_ESUCCESS);
		  nm_coll_gather(p_comm, root, &ack, sizeof(ack), NULL, 0, tag);
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

void nm_comm_destroy(nm_comm_t p_comm)
{
  if(p_comm != NULL)
    {
      nm_session_destroy(p_comm->p_session);
      nm_group_free(p_comm->group);
      free(p_comm);
    }
}
