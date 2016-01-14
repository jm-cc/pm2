/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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


#include "nm_mpi_private.h"
#include "nm_log.h"

#include <Padico/Module.h>
PADICO_MODULE_HOOK(MadMPI);

__PUK_SYM_INTERNAL
int nm_mpi_internal_init(void)
{
  int global_size  = -1;
  int process_rank = -1;
  nm_launcher_get_size(&global_size);
  nm_launcher_get_rank(&process_rank);

  /** Set the NewMadeleine interfaces */
  nm_session_t p_session = NULL;
  nm_launcher_get_session(&p_session);
  nm_sr_init(p_session);

  return MPI_SUCCESS;
}

__PUK_SYM_INTERNAL
int nm_mpi_internal_exit(void)
{
  return MPI_SUCCESS;
}

__PUK_SYM_INTERNAL
void nm_mpi_get_tag(nm_mpi_communicator_t*p_comm, int user_tag, nm_tag_t*nm_tag, nm_tag_t*tag_mask)
{
  if(user_tag == MPI_ANY_TAG)
    {
      *nm_tag   = 0;
      *tag_mask = NM_MPI_TAG_PRIVATE_BASE; /* mask out private tags */
    }
  else
    {
      *nm_tag  = user_tag;
      *tag_mask = NM_TAG_MASK_FULL;
    }
}
__PUK_SYM_INTERNAL
int nm_mpi_check_tag(int tag)
{
  if((tag != MPI_ANY_TAG) && (tag < 0 || tag > NM_MPI_TAG_MAX))
    {
      ERROR("invalid tag %d", tag);
    }
  return tag;
}

__PUK_SYM_INTERNAL
int nm_mpi_isend_init(nm_mpi_request_t *p_req, int dest, nm_mpi_communicator_t *p_comm)
{
  int err = MPI_SUCCESS;
  nm_gate_t p_gate = nm_mpi_communicator_get_gate(p_comm, dest);
  if(p_gate == NULL)
    {
      TBX_FAILUREF("Cannot find rank %d in comm %p.\n", dest, p_comm);
      return MPI_ERR_INTERN;
    }
  p_req->gate = p_gate;
  return err;
}

__PUK_SYM_INTERNAL
int nm_mpi_isend_start(nm_mpi_request_t *p_req)
{
  int err = MPI_SUCCESS;
  nm_tag_t nm_tag, tag_mask;
  nm_mpi_get_tag(p_req->p_comm, p_req->user_tag, &nm_tag, &tag_mask);
  nm_mpi_datatype_t*p_datatype = p_req->p_datatype;
  p_datatype->refcount++;
  nm_session_t p_session = nm_mpi_communicator_get_session(p_req->p_comm);
  struct nm_data_s data;
  nm_data_mpi_datatype_set(&data, (struct nm_data_mpi_datatype_s){ .ptr = (void*)p_req->sbuf, .p_datatype = p_req->p_datatype, .count = p_req->count });
  nm_sr_send_init(p_session, &(p_req->request_nmad));
  nm_sr_send_pack_data(p_session, &(p_req->request_nmad), &data);
  switch(p_req->communication_mode)
    {
    case NM_MPI_MODE_IMMEDIATE:
      err = nm_sr_send_isend(p_session, &(p_req->request_nmad), p_req->gate, nm_tag);
      break;
    case NM_MPI_MODE_READY:
      err = nm_sr_send_rsend(p_session, &(p_req->request_nmad), p_req->gate, nm_tag);
      break;
    case NM_MPI_MODE_SYNCHRONOUS:
      err = nm_sr_send_issend(p_session, &(p_req->request_nmad), p_req->gate, nm_tag);
      break;
    default:
      ERROR("madmpi: unkown mode %d for isend", p_req->communication_mode);
      break;
    }
  p_req->request_error = err;
  if(p_req->request_type != NM_MPI_REQUEST_ZERO)
    p_req->request_type = NM_MPI_REQUEST_SEND;
  return err;
}

__PUK_SYM_INTERNAL
int nm_mpi_isend(nm_mpi_request_t *p_req, int dest, nm_mpi_communicator_t *p_comm)
{
  int err;
  err = nm_mpi_isend_init(p_req, dest, p_comm);
  if(err == MPI_SUCCESS)
    {
      err = nm_mpi_isend_start(p_req);
    }
  return err;
}


__PUK_SYM_INTERNAL
int nm_mpi_irecv_init(nm_mpi_request_t *p_req, int source, nm_mpi_communicator_t *p_comm)
{
  if(tbx_unlikely(source == MPI_ANY_SOURCE)) 
    {
      p_req->gate = NM_ANY_GATE;
    }
  else
    {
      p_req->gate = nm_mpi_communicator_get_gate(p_comm, source);
      if(tbx_unlikely(p_req->gate == NULL))
	{
	  TBX_FAILUREF("Cannot find rank %d in comm %p\n", source, p_comm);
	  return MPI_ERR_INTERN;
	}
    }
  p_req->request_source = source;
  p_req->request_type = NM_MPI_REQUEST_RECV;
  return MPI_SUCCESS;
}



__PUK_SYM_INTERNAL
int nm_mpi_irecv_start(nm_mpi_request_t *p_req)
{
  nm_tag_t nm_tag, tag_mask;
  nm_mpi_get_tag(p_req->p_comm, p_req->user_tag, &nm_tag, &tag_mask);
  nm_mpi_datatype_t*p_datatype = p_req->p_datatype;
  p_datatype->refcount++;
  nm_session_t p_session = nm_mpi_communicator_get_session(p_req->p_comm);
  struct nm_data_s data;
  nm_data_mpi_datatype_set(&data, (struct nm_data_mpi_datatype_s){ .ptr = p_req->rbuf, .p_datatype = p_req->p_datatype, .count = p_req->count });
  nm_sr_recv_init(p_session, &(p_req->request_nmad));
  nm_sr_recv_unpack_data(p_session, &(p_req->request_nmad), &data);
  const int err = nm_sr_recv_irecv(p_session, &(p_req->request_nmad), p_req->gate, nm_tag, tag_mask);
  p_req->request_error = err;
  if(p_req->request_type != NM_MPI_REQUEST_ZERO) 
    p_req->request_type = NM_MPI_REQUEST_RECV;
  return p_req->request_error;
}

__PUK_SYM_INTERNAL
int nm_mpi_irecv(nm_mpi_request_t *p_req, int source, nm_mpi_communicator_t *p_comm)
{
  int err;
  err = nm_mpi_irecv_init(p_req, source, p_comm);
  if(err == MPI_SUCCESS)
    {
      err = nm_mpi_irecv_start(p_req);
    }
  return err;
}


