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

//#define MIN       (MIN_PACKS * 4)
//#define MAX       (8 * 1024 * 1024)

#define NB_LINES 10
#define SMALL 64
#define LARGE (256 * 1024)

//#define MIN_PACKS 1
//#define MAX_PACKS 4//MAX

#define NB_TESTS     100


MPI_Request *s_request[NB_LINES * 2];
MPI_Request *r_request[NB_LINES * 2];

static int	comm_rank	= -1;
static int	comm_size	= -1;
static char	host_name[1024]	= "";
static const int param_log_only_master   = 0;

static __inline__
uint32_t _next(uint32_t len)
{
  if(!len)
    return 4;
  //else if(len < 32)
  //  return len + 4;
  //else if(len < 1024)
  //  return len + 32;
  else
    return len << 1;
}

static
void
usage(void) {
  fprintf(stderr, "usage: hello [[-h <remote_hostname>] <remote url>]\n");
  exit(EXIT_FAILURE);
}

int
main(int	  argc,
     char	**argv) {
  int log;
  int ping_side;
  int rank_dst;
  char	*recv_buf		= NULL;
  char	*small_buf		= NULL;
  char	*large_buf		= NULL;
  int len, i, j, test;



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
    fprintf(stderr, "(%s): My rank is %d\n", host_name, comm_rank);
  }

  if (comm_rank == 0 && log) {
    fprintf(stderr, "The configuration size is %d\n", comm_size);
    fprintf(stderr, " src | dst |     size     | nb_blocks |     latency     |     10^6 B/s   |   MB/s    |\n");
  }

  if (comm_size & 1) {
    if (log)
      fprintf(stderr, "This program requires an even configuration size, aborting...\n");

    goto out;
  }

  ping_side	= !(comm_rank & 1);
  rank_dst	= ping_side?(comm_rank | 1):(comm_rank & ~1);

  if (ping_side) {
    fprintf(stderr, "(%d): ping with %d\n", comm_rank, rank_dst);
  } else {
    if (log)
      fprintf(stderr, "(%d): pong with %d\n", comm_rank, rank_dst);
  }


  for(i = 0; i < NB_LINES * 2; i++){
    s_request[i] = malloc(sizeof(MPI_Request));
    r_request[i] = malloc(sizeof(MPI_Request));
  }


  /* ------------------------------------------------- */
  /* ------------------- Multi-ping ------------------ */

  recv_buf = malloc((SMALL+LARGE) * NB_LINES);
  small_buf = malloc(SMALL);
  large_buf = malloc(LARGE);

  memset(recv_buf, 0, (SMALL+LARGE) * NB_LINES);
  memset(small_buf, 'a', SMALL);
  memset(large_buf, 'b', LARGE);

  if (!ping_side) {
    /* server */

    for(i = 0; i < NB_LINES; i++){

      for(test=0 ; test<NB_TESTS ; test++) {
        MPI_Barrier(MPI_COMM_WORLD);



        for(j = 0; j <= i; j++){
          MPI_Irecv(recv_buf + j * (SMALL + LARGE),  SMALL,
                    MPI_CHAR,
                    rank_dst, 0, MPI_COMM_WORLD,
                    r_request[j*2]);

          //printf("r_request[%d] = %p\n", j*2, r_request[j*2]);

          MPI_Irecv(recv_buf + j *(SMALL + LARGE) + SMALL,
                    LARGE, MPI_CHAR,
                    rank_dst, 0, MPI_COMM_WORLD,
                    r_request[j*2 + 1]);
          //printf("r_request[%d] = %p\n", j*2 + 1, r_request[j*2 + 1]);
        }

        //printf("Dernière request : %d --> %d\n", j-1, (j-1)*2 +1);


        //printf("-->les I-réception ok\n");

        for(j = 0; j < (i+1) * 2; j++){
          //printf("MPI_WAIT[%d]\n", j);
          MPI_Wait(r_request[j], NULL);
          //printf("-->fin du wait\n");
        }

        //printf("-->les wait des I-réception ok\n");

        for(j = 0; j <= i; j++){
          MPI_Isend(small_buf, SMALL, MPI_CHAR,
                    rank_dst, 0, MPI_COMM_WORLD,
                    s_request[j*2]);
          MPI_Isend(large_buf,  LARGE, MPI_CHAR,
                    rank_dst, 0, MPI_COMM_WORLD,
                    s_request[j*2 + 1]);
        }
        //printf("Dernière request : %d --> %d\n", j-1, (j-1)*2 +1);
        //printf("-->les I-send ok\n");

        for(j = 0; j < (i+1) * 2; j++){
          //printf("MPI_WAIT[%d]\n", j);
          MPI_Wait(s_request[j], NULL);
          //printf("-->fin du wait\n");
        }

        //printf("-->les wait des I-send ok\n");

      }
    }


    } else {
      /* client */

    double t1, t2, sum, lat, bw_million_byte, bw_mbyte;

    for(i = 0; i < NB_LINES; i++){
      sum = 0;

      for(test=0 ; test<NB_TESTS ; test++) {

        MPI_Barrier(MPI_COMM_WORLD);

        t1 = MPI_Wtime();

        for(j = 0; j <= i; j++){
          MPI_Isend(small_buf, SMALL, MPI_CHAR,
                   rank_dst, 0, MPI_COMM_WORLD,
                    s_request[j*2]);
          MPI_Isend(large_buf,  LARGE, MPI_CHAR,
                   rank_dst, 0, MPI_COMM_WORLD,
                    s_request[j*2 + 1]);
        }
        //printf("Dernière request : %d --> %d\n", j-1, (j-1)*2 +1);
        //printf("-->les I-send ok\n");

        for(j = 0; j < (i+1) * 2; j++){
          //printf("MPI_WAIT[%d]\n", j);
          MPI_Wait(s_request[j], NULL);
          //printf("-->fin du wait\n");
        }


        //printf("-->les wait des I-send ok\n");


        for(j = 0; j <= i; j++){
          MPI_Irecv(recv_buf + j * (SMALL + LARGE),  SMALL,
                    MPI_CHAR,
                    rank_dst, 0, MPI_COMM_WORLD,
                    r_request[j*2]);
          //printf("r_request[%d] = %p\n", j*2, r_request[j*2]);
          MPI_Irecv(recv_buf + j *(SMALL + LARGE) + SMALL,
                    LARGE, MPI_CHAR,
                    rank_dst, 0, MPI_COMM_WORLD,
                    r_request[j*2 + 1]);
          //printf("r_request[%d] = %p\n", j*2 + 1, r_request[j*2 + 1]);
        }
        //printf("Dernière request : %d --> %d\n", j-1, (j-1)*2 +1);
        //printf("-->les I-réception ok\n");

        for(j = 0; j < (i+1) * 2; j++){
          //printf("MPI_WAIT[%d] = %p\n", j, r_request[j]);
          MPI_Wait(r_request[j], NULL);
          //printf("-->fin du wait\n");
        }


        //printf("-->les wait des I-recv ok\n");


        t2 = MPI_Wtime();
        sum += (t2 - t1) * 1e6;
      }


      int len = (i+1) * (SMALL + LARGE);
      int nb_blocks = (i+1) * 2;

      lat	      = sum / (2 * NB_TESTS);
      bw_million_byte = len * (NB_TESTS / (sum / 2));
      bw_mbyte        = bw_million_byte / 1.048576;

      printf("%5d %5d %12d %9d  %12.3f\t %8.3f \t %8.3f\n",
             comm_rank, rank_dst, len, nb_blocks,
             lat, bw_million_byte, bw_mbyte);
    }
  }
 out:
  return 0;
}
