/*
 * NewMadeleine
 * Copyright (C) 2016 (see AUTHORS file)
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

const extern struct mpi_bench_s mpi_bench_sendrecv;
const extern struct mpi_bench_s mpi_bench_rma_put_active;
const extern struct mpi_bench_s mpi_bench_rma_put_passive;
const extern struct mpi_bench_s mpi_bench_rma_put_noncontig;
const extern struct mpi_bench_s mpi_bench_rma_get_active;
const extern struct mpi_bench_s mpi_bench_rma_accumulate_active;
const extern struct mpi_bench_s mpi_bench_rmaoverlap_put_origin;
const extern struct mpi_bench_s mpi_bench_rmaoverlap_put_target;
const extern struct mpi_bench_s mpi_bench_rmaoverlap_put_passive;
const extern struct mpi_bench_s mpi_bench_rmaoverlap_put_noncontig;
const extern struct mpi_bench_s mpi_bench_rmaoverlap_get;
const extern struct mpi_bench_s mpi_bench_rmaoverlap_accumulate;

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
  mpi_bench_run(&mpi_bench_sendrecv, &params);
  mpi_bench_run(&mpi_bench_rma_put_active, &params);
  mpi_bench_run(&mpi_bench_rma_put_passive, &params);
  mpi_bench_run(&mpi_bench_rma_put_noncontig, &params);
  mpi_bench_run(&mpi_bench_rma_get_active, &params);
  mpi_bench_run(&mpi_bench_rma_accumulate_active, &params);
  params.iterations = LOOPS_DEFAULT_PARAM;
  mpi_bench_run(&mpi_bench_rmaoverlap_put_origin, &params);
  mpi_bench_run(&mpi_bench_rmaoverlap_put_target, &params);
  mpi_bench_run(&mpi_bench_rmaoverlap_put_passive, &params);
  mpi_bench_run(&mpi_bench_rmaoverlap_put_noncontig, &params);
  mpi_bench_run(&mpi_bench_rmaoverlap_get, &params);
  mpi_bench_run(&mpi_bench_rmaoverlap_accumulate, &params);
  mpi_bench_finalize();
  exit(0);
}

