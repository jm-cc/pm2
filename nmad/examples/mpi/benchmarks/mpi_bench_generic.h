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


#include <mpi.h>
#include <tbx.h>

#ifndef MPI_BENCH_GENERIC_H
#define MPI_BENCH_GENERIC_H

struct mpi_bench_s
{
  const char*name;
  void (*server)(void*buf, size_t len);
  void (*client)(void*buf, size_t len);
  void (*init)(void*buf, size_t len);
};

/* ********************************************************* */

struct mpi_bench_common_s
{
  int size;
  int self, peer;
  MPI_Comm comm;
};

extern struct mpi_bench_common_s mpi_bench_common;


#endif /* MPI_BENCH_GENERIC_H */

