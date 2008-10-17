/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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

//#include <fcntl.h>
//#include <execinfo.h>

#include <mpi.h>

#define NB_PACKS 4
#define MIN     (NB_PACKS * 4)
#define MAX     (8 * 1024 * 1024)
#define LOOPS   1000


static int	comm_rank	= -1;
static int	comm_size	= -1;
static char	host_name[1024]	= "";
static const int param_log_only_master   = 0;

static __inline__
uint32_t _next(uint32_t len)
{
  if(!len)
    return 4;
  else if(len < 32)
    return len + 4;
  else if(len < 1024)
    return len + 32;
  else
    return len << 1;
}

int
main(int	  argc,
     char	**argv) {
  int log;
  int ping_side;
  int rank_dst;
  char	*buf		= NULL;
  int len, k;



  /* ------------------- Initialisation ------------------ */
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
    fprintf(stdout, "The configuration size is %d\n", comm_size);
    fprintf(stdout, " src | dst |     size     | nb_packs |     latency     |     10^6 B/s   |   MB/s    |\n");
  }

  if (comm_size & 1) {
    if (log)
      fprintf(stdout, "This program requires an even configuration size, aborting...\n");

    goto out;
  }

  ping_side	= !(comm_rank & 1);
  rank_dst	= ping_side?(comm_rank | 1):(comm_rank & ~1);

  if (ping_side) {
    fprintf(stdout, "(%d): ping with %d\n", comm_rank, rank_dst);
  } else {
    if (log)
      fprintf(stdout, "(%d): pong with %d\n", comm_rank, rank_dst);
  }

  /* ------------------------------------------------- */
  /* ------------------- Multi-ping ------------------ */

  buf = malloc(MAX);

  if (!ping_side) {
    /* server */

    memset(buf, 0, MAX);

    for(len = MIN; len <= MAX; len = _next(len)) {

      unsigned long n, chunk = len / NB_PACKS;

      MPI_Barrier(MPI_COMM_WORLD);

      for(k = 0; k < LOOPS; k++) {
        /* Phase de réception */
        for(n = 0; n < NB_PACKS; n++)
          MPI_Recv(buf + n * chunk,  chunk, MPI_CHAR,
                   rank_dst, 0, MPI_COMM_WORLD, NULL);

        /* Phase d'émission */
        for(n = 0; n < NB_PACKS; n++)
          MPI_Send(buf + n * chunk, chunk, MPI_CHAR,
                   rank_dst, 0, MPI_COMM_WORLD);
      }
    }

  } else {
    /* client */

    double t1, t2, sum, lat, bw_million_byte, bw_mbyte;

    memset(buf, 'a', MAX);

    for(len = MIN; len <= MAX; len = _next(len)) {
      unsigned long n, chunk = len / NB_PACKS;

      MPI_Barrier(MPI_COMM_WORLD);

      t1 = MPI_Wtime();

      for(k = 0; k < LOOPS; k++) {
        /* Phase d'émission */
        for(n = 0; n < NB_PACKS; n++)
          MPI_Send(buf + n * chunk, chunk, MPI_CHAR,
                   rank_dst, 0, MPI_COMM_WORLD);

        /* Phase de réception */
        for(n = 0; n < NB_PACKS; n++)
          MPI_Recv(buf + n * chunk,  chunk, MPI_CHAR,
                   rank_dst, 0, MPI_COMM_WORLD, NULL);
      }

      t2 = MPI_Wtime();
      sum = t2 - t1;
      sum *= 1e6;

      lat	      = sum / (2 * LOOPS);
      bw_million_byte = len * (LOOPS / (sum / 2));
      bw_mbyte        = bw_million_byte / 1.048576;

      printf("%5d %5d %12d %9d  %12.3f\t %8.3f \t %8.3f\n",
             comm_rank, rank_dst, len, NB_PACKS, lat, bw_million_byte, bw_mbyte);
    }
  }

 out:
  return 0;
}
