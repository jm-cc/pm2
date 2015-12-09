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
#include "mpi_bench_generic.h"


#define MIN_DEFAULT     0
#define MAX_DEFAULT     (128 * 1024 * 1024)
#define MULT_DEFAULT    1.4
#define INCR_DEFAULT    0
#define LOOPS_DEFAULT   50
#define LOOPS_REFERENCE 1000
#define PARAM_DEFAULT   -1

const extern struct mpi_bench_s mpi_bench_sendrecv;
const extern struct mpi_bench_s mpi_bench_noncontig;
const extern struct mpi_bench_s mpi_bench_send_overhead;
const extern struct mpi_bench_s mpi_bench_overlap_sender;
const extern struct mpi_bench_s mpi_bench_overlap_recv;
const extern struct mpi_bench_s mpi_bench_overlap_bidir;
const extern struct mpi_bench_s mpi_bench_overlap_sender_noncontig;
const extern struct mpi_bench_s mpi_bench_overlap_send_overhead;
const extern struct mpi_bench_s mpi_bench_overlap_Nload;

int main(int argc, char**argv)
{
  struct mpi_bench_param_s params =
    {
      .start_len   = MIN_DEFAULT,
      .end_len     = MAX_DEFAULT,
      .multiplier  = MULT_DEFAULT,
      .increment   = INCR_DEFAULT,
      .iterations  = LOOPS_DEFAULT,
      .param       = PARAM_DEFAULT
    };

  mpi_bench_init(&argc, &argv, 0);
  params.iterations = LOOPS_REFERENCE;
  mpi_bench_run(&mpi_bench_sendrecv, &params);
  mpi_bench_run(&mpi_bench_noncontig, &params);
  mpi_bench_run(&mpi_bench_send_overhead, &params);
  params.iterations = LOOPS_DEFAULT;
  mpi_bench_run(&mpi_bench_overlap_sender, &params);
  mpi_bench_run(&mpi_bench_overlap_recv, &params);
  mpi_bench_run(&mpi_bench_overlap_bidir, &params);
  mpi_bench_run(&mpi_bench_overlap_sender_noncontig, &params);
  mpi_bench_run(&mpi_bench_overlap_send_overhead, &params);
  mpi_bench_run(&mpi_bench_overlap_Nload, &params);
  mpi_bench_finalize();
  exit(0);
}

