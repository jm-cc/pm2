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
//......................
static const int param_warmup            = 100;
static const int param_nb_samples        = 1000;
static const int param_min_size          = 0;
static const int param_max_size          = 1024*1024*2;
static const int param_step              = 0; /* 0 = progression log. */
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

static unsigned char *main_buffer = NULL;

// Functions
//......................
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
main(int    argc,
     char **argv)
{
        int log, i;
        int ping_side;
        int rank_dst;

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
		fprintf(stderr, "src|dst|size        |latency     |10^6 B/s|MB/s    |\n");
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

        main_buffer = malloc(param_max_size);

        /* Warmup */
        MPI_Barrier(MPI_COMM_WORLD);
        for(i=0 ; i<param_warmup ; i++) {
          if (ping_side) {
            MPI_Send(main_buffer, param_max_size, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD);
            MPI_Recv(main_buffer, param_max_size, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          } else {
            MPI_Recv(main_buffer, param_max_size, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(main_buffer, param_max_size, MPI_CHAR, rank_dst, 0, MPI_COMM_WORLD);
          }
        }
        MPI_Barrier(MPI_COMM_WORLD);

        /* Test */
        {
                int size = 0;

                for (size = param_min_size;
                     size <= param_max_size;
                     size = param_step?size + param_step:_next(size)) {
                        if (ping_side) {
                                int		nb_tests	= param_nb_tests;
                                double		sum		= 0.0;
                                double		t1;
                                double		t2;
                                double		lat;
                                double		bw_million_byte;
				double          bw_mbyte;

                                sum = 0;

                                while (nb_tests--) {
                                        int nb_samples = param_nb_samples;

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

                                bw_million_byte	= (size * (double)param_nb_tests * (double)param_nb_samples)
                                        / (sum / 2);
                                lat		= sum / param_nb_tests / param_nb_samples / 2;
				bw_mbyte = bw_million_byte / 1.048576;


                                if (log)
                                        fprintf(stderr, "%3d %3d %12d %12.3f %8.3f %8.3f\n",
						comm_rank, rank_dst, size, lat, bw_million_byte, bw_mbyte);
                        } else {
                                int		nb_tests	= param_nb_tests;
                                double		sum		= 0.0;
                                double		t1;
                                double		t2;
                                double		lat;
                                double		bw_million_byte;
				double          bw_mbyte;

                                sum = 0;

                                while (nb_tests--) {
                                        int nb_samples = param_nb_samples;

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

                                bw_million_byte	= (size * (double)param_nb_tests * (double)param_nb_samples)
                                        / (sum / 2);
                                lat		= sum / param_nb_tests / param_nb_samples / 2;
				bw_mbyte = bw_million_byte / 1.048576;

                                if (log && ping_side)
                                        fprintf(stderr, "%3d %3d %12d %12.3f %8.3f %8.3f\n",
						comm_rank, rank_dst, size, lat, bw_million_byte, bw_mbyte);
                        }

                }
        }

        free(main_buffer);

        if (log)
                fprintf(stderr, "Exiting\n");

 out:
        MPI_Finalize();

        return 0;
}
