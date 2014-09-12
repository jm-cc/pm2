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

int MPI_Issend(void* buf,
	       int count,
	       MPI_Datatype datatype,
	       int dest,
               int tag,
	       MPI_Comm comm,
	       MPI_Request *request) __attribute__ ((alias ("mpi_issend")));

int MPI_Bsend(void *buffer,
	      int count,
	      MPI_Datatype datatype,
	      int dest,
	      int tag,
	      MPI_Comm comm) __attribute__ ((alias ("mpi_bsend")));

int MPI_Send(void *buffer,
             int count,
             MPI_Datatype datatype,
             int dest,
             int tag,
             MPI_Comm comm) __attribute__ ((alias ("mpi_send")));

int MPI_Isend(void *buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm,
              MPI_Request *request) __attribute__ ((alias ("mpi_isend")));

int MPI_Rsend(void* buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm) __attribute__ ((alias ("mpi_rsend")));

int MPI_Ssend(void* buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm) __attribute__ ((alias ("mpi_ssend")));

int MPI_Pack(void* inbuf,
             int incount,
             MPI_Datatype datatype,
             void *outbuf,
             int outsize,
             int *position,
             MPI_Comm comm) __attribute__ ((alias ("mpi_pack")));

int MPI_Recv(void *buffer,
             int count,
             MPI_Datatype datatype,
             int source,
             int tag,
             MPI_Comm comm,
             MPI_Status *status) __attribute__ ((alias ("mpi_recv")));

int MPI_Irecv(void *buffer,
              int count,
              MPI_Datatype datatype,
              int source,
              int tag,
              MPI_Comm comm,
              MPI_Request *request) __attribute__ ((alias ("mpi_irecv")));

int MPI_Sendrecv(void *sendbuf,
                 int sendcount,
                 MPI_Datatype sendtype,
                 int dest,
                 int sendtag,
                 void *recvbuf,
                 int recvcount,
                 MPI_Datatype recvtype,
                 int source,
                 int recvtag,
                 MPI_Comm comm,
                 MPI_Status *status) __attribute__ ((alias ("mpi_sendrecv")));

int MPI_Unpack(void* inbuf,
               int insize,
               int *position,
               void *outbuf,
               int outcount,
               MPI_Datatype datatype,
               MPI_Comm comm) __attribute__ ((alias ("mpi_unpack")));

int MPI_Iprobe(int source,
               int tag,
               MPI_Comm comm,
               int *flag,
               MPI_Status *status) __attribute__ ((alias ("mpi_iprobe")));

int MPI_Probe(int source,
              int tag,
              MPI_Comm comm,
              MPI_Status *status) __attribute__ ((alias ("mpi_probe")));

int MPI_Send_init(void* buf,
                  int count,
                  MPI_Datatype datatype,
                  int dest,
                  int tag,
                  MPI_Comm comm,
                  MPI_Request *request) __attribute__ ((alias ("mpi_send_init")));

int MPI_Recv_init(void* buf,
                  int count,
                  MPI_Datatype datatype,
                  int source,
                  int tag,
                  MPI_Comm comm,
                  MPI_Request *request) __attribute__ ((alias ("mpi_recv_init")));

/* ********************************************************* */

int mpi_issend(void* buf,
	       int count,
	       MPI_Datatype datatype,
	       int dest,
               int tag,
	       MPI_Comm comm,
	       MPI_Request *request) {
#warning Implement MPI_Issend !
  return MPI_ERR_UNKNOWN;
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
  MPI_Request           request = p_req->request_id;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  int                   err = 0;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Sending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }

  p_req->request_type = MPI_REQUEST_SEND;
  p_req->request_persistent_type = MPI_REQUEST_ZERO;
  p_req->request_ptr = NULL;
  p_req->contig_buffer = NULL;
  p_req->request_datatype = datatype;
  p_req->buffer = buffer;
  p_req->count = count;
  p_req->user_tag = tag;
  p_req->communication_mode = MPI_IMMEDIATE_MODE;
  p_req->p_comm = p_comm;

  err = mpir_isend(p_req, dest, p_comm);

  mpi_wait(&request, MPI_STATUS_IGNORE);

  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_isend(void *buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  nm_mpi_request_t *p_req = nm_mpi_request_alloc();
  *request = p_req->request_id;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  int err;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Isending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }
  p_req->request_type = MPI_REQUEST_SEND;
  p_req->request_persistent_type = MPI_REQUEST_ZERO;
  p_req->request_ptr = NULL;
  p_req->contig_buffer = NULL;
  p_req->request_datatype = datatype;
  p_req->buffer = buffer;
  p_req->count = count;
  p_req->user_tag = tag;
  p_req->communication_mode = MPI_IMMEDIATE_MODE;
  p_req->p_comm = p_comm;

  err = mpir_isend(p_req, dest, p_comm);

  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_rsend(void* buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
  nm_mpi_request_t       *p_req = nm_mpi_request_alloc();
  MPI_Request           request = p_req->request_id;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  int                   err = 0;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Rsending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }
  p_req->request_type = MPI_REQUEST_SEND;
  p_req->request_persistent_type = MPI_REQUEST_ZERO;
  p_req->request_ptr = NULL;
  p_req->contig_buffer = NULL;
  p_req->request_datatype = datatype;
  p_req->buffer = buffer;
  p_req->count = count;
  p_req->user_tag = tag;
  p_req->communication_mode = MPI_READY_MODE;
  p_req->p_comm = p_comm;

  err = mpir_isend(p_req, dest, p_comm);

  mpi_wait(&request, MPI_STATUS_IGNORE);

  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_ssend(void* buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
  nm_mpi_request_t       *p_req = nm_mpi_request_alloc();
  nm_mpi_communicator_t  *p_comm = nm_mpi_communicator_get(comm);
  int                   err = 0;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Ssending message to %d of datatype %d with tag %d\n", dest, datatype, tag);

  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }
  p_req->request_type = MPI_REQUEST_SEND;
  p_req->request_persistent_type = MPI_REQUEST_ZERO;
  p_req->request_ptr = NULL;
  p_req->contig_buffer = NULL;
  p_req->request_datatype = datatype;
  p_req->buffer = buffer;
  p_req->count = count;
  p_req->user_tag = tag;
  p_req->communication_mode = MPI_SYNCHRONOUS_MODE;
  p_req->p_comm = p_comm;

  err = mpir_isend(p_req, dest, p_comm);

  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_pack(void* inbuf, int incount, MPI_Datatype datatype, void *outbuf, int outsize, int *position, MPI_Comm comm)
{
  size_t size_datatype;
  void *ptr = outbuf;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Packing %d data of datatype %d at position %d\n", incount, datatype, *position);
  size_datatype = mpir_sizeof_datatype(datatype);
  ptr += *position;
  memcpy(ptr, inbuf, incount*size_datatype);
  *position += incount*size_datatype;
  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int mpi_recv(void *buffer, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
  nm_mpi_request_t       *p_req = nm_mpi_request_alloc();
  MPI_Request           request = p_req->request_id;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  int                   err = 0;

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Receiving message from %d of datatype %d with tag %d, count %d\n", source, datatype, tag, count);
  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }
  p_req->request_ptr = NULL;
  p_req->request_persistent_type = MPI_REQUEST_ZERO;
  p_req->request_type = MPI_REQUEST_RECV;
  p_req->contig_buffer = NULL;
  p_req->request_datatype = datatype;
  p_req->buffer = buffer;
  p_req->count = count;
  p_req->user_tag = tag;
  p_req->p_comm = p_comm;
  err = mpir_irecv(p_req, source, p_comm);
  err = mpi_wait(&request, status);
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_irecv(void *buffer, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
  nm_mpi_request_t *p_req = nm_mpi_request_alloc();
  *request = p_req->request_id;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);

  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Ireceiving message from %d of datatype %d with tag %d\n", source, datatype, tag);
  if (tbx_unlikely(tag == MPI_ANY_TAG))
    {
      ERROR("<Using MPI_ANY_TAG> not implemented yet!");
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }
  p_req->request_ptr = NULL;
  p_req->request_persistent_type = MPI_REQUEST_ZERO;
  p_req->request_type = MPI_REQUEST_RECV;
  p_req->contig_buffer = NULL;
  p_req->request_datatype = datatype;
  p_req->buffer = buffer;
  p_req->count = count;
  p_req->user_tag = tag;
  p_req->p_comm = p_comm;
  int err = mpir_irecv(p_req, source, p_comm);
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
  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Unpacking %d data of datatype %d at position %d\n", outcount, datatype, *position);
  size_t size_datatype = mpir_sizeof_datatype(datatype);
  ptr += *position;
  memcpy(outbuf, ptr, outcount*size_datatype);
  *position += outcount*size_datatype;
  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int mpi_iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status)
{
  int err      = 0;
  nm_gate_t gate;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  nm_tag_t nm_tag;
  MPI_NMAD_LOG_IN();
  if (tag == MPI_ANY_TAG)
    {
      ERROR("<Using MPI_ANY_TAG> not implemented yet!");
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }
  if (source == MPI_ANY_SOURCE) 
    {
      gate = NM_ANY_GATE;
    }
  else
    {
      gate = nm_mpi_communicator_get_gate(p_comm, source);
      if (source >= p_comm->size || gate == NM_ANY_GATE)
	{
	  fprintf(stderr, "Cannot find a in connection between %d and %d\n", p_comm->rank, source);
	  MPI_NMAD_LOG_OUT();
	  return MPI_ERR_INTERN;
	}
    }
  nm_tag = mpir_comm_and_tag(p_comm, tag);
  nm_gate_t out_gate = NULL;
  err = nm_sr_probe(nm_mpi_internal_data.p_session, gate, &out_gate, nm_tag, NM_TAG_MASK_FULL, NULL, NULL);;
  if (err == NM_ESUCCESS) 
    {
      *flag = 1;
      if (status != NULL)
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
  *request = p_req->request_id;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Init Isending message to %d of datatype %d with tag %d\n", dest, datatype, tag);
  if (tbx_unlikely(tag == MPI_ANY_TAG))
    {
      ERROR("<Using MPI_ANY_TAG> not implemented yet!");
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }
  p_req->request_type = MPI_REQUEST_SEND;
  p_req->request_persistent_type = MPI_REQUEST_SEND;
  p_req->request_ptr = NULL;
  p_req->contig_buffer = NULL;
  p_req->request_datatype = datatype;
  p_req->buffer = buf;
  p_req->count = count;
  p_req->user_tag = tag;
  p_req->communication_mode = MPI_IMMEDIATE_MODE;
  p_req->p_comm = p_comm;
  int err = mpir_isend_init(p_req, dest, p_comm);
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_recv_init(void* buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
  nm_mpi_request_t *p_req = nm_mpi_request_alloc();
  *request = p_req->request_id;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  MPI_NMAD_LOG_IN();
  MPI_NMAD_TRACE("Init Irecv message from %d of datatype %d with tag %d\n", source, datatype, tag);

  if (tbx_unlikely(tag == MPI_ANY_TAG)) {
    ERROR("<Using MPI_ANY_TAG> not implemented yet!");
    MPI_NMAD_LOG_OUT();
    return MPI_ERR_INTERN;
  }
  p_req->request_ptr = NULL;
  p_req->request_type = MPI_REQUEST_RECV;
  p_req->request_persistent_type = MPI_REQUEST_RECV;
  p_req->contig_buffer = NULL;
  p_req->request_datatype = datatype;
  p_req->buffer = buf;
  p_req->count = count;
  p_req->user_tag = tag;
  p_req->p_comm = p_comm;
  int err = mpir_irecv_init(p_req, source, p_comm);
  MPI_NMAD_LOG_OUT();
  return err;
}

