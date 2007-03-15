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

#include "helper.h"

#define MIN     0
#define MAX     (8 * 1024 * 1024)
#define WARMUP   100
#define LOOPS   2000

static __inline__
uint32_t _next(uint32_t len)
{
        if(!len)
                return 4;
//        else if(len < 32)
//                return len + 4;
//        else if(len < 1024)
//                return len + 32;
        else
                return len << 1;
}

int
main(int	  argc,
     char	**argv) {
        char		*buf		= NULL;
        uint32_t	 len;

        init(argc, argv);

        buf = malloc(MAX);
	memset(buf, 0, MAX);

        if (is_server) {
	  int k;
                /* server
                 */
		for(len = MIN; len <= MAX; len = _next(len)) {
		  for(k = 0; k < LOOPS + WARMUP; k++) {
		    nm_so_request request;

		    nm_so_sr_irecv(sr_if, gate_id, 0, buf, len, &request);
		    nm_so_sr_rwait(sr_if, request);

		    nm_so_sr_isend(sr_if, gate_id, 0, buf, len, &request);
		    nm_so_sr_swait(sr_if, request);
		  }
		}

        } else {
	  tbx_tick_t t1, t2;
          double sum, lat, bw_million_byte, bw_mbyte;
	  int k;
                /* client
                 */
                printf("# size |  latency     |   10^6 B/s   |   MB/s    |\n");

		for(len = MIN; len <= MAX; len = _next(len)) {

		  for(k = 0; k < WARMUP; k++) {
		    nm_so_request request;

		    nm_so_sr_isend(sr_if, gate_id, 0, buf, len, &request);
		    nm_so_sr_swait(sr_if, request);

                    nm_so_sr_irecv(sr_if, gate_id, 0, buf, len, &request);
		    nm_so_sr_rwait(sr_if, request);
		  }

		  TBX_GET_TICK(t1);

		  for(k = 0; k < LOOPS; k++) {
		    nm_so_request request;

		    nm_so_sr_isend(sr_if, gate_id, 0, buf, len, &request);
		    nm_so_sr_swait(sr_if, request);

		    nm_so_sr_irecv(sr_if, gate_id, 0, buf, len, &request);
		    nm_so_sr_rwait(sr_if, request);
		  }

		  TBX_GET_TICK(t2);

                   sum = TBX_TIMING_DELAY(t1, t2);

                  lat	      = sum / (2 * LOOPS);
                  bw_million_byte = len * (LOOPS / (sum / 2));
                  bw_mbyte        = bw_million_byte / 1.048576;

		  printf("%d\t%lf\t%8.3f\t%8.3f\n",
                         len, lat, bw_million_byte, bw_mbyte);
		}
        }

        nmad_exit();
        exit(0);
}
