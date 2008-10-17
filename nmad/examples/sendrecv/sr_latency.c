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
        init(&argc, argv);

        if (is_server) {
	  int k;
                /* server
                 */
		for(k = 0; k < LOOPS + WARMUP; k++) {
		  nm_sr_request_t request;

		  nm_sr_irecv(p_core, gate_id, 0, NULL, 0, &request);
		  nm_sr_rwait(p_core, &request);

		  nm_sr_isend(p_core, gate_id, 0, NULL, 0, &request);
		  nm_sr_swait(p_core, &request);
		  }

        } else {
	  tbx_tick_t t1, t2;
	  int k;
                /* client
                 */
		/* Warm up */
		for(k = 0; k < WARMUP; k++) {
		  nm_sr_request_t request;

		  nm_sr_isend(p_core, gate_id, 0, NULL, 0, &request);
		  nm_sr_swait(p_core, &request);

		  nm_sr_irecv(p_core, gate_id, 0, NULL, 0, &request);
		  nm_sr_rwait(p_core, &request);
		}

		TBX_GET_TICK(t1);

		for(k = 0; k < LOOPS; k++) {
		  nm_sr_request_t request;

		  nm_sr_isend(p_core, gate_id, 0, NULL, 0, &request);
		  nm_sr_swait(p_core, &request);

		  nm_sr_irecv(p_core, gate_id, 0, NULL, 0, &request);
		  nm_sr_rwait(p_core, &request);
		}

		TBX_GET_TICK(t2);

		printf("latency = %lf us\n", TBX_TIMING_DELAY(t1, t2)/(2*LOOPS));

        }

        nmad_exit();
        exit(0);
}
