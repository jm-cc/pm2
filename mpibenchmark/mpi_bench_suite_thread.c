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
const extern struct mpi_bench_s mpi_bench_thread_1toN_rr;
const extern struct mpi_bench_s mpi_bench_thread_1toN_single;

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

  mpi_bench_init(&argc, &argv, 1);
  params.iterations = LOOPS_REFERENCE;
  mpi_bench_run(&mpi_bench_sendrecv, &params);
  params.iterations = LOOPS_DEFAULT;
  mpi_bench_run(&mpi_bench_thread_1toN_rr, &params);
  mpi_bench_run(&mpi_bench_thread_1toN_single, &params);
  mpi_bench_finalize();
  exit(0);
}

