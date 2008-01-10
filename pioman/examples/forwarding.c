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

#define MAX     (8 * 1024 * 1024)
#define len     (32 * 1024)
#define LOOPS   2000
#define DELAY   1000000 // temps de calcul en microseconde


void
compute() {
	tbx_tick_t t1, t2;
	TBX_GET_TICK(t1);
	while(1) {
		TBX_GET_TICK(t2);
		if(TBX_TIMING_DELAY(t1, t2) > DELAY)
			return ;
	}
}
int
main(int	  argc,
     char	**argv) {
	struct nm_so_cnx         cnx;
        char			*buf		= NULL;
//        uint32_t		 len;
	fprintf(stderr, "Len = %d bytes\n",len);
        init(&argc, argv);

        buf = malloc(MAX);
	memset(buf, 0, MAX);

        if (is_server) {
	  int k;
                /* server
                 */
		  for(k = 0; k < LOOPS; k++) {
			  //fprintf(stderr, "iRecv  %d\n", k);
		    nm_so_begin_unpacking(pack_if, gate_id, 0, &cnx);
		    nm_so_unpack(&cnx, buf, len);
		    compute();
		    nm_so_end_unpacking(&cnx);
		    //fprintf(stderr, "data received\n");
		    nm_so_begin_packing(pack_if, gate_id, 0, &cnx);
		    nm_so_pack(&cnx, buf, 4);
//		    compute();
		    nm_so_end_packing(&cnx);
		    //fprintf(stderr, "send\n");

		  }

        } else {
	  tbx_tick_t t1, t2;
          double sum, lat, bw_million_byte, bw_mbyte;
	  int k;
	  sum = 0;
                /* client
                 */
                printf(" size     |  latency     |   10^6 B/s   |   MB/s    |\n");


		  for(k = 0; k < LOOPS; k++) {
			  //fprintf(stderr, "sending %d bytes (loop %d)\n", len, k);		    
		    TBX_GET_TICK(t1);
		    nm_so_begin_packing(pack_if, gate_id, 0, &cnx);
		    nm_so_pack(&cnx, buf, len);
//		    compute();
		    nm_so_end_packing(&cnx);		   
		    TBX_GET_TICK(t2);
		    //fprintf(stderr, "sent\n");
		    nm_so_begin_unpacking(pack_if, gate_id, 0, &cnx);
		    nm_so_unpack(&cnx, buf, 4);
		    nm_so_end_unpacking(&cnx);
		    //fprintf(stderr, "recv ok\n");
		    sum += TBX_TIMING_DELAY(t1, t2);
		    fprintf(stderr, "%d\t%lf\n", k, TBX_TIMING_DELAY(t1, t2));
		  }



                  lat	      = sum / (2 * LOOPS);
                  bw_million_byte = len * (LOOPS / (sum / 2));
                  bw_mbyte        = bw_million_byte / 1.048576;

		  printf("%lf\t%8.3f\t%8.3f\n",
                          lat, bw_million_byte, bw_mbyte);
        }

        nmad_exit();
        exit(0);
}
