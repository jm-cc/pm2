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
#include <mpi.h>

#define NB_LINES 10
#define SMALL 64
#define LARGE (256 * 1024)

#define NB_TESTS     100

int main(int argc, char **argv) {
  MPI_Request s_request[NB_LINES * 2];
  MPI_Request r_request[NB_LINES * 2];

  int	comm_rank	= -1;
  int	comm_size	= -1;
  char	host_name[1024]	= "";
  int   param_log_only_master   = 0;

  int    log;
  int    ping_side;
  int    rank_dst;
  int    i, j, k, test;
  double t1, t2, sum;

  char	buffer[(SMALL+LARGE) * NB_LINES];
  char	data[(SMALL+LARGE) * NB_LINES];
  char	data2[(SMALL+LARGE) * NB_LINES];

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
    fprintf(stdout, "src  | dest  | type	     | size    | blocks | time");
  }

  if (comm_size & 1) {
    if (log)
      fprintf(stdout, "This program requires an even configuration size, aborting...\n");
    MPI_Finalize();
    exit(0);
  }

  ping_side	= !(comm_rank & 1);
  rank_dst	= ping_side?(comm_rank | 1):(comm_rank & ~1);

  /* ------------------------------------------------- */
  /* ------------------- Multi-ping ------------------ */

  for(i = 0; i < NB_LINES; i++) {
    sum = 0;
    for(test=0 ; test<NB_TESTS ; test++) {
      if (!ping_side) {
        /* server */
        for(j = 0; j <= i; j++){
          MPI_Irecv(buffer + j * (SMALL + LARGE),  SMALL, MPI_CHAR,
                    rank_dst, 0, MPI_COMM_WORLD, &(r_request[j*2]));
          MPI_Irecv(buffer + j *(SMALL + LARGE) + SMALL, LARGE, MPI_CHAR,
                    rank_dst, 0, MPI_COMM_WORLD, &(r_request[j*2 + 1]));
        }
        for(j = 0; j < (i+1) * 2; j++){
          MPI_Wait(&(r_request[j]), MPI_STATUS_IGNORE);
        }
        for(j = 0; j <= i; j++){
          MPI_Isend(buffer + j * (SMALL+LARGE), SMALL, MPI_CHAR,
                    rank_dst, 0, MPI_COMM_WORLD, &(s_request[j*2]));
          MPI_Isend(buffer + j *(SMALL+LARGE) + SMALL,  LARGE, MPI_CHAR,
                    rank_dst, 0, MPI_COMM_WORLD, &(s_request[j*2 + 1]));
        }
        for(j = 0; j < (i+1) * 2; j++){
          MPI_Wait(&(s_request[j]), MPI_STATUS_IGNORE);
        }
      }
      else {
        for(k=0 ; k<(SMALL+LARGE) * i ; k++) {
          data[k] = 'a'+(i%26);
        }
        /* client */
        t1 = MPI_Wtime();
        for(j = 0; j <= i; j++){
          MPI_Isend(data + j * (SMALL+LARGE), SMALL, MPI_CHAR,
                    rank_dst, 0, MPI_COMM_WORLD, &(s_request[j*2]));
          MPI_Isend(data + j * (SMALL+LARGE) + SMALL,  LARGE, MPI_CHAR,
                    rank_dst, 0, MPI_COMM_WORLD, &(s_request[j*2 + 1]));
        }
        for(j = 0; j < (i+1) * 2; j++){
          MPI_Wait(&(s_request[j]), MPI_STATUS_IGNORE);
        }
        for(j = 0; j <= i; j++){
          MPI_Irecv(data2 + j * (SMALL + LARGE),  SMALL, MPI_CHAR,
                    rank_dst, 0, MPI_COMM_WORLD, &(r_request[j*2]));
          MPI_Irecv(data2 + j *(SMALL + LARGE) + SMALL, LARGE, MPI_CHAR,
                    rank_dst, 0, MPI_COMM_WORLD, &(r_request[j*2 + 1]));
        }
        for(j = 0; j < (i+1) * 2; j++) {
          MPI_Wait(&(r_request[j]), MPI_STATUS_IGNORE);
        }
        t2 = MPI_Wtime();
        sum += (t2 - t1) * 1e6;
      }
    }
    if (ping_side) {
      int len = (i+1) * (SMALL + LARGE);
      int nb_blocks = (i+1) * 2;

      sum /= 2;
      printf("%d\t%d\t%s\t%d\t%d\t%3.2f\n", comm_rank, rank_dst, "multi_isend", len, nb_blocks, sum/NB_TESTS);
    }
  }
  MPI_Finalize();
  exit(0);
}
