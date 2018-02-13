/*
 * NewMadeleine
 * Copyright (C) 2017-2018 (see AUTHORS file)
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

const extern struct mpi_bench_s mpi_bench_reqs_burst;
const extern struct mpi_bench_s mpi_bench_reqs_tags;
const extern struct mpi_bench_s mpi_bench_reqs_shuffle;
const extern struct mpi_bench_s mpi_bench_reqs_test;
const extern struct mpi_bench_s mpi_bench_reqs_any;

int main(int argc, char**argv)
{
  struct mpi_bench_param_s params =
    {
      .start_len   = 1,
      .end_len     = 500000,
      .multiplier  = MULT_DEFAULT,
      .increment   = INCR_DEFAULT,
      .iterations  = LOOPS_DEFAULT,
      .param       = PARAM_DEFAULT
    };

  mpi_bench_init(&argc, &argv, 0);
  mpi_bench_run(&mpi_bench_reqs_burst, &params);
  mpi_bench_run(&mpi_bench_reqs_tags, &params);
  mpi_bench_run(&mpi_bench_reqs_shuffle, &params);
  mpi_bench_run(&mpi_bench_reqs_test, &params);
  mpi_bench_run(&mpi_bench_reqs_any, &params);
  mpi_bench_finalize();
  exit(0);
}

