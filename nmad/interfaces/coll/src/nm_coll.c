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

/* ********************************************************* */

void nm_coll_barrier(nm_comm_t p_comm, nm_tag_t tag)
{
  const int root = 0;
  nm_coll_gather(p_comm, root, NULL, 0, NULL, 0, tag);
  nm_coll_bcast(p_comm, root, NULL, 0, tag);
}

void nm_coll_bcast(nm_comm_t p_comm, int root, void*buffer, nm_len_t len, nm_tag_t tag)
{
  if(p_comm->rank == root)
    {
      const int size = nm_comm_size(p_comm);
      nm_sr_request_t*requests = malloc(size * sizeof(nm_sr_request_t));
      int i;
      for(i = 0; i < size; i++)
	{
	  if(i != root)
	    {
	      nm_sr_isend(nm_comm_get_session(p_comm), nm_comm_get_gate(p_comm, i), tag, buffer, len, &requests[i]);
	    }
	}
      for(i = 0; i < size; i++)
	{
	  if(i != root)
	    {
	      nm_sr_swait(nm_comm_get_session(p_comm), &requests[i]);
	    }
	}
      free(requests);
    }
  else
    {
      nm_gate_t p_root_gate = nm_comm_get_gate(p_comm, root);
      nm_sr_recv(nm_comm_get_session(p_comm), p_root_gate, tag, buffer, len);
    }
}

void nm_coll_scatter(nm_comm_t p_comm, int root, const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag)
{
  if(p_comm->rank == root)
    {
      const int size = nm_comm_size(p_comm);
      nm_sr_request_t*requests = malloc(size * sizeof(nm_sr_request_t));
      int i;
      for(i = 0; i < size; i++)
	{
	  if(i != root)
	    {
	      nm_sr_isend(nm_comm_get_session(p_comm), nm_comm_get_gate(p_comm, i), tag, sbuf + i*slen, slen, &requests[i]);
	    }
	}
      if(slen > 0)
	memcpy(rbuf + root * rlen, sbuf, slen);
      for(i = 0; i < size; i++)
	{
	  if(i != root)
	    {
	      nm_sr_swait(nm_comm_get_session(p_comm), &requests[i]);
	    }
	}
    }
  else
    {
      nm_gate_t p_root_gate = nm_comm_get_gate(p_comm, root);
      nm_sr_recv(nm_comm_get_session(p_comm), p_root_gate, tag, rbuf, rlen);
    }
}

void nm_coll_gather(nm_comm_t p_comm, int root, const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag)
{
  if(p_comm->rank == root)
    {
      const int size = nm_comm_size(p_comm);
      nm_sr_request_t*requests = malloc(size * sizeof(nm_sr_request_t));
      int i;
      for(i = 0; i < size; i++)
	{
	  if(i != root)
	    {
	      nm_sr_irecv(nm_comm_get_session(p_comm), nm_comm_get_gate(p_comm, i), tag, rbuf + i*rlen, rlen, &requests[i]);
	    }
	}
      if(slen > 0)
	memcpy(rbuf + root * rlen, sbuf, slen);
      for(i = 0; i < size; i++)
	{
	  if(i != root)
	    {
	      nm_sr_rwait(nm_comm_get_session(p_comm), &requests[i]);
	    }
	}
      free(requests);
    }
  else
    {
      nm_gate_t p_root_gate = nm_comm_get_gate(p_comm, root);
      nm_sr_send(nm_comm_get_session(p_comm), p_root_gate, tag, sbuf, slen);
    }
}
