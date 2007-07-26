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
 * mpi_persistent.h
 * ================
 */

#ifndef MPI_PERSISTENT_H
#define MPI_PERSISTENT_H

/** \addtogroup mpi_interface */
/* @{ */

/**
 * Creates a persistent communication request for a standard mode send
 * operation, and binds to it all the arguments of a send operation.
 * @param buf initial address of send buffer
 * @param count number of elements sent
 * @param datatype type of each element
 * @param dest rank of destination
 * @param tag message tag
 * @param comm communicator
 * @param request communication request handle
 * @return MPI status
 */
int MPI_Send_init(void* buf,
                  int count,
                  MPI_Datatype datatype,
                  int dest,
                  int tag,
                  MPI_Comm comm,
                  MPI_Request *request);

/**
 * Creates a persistent communication request for a receive operation.
 * @param buf initial address of receive buffer
 * @param count number of elements received
 * @param datatype type of each element
 * @param source rank of source or MPI_ANY_SOURCE
 * @param tag message tag or MPI_ANY_TAG
 * @param comm communicator
 * @param request communication request handle
 * @return MPI status
 */
int MPI_Recv_init(void* buf,
                  int count,
                  MPI_Datatype datatype,
                  int source,
                  int tag,
                  MPI_Comm comm,
                  MPI_Request *request);

/**
 * Initiates a persistent request. The associated request should be
 * inactive. The request becomes active once the call is made. If the
 * request is for a send with ready mode, then a matching receive
 * should be posted before the call is made. The communication buffer
 * should not be accessed after the call, and until the operation
 * completes.
 * @param request communication request handle
 * @return MPI status
 */
int MPI_Start(MPI_Request *request);

/**
 * Start all communications associated with requests in
 * array_of_requests.
 * @param count list length
 * @param array_of_requests array of requests
 * @return MPI status
 */
int MPI_Startall(int count,
                 MPI_Request *array_of_requests);

/* @}*/

#endif /* MPI_PERSISTENT_H */
