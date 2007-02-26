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

#define PACK_SIZE_MIN 128
#define PACK_SIZE_MAX 1024
#define MSG_SIZE_MIN  512
#define MSG_SIZE_MAX  (30 * 1024)

#define WARMUP    1
#define LOOPS     1000

static __inline__
uint32_t _next_msg_size(uint32_t len)
{
  if (len < 10 * 1024) {
    return len + PACK_SIZE_MIN;
  }
  else {
    return len << 1;
  }
}

static __inline__
uint32_t _next_pack_size(uint32_t len)
{
  return len << 1;
}

int main(int argc, char	**argv) {
  char	  *buffer = NULL;
  uint32_t len;
  uint32_t pack;
  int      comm_rank;
  int      comm_size;
  int      ping_side;
  int      rank_dst;
  int      i, k;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

  buffer = malloc(MSG_SIZE_MAX);
  memset(buffer, 0, MSG_SIZE_MAX);

  ping_side	= !(comm_rank & 1);
  rank_dst	= ping_side?(comm_rank | 1):(comm_rank & ~1);

  if (comm_rank == 0) {
    fprintf(stderr, "The configuration size is %d\n", comm_size);
    fprintf(stderr, "src\t|dst\t|size\t|pack size\t|nb chunks\t|1-bloc\t\t|extended\t|benefit|\n");
  }

  /* Warmup */
  if (WARMUP) {
    MPI_Barrier(MPI_COMM_WORLD);
    for(i=0 ; i<LOOPS ; i++) {
      if (ping_side) {
        MPI_Send(buffer, MSG_SIZE_MAX, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD);
        MPI_Recv(buffer, MSG_SIZE_MAX, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD, NULL);
      } else {
        MPI_Recv(buffer, MSG_SIZE_MAX, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD, NULL);
        MPI_Send(buffer, MSG_SIZE_MAX, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD);
      }
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  /* Multi-pack pingpong */
  if (ping_side) {
    for(len = MSG_SIZE_MIN; len <= MSG_SIZE_MAX; len = _next_msg_size(len)) {
      for(pack = PACK_SIZE_MIN ; pack <= PACK_SIZE_MAX ; pack = _next_pack_size(pack)) {
        unsigned long n, chunk = len / pack;

        if (len < pack) continue;

        for(k=0 ; k<LOOPS ; k++) {
          for(n = 0; n < chunk; n++) {
            MPI_Recv(buffer + n * pack, pack, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, NULL);
          }
          MPI_Send(buffer, len, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD);
        }

        for(k=0 ; k<LOOPS ; k++) {
          MPI_Recv(buffer, len, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, NULL);
          MPI_Send(buffer, len, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD);
        }
      }
    }
  }
  else {
    double t1_bloc, t2_bloc;
    double t1_extended, t2_extended;
    double gain;
    MPI_Request *requests;
    MPI_Aint *offsets;
    MPI_Datatype *oldtypes;
    int *blockcounts;
    MPI_Datatype datatype;

    for(len = MSG_SIZE_MIN; len <= MSG_SIZE_MAX; len = _next_msg_size(len)) {
      for(pack = PACK_SIZE_MIN ; pack <= PACK_SIZE_MAX ; pack = _next_pack_size(pack)) {
        unsigned long n, chunk = len / pack;

        if (len < pack) continue;

        requests = malloc(chunk * sizeof(MPI_Request));
        offsets = malloc(chunk * sizeof(MPI_Aint));
        blockcounts = malloc(chunk * sizeof(int));
        oldtypes = malloc(chunk * sizeof(MPI_Datatype));

        for(k=0 ; k<chunk ; k++) {
          oldtypes[k] = MPI_CHAR;
          offsets[k] = k*pack;
          blockcounts[k] = len;
        }
        MPI_Type_struct(chunk, blockcounts, offsets, oldtypes, &datatype);
        MPI_Type_commit(&datatype);

        t1_extended = MPI_Wtime();
        for(k=0 ; k<LOOPS ; k++) {
          for(n = 0; n < chunk-1; n++) {
            MPI_Esend(buffer+n*pack, pack, MPI_CHAR, rank_dst, 10, MPI_IS_NOT_COMPLETED, MPI_COMM_WORLD, &requests[n]);
          }
          MPI_Esend(buffer+(chunk-1)*pack, pack, MPI_CHAR, rank_dst, 10, MPI_IS_COMPLETED, MPI_COMM_WORLD, &requests[chunk-1]);

          for(n = 0; n < chunk; n++) {
            MPI_Wait(&requests[n], NULL);
          }

          MPI_Recv(buffer, len, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, NULL);
        }
        t2_extended = MPI_Wtime();

        t1_bloc = MPI_Wtime();
        for(k=0 ; k<LOOPS ; k++) {
          MPI_Send(buffer, 1, datatype, rank_dst, 10, MPI_COMM_WORLD);

          MPI_Recv(buffer, len, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, NULL);
        }
        t2_bloc = MPI_Wtime();

        MPI_Type_free(&datatype);
        free(requests);
        free(blockcounts);
        free(offsets);
        free(oldtypes);

        gain = 100 - (((t2_extended - t1_extended) / (t2_bloc - t1_bloc)) * 100);
        fprintf(stderr, "%d\t%d\t%d\t%d\t\t%ld\t\t%lf\t%lf\t%3.2f%%\n", comm_rank, rank_dst, len, pack, chunk, (t2_bloc-t1_bloc)/(2*LOOPS), (t2_extended-t1_extended)/(2*LOOPS), gain);
      }
    }
  }

  MPI_Finalize();
  return 0;
}
