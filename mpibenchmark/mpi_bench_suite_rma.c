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

const extern struct mpi_bench_s mpi_bench_rma_pscw;
const extern struct mpi_bench_s mpi_bench_rma_fence;
const extern struct mpi_bench_s mpi_bench_rma_put_passive;
const extern struct mpi_bench_s mpi_bench_rma_get_passive;
const extern struct mpi_bench_s mpi_bench_rma_accumulate_passive;
const extern struct mpi_bench_s mpi_bench_rma_get_accumulate_passive;
const extern struct mpi_bench_s mpi_bench_rma_put_active;
const extern struct mpi_bench_s mpi_bench_rma_get_active;
const extern struct mpi_bench_s mpi_bench_rma_accumulate_active;
const extern struct mpi_bench_s mpi_bench_rma_get_accumulate_active;
const extern struct mpi_bench_s mpi_bench_rma_put_overhead;
const extern struct mpi_bench_s mpi_bench_rma_accumulate_overhead;
const extern struct mpi_bench_s mpi_bench_rma_put_noncontig;
const extern struct mpi_bench_s mpi_bench_rma_get_noncontig;
const extern struct mpi_bench_s mpi_bench_rma_accumulate_noncontig;
const extern struct mpi_bench_s mpi_bench_rma_get_accumulate_noncontig;

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
  mpi_bench_run(&mpi_bench_rma_pscw, &params);
  mpi_bench_run(&mpi_bench_rma_fence, &params);
  mpi_bench_run(&mpi_bench_rma_put_passive, &params);
  mpi_bench_run(&mpi_bench_rma_get_passive, &params);
  mpi_bench_run(&mpi_bench_rma_accumulate_passive, &params);
  mpi_bench_run(&mpi_bench_rma_get_accumulate_passive, &params);
  mpi_bench_run(&mpi_bench_rma_put_active, &params);
  mpi_bench_run(&mpi_bench_rma_get_active, &params);
  mpi_bench_run(&mpi_bench_rma_accumulate_active, &params);
  mpi_bench_run(&mpi_bench_rma_get_accumulate_active, &params);
  mpi_bench_run(&mpi_bench_rma_put_overhead, &params);
  mpi_bench_run(&mpi_bench_rma_accumulate_overhead, &params);
  mpi_bench_run(&mpi_bench_rma_put_noncontig, &params);
  mpi_bench_run(&mpi_bench_rma_get_noncontig, &params);
  mpi_bench_run(&mpi_bench_rma_accumulate_noncontig, &params);
  mpi_bench_run(&mpi_bench_rma_get_accumulate_noncontig, &params);
  
  mpi_bench_finalize();
  exit(EXIT_SUCCESS);
}

