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
 * mpi_pt2pt.h
 * ===========
 */

#ifndef MPI_PT2PT_H
#define MPI_PT2PT_H

/**
 * Performs a extended send
 */
int MPI_Esend(void *buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Communication_Mode is_completed,
              MPI_Comm comm,
              MPI_Request *request);

/**
 * Performs a standard-mode, blocking send.
 */
int MPI_Send(void *buffer,
             int count,
             MPI_Datatype datatype,
             int dest,
             int tag,
             MPI_Comm comm);

/**
 * Posts a standard-mode, non blocking send.
 */
int MPI_Isend(void *buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm,
              MPI_Request *request);

/**
 * Performs a ready-mode, blocking send.
 */
int MPI_Rsend(void* buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm);

/**
 * Performs a synchronous-mode, blocking send.
 */
int MPI_Ssend(void* buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm);

/**
 * Performs a standard-mode, blocking receive.
 */
int MPI_Recv(void *buffer,
             int count,
             MPI_Datatype datatype,
             int source,
             int tag,
             MPI_Comm comm,
             MPI_Status *status);

/**
 * Posts a nonblocking receive.
 */
int MPI_Irecv(void* buffer,
              int count,
              MPI_Datatype datatype,
              int source,
              int tag,
              MPI_Comm comm,
              MPI_Request *request);

/**
 * Executes a blocking send and receive operation.
 */
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
                 MPI_Status *status);

/**
 * Returns when the operation identified by request is complete.
 */
int MPI_Wait(MPI_Request *request,
	     MPI_Status *status);

/**
 * Returns when all the operations identified by requests are complete.
 */
int MPI_Waitall(int count,
                MPI_Request *request,
                MPI_Status *status);

/**
 * Blocks until one of the operations associated with the active
 * requests in the array has completed. If more then one operation is
 * enabled and can terminate, one is arbitrarily chosen. Returns in
 * index the index of that request in the array and returns in status
 * the status of the completing communication. If the request was
 * allocated by a nonblocking communication operation, then it is
 * deallocated and the request handle is set to MPI_REQUEST_NULL.
 */
int MPI_Waitany(int count,
                MPI_Request *array_of_requests,
                int *index,
                MPI_Status *status);

/**
 * Returns flag = true if the operation identified by request is
 * complete.
 */
int MPI_Test(MPI_Request *request,
             int *flag,
             MPI_Status *status);

/**
 * Tests for completion of the communication operations associated
 * with requests in the array.
 */
int MPI_Testany(int count,
                MPI_Request *array_of_requests,
                int *index,
                int *flag,
                MPI_Status *status);

/**
 * Nonblocking operation that returns flag = true if there is a
 * message that can be received and that matches the message envelope
 * specified by source, tag and comm.
 */
int MPI_Iprobe(int source,
               int tag,
               MPI_Comm comm,
               int *flag,
               MPI_Status *status);

/**
 * Blocks and returns only after a message that matches the message
 * envelope specified by source, tag and comm can be received.
 */
int MPI_Probe(int source,
              int tag,
              MPI_Comm comm,
              MPI_Status *status);

/**
 * Marks for cancellation a pending, nonblocking communication
 * operation (send or receive).
 */
int MPI_Cancel(MPI_Request *request);

/**
 * Marks the request object for deallocation and set request to
 * MPI_REQUEST_NULL. An ongoing communication that is associated with
 * the request will be allowed to complete. The request will be
 * deallocated only after its completion.
 */
int MPI_Request_free(MPI_Request *request);

/**
 * Computes the number of entries received.
 */
int MPI_Get_count(MPI_Status *status,
                  MPI_Datatype datatype,
                  int *count);

/**
 * This function does not belong to the MPI standard: compares two
 * request handles
 */
int MPI_Request_is_equal(MPI_Request request1, MPI_Request request2);

#endif /* MPI_PT2PT_H */
