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

#include "mpi.h"

#define PACK_SIZE 256
#define MIN       (512)
#define MAX       (8 * 1024 * 1024)
#define WARMUP    1
#define LOOPS     100

static __inline__
uint32_t _next(uint32_t len)
{
  if (len < 10 * 1024) {
    return len + PACK_SIZE;
  }
  else {
    return len << 1;
  }
}

int main(int argc, char	**argv) {
  char	  *buffer = NULL;
  uint32_t len;
  int      comm_rank;
  int      comm_size;
  int      ping_side;
  int      rank_dst;
  int      i, k;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

  buffer = malloc(MAX);
  memset(buffer, 0, MAX);

  ping_side	= !(comm_rank & 1);
  rank_dst	= ping_side?(comm_rank | 1):(comm_rank & ~1);

  if (comm_rank == 0) {
    fprintf(stderr, "The configuration size is %d\n", comm_size);
    fprintf(stderr, "src\t|dst\t|size\t|chunks\t|aggreg\t|extended|\n");
  }

  /* Warmup */
  if (WARMUP) {
    MPI_Barrier(MPI_COMM_WORLD);
    for(i=0 ; i<100 ; i++) {
      if (ping_side) {
        MPI_Send(buffer, MAX, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD);
        MPI_Recv(buffer, MAX, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD, NULL);
      } else {
        MPI_Recv(buffer, MAX, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD, NULL);
        MPI_Send(buffer, MAX, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD);
      }
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  /* Multi-pack pingpong */
  if (ping_side) {
    for(len = MIN; len <= MAX; len = _next(len)) {
      unsigned long n, chunk = len / PACK_SIZE;

      for(k=0 ; k<LOOPS ; k++) {
        for(n = 0; n < chunk; n++) {
          MPI_Recv(buffer + n * PACK_SIZE, PACK_SIZE, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, NULL);
        }
        for(n = 0; n < chunk; n++) {
          MPI_Send(buffer + n * PACK_SIZE, PACK_SIZE, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD);
        }
      }

      for(k=0 ; k<LOOPS ; k++) {
        for(n = 0; n < chunk; n++) {
          MPI_Recv(buffer + n * PACK_SIZE, PACK_SIZE, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, NULL);
        }
        for(n = 0; n < chunk; n++) {
          MPI_Send(buffer + n * PACK_SIZE, PACK_SIZE, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD);
        }
      }
    }
  }
  else {
    double t1_aggreg, t2_aggreg;
    double t1_extended, t2_extended;
    int flag;
    MPI_Request *requests;

    for(len = MIN; len <= MAX; len = _next(len)) {
      unsigned long n, chunk = len / PACK_SIZE;
      requests = malloc(chunk * sizeof(MPI_Request));

      t1_extended = MPI_Wtime();
      for(k=0 ; k<LOOPS ; k++) {
        for(n = 0; n < chunk-1; n++) {
          MPI_Esend(buffer+n*PACK_SIZE, PACK_SIZE, MPI_CHAR, rank_dst, 10, MPI_IS_NOT_COMPLETED, MPI_COMM_WORLD, &requests[n]);
          MPI_Test(&requests[n], &flag, NULL);
        }
        MPI_Esend(buffer+(chunk-1)*PACK_SIZE, PACK_SIZE, MPI_CHAR, rank_dst, 10, MPI_IS_COMPLETED, MPI_COMM_WORLD, &requests[chunk-1]);

        for(n = 0; n < chunk; n++) {
          MPI_Wait(&requests[n], NULL);
        }

        for(n = 0; n < chunk; n++) {
          MPI_Recv(buffer + n * PACK_SIZE, PACK_SIZE, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, NULL);
        }
      }
      t2_extended = MPI_Wtime();

      sleep(1);

      t1_aggreg = MPI_Wtime();
      for(k=0 ; k<LOOPS ; k++) {
        for(n = 0; n < chunk-1; n++) {
          MPI_Isend(buffer+n*PACK_SIZE, PACK_SIZE, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, &requests[n]);
          MPI_Test(&requests[n], &flag, NULL);
        }
        MPI_Isend(buffer+(chunk-1)*PACK_SIZE, PACK_SIZE, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, &requests[chunk-1]);

        for(n = 0; n < chunk; n++) {
          MPI_Wait(&requests[n], NULL);
        }
        for(n = 0; n < chunk; n++) {
          MPI_Recv(buffer + n * PACK_SIZE, PACK_SIZE, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, NULL);
        }
      }
      t2_aggreg = MPI_Wtime();

      free(requests);

      fprintf(stderr, "%d\t%d\t%d\t%ld\t%lf\t%lf\n", comm_rank, rank_dst, len, chunk, (t2_aggreg-t1_aggreg)/(2*LOOPS), (t2_extended-t1_extended)/(2*LOOPS));
    }
  }

  MPI_Finalize();
  return 0;
}
