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
#include <nm_private.h>
#include <nm_sendrecv_interface.h>
#include <nm_launcher_interface.h>
#include <Padico/Puk.h>
#include "nm_coll_private.h"


/* ** scatter ********************************************** */

void nm_coll_group_data_scatter(nm_session_t p_session, nm_group_t p_group, int root, int self,
                                struct nm_data_s p_sdata[], struct nm_data_s*p_rdata, nm_tag_t tag)
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
              nm_sr_isend_data(p_session, p_gate, tag, &p_sdata[i], &requests[i]);
            }
	  else if(!nm_data_isnull(p_rdata))
            {
              nm_data_copy(p_rdata, &p_sdata[i]);
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
      nm_sr_irecv_data(p_session, p_root_gate, tag, p_rdata, &request);
      nm_sr_rwait(p_session, &request);
    }
}

void nm_coll_group_scatter(nm_session_t p_session, nm_group_t p_group, int root, int self,
			   const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag)
{
  struct nm_data_s*p_send_data = NULL;
  struct nm_data_s recv_data;
  if(rbuf != NULL)
    {
      nm_data_contiguous_build(&recv_data, rbuf, rlen);
    }
  else
    {
      nm_data_null_build(&recv_data);
    }
  if(self == root)
    {
      const int size = nm_group_size(p_group);
      p_send_data = malloc(size * sizeof(struct nm_data_s));
      int i;
      for(i = 0; i < size; i++)
        {
          nm_data_contiguous_build(&p_send_data[i], (void*)sbuf + i * slen, slen);
        }
    }
  nm_coll_group_data_scatter(p_session, p_group, root, self, p_send_data, &recv_data, tag);
  if(p_send_data)
    free(p_send_data);
}

void nm_coll_data_scatter(nm_comm_t p_comm, int root, struct nm_data_s p_sdata[], struct nm_data_s*p_rdata, nm_tag_t tag)
{
  nm_coll_group_data_scatter(nm_comm_get_session(p_comm), nm_comm_group(p_comm),
                             root, nm_comm_rank(p_comm), p_sdata, p_rdata, tag);
}

void nm_coll_scatter(nm_comm_t p_comm, int root, const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag)
{
  nm_coll_group_scatter(nm_comm_get_session(p_comm), nm_comm_group(p_comm),
			root, nm_comm_rank(p_comm), sbuf, slen, rbuf, rlen, tag);
}

/* ** gather *********************************************** */

void nm_coll_group_data_gather(nm_session_t p_session, nm_group_t p_group, int root, int self,
                               struct nm_data_s*p_sdata, struct nm_data_s p_rdata[], nm_tag_t tag)
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
              nm_sr_irecv_data(p_session, p_gate, tag, &p_rdata[i], &requests[i]);
            }
	  else if(!nm_data_isnull(p_sdata))
            {
              nm_data_copy(&p_rdata[i], p_sdata);
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
      nm_sr_request_t request;
      nm_sr_isend_data(p_session, p_root_gate, tag, p_sdata, &request);
      nm_sr_swait(p_session, &request);
    }
}

void nm_coll_group_gather(nm_session_t p_session, nm_group_t p_group, int root, int self,
			  const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag)
{
  struct nm_data_s send_data;
  struct nm_data_s*p_recv_data = NULL;
  if(sbuf != NULL)
    {
      nm_data_contiguous_build(&send_data, (void*)sbuf, slen);
    }
  else
    {
      nm_data_null_build(&send_data);
    }
  if(self == root)
    {
      const int size = nm_group_size(p_group);
      p_recv_data = malloc(size * sizeof(struct nm_data_s));
      int i;
      for(i = 0; i < size; i++)
        {
          nm_data_contiguous_build(&p_recv_data[i], (void*)rbuf + i * rlen, rlen);
        }
    }
  nm_coll_group_data_gather(p_session, p_group, root, self, &send_data, p_recv_data, tag);
  if(p_recv_data)
    free(p_recv_data);
}

void nm_coll_data_gather(nm_comm_t p_comm, int root, struct nm_data_s*p_sdata, struct nm_data_s p_rdata[], nm_tag_t tag)
{
  nm_coll_group_data_gather(nm_comm_get_session(p_comm), nm_comm_group(p_comm),
                            root, nm_comm_rank(p_comm), p_sdata, p_rdata, tag);
}

void nm_coll_gather(nm_comm_t p_comm, int root, const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag)
{
  nm_coll_group_gather(nm_comm_get_session(p_comm), nm_comm_group(p_comm),
                       root, nm_comm_rank(p_comm), sbuf, slen, rbuf, rlen, tag);
}

/* ** barrier ********************************************** */

void nm_coll_group_barrier(nm_session_t p_session, nm_group_t p_group, int self, nm_tag_t tag)
{
  assert(nm_group_get_gate(p_group, self) == nm_launcher_self_gate());
  const int root = 0;
  nm_coll_group_gather(p_session, p_group, root, self, NULL, 0, NULL, 0, tag);
  nm_coll_group_bcast(p_session, p_group, root, self, NULL, 0, tag);
}

void nm_coll_barrier(nm_comm_t p_comm, nm_tag_t tag)
{
  nm_coll_group_barrier(nm_comm_get_session(p_comm), nm_comm_group(p_comm), nm_comm_rank(p_comm), tag);
}

/* ********************************************************* */

