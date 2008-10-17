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

#define CHUNK_SIZE_MIN 128
#define CHUNK_SIZE_MAX 1024
#define MSG_SIZE_MIN  512
#define MSG_SIZE_MAX  (128 * 1024)

#define WARMUP    1
#define LOOPS     1000

static __inline__
uint32_t _next_msg_size(uint32_t len)
{
  if (len < 10 * 1024) {
    return len + CHUNK_SIZE_MIN;
  }
  else {
    return len << 1;
  }
}

static __inline__
uint32_t _next_chunk_size(uint32_t len)
{
  return len << 1;
}

int main(int argc, char	**argv) {
  char	  *buffer = NULL;
  uint32_t len;
  uint32_t chunk;
  int      comm_rank;
  int      comm_size;
  int      ping_side;
  int      rank_dst;
  int      i, k, flag;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

  buffer = malloc(MSG_SIZE_MAX);
  memset(buffer, 0, MSG_SIZE_MAX);

  ping_side	= !(comm_rank & 1);
  rank_dst	= ping_side?(comm_rank | 1):(comm_rank & ~1);

  if (comm_rank == 0) {
    fprintf(stdout, "The configuration size is %d\n", comm_size);
    fprintf(stdout, "src\t|dst\t|size\t|chunk size\t|nb chunks\t|1-bloc\t\t|extended\t|benefit\t|isend\t\t|benefit|\n");
  }

  /* Warmup */
  if (WARMUP) {
    MPI_Barrier(MPI_COMM_WORLD);
    for(i=0 ; i<LOOPS ; i++) {
      if (ping_side) {
        MPI_Send(buffer, MSG_SIZE_MAX, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD);
        MPI_Recv(buffer, MSG_SIZE_MAX, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      } else {
        MPI_Recv(buffer, MSG_SIZE_MAX, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(buffer, MSG_SIZE_MAX, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD);
      }
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  /* Multi-chunk pingpong */
  for(len = MSG_SIZE_MIN; len <= MSG_SIZE_MAX; len = _next_msg_size(len)) {
    for(chunk = CHUNK_SIZE_MIN ; chunk <= CHUNK_SIZE_MAX ; chunk = _next_chunk_size(chunk)) {
      long n, nb_chunks = len / chunk;
      MPI_Aint *offsets;
      MPI_Datatype *oldtypes;
      int *blockcounts;
      MPI_Datatype datatype;
      MPI_Request *requests;

      if (len < chunk) continue;

      // Create the struct derived datatype
      offsets = malloc(nb_chunks * sizeof(MPI_Aint));
      blockcounts = malloc(nb_chunks * sizeof(int));
      oldtypes = malloc(nb_chunks * sizeof(MPI_Datatype));
      for(k=0 ; k<nb_chunks ; k++) {
        oldtypes[k] = MPI_CHAR;
        offsets[k] = k*chunk;
        blockcounts[k] = len;
      }
      MPI_Type_struct(nb_chunks, blockcounts, offsets, oldtypes, &datatype);
      MPI_Type_commit(&datatype);

      requests = malloc(nb_chunks * sizeof(MPI_Request));

      // Exchange message
      if (!ping_side) {
        // server: receive - send
        for(k=0 ; k<LOOPS ; k++) {
          for(n = 0; n < nb_chunks; n++) {
            MPI_Recv(buffer + n * chunk, chunk, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          }
          for(n = 0; n < nb_chunks-1; n++) {
            MPI_Esend(buffer+n*chunk, chunk, MPI_CHAR, rank_dst, 10, MPI_IS_NOT_COMPLETED, MPI_COMM_WORLD, &requests[n]);
            MPI_Test(&requests[n], &flag, MPI_STATUS_IGNORE);
          }
          MPI_Esend(buffer+(nb_chunks-1)*chunk, chunk, MPI_CHAR, rank_dst, 10, MPI_IS_COMPLETED, MPI_COMM_WORLD, &requests[nb_chunks-1]);

          for(n = 0; n < nb_chunks; n++) {
            MPI_Wait(&requests[n], MPI_STATUS_IGNORE);
          }
        }

        for(k=0 ; k<LOOPS ; k++) {
          for(n = 0; n < nb_chunks; n++) {
            MPI_Recv(buffer + n * chunk, chunk, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          }
          for(n = 0; n < nb_chunks; n++) {
            MPI_Isend(buffer+n*chunk, chunk, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, &requests[n]);
            MPI_Test(&requests[n], &flag, MPI_STATUS_IGNORE);
          }
          for(n = 0; n < nb_chunks; n++) {
            MPI_Wait(&requests[n], MPI_STATUS_IGNORE);
          }
        }

        for(k=0 ; k<LOOPS ; k++) {
          MPI_Recv(buffer, len, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          MPI_Send(buffer, 1, datatype, rank_dst, 10, MPI_COMM_WORLD);
        }
      }
      else {
        // client: send - receive
        double t1_bloc, t2_bloc;
        double t1_isend, t2_isend;
        double t1_extended, t2_extended;
        double gain_extended_bloc;
        double gain_isend_bloc;

        t1_extended = MPI_Wtime();
        for(k=0 ; k<LOOPS ; k++) {
          for(n = 0; n < nb_chunks-1; n++) {
            MPI_Esend(buffer+n*chunk, chunk, MPI_CHAR, rank_dst, 10, MPI_IS_NOT_COMPLETED, MPI_COMM_WORLD, &requests[n]);
            MPI_Test(&requests[n], &flag, MPI_STATUS_IGNORE);
          }
          MPI_Esend(buffer+(nb_chunks-1)*chunk, chunk, MPI_CHAR, rank_dst, 10, MPI_IS_COMPLETED, MPI_COMM_WORLD, &requests[nb_chunks-1]);

          for(n = 0; n < nb_chunks; n++) {
            MPI_Wait(&requests[n], MPI_STATUS_IGNORE);
          }

          for(n = 0; n < nb_chunks; n++) {
            MPI_Recv(buffer + n * chunk, chunk, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          }
        }
        t2_extended = MPI_Wtime();

        t1_isend = MPI_Wtime();
        for(k=0 ; k<LOOPS ; k++) {
          for(n = 0; n < nb_chunks; n++) {
            MPI_Isend(buffer+n*chunk, chunk, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, &requests[n]);
            MPI_Test(&requests[n], &flag, MPI_STATUS_IGNORE);
          }
          for(n = 0; n < nb_chunks; n++) {
            MPI_Wait(&requests[n], MPI_STATUS_IGNORE);
          }

          for(n = 0; n < nb_chunks; n++) {
            MPI_Recv(buffer + n * chunk, chunk, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          }
        }
        t2_isend = MPI_Wtime();

        t1_bloc = MPI_Wtime();
        for(k=0 ; k<LOOPS ; k++) {
          MPI_Send(buffer, 1, datatype, rank_dst, 10, MPI_COMM_WORLD);
          MPI_Recv(buffer, len, MPI_CHAR, rank_dst, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        t2_bloc = MPI_Wtime();

        gain_extended_bloc = 100 - (((t2_extended - t1_extended) / (t2_bloc - t1_bloc)) * 100);
        gain_isend_bloc = 100 - (((t2_isend - t1_isend) / (t2_bloc - t1_bloc)) * 100);
        fprintf(stdout, "%d\t%d\t%d\t%d\t\t%ld\t\t%lf\t%lf\t%3.2f%%\t\t%lf\t%3.2f%%\n", comm_rank, rank_dst, len, chunk, nb_chunks, (t2_bloc-t1_bloc)/(2*LOOPS), (t2_extended-t1_extended)/(2*LOOPS), gain_extended_bloc, (t2_isend-t1_isend)/(2*LOOPS), gain_isend_bloc);
      }

      // Free memory
      MPI_Type_free(&datatype);
      free(requests);
      free(blockcounts);
      free(offsets);
      free(oldtypes);
    }
  }

  MPI_Finalize();
  return 0;
}
