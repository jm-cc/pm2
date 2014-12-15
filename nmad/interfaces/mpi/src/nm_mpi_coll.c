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

NM_MPI_ALIAS(MPI_Barrier,        mpi_barrier);
NM_MPI_ALIAS(MPI_Bcast,          mpi_bcast);
NM_MPI_ALIAS(MPI_Gather,         mpi_gather);
NM_MPI_ALIAS(MPI_Gatherv,        mpi_gatherv);
NM_MPI_ALIAS(MPI_Allgather,      mpi_allgather);
NM_MPI_ALIAS(MPI_Allgatherv,     mpi_allgatherv);
NM_MPI_ALIAS(MPI_Scatter,        mpi_scatter);
NM_MPI_ALIAS(MPI_Alltoall,       mpi_alltoall);
NM_MPI_ALIAS(MPI_Alltoallv,      mpi_alltoallv);
NM_MPI_ALIAS(MPI_Reduce,         mpi_reduce);
NM_MPI_ALIAS(MPI_Allreduce,      mpi_allreduce);
NM_MPI_ALIAS(MPI_Reduce_scatter, mpi_reduce_scatter);

/* ********************************************************* */

/* ** building blocks */

static nm_mpi_request_t*nm_mpi_coll_isend(void*buffer, int count, nm_mpi_datatype_t*p_datatype, int dest, int tag, nm_mpi_communicator_t*p_comm)
{
  nm_mpi_request_t*p_req = nm_mpi_request_alloc();
  p_req->request_type            = NM_MPI_REQUEST_SEND;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  p_req->p_datatype              = p_datatype;
  p_req->buffer                  = buffer;
  p_req->count                   = count;
  p_req->user_tag                = tag;
  p_req->communication_mode      = NM_MPI_MODE_IMMEDIATE;
  p_req->p_comm                  = p_comm;
  int err = nm_mpi_isend(p_req, dest, p_comm);
  if(err != MPI_SUCCESS)
    {
      ERROR("nm_mpi_isend returned %d in collective.\n", err);
    }
  return p_req;
}
static nm_mpi_request_t*nm_mpi_coll_irecv(void*buffer, int count, nm_mpi_datatype_t*p_datatype, int source, int tag, nm_mpi_communicator_t*p_comm)
{
  nm_mpi_request_t*p_req = nm_mpi_request_alloc();
  p_req->request_type            = NM_MPI_REQUEST_RECV;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  p_req->p_datatype              = p_datatype;
  p_req->buffer                  = buffer;
  p_req->count                   = count;
  p_req->user_tag                = tag;
  p_req->communication_mode      = NM_MPI_MODE_IMMEDIATE;
  p_req->p_comm                  = p_comm;
  int err = nm_mpi_irecv(p_req, source, p_comm);
  if(err != MPI_SUCCESS)
    {
      ERROR("nm_mpi_irecv returned %d in collective.\n", err);
    }
  return p_req;
}

static void nm_mpi_coll_wait(nm_mpi_request_t*p_req)
{
  int err = nm_mpi_request_wait(p_req);
  if(err != MPI_SUCCESS)
    {
      ERROR("nm_mpi_request_wait returned %d in collective.\n", err);
    }
  nm_mpi_request_complete(p_req);
  nm_mpi_request_free(p_req);
}


/* ********************************************************* */

int mpi_barrier(MPI_Comm comm)
{
  const int tag = NM_MPI_TAG_PRIVATE_BARRIER;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  nm_coll_barrier(p_comm->p_comm, tag);
  return MPI_SUCCESS;
}

int mpi_bcast(void*buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  const int tag = NM_MPI_TAG_PRIVATE_BCAST;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_datatype == NULL)
    return MPI_ERR_TYPE;
  if(nm_comm_rank(p_comm->p_comm) == root)
    {
      nm_mpi_request_t**requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(nm_mpi_request_t*));
      int i;
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if(i == root) continue;
	  requests[i] = nm_mpi_coll_isend(buffer, count, p_datatype, i, tag, p_comm);
	}
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if(i == root) continue;
	  nm_mpi_coll_wait(requests[i]);
	}
      FREE_AND_SET_NULL(requests);
    }
  else
    {
      nm_mpi_request_t*p_req = nm_mpi_coll_irecv(buffer, count, p_datatype, root, tag, p_comm);
      nm_mpi_coll_wait(p_req);
    }
  return MPI_SUCCESS;
}

int mpi_gather(void*sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  const int tag = NM_MPI_TAG_PRIVATE_GATHER;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  nm_mpi_datatype_t*p_recv_datatype = nm_mpi_datatype_get(recvtype);
  nm_mpi_datatype_t*p_send_datatype = nm_mpi_datatype_get(sendtype);
  if(nm_comm_rank(p_comm->p_comm) == root)
    {
      nm_mpi_request_t**requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(nm_mpi_request_t*));
      int i;
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if(i == root) continue;
	  requests[i] = nm_mpi_coll_irecv(recvbuf + (i * recvcount * p_recv_datatype->extent),
					  recvcount, p_recv_datatype, i, tag, p_comm);
	}
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if(i == root) continue;
	  nm_mpi_coll_wait(requests[i]);
	}
      // copy local data for self
      if(sendbuf != MPI_IN_PLACE)
	{
	  nm_mpi_datatype_copy(sendbuf, p_send_datatype, sendcount,
			       recvbuf + (nm_comm_rank(p_comm->p_comm) * recvcount * p_recv_datatype->extent), p_send_datatype , sendcount);
	}
      FREE_AND_SET_NULL(requests);
    }
  else
    {
      nm_mpi_request_t*p_req = nm_mpi_coll_isend(sendbuf, sendcount, p_send_datatype, root, tag, p_comm);
      nm_mpi_coll_wait(p_req);
    }
  return MPI_SUCCESS;
}

int mpi_gatherv(void*sendbuf, int sendcount, MPI_Datatype sendtype, void*recvbuf, int*recvcounts, int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  const int tag = NM_MPI_TAG_PRIVATE_GATHERV;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  nm_mpi_datatype_t*p_recv_datatype = nm_mpi_datatype_get(recvtype);
  nm_mpi_datatype_t*p_send_datatype = nm_mpi_datatype_get(sendtype);
  if(nm_comm_rank(p_comm->p_comm) == root)
    {
      nm_mpi_request_t**requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(nm_mpi_request_t*));
      int i;
      // receive data from other processes
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if(i == root)
	    continue;
	  requests[i] = nm_mpi_coll_irecv(recvbuf + (displs[i] * p_recv_datatype->extent),
					  recvcounts[i], p_recv_datatype, i, tag, p_comm);
	}
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if(i == root)
	    continue;
	  nm_mpi_coll_wait(requests[i]);
	}
      // copy local data for self
      if(sendbuf != MPI_IN_PLACE)
	{
	  nm_mpi_datatype_copy(sendbuf, p_send_datatype, sendcount,
			       recvbuf + (displs[nm_comm_rank(p_comm->p_comm)] * p_recv_datatype->extent), p_recv_datatype, recvcounts[nm_comm_rank(p_comm->p_comm)]);
	}
      // free memory
      FREE_AND_SET_NULL(requests);
    }
  else
    {
      nm_mpi_request_t*p_req = nm_mpi_coll_isend(sendbuf, sendcount, p_send_datatype, root, tag, p_comm);
      nm_mpi_coll_wait(p_req);
    }
  return MPI_SUCCESS;
}

int mpi_allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  int err;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  mpi_gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, 0, comm);
  // Broadcast the result to all processes
  err = mpi_bcast(recvbuf, nm_comm_size(p_comm->p_comm)*recvcount, recvtype, 0, comm);
  return err;
}

int mpi_allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
  int err, i, recvcount = 0;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  // Broadcast the total receive count to all processes
  if (nm_comm_rank(p_comm->p_comm) == 0)
    {
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  MPI_NMAD_TRACE("recvcounts[%d] = %d\n", i, recvcounts[i]);
	  recvcount += recvcounts[i];
	}
      MPI_NMAD_TRACE("recvcount = %d\n", recvcount);
    }
  err = mpi_bcast(&recvcount, 1, MPI_INT, 0, comm);
  // Gather on process 0
  mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, 0, comm);
  // Broadcast the result to all processes
  err = mpi_bcast(recvbuf, recvcount, recvtype, 0, comm);
  return err;
}

int mpi_scatter(void*sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  const int tag = NM_MPI_TAG_PRIVATE_SCATTER;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  nm_mpi_datatype_t*p_recv_datatype = nm_mpi_datatype_get(recvtype);
  nm_mpi_datatype_t*p_send_datatype = nm_mpi_datatype_get(sendtype);
  if (nm_comm_rank(p_comm->p_comm) == root)
    {
      int i;
      nm_mpi_request_t**requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(nm_mpi_request_t*));
      // send data to other processes
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if(i == root) continue;
	  requests[i] = nm_mpi_coll_isend(sendbuf + (i * sendcount * p_send_datatype->extent),
					  sendcount, p_send_datatype, i, tag, p_comm);
	}
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if(i == root) continue;
	  nm_mpi_coll_wait(requests[i]);
	}
      // copy local data for self
      if(sendbuf != MPI_IN_PLACE)
	{
	  nm_mpi_datatype_copy(sendbuf + (nm_comm_rank(p_comm->p_comm) * sendcount * p_recv_datatype->extent), p_send_datatype, sendcount,
			       recvbuf, p_recv_datatype, recvcount);
	}
      FREE_AND_SET_NULL(requests);
    }
  else
    {
      nm_mpi_request_t*p_req = nm_mpi_coll_irecv(recvbuf, recvcount, p_recv_datatype, root, tag, p_comm);
      nm_mpi_coll_wait(p_req);
    }
  return MPI_SUCCESS;
}

int mpi_alltoall(void* sendbuf, int sendcount, MPI_Datatype sendtype, void*recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  const int tag = NM_MPI_TAG_PRIVATE_ALLTOALL;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  nm_mpi_request_t**send_requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(nm_mpi_request_t*));
  nm_mpi_request_t**recv_requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(nm_mpi_request_t*));
  nm_mpi_datatype_t*p_send_datatype = nm_mpi_datatype_get(sendtype);
  nm_mpi_datatype_t*p_recv_datatype = nm_mpi_datatype_get(recvtype);
  int i;
  for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
    {
      if(i == nm_comm_rank(p_comm->p_comm))
	{
	  nm_mpi_datatype_copy(sendbuf + (i * sendcount * p_send_datatype->extent), p_send_datatype, sendcount,
			       recvbuf + (i * recvcount * p_recv_datatype->extent), p_recv_datatype, recvcount);
	}
      else
	{
	  send_requests[i] = nm_mpi_coll_isend(sendbuf + (i * sendcount * p_send_datatype->extent),
					       sendcount, p_send_datatype, i, tag, p_comm);
	  recv_requests[i] = nm_mpi_coll_irecv(recvbuf + (i * recvcount * p_recv_datatype->extent),
					       recvcount, p_recv_datatype, i, tag, p_comm);
	}
    }
  for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
    {
      if (i == nm_comm_rank(p_comm->p_comm)) continue;
      nm_mpi_coll_wait(send_requests[i]);
      nm_mpi_coll_wait(recv_requests[i]);
    }
  FREE_AND_SET_NULL(send_requests);
  FREE_AND_SET_NULL(recv_requests);
  return MPI_SUCCESS;
}

int mpi_alltoallv(void* sendbuf, int *sendcounts, int *sdispls, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
  int tag = NM_MPI_TAG_PRIVATE_ALLTOALLV;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  nm_mpi_request_t**send_requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(nm_mpi_request_t*));
  nm_mpi_request_t**recv_requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(nm_mpi_request_t*));
  nm_mpi_datatype_t*p_send_datatype = nm_mpi_datatype_get(sendtype);
  nm_mpi_datatype_t*p_recv_datatype = nm_mpi_datatype_get(recvtype);
  int i;
  for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
    {
      if(i == nm_comm_rank(p_comm->p_comm))
	{
	  recv_requests[i] = NULL;
	  send_requests[i] = NULL;
	  nm_mpi_datatype_copy(sendbuf + (sdispls[i] * p_send_datatype->extent), p_send_datatype, sendcounts[i],
			       recvbuf + (rdispls[i] * p_recv_datatype->extent), p_recv_datatype, recvcounts[i]);
	}
      else
	{
	  if(recvcounts[i] > 0)
	    recv_requests[i] = nm_mpi_coll_irecv(recvbuf + (rdispls[i] * p_recv_datatype->extent),
						 recvcounts[i], p_recv_datatype, i, tag, p_comm);
	  else
	    recv_requests[i] = NULL;
	  if(sendcounts[i] > 0)
	    send_requests[i] = nm_mpi_coll_isend(sendbuf + (sdispls[i] * p_send_datatype->extent),
						 sendcounts[i], p_send_datatype, i, tag, p_comm);
	  else
	    send_requests[i] = NULL;
	}
    }
  for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
    {
      if(recv_requests[i] != NULL)
	nm_mpi_coll_wait(recv_requests[i]);
      if(send_requests[i] != NULL)
	nm_mpi_coll_wait(send_requests[i]);
    }
  FREE_AND_SET_NULL(send_requests);
  FREE_AND_SET_NULL(recv_requests);
  return MPI_SUCCESS;
}

int mpi_reduce(void*sendbuf, void*recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  const int tag = NM_MPI_TAG_PRIVATE_REDUCE;
  if(op == MPI_OP_NULL)
    {
      return MPI_ERR_OP;
    }
  if(comm == MPI_COMM_NULL)
    {
      return MPI_ERR_COMM;
    }
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  nm_mpi_operator_t*p_operator = nm_mpi_operator_get(op);
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  assert(p_datatype->is_contig);
  assert(p_datatype->size == p_datatype->extent);
  const int size = nm_comm_size(p_comm->p_comm);
  if(p_operator->function == NULL)
    {
      ERROR("Operation %d not implemented\n", op);
      return MPI_ERR_INTERN;
    }
  if (nm_comm_rank(p_comm->p_comm) == root)
    {
      // Get the input buffers of all the processes
      int i;
      const int slot_size = count * nm_mpi_datatype_size(p_datatype);
      char*gather_buf = malloc(size * slot_size);
      nm_mpi_request_t**requests = malloc(size * sizeof(nm_mpi_request_t*));
      for(i = 0; i < size; i++)
	{
	  if(i == root) continue;
	  requests[i] = nm_mpi_coll_irecv(&gather_buf[i * slot_size], count, p_datatype, i, tag, p_comm);
	}
      for(i = 0; i < size; i++)
	{
	  if(i == root) continue;
	  nm_mpi_coll_wait(requests[i]);
	}
      // input buffer from self
      if(sendbuf == MPI_IN_PLACE)
	memcpy(&gather_buf[root * slot_size], recvbuf, slot_size);
      else
	memcpy(&gather_buf[root * slot_size], sendbuf, slot_size);
      // Do the reduction operation
      memcpy(recvbuf, &gather_buf[slot_size * (size - 1)], slot_size);
      for(i = size - 2; i >= 0; i--)
	{
	  p_operator->function(&gather_buf[i * slot_size], recvbuf, &count, &datatype);
	}
      FREE_AND_SET_NULL(gather_buf);
      FREE_AND_SET_NULL(requests);
      return MPI_SUCCESS;
    }
  else
    {
      void*ptr = (sendbuf == MPI_IN_PLACE) ? recvbuf : sendbuf;
      nm_mpi_request_t*p_req = nm_mpi_coll_isend(ptr, count, p_datatype, root, tag, p_comm);
      nm_mpi_coll_wait(p_req);
      return MPI_SUCCESS;
    }
}

int mpi_allreduce(void*sendbuf, void*recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int err;
  const int root = 0;
  err = mpi_reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
  // Broadcast the result to all processes
  err = mpi_bcast(recvbuf, count, datatype, root, comm);
  return err;
}

int mpi_reduce_scatter(void*sendbuf, void*recvbuf, int*recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int count = 0, i;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  const int tag = NM_MPI_TAG_PRIVATE_REDUCESCATTER;
  void *reducebuf = NULL;
  assert(p_datatype->is_contig);
  for(i = 0; i < nm_comm_size(p_comm->p_comm) ; i++)
    {
      count += recvcounts[i];
    }
  if(nm_comm_rank(p_comm->p_comm) == 0)
    {
      reducebuf = malloc(count * p_datatype->size);
    }
  mpi_reduce(sendbuf, reducebuf, count, datatype, op, 0, comm);

  // Scatter the result
  if (nm_comm_rank(p_comm->p_comm) == 0)
    {
      nm_mpi_request_t**requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(nm_mpi_request_t*));
      // send data to other processes
      for(i = 1 ; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  requests[i] = nm_mpi_coll_isend(reducebuf + (i * recvcounts[i] * p_datatype->extent),
					  recvcounts[i], p_datatype, i, tag, p_comm);
	}
      for(i = 1; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  nm_mpi_coll_wait(requests[i]);
	}
      // copy local data for self
      memcpy(recvbuf, reducebuf, recvcounts[0] * p_datatype->extent);
      FREE_AND_SET_NULL(requests);
    }
  else 
    {
      nm_mpi_request_t*p_req = nm_mpi_coll_irecv(recvbuf, recvcounts[nm_comm_rank(p_comm->p_comm)], p_datatype, 0, tag, p_comm);
      nm_mpi_coll_wait(p_req);
    }
  if (nm_comm_rank(p_comm->p_comm) == 0)
    {
      FREE_AND_SET_NULL(reducebuf);
    }
  return MPI_SUCCESS;
}


