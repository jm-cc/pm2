/*
 * NewMadeleine
 * Copyright (C) 2015 (see AUTHORS file)
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

#include "mpi_bench_generic.h"

static void*sparse_buf = NULL;
static MPI_Datatype dtype = MPI_DATATYPE_NULL;

#define DEFAULT_BLOCKSIZE 8

static int mpi_bench_sv_blocksize(void)
{
  static int blocksize = 0;
  if(blocksize == 0)
    {
      const char*s_blocksize = getenv("BLOCKSIZE");
      if(s_blocksize != NULL)
	{
	  blocksize = atoi(s_blocksize);
	}
      else
	{
	  blocksize = DEFAULT_BLOCKSIZE;
	}
      fprintf(stderr, "# madmpi: sparse vector block size = %d\n", blocksize);
    }
  return blocksize;

}

static void mpi_bench_sv_init(void*buf, size_t len)
{
  const int blocksize = mpi_bench_sv_blocksize();
  sparse_buf = realloc(sparse_buf, len * 2 + blocksize);
  int rc = MPI_Type_vector(len / blocksize, blocksize, 2 * blocksize, MPI_CHAR, &dtype);
  MPI_Type_commit(&dtype);
}

static void mpi_bench_sv_server(void*buf, size_t len)
{
  MPI_Recv(sparse_buf, 1, dtype, mpi_bench_common.peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Send(sparse_buf, 1, dtype, mpi_bench_common.peer, 0, MPI_COMM_WORLD);
}

static void mpi_bench_sv_client(void*buf, size_t len)
{
  MPI_Send(sparse_buf, 1, dtype, mpi_bench_common.peer, 0, MPI_COMM_WORLD);
  MPI_Recv(sparse_buf, 1, dtype, mpi_bench_common.peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

const struct mpi_bench_s mpi_bench_sparse_vector =
  {
    .name   = "MPI sparse vector",
    .init   = &mpi_bench_sv_init,
    .server = &mpi_bench_sv_server,
    .client = &mpi_bench_sv_client
  };

