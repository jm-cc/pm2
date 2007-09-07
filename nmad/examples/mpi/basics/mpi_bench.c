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
#include <stdint.h>
#include <stdlib.h>

#include "mpi.h"

// Setup

#define MIN_DEFAULT      0
#define MAX_DEFAULT      (8 * 1024 * 1024)
#define MULT_DEFAULT     2
#define INCR_DEFAULT     0
#define WARMUPS_DEFAULT  100
#define LOOPS_DEFAULT    2000
#define NB_TESTS_DEFAULT 1

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

static unsigned char *main_buffer = NULL;

// Functions
//......................
static __inline__
uint32_t _next(uint32_t len, uint32_t multiplier, uint32_t increment)
{
  if (!len)
    return 1+increment;
  else
    return len*multiplier+increment;
}

static void usage_bench(void) {
  fprintf(stderr, "-S start_len  - starting length [%d]\n", MIN_DEFAULT);
  fprintf(stderr, "-E end_len    - ending length [%d]\n", MAX_DEFAULT);
  fprintf(stderr, "-I incr       - length increment [%d]\n", INCR_DEFAULT);
  fprintf(stderr, "-M mult       - length multiplier [%d]\n", MULT_DEFAULT);
  fprintf(stderr, "    Next(0)      = 1+increment\n");
  fprintf(stderr, "    Next(length) = length*multiplier+increment\n");
  fprintf(stderr, "-N iterations - iterations per length [%d]\n", LOOPS_DEFAULT);
  fprintf(stderr, "-T tests      - number of tests [%d]\n", NB_TESTS_DEFAULT);
  fprintf(stderr, "-W warmup     - number of warmup iterations [%d]\n", WARMUPS_DEFAULT);
}

int
main(int    argc,
     char **argv)
{
        int log, i;
        int ping_side;
        int rank_dst;
        uint32_t	 start_len      = MIN_DEFAULT;
        uint32_t	 end_len        = MAX_DEFAULT;
        uint32_t         multiplier     = MULT_DEFAULT;
        uint32_t         increment      = INCR_DEFAULT;
        int              iterations     = LOOPS_DEFAULT;
        int              warmups        = WARMUPS_DEFAULT;
        int              tests          = NB_TESTS_DEFAULT;

        MPI_Init(&argc,&argv);

        MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);

        if (argc > 1 && !strcmp(argv[1], "--help")) {
          if (!comm_rank) {
            usage_bench();
          }
          goto out;
        }

        if (gethostname(host_name, 1023) < 0) {
                perror("gethostname");
                exit(1);
        }


        if (comm_size & 1) {
                fprintf(stderr, "This program requires an even configuration size, aborting...\n");
                goto out;
        }

        for(i=1 ; i<argc ; i+=2) {
                if (!strcmp(argv[i], "-S")) {
                        start_len = atoi(argv[i+1]);
                }
                else if (!strcmp(argv[i], "-E")) {
                        end_len = atoi(argv[i+1]);
                }
                else if (!strcmp(argv[i], "-I")) {
                        increment = atoi(argv[i+1]);
                }
                else if (!strcmp(argv[i], "-M")) {
                        multiplier = atoi(argv[i+1]);
                }
                else if (!strcmp(argv[i], "-N")) {
                        iterations = atoi(argv[i+1]);
                }
                else if (!strcmp(argv[i], "-T")) {
                        tests = atoi(argv[i+1]);
                }
                else if (!strcmp(argv[i], "-W")) {
                        warmups = atoi(argv[i+1]);
                }
                else {
                        fprintf(stderr, "Illegal argument %s\n", argv[i]);
                        usage_bench();
                        goto out;
                }
        }

        fprintf(stdout, "(%s): My rank is %d\n", host_name, comm_rank);

        ping_side	= !(comm_rank & 1);
        rank_dst	= ping_side?(comm_rank | 1):(comm_rank & ~1);

        if (ping_side) {
                fprintf(stdout, "(%d): ping with %d\n", comm_rank, rank_dst);
                log = 1;
        } else {
                fprintf(stdout, "(%d): pong with %d\n", comm_rank, rank_dst);
                log = 0;
        }

        if (comm_rank == 0) {
                fprintf(stdout, "The configuration size is %d\n", comm_size);
		fprintf(stdout, "src|dst|size        |latency     |10^6 B/s|MB/s    |\n");
	}

        main_buffer = malloc(end_len);

        /* Warmup */
        MPI_Barrier(MPI_COMM_WORLD);
        for(i=0 ; i<warmups ; i++) {
          if (ping_side) {
            MPI_Send(main_buffer, end_len, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD);
            MPI_Recv(main_buffer, end_len, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          } else {
            MPI_Recv(main_buffer, end_len, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(main_buffer, end_len, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD);
          }
        }
        MPI_Barrier(MPI_COMM_WORLD);

        /* Test */
        {
                uint32_t size = 0;

                for (size = start_len;
                     size <= end_len;
                     size = _next(size, multiplier, increment)) {
                        if (ping_side) {
                                int		nb_tests	= tests;
                                double		sum		= 0.0;
                                double		t1;
                                double		t2;
                                double		lat;
                                double		bw_million_byte;
				double          bw_mbyte;

                                sum = 0;

                                while (nb_tests--) {
                                        int nb_samples = iterations;

                                        MPI_Barrier(MPI_COMM_WORLD);
                                        t1 = MPI_Wtime();
                                        while (nb_samples--) {
                                                MPI_Send(main_buffer, size, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD);
                                                MPI_Recv(main_buffer, size, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                                        }
                                        t2 = MPI_Wtime();
                                        MPI_Barrier(MPI_COMM_WORLD);

                                        sum += t2 - t1;
                                }

                                sum *= 1e6;

                                bw_million_byte	= (size * (double)tests * (double)iterations)
                                        / (sum / 2);
                                lat		= sum / tests / iterations / 2;
				bw_mbyte = bw_million_byte / 1.048576;


                                if (log)
                                        fprintf(stdout, "%3d %3d %12d %12.3f %8.3f %8.3f\n",
						comm_rank, rank_dst, size, lat, bw_million_byte, bw_mbyte);
                        } else {
                                int		nb_tests	= tests;
                                double		sum		= 0.0;
                                double		t1;
                                double		t2;
                                double		lat;
                                double		bw_million_byte;
				double          bw_mbyte;

                                sum = 0;

                                while (nb_tests--) {
                                        int nb_samples = iterations;

                                        MPI_Barrier(MPI_COMM_WORLD);
                                        t1 = MPI_Wtime();
                                        while (nb_samples--) {
                                                MPI_Recv(main_buffer, size, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                                                MPI_Send(main_buffer, size, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD);
                                        }
                                        MPI_Barrier(MPI_COMM_WORLD);
                                        t2 = MPI_Wtime();
                                        sum += t2 - t1;
                                }


                                sum *= 1e6;

                                bw_million_byte	= (size * (double)tests * (double)iterations)
                                        / (sum / 2);
                                lat		= sum / tests / iterations / 2;
				bw_mbyte = bw_million_byte / 1.048576;

                                if (log && ping_side)
                                        fprintf(stdout, "%3d %3d %12d %12.3f %8.3f %8.3f\n",
						comm_rank, rank_dst, size, lat, bw_million_byte, bw_mbyte);
                        }

                }
        }

        free(main_buffer);

        if (log)
                fprintf(stdout, "Exiting\n");

 out:
        MPI_Finalize();

        return 0;
}
