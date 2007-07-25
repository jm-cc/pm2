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

/**
 * User combination function.
 */
typedef void MPI_User_function(void *, void *, int *, MPI_Datatype *);

/**
 * Blocks the caller until all group members have called the routine.
 */
int MPI_Barrier(MPI_Comm comm);

/**
 * Broadcasts a message from the process with rank root to all
 * processes of the group, itself included.
 */
int MPI_Bcast(void* buffer,
              int count,
              MPI_Datatype datatype,
              int root,
              MPI_Comm comm);

/**
 * Each process sends the contents of its send buffer to the root
 * process.
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
 * MPI_GATHERV extends the functionality of MPI_GATHER by allowing a
 * varying count of data.
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
 * MPI_ALLGATHER can be thought of as MPI_GATHER, except all processes
 * receive the result.
 */
int MPI_Allgather(void *sendbuf,
                  int sendcount,
                  MPI_Datatype sendtype,
                  void *recvbuf,
                  int recvcount,
                  MPI_Datatype recvtype,
                  MPI_Comm comm);

/**
 * MPI_ALLGATHERV can be thought of as MPI_GATHERV, except all processes
 * receive the result.
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
 * Inverse operation of MPI_GATHER
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
 * MPI_ALLTOALL is an extension of MPI_ALLGATHER to the case where
 * each process sends distinct data to each of the receivers. The jth
 * block sent from process i is received by process j and is placed in
 * the ith block of recvbuf.
 */
int MPI_Alltoall(void* sendbuf,
		 int sendcount,
		 MPI_Datatype sendtype,
		 void *recvbuf,
		 int recvcount,
		 MPI_Datatype recvType,
		 MPI_Comm comm);

/**
 * MPI_ALLTOALLV adds flexibility to MPI_ALLTOALL in that the location
 * of data for the send is specified by sdispls and the location of
 * the placement of the data on the receive side is specified by
 * rdispls.
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
 */
int MPI_Op_create(MPI_User_function *function,
                  int commute,
                  MPI_Op *op);

/**
 * Marks a user-defined reduction operation for deallocation.
 */
int MPI_Op_free(MPI_Op *op);

/**
 * Combines the elements provided in the input buffer of each process
 * in the group, using the operation op, and returns the combined
 * value in the output buffer of the process with rank root.
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
 */
int MPI_Allreduce(void* sendbuf,
                  void* recvbuf,
                  int count,
                  MPI_Datatype datatype,
                  MPI_Op op,
                  MPI_Comm comm);

/**
 * MPI_REDUCE_SCATTER first does an element-wise reduction on vector
 * of elements in the send buffer defined by sendbuf, count and
 * datatype. Next, the resulting vector of results is split into n
 * disjoint segments, where n is the number of members in the group.
 * Segment i contains recvcounts[i] elements. The ith segment is sent
 * to process i and stored in the receive buffer defined by recvbuf,
 * recvcounts[i] and datatype.
 */
int MPI_Reduce_scatter(void *sendbuf,
                       void *recvbuf,
                       int *recvcounts,
                       MPI_Datatype datatype,
                       MPI_Op op,
                       MPI_Comm comm);

#endif /* MPI_COLLECTIVE_H */
