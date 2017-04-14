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

nm_comm_t nm_comm_world(const char*label)
{
  nm_group_t group = nm_gate_vect_new();
  int size = -1;
  nm_launcher_get_size(&size);
  int i;
  for(i = 0; i < size; i++)
    {
      nm_gate_t p_gate = NULL;
      nm_launcher_get_gate(i, &p_gate);
      assert(p_gate != NULL);
      nm_gate_vect_push_back(group, p_gate);
    }
  nm_comm_t p_comm = malloc(sizeof(struct nm_comm_s));
  p_comm->group = group;
  p_comm->rank = nm_group_rank(group);
  padico_string_t s_name = padico_string_new();
  padico_string_catf(s_name, "nm_comm_world/%s", label);
  nm_session_open(&p_comm->p_session, padico_string_get(s_name));
  padico_string_delete(s_name);
  nm_comm_commit(p_comm);
  return p_comm;
}

nm_comm_t nm_comm_self(const char*label)
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
  padico_string_t s_name = padico_string_new();
  padico_string_catf(s_name, "nm_comm_self/%s", label);
  nm_session_open(&p_comm->p_session, padico_string_get(s_name));
  nm_comm_commit(p_comm);
  padico_string_delete(s_name);
  return p_comm;
}


nm_comm_t nm_comm_dup(nm_comm_t p_comm)
{
  nm_group_t group = nm_comm_group(p_comm);
  nm_comm_t p_newcomm = nm_comm_create(p_comm, group);
  return p_newcomm;
}

nm_comm_t nm_comm_create_group(nm_comm_t p_comm, nm_group_t p_newcomm_group, nm_group_t p_bcast_group)
{
  nm_session_t p_session = nm_comm_get_session(p_comm);
  nm_gate_t p_self_gate  = nm_comm_gate_self(p_comm);
  nm_gate_t p_root_gate  = nm_group_get_gate(p_bcast_group, 0);
  const int newrank      = nm_group_rank(p_newcomm_group);
  struct nm_comm_create_header_s
  {
    int commit;
    char session_name[64];
  } header = { .session_name = { '\0'}, .commit = 0 };
  assert(p_root_gate != NULL);
  const nm_tag_t tag1 = NM_COLL_TAG_COMM_CREATE_1;
  const nm_tag_t tag2 = NM_COLL_TAG_COMM_CREATE_2;

  nm_session_t p_new_session = NULL;
  while(!header.commit)
    {
      if(p_self_gate == p_root_gate)
	{
	  snprintf(&header.session_name[0], 64, "nm_comm-%p%08x", p_newcomm_group, (unsigned)random());
	  nm_session_open(&p_new_session, header.session_name);
	  if(p_new_session != NULL)
	    {
	      int i;
	      header.commit = 0;
	      nm_coll_group_bcast(p_session, p_bcast_group, p_root_gate, p_self_gate, &header, sizeof(header), tag1);
	      int*acks = malloc(sizeof(int) * nm_group_size(p_bcast_group));
	      int ack = 1;
	      nm_coll_group_gather(p_session, p_bcast_group, p_root_gate, p_self_gate, &ack, sizeof(ack), acks, sizeof(ack), tag2);
	      int total_acks = 0;
	      for(i = 0; i < nm_group_size(p_bcast_group); i++)
		{
		  total_acks += acks[i];
		}
	      free(acks);
	      if(total_acks == nm_group_size(p_bcast_group))
		{
		  header.commit = 1;
		  nm_coll_group_bcast(p_session, p_bcast_group, p_root_gate, p_self_gate, &header, sizeof(header), tag1);
		}
	    }
	}
      else
	{
	  nm_coll_group_bcast(p_session, p_bcast_group, p_root_gate, p_self_gate, &header, sizeof(header), tag1);
	  if(header.commit == 0)
	    {
	      if(p_new_session != NULL)
		{
		  /* release previous round failed session creation */
		  nm_session_destroy(p_new_session);
		}
	      nm_session_open(&p_new_session, header.session_name);
	      int ack = (p_new_session != NULL);
	      nm_coll_group_gather(p_session, p_bcast_group, p_root_gate, p_self_gate, &ack, sizeof(ack), NULL, 0, tag2);
	    }
	}
    }
  if(newrank != -1)
    {
      nm_comm_t p_newcomm  = malloc(sizeof(struct nm_comm_s));
      p_newcomm->rank      = newrank;
      p_newcomm->group     = nm_group_dup(p_newcomm_group);
      p_newcomm->p_session = NULL;
      p_newcomm->reverse   = NULL;
      p_newcomm->p_session = p_new_session;
      nm_comm_commit(p_newcomm);
      return p_newcomm;
    }
  else
    {
      return NULL;
    }
}

nm_comm_t nm_comm_create(nm_comm_t p_comm, nm_group_t group)
{
  return nm_comm_create_group(p_comm, group, nm_comm_group(p_comm));
}

void nm_comm_destroy(nm_comm_t p_comm)
{
  if(p_comm != NULL)
    {
      nm_session_destroy(p_comm->p_session);
      nm_group_free(p_comm->group);
      if(p_comm->reverse)
	puk_hashtable_delete(p_comm->reverse);
      free(p_comm);
    }
}
