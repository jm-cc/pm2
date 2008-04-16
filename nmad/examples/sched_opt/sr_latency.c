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

#define WARMUP  100
#define LOOPS   5000

int
main(int	  argc,
     char	**argv) {

	struct nm_so_interface *sr_if;
	nm_gate_id_t gate_id;

        nm_so_init(&argc, argv);
	nm_so_get_sr_if(&sr_if);

	if (is_server()) {
	  nm_so_get_gate_in_id(1, &gate_id);
	}
	else {
	  nm_so_get_gate_out_id(0, &gate_id);
	}

        if (is_server()) {
	  int k;
                /* server
                 */
		for(k = 0; k < LOOPS + WARMUP; k++) {
		  nm_so_request request;

		  nm_so_sr_irecv(sr_if, gate_id, 0, NULL, 0, &request);
		  nm_so_sr_rwait(sr_if, request);

		  nm_so_sr_isend(sr_if, gate_id, 0, NULL, 0, &request);
		  nm_so_sr_swait(sr_if, request);
		  }

        } else {
	  tbx_tick_t t1, t2;
	  int k;
                /* client
                 */
		/* Warm up */
		for(k = 0; k < WARMUP; k++) {
		  nm_so_request request;

		  nm_so_sr_isend(sr_if, gate_id, 0, NULL, 0, &request);
		  nm_so_sr_swait(sr_if, request);

		  nm_so_sr_irecv(sr_if, gate_id, 0, NULL, 0, &request);
		  nm_so_sr_rwait(sr_if, request);
		}

		TBX_GET_TICK(t1);

		for(k = 0; k < LOOPS; k++) {
		  nm_so_request request;

		  nm_so_sr_isend(sr_if, gate_id, 0, NULL, 0, &request);
		  nm_so_sr_swait(sr_if, request);

		  nm_so_sr_irecv(sr_if, gate_id, 0, NULL, 0, &request);
		  nm_so_sr_rwait(sr_if, request);
		}

		TBX_GET_TICK(t2);

		printf("latency = %lf us\n", TBX_TIMING_DELAY(t1, t2)/(2*LOOPS));

        }

        nm_so_exit();
        exit(0);
}
