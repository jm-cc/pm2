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


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <values.h>

#include <mpi.h>
#include "mpi_bench_generic.h"


#define MIN_DEFAULT     0
#define MAX_DEFAULT     (512 * 1024 * 1024)
#define MULT_DEFAULT    1.5
#define INCR_DEFAULT    0
#define LOOPS_DEFAULT   25

extern struct mpi_bench_s mpi_bench_overlap_sender;
extern struct mpi_bench_s mpi_bench_overlap_recv;
extern struct mpi_bench_s mpi_bench_overlap_bidir;
extern struct mpi_bench_s mpi_bench_overlap_sender_noncontig;
extern struct mpi_bench_s mpi_bench_overlap_send_overhead;

int main(int argc, char	**argv)
{
  struct mpi_bench_param_s params =
    {
      .start_len   = MIN_DEFAULT,
      .end_len     = MAX_DEFAULT,
      .multiplier  = MULT_DEFAULT,
      .increment   = INCR_DEFAULT,
      .iterations  = LOOPS_DEFAULT
    };

  mpi_bench_init(argc, argv);
  mpi_bench_run(&mpi_bench_overlap_sender, &params);
  mpi_bench_run(&mpi_bench_overlap_recv, &params);
  mpi_bench_run(&mpi_bench_overlap_bidir, &params);
  mpi_bench_run(&mpi_bench_overlap_sender_noncontig, &params);
  mpi_bench_run(&mpi_bench_overlap_send_overhead, &params);
  mpi_bench_finalize();
  exit(0);
}

