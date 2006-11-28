/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2001 "the PM2 team" (see AUTHORS file)
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

/* User combination function */
typedef void MPI_User_function(void *, void *, int *, MPI_Datatype *);

int MPI_Barrier(MPI_Comm comm);

int MPI_Bcast(void* buffer,
              int count,
              MPI_Datatype datatype,
              int root,
              MPI_Comm comm);

int MPI_Op_create(MPI_User_function *function,
                  int commute,
                  MPI_Op *op);

int MPI_Op_free(MPI_Op *op);

int MPI_Reduce(void* sendbuf,
               void* recvbuf,
               int count,
               MPI_Datatype datatype,
               MPI_Op op,
               int root,
               MPI_Comm comm);

int MPI_Allreduce(void* sendbuf,
                  void* recvbuf,
                  int count,
                  MPI_Datatype datatype,
                  MPI_Op op,
                  MPI_Comm comm);

#endif /* MPI_COLLECTIVE_H */
