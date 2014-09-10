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

static nm_comm_t world = NULL;

nm_comm_t nm_comm_world(void)
{
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
      comm->p_session = p_session;
      void*old = __sync_val_compare_and_swap(&world, NULL, comm);
      if(old != NULL)
	{
	  nm_group_free(group);
	  free(comm);
	}
    }
  return world;
}

int nm_comm_size(nm_comm_t comm)
{
  return nm_group_size(comm->group);
}

int nm_comm_rank(nm_comm_t comm)
{
  return comm->rank;
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
  if(rank == -1)
    {
      /* not in group -> not in newcomm */
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
	  struct nm_comm_create_header_s
	  {
	    int commit;
	    char session_name[32];
	  } header = { .session_name = { '\0'} };
	  const int root = 0;
	  const nm_tag_t tag = NM_COLL_TAG_COMM_CREATE;
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
      return newcomm;
    }
}
