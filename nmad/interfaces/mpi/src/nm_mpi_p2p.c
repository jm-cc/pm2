/*
 * NewMadeleine
 * Copyright (C) 2014-2016 (see AUTHORS file)
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
#include <tbx.h>

#include <Padico/Module.h>
PADICO_MODULE_HOOK(MadMPI);


/* ********************************************************* */

NM_MPI_ALIAS(MPI_Issend,    mpi_issend);
NM_MPI_ALIAS(MPI_Bsend,     mpi_bsend);
NM_MPI_ALIAS(MPI_Send,      mpi_send);
NM_MPI_ALIAS(MPI_Isend,     mpi_isend);
NM_MPI_ALIAS(MPI_Rsend,     mpi_rsend);
NM_MPI_ALIAS(MPI_Ssend,     mpi_ssend);
NM_MPI_ALIAS(MPI_Recv,      mpi_recv);
NM_MPI_ALIAS(MPI_Irecv,     mpi_irecv);
NM_MPI_ALIAS(MPI_Sendrecv,  mpi_sendrecv);
NM_MPI_ALIAS(MPI_Sendrecv_replace, mpi_sendrecv_replace);
NM_MPI_ALIAS(MPI_Iprobe,           mpi_iprobe);
NM_MPI_ALIAS(MPI_Probe,            mpi_probe);
NM_MPI_ALIAS(MPI_Send_init,        mpi_send_init);
NM_MPI_ALIAS(MPI_Rsend_init,       mpi_rsend_init);
NM_MPI_ALIAS(MPI_Ssend_init,       mpi_ssend_init);
NM_MPI_ALIAS(MPI_Recv_init,        mpi_recv_init);

/* ********************************************************* */

/** get the nmad tag for the given user tag and communicator. */
static inline void nm_mpi_get_tag(nm_mpi_communicator_t*p_comm, int user_tag, nm_tag_t*nm_tag, nm_tag_t*tag_mask)
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

/** Checks whether the given tag is in the permitted bounds. */
static inline int nm_mpi_check_tag(int tag)
{
  if((tag != MPI_ANY_TAG) && (tag < 0 || tag > NM_MPI_TAG_MAX))
    {
      NM_MPI_FATAL_ERROR("invalid tag %d", tag);
    }
  return tag;
}

__PUK_SYM_INTERNAL
int nm_mpi_isend_init(nm_mpi_request_t *p_req, int dest, nm_mpi_communicator_t *p_comm)
{
  int err = MPI_SUCCESS;
  nm_gate_t p_gate = nm_mpi_communicator_get_gate(p_comm, dest);
  nm_mpi_datatype_ref_inc(p_req->p_datatype);
  if(p_gate == NULL)
    {
      NM_MPI_FATAL_ERROR("Cannot find rank %d in comm %p.\n", dest, p_comm);
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
  nm_session_t p_session = nm_mpi_communicator_get_session(p_req->p_comm);
  if(!p_req->p_datatype->committed)
    {
      NM_MPI_WARNING("trying to send with a non-committed datatype.\n");
      return MPI_ERR_TYPE;
    }
  struct nm_data_s data;
  nm_mpi_data_build(&data, (void*)p_req->sbuf, p_req->p_datatype, p_req->count);
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
      NM_MPI_FATAL_ERROR("madmpi: unkown mode %d for isend", p_req->communication_mode);
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
	  NM_MPI_FATAL_ERROR("Cannot find rank %d in comm %p\n", source, p_comm);
	  return MPI_ERR_INTERN;
	}
    }
  p_req->request_source = source;
  p_req->request_type = NM_MPI_REQUEST_RECV;
  nm_mpi_datatype_ref_inc(p_req->p_datatype);
  return MPI_SUCCESS;
}



__PUK_SYM_INTERNAL
int nm_mpi_irecv_start(nm_mpi_request_t *p_req)
{
  nm_tag_t nm_tag, tag_mask;
  nm_mpi_get_tag(p_req->p_comm, p_req->user_tag, &nm_tag, &tag_mask);
  nm_session_t p_session = nm_mpi_communicator_get_session(p_req->p_comm);
  if(!p_req->p_datatype->committed)
    {
      NM_MPI_WARNING("trying to recv with a non-committed datatype.\n");
      return MPI_ERR_TYPE;
    }
  struct nm_data_s data;
  nm_mpi_data_build(&data, p_req->rbuf, p_req->p_datatype, p_req->count);
  nm_sr_recv_init(p_session, &(p_req->request_nmad));
  nm_sr_recv_unpack_data(p_session, &(p_req->request_nmad), &data);
  nm_sr_recv_irecv(p_session, &(p_req->request_nmad), p_req->gate, nm_tag, tag_mask);
  p_req->request_error = NM_ESUCCESS;
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

/* ********************************************************* */

int mpi_issend(const void*buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  nm_mpi_request_t *p_req = nm_mpi_request_alloc();
  *request = p_req->id;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  int err;
  if(tbx_unlikely(tag == MPI_ANY_TAG))
    {
      NM_MPI_FATAL_ERROR("Cannot use MPI_ANY_TAG for send.");
      return MPI_ERR_TAG;
    }
  p_req->request_type = NM_MPI_REQUEST_SEND;
  p_req->p_datatype = nm_mpi_datatype_get(datatype);
  p_req->sbuf = buffer;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->communication_mode = NM_MPI_MODE_SYNCHRONOUS;
  p_req->p_comm = p_comm;
  err = nm_mpi_isend(p_req, dest, p_comm);
  return err;
}


int mpi_bsend(const void *buffer,
	      int count,
	      MPI_Datatype datatype,
	      int dest,
	      int tag,
	      MPI_Comm comm) {
  /* todo: only valid for small messages ? */
  NM_MPI_WARNING("Warning: MPI_Bsend called. it may be invalid\n");
  return mpi_send(buffer, count, datatype, dest, tag, comm);
}

int mpi_send(const void *buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
  nm_mpi_request_t       *p_req = nm_mpi_request_alloc();
  MPI_Request           request = p_req->id;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  int                   err = 0;
  if(tbx_unlikely(tag == MPI_ANY_TAG))
    {
      NM_MPI_FATAL_ERROR("Cannot use MPI_ANY_TAG for send.");
      return MPI_ERR_TAG;
    }
  p_req->request_type = NM_MPI_REQUEST_SEND;
  p_req->p_datatype = nm_mpi_datatype_get(datatype);
  p_req->sbuf = buffer;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->communication_mode = NM_MPI_MODE_IMMEDIATE;
  p_req->p_comm = p_comm;
  err = nm_mpi_isend(p_req, dest, p_comm);
  mpi_wait(&request, MPI_STATUS_IGNORE);
  return err;
}

int mpi_isend(const void *buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  nm_mpi_request_t *p_req = nm_mpi_request_alloc();
  *request = p_req->id;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  int err;
  if(tbx_unlikely(tag == MPI_ANY_TAG))
    {
      NM_MPI_FATAL_ERROR("Cannot use MPI_ANY_TAG for send.");
      return MPI_ERR_TAG;
    }
  p_req->request_type = NM_MPI_REQUEST_SEND;
  p_req->p_datatype = nm_mpi_datatype_get(datatype);
  p_req->sbuf = buffer;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->communication_mode = NM_MPI_MODE_IMMEDIATE;
  p_req->p_comm = p_comm;
  err = nm_mpi_isend(p_req, dest, p_comm);
  return err;
}

int mpi_rsend(const void* buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
  nm_mpi_request_t       *p_req = nm_mpi_request_alloc();
  MPI_Request           request = p_req->id;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  int                   err = 0;
  if(tbx_unlikely(tag == MPI_ANY_TAG))
    {
      NM_MPI_FATAL_ERROR("Cannot use MPI_ANY_TAG for send.");
      return MPI_ERR_TAG;
    }
  p_req->request_type = NM_MPI_REQUEST_SEND;
  p_req->p_datatype = nm_mpi_datatype_get(datatype);
  p_req->sbuf = buffer;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->communication_mode = NM_MPI_MODE_READY;
  p_req->p_comm = p_comm;
  err = nm_mpi_isend(p_req, dest, p_comm);
  mpi_wait(&request, MPI_STATUS_IGNORE);
  return err;
}

int mpi_ssend(const void* buffer, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
  MPI_Request req;
  int err = mpi_issend(buffer, count, datatype, dest, tag, comm, &req);
  if(err == MPI_SUCCESS)
    {
      err = mpi_wait(&req, MPI_STATUS_IGNORE);
    }
  return err;
}


int mpi_recv(void *buffer, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
  nm_mpi_request_t       *p_req = nm_mpi_request_alloc();
  MPI_Request           request = p_req->id;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  int                   err = 0;
  p_req->request_type = NM_MPI_REQUEST_RECV;
  p_req->p_datatype = nm_mpi_datatype_get(datatype);
  p_req->rbuf = buffer;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->p_comm = p_comm;
  err = nm_mpi_irecv(p_req, source, p_comm);
  err = mpi_wait(&request, status);
  return err;
}

int mpi_irecv(void *buffer, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
  nm_mpi_request_t *p_req = nm_mpi_request_alloc();
  *request = p_req->id;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  p_req->request_type = NM_MPI_REQUEST_RECV;
  p_req->p_datatype = nm_mpi_datatype_get(datatype);
  p_req->rbuf = buffer;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->p_comm = p_comm;
  int err = nm_mpi_irecv(p_req, source, p_comm);
  return err;
}

int mpi_sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag,
                 MPI_Comm comm, MPI_Status *status)
{
  int err;
  MPI_Request srequest, rrequest;
  err = mpi_isend(sendbuf, sendcount, sendtype, dest, sendtag, comm, &srequest);
  err = mpi_irecv(recvbuf, recvcount, recvtype, source, recvtag, comm, &rrequest);
  err = mpi_wait(&srequest, MPI_STATUS_IGNORE);
  err = mpi_wait(&rrequest, status);
  return err;
}

int mpi_sendrecv_replace(void*buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag,
			 MPI_Comm comm, MPI_Status*status)
{
  int err;
  MPI_Request srequest;
  err = mpi_isend(buf, count, datatype, dest, sendtag, comm, &srequest);
  int flag = 0;
  err = mpi_test(&srequest, &flag, MPI_STATUS_IGNORE);
  if(flag)
    {
      /* send completed immediately- reuse buffer */
      err = mpi_recv(buf, count, datatype, source, recvtag, comm, status);
    }
  else
    {
      /* send is pending- use tmp buffer */
      MPI_Request rrequest;
      int size = 0;
      mpi_pack_size(count, datatype, comm, &size);
      void*tmpbuf = malloc(size);
      err = mpi_irecv(tmpbuf, size, MPI_PACKED, source, recvtag, comm, &rrequest);
      err = mpi_wait(&srequest, MPI_STATUS_IGNORE);
      err = mpi_wait(&rrequest, status);
      int position = 0;
      mpi_unpack(tmpbuf, size, &position, buf, count, datatype, comm);
      free(tmpbuf);
    }
  return err;
}

int mpi_iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status)
{
  int err = MPI_SUCCESS;
  nm_gate_t gate = NULL;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(source == MPI_ANY_SOURCE) 
    {
      gate = NM_ANY_GATE;
    }
  else
    {
      gate = nm_mpi_communicator_get_gate(p_comm, source);
      if(gate == NULL)
	{
	  NM_MPI_WARNING("Cannot find a in connection for source %d\n", source);
	  return MPI_ERR_INTERN;
	}
    }
  nm_tag_t nm_tag, tag_mask, out_tag;
  nm_mpi_get_tag(p_comm, tag, &nm_tag, &tag_mask);
  nm_gate_t out_gate = NULL;
  nm_len_t out_len = -1;
  err = nm_sr_probe(nm_mpi_communicator_get_session(p_comm), gate, &out_gate, nm_tag, tag_mask, &out_tag, &out_len);;
  if(err == NM_ESUCCESS) 
    {
      *flag = 1;
      if(status != NULL)
	{
	  status->MPI_TAG = out_tag;
	  status->MPI_SOURCE = nm_mpi_communicator_get_dest(p_comm, out_gate);
	  status->size = out_len;
	}
    }
  else
    { /* err == -NM_EAGAIN */
      *flag = 0;
    }
  return err;
}

int mpi_probe(int source, int tag, MPI_Comm comm, MPI_Status *status)
{
  int flag = 0;
  int err = MPI_ERR_UNKNOWN;
  do
    {
      err = mpi_iprobe(source, tag, comm, &flag, status);
    }
  while(flag != 1);
  return err;
}

int mpi_send_init(const void*buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    {
      return MPI_ERR_COMM;
    }
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      return MPI_ERR_TYPE;
    }
  if((tag < 0) || (tag == MPI_ANY_TAG))
    {
      return MPI_ERR_TAG;
    }
  nm_mpi_request_t *p_req = nm_mpi_request_alloc();
  p_req->request_type = NM_MPI_REQUEST_SEND;
  p_req->status |= NM_MPI_REQUEST_PERSISTENT;
  p_req->p_datatype = p_datatype;
  p_req->sbuf = buf;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->communication_mode = NM_MPI_MODE_IMMEDIATE;
  p_req->p_comm = p_comm;
  int err = nm_mpi_isend_init(p_req, dest, p_comm);
  *request = p_req->id;
  return err;
}

int mpi_rsend_init(const void*buf, int count, MPI_Datatype datatype,
                   int dest, int tag, MPI_Comm comm, MPI_Request*request)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    {
      return MPI_ERR_COMM;
    }
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      return MPI_ERR_TYPE;
    }
  if((tag < 0) || (tag == MPI_ANY_TAG))
    {
      return MPI_ERR_TAG;
    }
  nm_mpi_request_t *p_req = nm_mpi_request_alloc();
  p_req->request_type = NM_MPI_REQUEST_SEND;
  p_req->status |= NM_MPI_REQUEST_PERSISTENT;
  p_req->p_datatype = p_datatype;
  p_req->sbuf = buf;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->communication_mode = NM_MPI_MODE_IMMEDIATE;
  p_req->p_comm = p_comm;
  int err = nm_mpi_isend_init(p_req, dest, p_comm);
  *request = p_req->id;
  return err;
}

int mpi_ssend_init(const void*buf, int count, MPI_Datatype datatype,
                   int dest, int tag, MPI_Comm comm, MPI_Request*request)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    {
      return MPI_ERR_COMM;
    }
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      return MPI_ERR_TYPE;
    }
  if((tag < 0) || (tag == MPI_ANY_TAG))
    {
      return MPI_ERR_TAG;
    }
  nm_mpi_request_t *p_req = nm_mpi_request_alloc();
  p_req->request_type = NM_MPI_REQUEST_SEND;
  p_req->status |= NM_MPI_REQUEST_PERSISTENT;
  p_req->p_datatype = p_datatype;
  p_req->sbuf = buf;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->communication_mode = NM_MPI_MODE_SYNCHRONOUS;
  p_req->p_comm = p_comm;
  int err = nm_mpi_isend_init(p_req, dest, p_comm);
  *request = p_req->id;
  return err;
}


int mpi_recv_init(void* buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  if(p_comm == NULL)
    {
      return MPI_ERR_COMM;
    }
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    {
      return MPI_ERR_TYPE;
    }
  nm_mpi_request_t*p_req = nm_mpi_request_alloc();
  p_req->request_type = NM_MPI_REQUEST_RECV;
  p_req->status |= NM_MPI_REQUEST_PERSISTENT;
  p_req->p_datatype = p_datatype;
  p_req->rbuf = buf;
  p_req->count = count;
  p_req->user_tag = nm_mpi_check_tag(tag);
  p_req->p_comm = p_comm;
  int err = nm_mpi_irecv_init(p_req, source, p_comm);
  *request = p_req->id;
  return err;
}

