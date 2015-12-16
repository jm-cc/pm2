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

static void mpi_bench_compute_vector_square_omp(void*buf, size_t len)
{
  unsigned char*m = buf;
  size_t i;
#pragma omp parallel for
  for(i = 0; i < len; i++)
    {
      const double v = (double)m[i];
      const double s = v * v;
      m[i] = (unsigned char)s;
    }
}


static void mpi_bench_noise_omp_nocomm_server(void*buf, size_t len)
{
  mpi_bench_compute_vector_square_omp(buf, len);
}

static void mpi_bench_noise_omp_nocomm_client(void*buf, size_t len)
{
  mpi_bench_compute_vector_square_omp(buf, len);
}

const struct mpi_bench_s mpi_bench_noise_omp_nocomm =
  {
    .label      = "mpi_bench_noise_omp_nocomm",
    .name       = "MPI system noise without communication (OpenMP)",
    .rtt        = 0,
    .server     = &mpi_bench_noise_omp_nocomm_server,
    .client     = &mpi_bench_noise_omp_nocomm_client,
    .setparam   = NULL,
    .getparams  = NULL,
  };
