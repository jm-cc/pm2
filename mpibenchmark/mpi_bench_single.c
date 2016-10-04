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

static void mpi_bench_usage(char**argv)
{
  fprintf(stderr, "Usage: %s [<option> <...>]\n", argv[0]);
  fprintf(stderr, "  -S START     set start length to START [default %d]\n", MIN_DEFAULT);
  fprintf(stderr, "  -E END       set end length to END [default %d]\n", MAX_DEFAULT);
  fprintf(stderr, "  -I INCR      set length increment to INCR [default %d]\n", INCR_DEFAULT);
  fprintf(stderr, "  -M MULT      set length multiplier to MULT [default %g]\n", MULT_DEFAULT);
  fprintf(stderr, "\tNext(0)      = 1+increment\n");
  fprintf(stderr, "\tNext(length) = length*multiplier+increment\n");
  fprintf(stderr, "  -N ITER      set iterations per length to ITER [default %d]\n", LOOPS_DEFAULT);
  fprintf(stderr, "  -P PARAM     set benchmark parameter to fixed value PARAM\n");
}

/* variable defined in stub */
const extern struct mpi_bench_s*mpi_bench_default;

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

  if(mpi_bench_default->setparam != NULL)
    {
      /* shorter loop count by default for parameterized benchmarks */
      params.iterations = LOOPS_DEFAULT_PARAM;
    }

  if (argc > 1 && strcmp(argv[1], "--help") == 0)
    {
      mpi_bench_usage(argv);
      exit(0);
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
	  params.iterations = atoll(argv[i+1]);
	}
      else if(strcmp(argv[i], "-P") == 0)
	{
	  params.param = atoi(argv[i+1]);
	}
      else
	{
	  fprintf(stderr, "%s: illegal argument %s\n", argv[0], argv[i]);
	  mpi_bench_usage(argv);
	  MPI_Abort(mpi_bench_common.comm, -1);
	}
    }

  mpi_bench_init(&argc, &argv, mpi_bench_default->threads);
    
  mpi_bench_run(mpi_bench_default, &params);

  mpi_bench_finalize();
  exit(0);
}

