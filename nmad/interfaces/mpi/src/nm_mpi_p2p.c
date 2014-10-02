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

#include "nm_mpi_private.h"
#include <assert.h>
#include <errno.h>

#include <Padico/Module.h>
PADICO_MODULE_HOOK(NewMad_Core);


/* ********************************************************* */

NM_MPI_ALIAS(MPI_Issend,    mpi_issend);
NM_MPI_ALIAS(MPI_Bsend,     mpi_bsend);
NM_MPI_ALIAS(MPI_Send,      mpi_send);
NM_MPI_ALIAS(MPI_Isend,     mpi_isend);
NM_MPI_ALIAS(MPI_Rsend,     mpi_rsend);
NM_MPI_ALIAS(MPI_Ssend,     mpi_ssend);
NM_MPI_ALIAS(MPI_Pack,      mpi_pack);
NM_MPI_ALIAS(MPI_Recv,      mpi_recv);
NM_MPI_ALIAS(MPI_Irecv,     mpi_irecv);
NM_MPI_ALIAS(MPI_Sendrecv,  mpi_sendrecv);
NM_MPI_ALIAS(MPI_Unpack,    mpi_unpack);
NM_MPI_ALIAS(MPI_Iprobe,    mpi_iprobe);
NM_MPI_ALIAS(MPI_Probe,     mpi_probe);
NM_MPI_ALIAS(MPI_Send_init, mpi_send_init);
NM_MPI_ALIAS(MPI_Recv_init, mpi_recv_init);

/* ********************************************************* */

int mpi_issend(void*buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  nm_mpi_request_t *p_req = nm_mpi_request_alloc();
  *request = p_req->id;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  int err;
  if(tbx_unlikely(tag == MPI_ANY_TAG))
    {
      ERROR("Cannot use MPI_ANY_TAG for send.");
      return MPI_ERR_TAG;
    }
  p_req->request_type = NM_MPI_REQUEST_SEND;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  p_req->request_ptr = NULL;
  p_req->contig_buffer = NULL;
  p_req->p_datatype = nm_mpi_datatype_get(datatype);
  p_req->buffer = buffer;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->communication_mode = NM_MPI_MODE_SYNCHRONOUS;
  p_req->p_comm = p_comm;
  err = nm_mpi_isend(p_req, dest, p_comm);
  return err;
}


int mpi_bsend(void *buffer,
	      int count,
	      MPI_Datatype datatype,
	      int dest,
	      int tag,
	      MPI_Comm comm) {
	/* todo: only valid for small messages ? */
	fprintf(stderr, "Warning: bsend called. it may be invalid\n");
	return mpi_send(buffer, count, datatype, dest, tag, comm);
}

int mpi_send(void *buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
  nm_mpi_request_t       *p_req = nm_mpi_request_alloc();
  MPI_Request           request = p_req->id;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  int                   err = 0;
  if(tbx_unlikely(tag == MPI_ANY_TAG))
    {
      ERROR("Cannot use MPI_ANY_TAG for send.");
      return MPI_ERR_TAG;
    }
  p_req->request_type = NM_MPI_REQUEST_SEND;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  p_req->request_ptr = NULL;
  p_req->contig_buffer = NULL;
  p_req->p_datatype = nm_mpi_datatype_get(datatype);
  p_req->buffer = buffer;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->communication_mode = NM_MPI_MODE_IMMEDIATE;
  p_req->p_comm = p_comm;
  err = nm_mpi_isend(p_req, dest, p_comm);
  mpi_wait(&request, MPI_STATUS_IGNORE);
  return err;
}

int mpi_isend(void *buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  nm_mpi_request_t *p_req = nm_mpi_request_alloc();
  *request = p_req->id;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  int err;
  if(tbx_unlikely(tag == MPI_ANY_TAG))
    {
      ERROR("Cannot use MPI_ANY_TAG for send.");
      return MPI_ERR_TAG;
    }
  p_req->request_type = NM_MPI_REQUEST_SEND;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  p_req->request_ptr = NULL;
  p_req->contig_buffer = NULL;
  p_req->p_datatype = nm_mpi_datatype_get(datatype);
  p_req->buffer = buffer;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->communication_mode = NM_MPI_MODE_IMMEDIATE;
  p_req->p_comm = p_comm;
  err = nm_mpi_isend(p_req, dest, p_comm);
  return err;
}

int mpi_rsend(void* buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
  nm_mpi_request_t       *p_req = nm_mpi_request_alloc();
  MPI_Request           request = p_req->id;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  int                   err = 0;
  if(tbx_unlikely(tag == MPI_ANY_TAG))
    {
      ERROR("Cannot use MPI_ANY_TAG for send.");
      return MPI_ERR_TAG;
    }
  p_req->request_type = NM_MPI_REQUEST_SEND;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  p_req->request_ptr = NULL;
  p_req->contig_buffer = NULL;
  p_req->p_datatype = nm_mpi_datatype_get(datatype);
  p_req->buffer = buffer;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->communication_mode = NM_MPI_MODE_READY;
  p_req->p_comm = p_comm;
  err = nm_mpi_isend(p_req, dest, p_comm);
  mpi_wait(&request, MPI_STATUS_IGNORE);
  return err;
}

int mpi_ssend(void* buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
  MPI_Request req;
  int err = mpi_issend(buffer, count, datatype, dest, tag, comm, &req);
  if(err == MPI_SUCCESS)
    {
      err = mpi_wait(&req, MPI_STATUS_IGNORE);
    }
  return err;
}

int mpi_pack(void* inbuf, int incount, MPI_Datatype datatype, void *outbuf, int outsize, int *position, MPI_Comm comm)
{
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  void *ptr = outbuf;
  size_t size_datatype = nm_mpi_datatype_size(p_datatype);
  ptr += *position;
  memcpy(ptr, inbuf, incount*size_datatype);
  *position += incount*size_datatype;
  return MPI_SUCCESS;
}

int mpi_recv(void *buffer, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
  nm_mpi_request_t       *p_req = nm_mpi_request_alloc();
  MPI_Request           request = p_req->id;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  int                   err = 0;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Receiving message from %d of datatype %d with tag %d, count %d\n", source, datatype, tag, count);
  if(tbx_unlikely(tag == MPI_ANY_TAG)) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }
  p_req->request_ptr = NULL;
  p_req->request_type = NM_MPI_REQUEST_RECV;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  p_req->contig_buffer = NULL;
  p_req->p_datatype = nm_mpi_datatype_get(datatype);
  p_req->buffer = buffer;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->p_comm = p_comm;
  err = nm_mpi_irecv(p_req, source, p_comm);
  err = mpi_wait(&request, status);
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_irecv(void *buffer, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
  nm_mpi_request_t *p_req = nm_mpi_request_alloc();
  *request = p_req->id;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Ireceiving message from %d of datatype %d with tag %d\n", source, datatype, tag);
  if(tbx_unlikely(tag == MPI_ANY_TAG))
    {
      ERROR("<Using MPI_ANY_TAG> not implemented yet!");
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }
  p_req->request_ptr = NULL;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  p_req->request_type = NM_MPI_REQUEST_RECV;
  p_req->contig_buffer = NULL;
  p_req->p_datatype = nm_mpi_datatype_get(datatype);
  p_req->buffer = buffer;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->p_comm = p_comm;
  int err = nm_mpi_irecv(p_req, source, p_comm);
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag,
                 MPI_Comm comm, MPI_Status *status)
{
  int err;
  MPI_Request srequest, rrequest;
  MPI_NMAD_LOG_IN();
  err = mpi_isend(sendbuf, sendcount, sendtype, dest, sendtag, comm, &srequest);
  err = mpi_irecv(recvbuf, recvcount, recvtype, source, recvtag, comm, &rrequest);
  err = mpi_wait(&srequest, MPI_STATUS_IGNORE);
  err = mpi_wait(&rrequest, status);
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_unpack(void* inbuf, int insize, int *position, void *outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm)
{
  void *ptr = inbuf;
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  size_t size_datatype = nm_mpi_datatype_size(p_datatype);
  ptr += *position;
  memcpy(outbuf, ptr, outcount*size_datatype);
  *position += outcount*size_datatype;
  return MPI_SUCCESS;
}

int mpi_iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status)
{
  int err      = 0;
  nm_gate_t gate;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  nm_tag_t nm_tag;
  MPI_NMAD_LOG_IN();
  if(tag == MPI_ANY_TAG)
    {
      ERROR("<Using MPI_ANY_TAG> not implemented yet!");
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }
  if(source == MPI_ANY_SOURCE) 
    {
      gate = NM_ANY_GATE;
    }
  else
    {
      gate = nm_mpi_communicator_get_gate(p_comm, source);
      if(gate == NULL)
	{
	  fprintf(stderr, "Cannot find a in connection for source %d\n", source);
	  MPI_NMAD_LOG_OUT();
	  return MPI_ERR_INTERN;
	}
    }
  nm_tag = nm_mpi_get_tag(p_comm, tag);
  nm_gate_t out_gate = NULL;
  err = nm_sr_probe(nm_comm_get_session(p_comm->p_comm), gate, &out_gate, nm_tag, NM_TAG_MASK_FULL, NULL, NULL);;
  if(err == NM_ESUCCESS) 
    {
      *flag = 1;
      if(status != NULL)
	{
	  status->MPI_TAG = tag;
	  status->MPI_SOURCE = nm_mpi_communicator_get_dest(p_comm, out_gate);
	}
    }
  else
    { /* err == -NM_EAGAIN */
      *flag = 0;
    }
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_probe(int source, int tag, MPI_Comm comm, MPI_Status *status)
{
  int flag = 0;
  MPI_NMAD_LOG_IN();
  int err = mpi_iprobe(source, tag, comm, &flag, status);
  while(flag != 1)
    {
      err = mpi_iprobe(source, tag, comm, &flag, status);
    }
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_send_init(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  nm_mpi_request_t *p_req = nm_mpi_request_alloc();
  *request = p_req->id;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Init Isending message to %d of datatype %d with tag %d\n", dest, datatype, tag);
  if(tbx_unlikely(tag == MPI_ANY_TAG))
    {
      ERROR("<Using MPI_ANY_TAG> not implemented yet!");
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }
  p_req->request_type = NM_MPI_REQUEST_SEND;
  p_req->request_persistent_type = NM_MPI_REQUEST_SEND;
  p_req->request_ptr = NULL;
  p_req->contig_buffer = NULL;
  p_req->p_datatype = nm_mpi_datatype_get(datatype);
  p_req->buffer = buf;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->communication_mode = NM_MPI_MODE_IMMEDIATE;
  p_req->p_comm = p_comm;
  int err = nm_mpi_isend_init(p_req, dest, p_comm);
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_recv_init(void* buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
  nm_mpi_request_t *p_req = nm_mpi_request_alloc();
  *request = p_req->id;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Init Irecv message from %d of datatype %d with tag %d\n", source, datatype, tag);

  if(tbx_unlikely(tag == MPI_ANY_TAG))
    {
      ERROR("<Using MPI_ANY_TAG> not implemented yet!");
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }
  p_req->request_ptr = NULL;
  p_req->request_type = NM_MPI_REQUEST_RECV;
  p_req->request_persistent_type = NM_MPI_REQUEST_RECV;
  p_req->contig_buffer = NULL;
  p_req->p_datatype = nm_mpi_datatype_get(datatype);
  p_req->buffer = buf;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->p_comm = p_comm;
  int err = nm_mpi_irecv_init(p_req, source, p_comm);
  MPI_NMAD_LOG_OUT();
  return err;
}

