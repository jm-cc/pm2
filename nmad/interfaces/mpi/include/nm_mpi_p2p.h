/*
 * NewMadeleine
 * Copyright (C) 2006-2014 (see AUTHORS file)
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


#ifndef NM_MPI_P2P_H
#define NM_MPI_P2P_H

/** \addtogroup mpi_interface */
/* @{ */

/** @name Functions: Point-to-point communications */
/* @{ */

/**
 * Performs a standard-mode, blocking send.
 * @param buffer initial address of send buffer
 * @param count number of elements in send buffer
 * @param datatype datatype of each send buffer element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm communicator
 * @return MPI status
 */
int MPI_Send(const void*buffer,
             int count,
             MPI_Datatype datatype,
             int dest,
             int tag,
             MPI_Comm comm);


int MPI_Issend(const void*buf,
	       int count,
	       MPI_Datatype datatype,
	       int dest,
               int tag,
	       MPI_Comm comm,
	       MPI_Request *request);

/**
 * Posts a standard-mode, non blocking send.
 * @param buffer initial address of send buffer
 * @param count number of elements in send buffer
 * @param datatype datatype of each send buffer element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm communicator
 * @param request pointer to request
 * @return MPI status
 */
int MPI_Isend(const void*buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm,
              MPI_Request *request);

/**
 * Performs a ready-mode, blocking send.
 * @param buffer initial address of send buffer
 * @param count number of elements in send buffer
 * @param datatype datatype of each send buffer element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm communicator
 * @return MPI status
 */
int MPI_Rsend(const void*buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm);

/**
 * Performs a synchronous-mode, blocking send.
 * @param buffer initial address of send buffer
 * @param count number of elements in send buffer
 * @param datatype datatype of each send buffer element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm communicator
 * @return MPI status
 */
int MPI_Ssend(const void* buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm);

int MPI_Bsend(const void *buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm);


/**
 * Performs a standard-mode, blocking receive.
 * @param buffer initial address of receive buffer
 * @param count number of elements in receive buffer
 * @param datatype datatype of each receive buffer element
 * @param source rank of source
 * @param tag message tag
 * @param comm communicator
 * @param status status object
 * @return MPI status
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
 * @param buffer initial address of receive buffer
 * @param count number of elements in receive buffer
 * @param datatype datatype of each receive buffer element
 * @param source rank of source
 * @param tag message tag
 * @param comm communicator
 * @param request pointer to request
 * @return MPI status
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
 * @param sendbuf initial address of send buffer
 * @param sendcount number of elements in send buffer
 * @param sendtype type of elements in send buffer
 * @param dest rank of destination
 * @param sendtag send tag
 * @param recvbuf initial address of receive buffer
 * @param recvcount number of elements in receive buffer
 * @param recvtype type of elements in receive buffer
 * @param source rank of source
 * @param recvtag receive tag
 * @param comm communicator
 * @param status status object
 * @return MPI status
 */
int MPI_Sendrecv(const void *sendbuf,
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

int MPI_Sendrecv_replace(void*buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag,
			 MPI_Comm comm, MPI_Status*status);

/**
 * Returns when the operation identified by request is complete.
 * @param request request
 * @param status status object
 * @return MPI status
 */
int MPI_Wait(MPI_Request *request,
	     MPI_Status *status);

/**
 * Returns when all the operations identified by requests are complete.
 * @param count lists length
 * @param array_of_requests array of requests
 * @param array_of_statuses array of status objects
 * @return MPI status
 */
int MPI_Waitall(int count,
                MPI_Request *array_of_requests,
                MPI_Status *array_of_statuses);

/**
 * Blocks until one of the operations associated with the active
 * requests in the array has completed. If more then one operation is
 * enabled and can terminate, one is arbitrarily chosen. Returns in
 * index the index of that request in the array and returns in status
 * the status of the completing communication. If the request was
 * allocated by a nonblocking communication operation, then it is
 * deallocated and the request handle is set to MPI_REQUEST_NULL.
 * @param count list length
 * @param array_of_requests array of requests
 * @param index index of handle for operation that completed
 * @param status status object
 * @return MPI status
 */
int MPI_Waitany(int count,
                MPI_Request *array_of_requests,
                int *index,
                MPI_Status *status);

int MPI_Waitsome(int incount,
		 MPI_Request *array_of_requests,
		 int *outcount, 
		 int *array_of_indices,
		 MPI_Status *array_of_statuses);

/**
 * Returns flag = true if the operation identified by request is
 * complete.
 * @param request communication request handle
 * @param flag true if operation completed
 * @param status status object
 * @return MPI status
 */
int MPI_Test(MPI_Request *request,
             int *flag,
             MPI_Status *status);

/**
 * Tests for completion of one communication operation associated
 * with requests in the array.
 * @param count list length
 * @param array_of_requests array of request handles
 * @param index index of request handle that completed
 * @param flag true if one has completed
 * @param status status object
 * @return MPI status
 */
int MPI_Testany(int count,
                MPI_Request *array_of_requests,
                int *index,
                int *flag,
                MPI_Status *status);

/**
 * Tests for completion of several communication operations associated
 * with requests in the array.
 * @param count list length
 * @param array_of_requests array of request handles
 * @param outcount is the number of request handle completed
 * @param indices are the indices or request handle completed in array_of_requests
 * @param statuses array of statuses of the completed operations
 * @return MPI status
 */
int MPI_Testsome(int count,
		 MPI_Request *array_of_requests,
		 int *outcount,
		 int *indices,
		 MPI_Status *statuses);
/**
 * Tests for the completion of all previously initiated requests.
 * @param count list length
 * @param array_of_requests array of request handles
 * @param flag  True if all requests have completed; false otherwise (logical)
 * @param statuses array of statuses of the completed operations
 * @return MPI status
 */
int MPI_Testall(int count,
		MPI_Request *array_of_requests,
		int *flag,
		MPI_Status *statuses);


/**
 * Nonblocking operation that returns flag = true if there is a
 * message that can be received and that matches the message envelope
 * specified by source, tag and comm.
 * @param source source rank, or MPI_ANY_SOURCE
 * @param tag tag value or MPI_ANY_TAG
 * @param comm communicator
 * @param flag true if operation has completed
 * @param status status object
 * @return MPI status
 */
int MPI_Iprobe(int source,
               int tag,
               MPI_Comm comm,
               int *flag,
               MPI_Status *status);

/**
 * Blocks and returns only after a message that matches the message
 * envelope specified by source, tag and comm can be received.
 * @param source source rank, or MPI_ANY_SOURCE
 * @param tag tag value or MPI_ANY_TAG
 * @param comm communicator
 * @param status status object
 * @return MPI status
 */
int MPI_Probe(int source,
              int tag,
              MPI_Comm comm,
              MPI_Status *status);

/**
 * Marks for cancellation a pending, nonblocking communication
 * operation (send or receive).
 * @param request communication request handle
 * @return MPI status
 */
int MPI_Cancel(MPI_Request *request);

/**
 * Marks the request object for deallocation and set request to
 * MPI_REQUEST_NULL. An ongoing communication that is associated with
 * the request will be allowed to complete. The request will be
 * deallocated only after its completion.
 * @param request communication request handle
 * @return MPI status
 */
int MPI_Request_free(MPI_Request *request);

/**
 * Computes the number of entries received.
 * @param status return status of receive operation
 * @param datatype datatype of each receive buffer entry
 * @param count number of received entries
 * @return MPI status
 */
int MPI_Get_count(MPI_Status *status,
                  MPI_Datatype datatype,
                  int *count);

int MPI_Get_elements(const MPI_Status*status,
		     MPI_Datatype datatype,
		     int *count);

int MPI_Get_elements_x(const MPI_Status*status,
		       MPI_Datatype datatype,
		       MPI_Count*count);

int MPI_Test_cancelled(const MPI_Status*status, int*flag);
  

/* @}*/
/* @}*/

#endif /* NM_MPI_P2P_H */
