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
#include <nm_launcher_interface.h>
#include <Padico/Puk.h>
#include "nm_coll_private.h"

/* ********************************************************* */

void nm_coll_group_data_bcast(nm_session_t p_session, nm_group_t p_group, int root, int self,
                              struct nm_data_s*p_data, nm_tag_t tag)
{
  assert(nm_group_get_gate(p_group, self) == nm_launcher_self_gate());
  if(self == root)
    {
      const int size = nm_group_size(p_group);
      nm_sr_request_t*requests = malloc(size * sizeof(nm_sr_request_t));
      int i;
      for(i = 0; i < size; i++)
	{
	  if(i != self)
            {
              nm_gate_t p_gate = nm_group_get_gate(p_group, i);
              nm_sr_isend_data(p_session, p_gate, tag, p_data, &requests[i]);
            }
	}
      for(i = 0; i < size; i++)
	{
	  if(i != self)
            {
              nm_sr_swait(p_session, &requests[i]);
            }
	}
      free(requests);
    }
  else
    {
      nm_gate_t p_root_gate = nm_group_get_gate(p_group, root);
      nm_sr_request_t request;
      nm_sr_irecv_data(p_session, p_root_gate, tag, p_data, &request);
      nm_sr_rwait(p_session, &request);
    }
}

/* ********************************************************* */

void nm_coll_group_barrier(nm_session_t p_session, nm_group_t p_group, int self, nm_tag_t tag)
{
  assert(nm_group_get_gate(p_group, self) == nm_launcher_self_gate());
  const int root = 0;
  nm_coll_group_gather(p_session, p_group, root, self, NULL, 0, NULL, 0, tag);
  nm_coll_group_bcast(p_session, p_group, root, self, NULL, 0, tag);
}

void nm_coll_group_bcast(nm_session_t p_session, nm_group_t p_group, int root, int self,
			 void*buffer, nm_len_t len, nm_tag_t tag)
{
  assert(nm_group_get_gate(p_group, self) == nm_launcher_self_gate());
  struct nm_data_s data;
  nm_data_contiguous_build(&data, (void*)buffer, len);
  nm_coll_group_data_bcast(p_session, p_group, root, self, &data, tag);
}

void nm_coll_group_gather(nm_session_t p_session, nm_group_t p_group, int root, int self,
			  const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag)
{
  assert(nm_group_get_gate(p_group, self) == nm_launcher_self_gate());
  if(self == root)
    {
      const int size = nm_group_size(p_group);
      nm_sr_request_t*requests = malloc(size * sizeof(nm_sr_request_t));
      int i;
      for(i = 0; i < size; i++)
	{
	  if(i != self)
            {
              nm_gate_t p_gate = nm_group_get_gate(p_group, i);
              nm_sr_irecv(p_session, p_gate, tag, rbuf + i * rlen, rlen, &requests[i]);
            }
	  else if(slen > 0)
            {
              memcpy(rbuf + i * rlen, sbuf, slen);
            }
	}
      for(i = 0; i < size; i++)
	{
	  if(i != self)
            {
              nm_sr_rwait(p_session, &requests[i]);
            }
	}
      free(requests);
    }
  else
    {
      nm_gate_t p_root_gate = nm_group_get_gate(p_group, root);
      nm_sr_send(p_session, p_root_gate, tag, sbuf, slen);
    }
}

void nm_coll_group_scatter(nm_session_t p_session, nm_group_t p_group, int root, int self,
			   const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag)
{
  assert(nm_group_get_gate(p_group, self) == nm_launcher_self_gate());
  if(self == root)
    {
      const int size = nm_group_size(p_group);
      nm_sr_request_t*requests = malloc(size * sizeof(nm_sr_request_t));
      int i;
      for(i = 0; i < size; i++)
	{
	  if(i != self)
            {
              nm_gate_t p_gate = nm_group_get_gate(p_group, i);
              nm_sr_isend(p_session, p_gate, tag, sbuf + i * slen, slen, &requests[i]);
            }
	  else if(slen > 0)
            {
              memcpy(rbuf + i * rlen, sbuf, slen);
            }
	}
      for(i = 0; i < size; i++)
	{
	  if(i != self)
            {
              nm_sr_swait(p_session, &requests[i]);
            }
	}
    }
  else
    {
      nm_gate_t p_root_gate = nm_group_get_gate(p_group, root);
      nm_sr_recv(p_session, p_root_gate, tag, rbuf, rlen);
    }
}


/* ********************************************************* */

void nm_coll_barrier(nm_comm_t p_comm, nm_tag_t tag)
{
  nm_coll_group_barrier(nm_comm_get_session(p_comm), nm_comm_group(p_comm), nm_comm_rank(p_comm), tag);
}

void nm_coll_bcast(nm_comm_t p_comm, int root, void*buffer, nm_len_t len, nm_tag_t tag)
{
  nm_coll_group_bcast(nm_comm_get_session(p_comm), nm_comm_group(p_comm),
                      root, nm_comm_rank(p_comm), buffer, len, tag);
}

void nm_coll_scatter(nm_comm_t p_comm, int root, const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag)
{
  nm_coll_group_scatter(nm_comm_get_session(p_comm), nm_comm_group(p_comm),
			root, nm_comm_rank(p_comm), sbuf, slen, rbuf, rlen, tag);
}

void nm_coll_gather(nm_comm_t p_comm, int root, const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag)
{
  nm_coll_group_gather(nm_comm_get_session(p_comm), nm_comm_group(p_comm),
                       root, nm_comm_rank(p_comm), sbuf, slen, rbuf, rlen, tag);
}

/* ********************************************************* */

void nm_coll_data_bcast(nm_comm_t p_comm, int root, struct nm_data_s*p_data, nm_tag_t tag)
{
  nm_coll_group_data_bcast(nm_comm_get_session(p_comm), nm_comm_group(p_comm),
                           root, nm_comm_rank(p_comm), p_data, tag);
}
