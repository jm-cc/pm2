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

#define MAX     (8*1024)
#define LOOPS   1000

int
main(int	  argc,
     char	**argv) {
        char			*buf		= NULL;
        uint32_t		 len;
	struct nm_so_cnx         cnx;

        init(argc, argv);
        buf = malloc(MAX);
	memset(buf, 0, MAX);

        if (is_server) {
	  int k;
                /* server
                 */
		for(len = 16; len <= MAX; len *= 2) {
		  for(k = 0; k < LOOPS; k++) {

		    nm_so_begin_unpacking(pack_if, gate_id, 0, &cnx);
		    nm_so_unpack(&cnx, buf, len/4);
		    nm_so_unpack(&cnx, buf+len/4, len/4);
		    nm_so_unpack(&cnx, buf+len/2, len/4);
		    nm_so_unpack(&cnx, buf+(len/4)*3, len/4);
		    nm_so_end_unpacking(&cnx);

		    nm_so_begin_packing(pack_if, gate_id, 0, &cnx);
		    nm_so_pack(&cnx, buf, len/4);
		    nm_so_pack(&cnx, buf+len/4, len/4);
		    nm_so_pack(&cnx, buf+len/2, len/4);
		    nm_so_pack(&cnx, buf+(len/4)*3, len/4);
		    nm_so_end_packing(&cnx);

		  }
		}

        } else {
	  tbx_tick_t t1, t2;
	  int k;
                /* client
                 */
		for(len = 16; len <= MAX; len *= 2) {

		  TBX_GET_TICK(t1);

		  for(k = 0; k < LOOPS; k++) {
		    nm_so_begin_packing(pack_if, gate_id, 0, &cnx);
		    nm_so_pack(&cnx, buf, len/4);
		    nm_so_pack(&cnx, buf+len/4, len/4);
		    nm_so_pack(&cnx, buf+len/2, len/4);
		    nm_so_pack(&cnx, buf+(len/4)*3, len/4);
		    nm_so_end_packing(&cnx);

		    nm_so_begin_unpacking(pack_if, gate_id, 0, &cnx);
		    nm_so_unpack(&cnx, buf, len/4);
		    nm_so_unpack(&cnx, buf+len/4, len/4);
		    nm_so_unpack(&cnx, buf+len/2, len/4);
		    nm_so_unpack(&cnx, buf+(len/4)*3, len/4);
		    nm_so_end_unpacking(&cnx);
		  }

		  TBX_GET_TICK(t2);

		  printf("%d\t%lf\n", len, TBX_TIMING_DELAY(t1, t2)/(2*LOOPS));
		}
        }

        nmad_exit();
        exit(0);
}
