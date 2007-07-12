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

int MPI_Esend(void *buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Communication_Mode is_completed,
              MPI_Comm comm,
              MPI_Request *request);

int MPI_Send(void *buffer,
             int count,
             MPI_Datatype datatype,
             int dest,
             int tag,
             MPI_Comm comm);

int MPI_Recv(void *buffer,
             int count,
             MPI_Datatype datatype,
             int source,
             int tag,
             MPI_Comm comm,
             MPI_Status *status);

int MPI_Isend(void *buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm,
              MPI_Request *request);

int MPI_Rsend(void* buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm);

int MPI_Ssend(void* buffer,
              int count,
              MPI_Datatype datatype,
              int dest,
              int tag,
              MPI_Comm comm);

int MPI_Irecv(void* buffer,
              int count,
              MPI_Datatype datatype,
              int source,
              int tag,
              MPI_Comm comm,
              MPI_Request *request);

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

int MPI_Wait(MPI_Request *request,
	     MPI_Status *status);

int MPI_Waitall(int count,
                MPI_Request *request,
                MPI_Status *status);

int MPI_Waitany(int count,
                MPI_Request *array_of_requests,
                int *index,
                MPI_Status *status);

int MPI_Test(MPI_Request *request,
             int *flag,
             MPI_Status *status);

int MPI_Testany(int count,
                MPI_Request *array_of_requests,
                int *index,
                int *flag,
                MPI_Status *status);

int MPI_Iprobe(int source,
               int tag,
               MPI_Comm comm,
               int *flag,
               MPI_Status *status);

int MPI_Probe(int source,
              int tag,
              MPI_Comm comm,
              MPI_Status *status);

int MPI_Cancel(MPI_Request *request);

int MPI_Get_count(MPI_Status *status,
                  MPI_Datatype datatype,
                  int *count);

int MPI_Request_is_equal(MPI_Request request1, MPI_Request request2);

#endif /* MPI_PT2PT_H */
