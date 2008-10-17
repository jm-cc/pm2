/*
 * mpi_para_ping.c
 * ===============
 *
 * Copyright (C) 2006 Olivier Aumage
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
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>

// Setup
//......................
static const int param_warmup            = 100;
static const int param_nb_samples        = 1000;
static const int param_nb_tests          = 5;
static const int param_log_only_master   = 0;

// Types
//......................
typedef struct s_ping_result
{
  double	latency;
  double	bandwidth_mbit;
  double	bandwidth_mbyte;
} ping_result_t, *p_ping_result_t;

// Static variables
//......................


static int	comm_rank	= -1;
static int	comm_size	= -1;
static char	host_name[1024]	= "";

// Functions
//......................

int
main(int    argc,
     char **argv)
{
  int log, i;
  int ping_side;
  int rank_dst;
  void *buffer = NULL;

  MPI_Init(&argc,&argv);

  if (gethostname(host_name, 1023) < 0) {
    perror("gethostname");
    exit(1);
  }

  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);

  log = (!comm_rank || !param_log_only_master);

  if (!param_log_only_master) {
    fprintf(stdout, "(%s): My rank is %d\n", host_name, comm_rank);
  }

  if (comm_rank == 0 && log) {
    fprintf(stdout, "src|dst|size        |latency\n");
  }

  if (comm_size & 1) {
    if (log)
      fprintf(stdout, "This program requires an even configuration size, aborting...\n");

    goto out;
  }

  ping_side	= !(comm_rank & 1);
  rank_dst	= ping_side?(comm_rank | 1):(comm_rank & ~1);

  /* Warmup */
  MPI_Barrier(MPI_COMM_WORLD);
  for(i=0 ; i<param_warmup ; i++) {
    if (ping_side) {
      MPI_Send(buffer, 0, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD);
      MPI_Recv(buffer, 0, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
      MPI_Recv(buffer, 0, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Send(buffer, 0, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD);
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);

  /* Test */
  {
    if (ping_side) {
      int		nb_tests	= param_nb_tests;
      double		sum		= 0.0;
      double		t1;
      double		t2;
      double		lat;

      sum = 0;

      while (nb_tests--) {
        int nb_samples = param_nb_samples;

        MPI_Barrier(MPI_COMM_WORLD);
        t1 = MPI_Wtime();
        while (nb_samples--) {
          MPI_Send(buffer, 0, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD);
          MPI_Recv(buffer, 0, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        t2 = MPI_Wtime();
        MPI_Barrier(MPI_COMM_WORLD);

        sum += t2 - t1;
      }

      sum *= 1e6;
      lat		= sum / param_nb_tests / param_nb_samples / 2;

      if (log)
        fprintf(stdout, "%3d %3d 0 %12.3f\n", comm_rank, rank_dst, lat);

    } else {
      int		nb_tests	= param_nb_tests;
      double		sum		= 0.0;
      double		t1;
      double		t2;
      double		lat;

      sum = 0;

      while (nb_tests--) {
        int nb_samples = param_nb_samples;

        MPI_Barrier(MPI_COMM_WORLD);
        t1 = MPI_Wtime();
        while (nb_samples--) {
          MPI_Recv(buffer, 0, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          MPI_Send(buffer, 0, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        t2 = MPI_Wtime();
        sum += t2 - t1;
      }

      sum *= 1e6;
      lat		= sum / param_nb_tests / param_nb_samples / 2;
      if (log && ping_side)
        fprintf(stdout, "%3d %3d %12.3f\n",
                comm_rank, rank_dst, lat);

    }
  }

  if (log)
    fprintf(stdout, "Exiting\n");

 out:
  MPI_Finalize();

  return 0;
}
