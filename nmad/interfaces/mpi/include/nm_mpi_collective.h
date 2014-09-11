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

/*
 * mpi_collective.h
 * ================
 */

#ifndef MPI_COLLECTIVE_H
#define MPI_COLLECTIVE_H

/** \addtogroup mpi_interface */
/* @{ */

/** @name Functions: Collective communications */
/* @{ */

/**
 * User combination function.
 */
typedef void MPI_User_function(void *, void *, int *, MPI_Datatype *);

/**
 * Blocks the caller until all group members have called the routine.
 * @param comm communicator
 * @return MPI status
 */
int MPI_Barrier(MPI_Comm comm);

/**
 * Broadcasts a message from the process with rank root to all
 * processes of the group, itself included.
 * @param buffer starting address of buffer
 * @param count number of entries in buffer
 * @param datatype data type of buffer
 * @param root rank of broadcast root
 * @param comm communicator
 * @return MPI status
 */
int MPI_Bcast(void* buffer,
              int count,
              MPI_Datatype datatype,
              int root,
              MPI_Comm comm);

/**
 * Each process sends the contents of its send buffer to the root
 * process.
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements for any single receive
 * @param recvtype data type of recv buffer elements
 * @param root rank of receiving process
 * @param comm communicator
 * @return MPI status
 */
int MPI_Gather(void *sendbuf,
               int sendcount,
               MPI_Datatype sendtype,
               void *recvbuf,
               int recvcount,
               MPI_Datatype recvtype,
               int root,
               MPI_Comm comm);

/**
 * Extends the functionality of MPI_Gather() by allowing a varying
 * count of data.
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer (significant only at root)
 * @param recvcounts integer array of length group size containing the
 * number of elements that are received from each process (significant
 * only at root)
 * @param displs integer array of length group size. Entry i specifies
 * the displacement relative to recvbuf at which to place the incoming
 * data from process i (significant only at root)
 * @param recvtype data type of recv buffer elements (significant only
 * at root)
 * @param root rank of receiving process
 * @param comm communicator
 * @return MPI status
 */
int MPI_Gatherv(void *sendbuf,
                int sendcount,
                MPI_Datatype sendtype,
                void *recvbuf,
                int *recvcounts,
                int *displs,
                MPI_Datatype recvtype,
                int root,
                MPI_Comm comm);

/**
 * Extends the functionality of MPI_Gather(), except all processes
 * receive the result.
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements received from any process
 * @param recvtype data type of receive buffer elements
 * @param comm communicator
 * @return MPI status
 */
int MPI_Allgather(void *sendbuf,
                  int sendcount,
                  MPI_Datatype sendtype,
                  void *recvbuf,
                  int recvcount,
                  MPI_Datatype recvtype,
                  MPI_Comm comm);

/**
 * Extends the functionality of MPI_Gatherv(), except all processes
 * receive the result.
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcounts integer array
 * @param displs integer array
 * @param recvtype data type of receive buffer elements
 * @param comm communicator
 * @return MPI status
 */
int MPI_Allgatherv(void *sendbuf,
                   int sendcount,
                   MPI_Datatype sendtype,
                   void *recvbuf,
                   int *recvcounts,
                   int *displs,
                   MPI_Datatype recvtype,
                   MPI_Comm comm);

/**
 * Inverse operation of MPI_Gather()
 * @param sendbuf address of send buffer
 * @param sendcount number of elements sent to each process
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements in receive buffer
 * @param recvtype data type of receive buffer elements
 * @param root rank of sending process
 * @param comm communicator
 * @return MPI status
 */
int MPI_Scatter(void *sendbuf,
                int sendcount,
                MPI_Datatype sendtype,
                void *recvbuf,
                int recvcount,
                MPI_Datatype recvtype,
                int root,
                MPI_Comm comm);

/**
 * Extension of MPI_Allgather() to the case where each process sends
 * distinct data to each of the receivers. The jth block sent from
 * process i is received by process j and is placed in the ith block
 * of recvbuf.
 * @param sendbuf starting address of send buffer
 * @param sendcount number of elements sent to each process
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount number of elements received from any process
 * @param recvType data type of receive buffer elements
 * @param comm communicator
 * @return MPI status
 */
int MPI_Alltoall(void* sendbuf,
		 int sendcount,
		 MPI_Datatype sendtype,
		 void *recvbuf,
		 int recvcount,
		 MPI_Datatype recvType,
		 MPI_Comm comm);

/**
 * Adds flexibility to MPI_Alltoall() in that the location
 * of data for the send is specified by sdispls and the location of
 * the placement of the data on the receive side is specified by
 * rdispls.
 * @param sendbuf starting address of send buffer
 * @param sendcount integer array equal to the group size specifying
 * the number of elements to send to each processor
 * @param sdispls integer array of length group size. Entry j
 * specifies the displacement relative to sendbuf from which to take
 * the outgoing data destined for process j
 * @param sendtype data type of send buffer elements
 * @param recvbuf address of receive buffer
 * @param recvcount integer array equal to the group size specifying
 * the number of elements that can be received from each processor
 * @param recvdispls integer array
 * @param recvType data type of receive buffer elements
 * @param comm communicator
 * @return MPI status
 */
int MPI_Alltoallv(void* sendbuf,
		  int *sendcount,
		  int *sdispls,
		  MPI_Datatype sendtype,
		  void *recvbuf,
		  int *recvcount,
		  int *recvdispls,
		  MPI_Datatype recvType,
		  MPI_Comm comm);

/**
 * Binds a user-defined global operation to an op handle that can
 * subsequently used in a global reduction operation.
 * @param function user defined function
 * @param commute true if commutative; false otherwise
 * @param op operation
 * @return MPI status
 */
int MPI_Op_create(MPI_User_function *function,
                  int commute,
                  MPI_Op *op);

/**
 * Marks a user-defined reduction operation for deallocation.
 * @param op operation to be freed
 * @return MPI status
 */
int MPI_Op_free(MPI_Op *op);

/**
 * Combines the elements provided in the input buffer of each process
 * in the group, using the operation op, and returns the combined
 * value in the output buffer of the process with rank root.
 * @param sendbuf address of send buffer
 * @param recvbuf address of receive buffer
 * @param count number of elements in send buffer
 * @param datatype data type of elements of send buffer
 * @param op reduce operation
 * @param root rank of root process
 * @param comm communicator
 * @return MPI status
 */
int MPI_Reduce(void* sendbuf,
               void* recvbuf,
               int count,
               MPI_Datatype datatype,
               MPI_Op op,
               int root,
               MPI_Comm comm);

/**
 * Combines the elements provided in the input buffer of each process
 * in the group, using the operation op, and returns the combined
 * value in the output buffer of each process in the group.
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param count number of elements in send buffer
 * @param datatype data type of elements of send buffer
 * @param op operation
 * @param comm communicator
 * @return MPI status
 */
int MPI_Allreduce(void* sendbuf,
                  void* recvbuf,
                  int count,
                  MPI_Datatype datatype,
                  MPI_Op op,
                  MPI_Comm comm);

/**
 * First does an element-wise reduction on vector
 * of elements in the send buffer defined by sendbuf, count and
 * datatype. Next, the resulting vector of results is split into n
 * disjoint segments, where n is the number of members in the group.
 * Segment i contains recvcounts[i] elements. The ith segment is sent
 * to process i and stored in the receive buffer defined by recvbuf,
 * recvcounts[i] and datatype.
 * @param sendbuf starting address of send buffer
 * @param recvbuf starting address of receive buffer
 * @param recvcounts integer array specifying the number of elements
 * in result distributed to each process. Array must be identical on
 * all calling processes
 * @param datatype data type of elements of input buffer
 * @param op operation
 * @param comm communicator
 * @return MPI status
 */
int MPI_Reduce_scatter(void *sendbuf,
                       void *recvbuf,
                       int *recvcounts,
                       MPI_Datatype datatype,
                       MPI_Op op,
                       MPI_Comm comm);
/* @}*/
/* @}*/

#endif /* MPI_COLLECTIVE_H */
