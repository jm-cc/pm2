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

int mpi_barrier(MPI_Comm comm)
{
  tbx_bool_t termination;

  MPI_NMAD_LOG_IN();

  termination = mpir_test_termination(comm);
  MPI_NMAD_TRACE("Result %d\n", termination);
  while (termination == tbx_false) {
    sleep(1);
    termination = mpir_test_termination(comm);
  }

  MPI_NMAD_LOG_OUT();
  return MPI_SUCCESS;
}

int mpi_bcast(void*buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  int tag = 30;
  int err = NM_ESUCCESS;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);

  MPI_NMAD_TRACE("Entering a bcast from root %d for buffer %p of type %d\n", root, buffer, datatype);
  if(nm_comm_rank(p_comm->p_comm) == root)
    {
      MPI_Request *requests;
      int i;
      requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(MPI_Request));
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if (i == root) continue;
	  err = MPI_Isend(buffer, count, datatype, i, tag, comm, &requests[i]);
	  if (err != 0) 
	    {
	      MPI_NMAD_LOG_OUT();
	      return err;
	    }
	}
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if(i == root) continue;
	  err = MPI_Wait(&requests[i], MPI_STATUS_IGNORE);
	  if (err != 0)
	    {
	      MPI_NMAD_LOG_OUT();
	      return err;
	    }
	}
      FREE_AND_SET_NULL(requests);
      err = MPI_SUCCESS;
    }
  else
    {
      MPI_Request request;
      mpi_irecv(buffer, count, datatype, root, tag, comm, &request);
      err = MPI_Wait(&request, MPI_STATUS_IGNORE);
    }
  MPI_NMAD_TRACE("End of bcast from root %d for buffer %p of type %d\n", root, buffer, datatype);
  return err;
}

int mpi_gather(void*sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int tag = 29;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  if(nm_comm_rank(p_comm->p_comm) == root)
    {
      MPI_Request *requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(MPI_Request));
      int i, err;
      nm_mpi_datatype_t *mpir_recv_datatype, *mpir_send_datatype;
      mpir_recv_datatype = nm_mpi_datatype_get(recvtype);
      mpir_send_datatype = nm_mpi_datatype_get(sendtype);
      // receive data from other processes
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if(i == root) continue;
	  mpi_irecv(recvbuf + (i * recvcount * mpir_recv_datatype->extent),
		    recvcount, recvtype, i, tag, comm, &requests[i]);
	}
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if(i == root) continue;
	  err = mpi_wait(&requests[i], MPI_STATUS_IGNORE);
	}
      // copy local data for itself
      memcpy(recvbuf + (nm_comm_rank(p_comm->p_comm) * mpir_recv_datatype->extent),
	     sendbuf, sendcount * mpir_send_datatype->extent);
      // free memory
      FREE_AND_SET_NULL(requests);
    }
  else
    {
      mpi_send(sendbuf, sendcount, sendtype, root, tag, comm);
    }
  return MPI_SUCCESS;
}

int mpi_gatherv(void*sendbuf, int sendcount, MPI_Datatype sendtype, void*recvbuf, int*recvcounts, int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  int tag = 28;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  if(nm_comm_rank(p_comm->p_comm) == root)
    {
      MPI_Request *requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(MPI_Request));
      int i, err;
      nm_mpi_datatype_t *mpir_recv_datatype, *mpir_send_datatype;
      mpir_recv_datatype = nm_mpi_datatype_get(recvtype);
      mpir_send_datatype = nm_mpi_datatype_get(sendtype);
      // receive data from other processes
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if(i == root) continue;
	  mpi_irecv(recvbuf + (displs[i] * mpir_recv_datatype->extent),
		    recvcounts[i], recvtype, i, tag, comm, &requests[i]);
	}
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if (i==root) continue;
	  err = mpi_wait(&requests[i], MPI_STATUS_IGNORE);
	}
      // copy local data for itself
      MPI_NMAD_TRACE("Copying local data from %p to %p with len %ld\n", sendbuf,
		     recvbuf + (displs[nm_comm_rank(p_comm->p_comm)] * mpir_recv_datatype->extent),
		     (long) sendcount * mpir_send_datatype->extent);
      memcpy(recvbuf + (displs[nm_comm_rank(p_comm->p_comm)] * mpir_recv_datatype->extent),
	     sendbuf, sendcount * mpir_send_datatype->extent);
      // free memory
      FREE_AND_SET_NULL(requests);
    }
  else 
    {
      mpi_send(sendbuf, sendcount, sendtype, root, tag, comm);
    }
  return MPI_SUCCESS;
}

int mpi_allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  int err;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
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
  int tag = 27;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  if (nm_comm_rank(p_comm->p_comm) == root)
    {
      int i, err;
      nm_mpi_datatype_t *mpir_recv_datatype, *mpir_send_datatype;
      MPI_Request *requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(MPI_Request));
      mpir_recv_datatype = nm_mpi_datatype_get(recvtype);
      mpir_send_datatype = nm_mpi_datatype_get(sendtype);
      // send data to other processes
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if(i == root) continue;
	  mpi_isend(sendbuf + (i * sendcount * mpir_send_datatype->extent),
		    sendcount, sendtype, i, tag, comm, &requests[i]);
	}
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if (i==root) continue;
	  err = mpi_wait(&requests[i], MPI_STATUS_IGNORE);
	}
      // copy local data for itself
      memcpy(recvbuf + (nm_comm_rank(p_comm->p_comm) * mpir_recv_datatype->extent),
	     sendbuf, sendcount * mpir_send_datatype->extent);
      FREE_AND_SET_NULL(requests);
    }
  else
    {
      mpi_recv(recvbuf, recvcount, recvtype, root, tag, comm, MPI_STATUS_IGNORE);
    }
  return MPI_SUCCESS;
}

int mpi_alltoall(void* sendbuf, int sendcount, MPI_Datatype sendtype, void*recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  int tag = 26;
  int err;
  int i;
  MPI_Request *send_requests, *recv_requests;
  nm_mpi_datatype_t *mpir_send_datatype,*mpir_recv_datatype;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  send_requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(MPI_Request));
  recv_requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(MPI_Request));

  mpir_send_datatype = nm_mpi_datatype_get(sendtype);
  mpir_recv_datatype = nm_mpi_datatype_get(recvtype);

  for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
    {
      if(i == nm_comm_rank(p_comm->p_comm))
	{
	  memcpy(recvbuf + (i * recvcount * mpir_recv_datatype->extent),
		 sendbuf + (i * sendcount * mpir_send_datatype->extent),
		 sendcount * mpir_send_datatype->extent);
	}
      else
	{
	  mpi_irecv(recvbuf + (i * recvcount * mpir_recv_datatype->extent),
		    recvcount, recvtype, i, tag, comm, &recv_requests[i]);
	  
	  err = mpi_isend(sendbuf + (i * sendcount * mpir_send_datatype->extent),
			  sendcount, sendtype, i, tag, comm, &send_requests[i]);
	  
	  if (err != 0)
	    {
	      MPI_NMAD_LOG_OUT();
	      return err;
	    }
	}
    }
  for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
    {
      if (i == nm_comm_rank(p_comm->p_comm)) continue;
      mpi_wait(&recv_requests[i], MPI_STATUS_IGNORE);
      MPI_Wait(&send_requests[i], MPI_STATUS_IGNORE);
    }
  FREE_AND_SET_NULL(send_requests);
  FREE_AND_SET_NULL(recv_requests);
  return MPI_SUCCESS;
}

int mpi_alltoallv(void* sendbuf, int *sendcounts, int *sdispls, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
  int tag = 25;
  int err;
  int i;
  MPI_Request *send_requests, *recv_requests;
  nm_mpi_datatype_t *mpir_send_datatype,*mpir_recv_datatype;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  send_requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(MPI_Request));
  recv_requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(MPI_Request));

  mpir_send_datatype = nm_mpi_datatype_get(sendtype);
  mpir_recv_datatype = nm_mpi_datatype_get(recvtype);

  for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
    {
      if(i == nm_comm_rank(p_comm->p_comm))
	{
	  memcpy(recvbuf + (rdispls[i] * mpir_recv_datatype->extent),
		 sendbuf + (sdispls[i] * mpir_send_datatype->extent),
		 sendcounts[i] * mpir_send_datatype->extent);
	}
      else
	{
	  mpi_irecv(recvbuf + (rdispls[i] * mpir_recv_datatype->extent),
		    recvcounts[i], recvtype, i, tag, comm, &recv_requests[i]);
	  err = mpi_isend(sendbuf + (sdispls[i] * mpir_send_datatype->extent),
			  sendcounts[i], sendtype, i, tag, comm, &send_requests[i]);
	  if (err != 0)
	    {
	      MPI_NMAD_LOG_OUT();
	      return err;
	    }
	}
    }
  for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
    {
      if (i == nm_comm_rank(p_comm->p_comm)) continue;
      mpi_wait(&recv_requests[i], MPI_STATUS_IGNORE);
      mpi_wait(&send_requests[i], MPI_STATUS_IGNORE);
    }
  FREE_AND_SET_NULL(send_requests);
  FREE_AND_SET_NULL(recv_requests);
  return MPI_SUCCESS;
}

int mpi_op_create(MPI_User_function *function, int commute, MPI_Op *op)
{
  int err;
  MPI_NMAD_LOG_IN();
  err = mpir_op_create(function, commute, op);
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_op_free(MPI_Op *op)
{
  int err;
  MPI_NMAD_LOG_IN();
  err = mpir_op_free(op);
  MPI_NMAD_LOG_OUT();
  return err;
}

int mpi_reduce(void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
  int tag = 24;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  mpir_operator_t *operator = mpir_get_operator(op);
  if(operator->function == NULL)
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
      MPI_Request *requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(MPI_Request));
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if(i == root) continue;
	  remote_sendbufs[i] = malloc(count * mpir_sizeof_datatype(datatype));
	  mpi_irecv(remote_sendbufs[i], count, datatype, i, tag, comm, &requests[i]);
	}
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if (i == root) continue;
	  mpi_wait(&requests[i], MPI_STATUS_IGNORE);
	}
      // Do the reduction operation
      if(recvbuf != sendbuf)
	memcpy(recvbuf, sendbuf, count*mpir_sizeof_datatype(datatype));
      for(i = 0; i < nm_comm_size(p_comm->p_comm); i++)
	{
	  if (i == root) continue;
	  operator->function(remote_sendbufs[i], recvbuf, &count, &datatype);
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
      int err;
      err = mpi_send(sendbuf, count, datatype, root, tag, comm);
      MPI_NMAD_LOG_OUT();
      return err;
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
  int err = MPI_SUCCESS, count = 0, i;
  nm_mpi_communicator_t *p_comm = nm_mpi_communicator_get(comm);
  nm_mpi_datatype_t *nm_mpi_datatype = nm_mpi_datatype_get(datatype);
  int tag = 23;
  void *reducebuf = NULL;

  MPI_NMAD_LOG_IN();

  for(i = 0; i < nm_comm_size(p_comm->p_comm) ; i++)
    {
      count += recvcounts[i];
    }
  if(nm_comm_rank(p_comm->p_comm) == 0)
    {
      reducebuf = malloc(count * nm_mpi_datatype->size);
    }
  mpi_reduce(sendbuf, reducebuf, count, datatype, op, 0, comm);

  // Scatter the result
  if (nm_comm_rank(p_comm->p_comm) == 0)
    {
      MPI_Request *requests = malloc(nm_comm_size(p_comm->p_comm) * sizeof(MPI_Request));
      // send data to other processes
      for(i = 1 ; i<nm_comm_size(p_comm->p_comm); i++)
	{
	  mpi_isend(reducebuf + (i * recvcounts[i] * nm_mpi_datatype->extent),
		    recvcounts[i], datatype, i, tag, comm, &requests[i]);
	}
      for(i=1 ; i<nm_comm_size(p_comm->p_comm) ; i++)
	{
	  err = mpi_wait(&requests[i], MPI_STATUS_IGNORE);
	}
    // copy local data for itself
    memcpy(recvbuf, reducebuf, recvcounts[0] * nm_mpi_datatype->extent);
    FREE_AND_SET_NULL(requests);
    }
  else 
    {
      err = mpi_recv(recvbuf, recvcounts[nm_comm_rank(p_comm->p_comm)], datatype, 0, tag, comm, MPI_STATUS_IGNORE);
    }
  if (nm_comm_rank(p_comm->p_comm) == 0)
    {
      FREE_AND_SET_NULL(reducebuf);
    }
  MPI_NMAD_LOG_OUT();
  return err;
}
