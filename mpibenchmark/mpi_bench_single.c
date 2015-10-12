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
#define MULT_DEFAULT    2
#define INCR_DEFAULT    0
#define LOOPS_DEFAULT   1000

static void usage_ping(void)
{
  fprintf(stderr, "-S start_len - starting length [%d]\n", MIN_DEFAULT);
  fprintf(stderr, "-E end_len - ending length [%d]\n", MAX_DEFAULT);
  fprintf(stderr, "-I incr - length increment [%d]\n", INCR_DEFAULT);
  fprintf(stderr, "-M mult - length multiplier [%d]\n", MULT_DEFAULT);
  fprintf(stderr, "\tNext(0)      = 1+increment\n");
  fprintf(stderr, "\tNext(length) = length*multiplier+increment\n");
  fprintf(stderr, "-N iterations - iterations per length [%d]\n", LOOPS_DEFAULT);
}

/* variable defined in stub */
const extern struct mpi_bench_s*mpi_bench_default;

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

  mpi_bench_init(argc, argv, mpi_bench_default->threads);
  
  if (argc > 1 && strcmp(argv[1], "--help") == 0)
    {
      usage_ping();
      MPI_Abort(mpi_bench_common.comm, -1);
    }
  
  int i;
  for(i = 1; i < argc; i += 2)
    {
      if(strcmp(argv[i], "-S") == 0)
	{
	  params.start_len = atoll(argv[i+1]);
	}
      else if(strcmp(argv[i], "-E") == 0)
	{
	  params.end_len = atoll(argv[i+1]);
	}
      else if(strcmp(argv[i], "-I") == 0)
	{
	  params.increment = atoll(argv[i+1]);
	}
      else if(strcmp(argv[i], "-M") == 0)
	{
	  params.multiplier = atof(argv[i+1]);
	}
      else if(strcmp(argv[i], "-N") == 0)
	{
	  params.iterations = atoi(argv[i+1]);
	}
      else
	{
	  fprintf(stderr, "%s: illegal argument %s\n", argv[0], argv[i]);
	  usage_ping();
	  MPI_Abort(mpi_bench_common.comm, -1);
	}
    }
  
  mpi_bench_run(mpi_bench_default, &params);

  mpi_bench_finalize();
  exit(0);
}

