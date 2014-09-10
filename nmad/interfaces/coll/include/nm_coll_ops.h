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

#include <nm_public.h>
#include <nm_sendrecv_interface.h>
#include <Padico/Puk.h>

/*
  int MPI_Barrier(MPI_Comm comm)

  int MPI_Bcast(void* buffer, int count, MPI_Datatype datatype,
     int root, MPI_Comm comm)
  int MPI_Gather(void* sbuf, int scount, MPI_Datatype stype,
     void* rbuf, int rcount, MPI_Datatype rtype,
     int root, MPI_Comm comm )
  int MPI_Scatter(void* sbuf, int scount, MPI_Datatype stype,
     void* rbuf, int rcount, MPI_Datatype rtype,
     int root, MPI_Comm comm)
  int MPI_Allgather(void* sbuf, int scount, MPI_Datatype stype,
     void* rbuf, int rcount, MPI_Datatype rtype, MPI_Comm comm)
  int MPI_Allgatherv(void* sbuf, int scounts, MPI_Datatype stype,
     void* rbuf, int* rcounts, int* displs, MPI_Datatype rtype,
     MPI_Comm comm)
  int MPI_Alltoall(void* sbuf, int scount, MPI_Datatype stype,
     void* rbuf, int rcount, MPI_Datatype rtype, MPI_Comm comm)
  int MPI_Alltoallv(void* sbuf, int* scounts, int* sdispls,
     MPI_Datatype stype, void* rbuf, int* rcounts, int* rdispls,
     MPI_Datatype rtype, MPI_Comm comm)

  int MPI_Reduce(void* sbuf, void* rbuf, int count,
     MPI_Datatype stype, MPI_Op op, int root, MPI_Comm comm)
  int MPI_Allreduce(void* sbuf, void* rbuf, int count
     MPI_Datatype stype, MPI_Op op, MPI_Comm comm)
  int MPI_Reduce_scatter(void* sbuf, void* rbuf, int* rcounts,
     MPI_Datatype stype, MPI_Op op, MPI_Comm comm)
  int MPI_Scan(void* sbuf, void* rbuf, int count,
     MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)

 */
/*
  MPI_Comm_size	 returns number of processes in communicator's group
  MPI_Comm_rank	returns rank of calling process in communicator's group
  MPI_Comm_compare	compares two communicators
  MPI_Comm_dup	duplicates a communicator
  MPI_Comm_create	creates a new communicator for a group
  MPI_Comm_split	splits a communicator into multiple, non-overlapping communicators
  MPI_Comm_free	marks a communicator for deallocation
*/


extern void nm_coll_barrier(nm_comm_t comm, nm_tag_t tag);

extern void nm_coll_bcast(nm_comm_t comm, int root, void*buffer, nm_len_t len, nm_tag_t tag);

extern void nm_coll_scatter(nm_comm_t comm, int root, const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag);

extern void nm_coll_gather(nm_comm_t comm, int root, const void*sbuf, nm_len_t slen, void*rbuf, nm_len_t rlen, nm_tag_t tag);

