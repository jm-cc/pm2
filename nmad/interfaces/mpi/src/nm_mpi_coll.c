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

int MPI_Barrier(MPI_Comm comm) __attribute__ ((alias ("mpi_barrier")));


int MPI_Bcast(void* buffer,
              int count,
              MPI_Datatype datatype,
              int root,
              MPI_Comm comm) __attribute__ ((alias ("mpi_bcast")));

int MPI_Gather(void *sendbuf,
               int sendcount,
               MPI_Datatype sendtype,
               void *recvbuf,
               int recvcount,
               MPI_Datatype recvtype,
               int root,
               MPI_Comm comm) __attribute__ ((alias ("mpi_gather")));

int MPI_Gatherv(void *sendbuf,
                int sendcount,
                MPI_Datatype sendtype,
                void *recvbuf,
                int *recvcounts,
                int *displs,
                MPI_Datatype recvtype,
                int root,
                MPI_Comm comm) __attribute__ ((alias ("mpi_gatherv")));

int MPI_Allgather(void *sendbuf,
                  int sendcount,
                  MPI_Datatype sendtype,
                  void *recvbuf,
                  int recvcount,
                  MPI_Datatype recvtype,
                  MPI_Comm comm) __attribute__ ((alias ("mpi_allgather")));

int MPI_Allgatherv(void *sendbuf,
                   int sendcount,
                   MPI_Datatype sendtype,
                   void *recvbuf,
                   int *recvcounts,
                   int *displs,
                   MPI_Datatype recvtype,
                   MPI_Comm comm) __attribute__ ((alias ("mpi_allgatherv")));

int MPI_Scatter(void *sendbuf,
                int sendcount,
                MPI_Datatype sendtype,
                void *recvbuf,
                int recvcount,
                MPI_Datatype recvtype,
                int root,
                MPI_Comm comm) __attribute__ ((alias ("mpi_scatter")));

int MPI_Alltoall(void* sendbuf,
		 int sendcount,
		 MPI_Datatype sendtype,
		 void *recvbuf,
		 int recvcount,
		 MPI_Datatype recvtype,
		 MPI_Comm comm) __attribute__ ((alias ("mpi_alltoall")));

int MPI_Alltoallv(void* sendbuf,
		  int *sendcounts,
		  int *sdispls,
		  MPI_Datatype sendtype,
		  void *recvbuf,
		  int *recvcounts,
		  int *rdispls,
		  MPI_Datatype recvtype,
		  MPI_Comm comm) __attribute__ ((alias ("mpi_alltoallv")));

int MPI_Op_create(MPI_User_function *function,
                  int commute,
                  MPI_Op *op) __attribute__ ((alias ("mpi_op_create")));

int MPI_Op_free(MPI_Op *op) __attribute__ ((alias ("mpi_op_free")));

int MPI_Reduce(void* sendbuf,
               void* recvbuf,
               int count,
               MPI_Datatype datatype,
               MPI_Op op,
               int root,
               MPI_Comm comm) __attribute__ ((alias ("mpi_reduce")));

int MPI_Allreduce(void* sendbuf,
                  void* recvbuf,
                  int count,
                  MPI_Datatype datatype,
                  MPI_Op op,
                  MPI_Comm comm) __attribute__ ((alias ("mpi_allreduce")));

int MPI_Reduce_scatter(void *sendbuf,
                       void *recvbuf,
                       int *recvcounts,
                       MPI_Datatype datatype,
                       MPI_Op op,
                       MPI_Comm comm) __attribute__ ((alias ("mpi_reduce_scatter")));

/* ********************************************************* */

/** Maximum number of reduce operators */
#define NUMBER_OF_OPERATORS (_MPI_OP_LAST+1)

static struct
{
  /** all the defined reduce operations */
  nm_mpi_operator_t *operators[NUMBER_OF_OPERATORS];
  /** pool of ids of reduce operations that can be created by end-users */
  puk_int_vect_t operators_pool;
} nm_mpi_collectives;

__PUK_SYM_INTERNAL
void nm_mpi_coll_init(void)
{
  /** Initialise the collective operators */
  int i;
  for(i = _MPI_OP_FIRST; i <= _MPI_OP_LAST; i++)
    {
      nm_mpi_collectives.operators[i] = malloc(sizeof(nm_mpi_operator_t));
      nm_mpi_collectives.operators[i]->commute = 1;
    }
  nm_mpi_collectives.operators[MPI_MAX]->function = &mpir_op_max;
  nm_mpi_collectives.operators[MPI_MIN]->function = &mpir_op_min;
  nm_mpi_collectives.operators[MPI_SUM]->function = &mpir_op_sum;
  nm_mpi_collectives.operators[MPI_PROD]->function = &mpir_op_prod;
  nm_mpi_collectives.operators[MPI_LAND]->function = &mpir_op_land;
  nm_mpi_collectives.operators[MPI_BAND]->function = &mpir_op_band;
  nm_mpi_collectives.operators[MPI_LOR]->function = &mpir_op_lor;
  nm_mpi_collectives.operators[MPI_BOR]->function = &mpir_op_bor;
  nm_mpi_collectives.operators[MPI_LXOR]->function = &mpir_op_lxor;
  nm_mpi_collectives.operators[MPI_BXOR]->function = &mpir_op_bxor;
  nm_mpi_collectives.operators[MPI_MINLOC]->function = &mpir_op_minloc;
  nm_mpi_collectives.operators[MPI_MAXLOC]->function = &mpir_op_maxloc;

  nm_mpi_collectives.operators_pool = puk_int_vect_new();
  for(i = 1; i < MPI_MAX; i++)
    {
      puk_int_vect_push_back(nm_mpi_collectives.operators_pool, i);
    }
}

__PUK_SYM_INTERNAL
void nm_mpi_coll_exit(void)
{
  int i;
  for(i = _MPI_OP_FIRST; i <= _MPI_OP_LAST; i++)
    {
      FREE_AND_SET_NULL(nm_mpi_collectives.operators[i]);
    }

  puk_int_vect_delete(nm_mpi_collectives.operators_pool);
  nm_mpi_collectives.operators_pool = NULL;

}

static nm_mpi_request_t*nm_mpi_coll_isend(void*buffer, int count, nm_mpi_datatype_t*p_datatype, int dest, int tag, nm_mpi_communicator_t*p_comm)
{
  nm_mpi_request_t*p_req = nm_mpi_request_alloc();
  p_req->request_type            = NM_MPI_REQUEST_SEND;
  p_req->request_persistent_type = NM_MPI_REQUEST_ZERO;
  p_req->request_ptr             = NULL;
  p_req->contig_buffer           = NULL;
  p_req->p_datatype              = p_datatype;
  p_req->buffer                  = buffer;
  p_req->count                   = count;
  p_req->user_tag                = tag;
  p_req->communication_mode      = MPI_IMMEDIATE_MODE;
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
  p_req->request_ptr             = NULL;
  p_req->contig_buffer           = NULL;
  p_req->p_datatype              = p_datatype;
  p_req->buffer                  = buffer;
  p_req->count                   = count;
  p_req->user_tag                = tag;
  p_req->communication_mode      = MPI_IMMEDIATE_MODE;
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
      // copy local data for itself
      memcpy(recvbuf + (nm_comm_rank(p_comm->p_comm) * p_recv_datatype->extent),
	     sendbuf, sendcount * p_send_datatype->extent);
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
      memcpy(recvbuf + (displs[nm_comm_rank(p_comm->p_comm)] * p_recv_datatype->extent),
	     sendbuf, sendcount * p_send_datatype->extent);
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
	  if (i==root) continue;
	  nm_mpi_coll_wait(requests[i]);
	}
      // copy local data for itself
      memcpy(recvbuf + (nm_comm_rank(p_comm->p_comm) * p_recv_datatype->extent),
	     sendbuf, sendcount * p_send_datatype->extent);
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
	  memcpy(recvbuf + (i * recvcount * p_recv_datatype->extent),
		 sendbuf + (i * sendcount * p_send_datatype->extent),
		 sendcount * p_send_datatype->extent);
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
	  memcpy(recvbuf + (rdispls[i] * p_recv_datatype->extent),
		 sendbuf + (sdispls[i] * p_send_datatype->extent),
		 sendcounts[i] * p_send_datatype->extent);
	}
      else
	{
	  recv_requests[i] = nm_mpi_coll_irecv(recvbuf + (rdispls[i] * p_recv_datatype->extent),
					       recvcounts[i], p_recv_datatype, i, tag, p_comm);
	  send_requests[i] = nm_mpi_coll_isend(sendbuf + (sdispls[i] * p_send_datatype->extent),
					       sendcounts[i], p_send_datatype, i, tag, p_comm);
	}
    }
  for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
    {
      if (i == nm_comm_rank(p_comm->p_comm)) continue;
      nm_mpi_coll_wait(recv_requests[i]);
      nm_mpi_coll_wait(send_requests[i]);
    }
  FREE_AND_SET_NULL(send_requests);
  FREE_AND_SET_NULL(recv_requests);
  return MPI_SUCCESS;
}

int mpi_op_create(MPI_User_function *function, int commute, MPI_Op *op)
{
  int err;
  if(puk_int_vect_empty(nm_mpi_collectives.operators_pool))
    {
      ERROR("Maximum number of operations created");
      return MPI_ERR_INTERN;
    }
  else
    {
      *op = puk_int_vect_pop_back(nm_mpi_collectives.operators_pool);
      nm_mpi_collectives.operators[*op] = malloc(sizeof(nm_mpi_operator_t));
      nm_mpi_collectives.operators[*op]->function = function;
      nm_mpi_collectives.operators[*op]->commute = commute;
      return MPI_SUCCESS;
    }
  return err;
}

int mpi_op_free(MPI_Op *op)
{
  int err;
  if (*op > NUMBER_OF_OPERATORS || nm_mpi_collectives.operators[*op] == NULL) 
    {
      ERROR("Operator %d unknown\n", *op);
      return MPI_ERR_OTHER;
    }
  else
    {
      FREE_AND_SET_NULL(nm_mpi_collectives.operators[*op]);
      puk_int_vect_push_back(nm_mpi_collectives.operators_pool, *op);
      *op = MPI_OP_NULL;
      return MPI_SUCCESS;
    }
  return err;
}

int mpi_reduce(void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  const int tag = NM_MPI_TAG_PRIVATE_REDUCE;
  nm_mpi_communicator_t*p_comm = nm_mpi_communicator_get(comm);
  nm_mpi_operator_t*p_operator = nm_mpi_operator_get(op);
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  if(p_operator->function == NULL)
    {
      ERROR("Operation %d not implemented\n", op);
      MPI_NMAD_LOG_OUT();
      return MPI_ERR_INTERN;
    }
  if (nm_comm_rank(p_comm->p_comm) == root)
    {
      // Get the input buffers of all the processes
      int i;
      void **remote_sendbufs = malloc(nm_comm_size(p_comm->p_comm) * sizeof(void *));
      nm_mpi_request_t**requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(nm_mpi_request_t*));
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if(i == root) continue;
	  remote_sendbufs[i] = malloc(count * nm_mpi_datatype_size(p_datatype));
	  requests[i] = nm_mpi_coll_irecv(remote_sendbufs[i], count, p_datatype, i, tag, p_comm);
	}
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if (i == root) continue;
	  nm_mpi_coll_wait(requests[i]);
	}
      // Do the reduction operation
      if(recvbuf != sendbuf)
	memcpy(recvbuf, sendbuf, count*nm_mpi_datatype_size(p_datatype));
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if (i == root) continue;
	  p_operator->function(remote_sendbufs[i], recvbuf, &count, &datatype);
	}
      // Free memory
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if (i == root) continue;
	  FREE_AND_SET_NULL(remote_sendbufs[i]);
	}
      FREE_AND_SET_NULL(remote_sendbufs);
      FREE_AND_SET_NULL(requests);
      return MPI_SUCCESS;
    }
  else
    {
      nm_mpi_request_t*p_req = nm_mpi_coll_isend(sendbuf, count, p_datatype, root, tag, p_comm);
      nm_mpi_coll_wait(p_req);
      return MPI_SUCCESS;
    }
}

int mpi_allreduce(void*sendbuf, void*recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int err;
  MPI_NMAD_LOG_IN();
  mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm);
  // Broadcast the result to all processes
  err = mpi_bcast(recvbuf, count, datatype, 0, comm);
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_reduce_scatter(void*sendbuf, void*recvbuf, int*recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
  int count = 0, i;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  nm_mpi_datatype_t*p_datatype = nm_mpi_datatype_get(datatype);
  const int tag = NM_MPI_TAG_PRIVATE_REDUCESCATTER;
  void *reducebuf = NULL;
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
    // copy local data for itself
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

__PUK_SYM_INTERNAL
nm_mpi_operator_t*nm_mpi_operator_get(MPI_Op op)
 {
  if(nm_mpi_collectives.operators[op] != NULL)
    {
      return nm_mpi_collectives.operators[op];
    }
  else
    {
      ERROR("Operation %d unknown", op);
      return NULL;
    }
}

