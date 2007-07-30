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
#include <nm_log.h>

#define SIZE (8 * 1024 * 1024)

int
main(int	  argc,
     char	**argv) {
        char			*buf		= NULL;

        init(&argc, argv);
        buf = malloc(SIZE);
	memset(buf, 0, SIZE);

        if (is_server) {
          /* server
           */
          nm_so_request request[5];

          nm_so_sr_irecv(sr_if, gate_id, 0, buf, SIZE, &request[0]);
          nm_so_sr_rwait(sr_if, request[0]);

          nm_so_sr_irecv(sr_if, NM_SO_ANY_SRC, 0, buf, SIZE, &request[1]);
          nm_so_sr_rwait(sr_if, request[1]);

          nm_so_sr_irecv(sr_if, NM_SO_ANY_SRC, 0, buf, SIZE, &request[2]);
          nm_so_sr_irecv(sr_if, NM_SO_ANY_SRC, 0, buf, SIZE, &request[3]);
          nm_so_sr_irecv(sr_if, NM_SO_ANY_SRC, 0, buf, SIZE, &request[4]);

          nm_so_sr_rwait(sr_if, request[3]);
          nm_so_sr_rwait(sr_if, request[2]);
          nm_so_sr_rwait(sr_if, request[4]);

          printf("success\n");
        }
        else {
          /* client
           */
          nm_so_request request[5];

          nm_so_sr_isend(sr_if, gate_id, 0, buf, SIZE, &request[0]);
          nm_so_sr_isend(sr_if, gate_id, 0, buf, SIZE, &request[1]);
          nm_so_sr_isend(sr_if, gate_id, 0, buf, SIZE, &request[2]);
          nm_so_sr_isend(sr_if, gate_id, 0, buf, SIZE, &request[3]);
          nm_so_sr_isend(sr_if, gate_id, 0, buf, SIZE, &request[4]);

          nm_so_sr_swait(sr_if, request[0]);
          nm_so_sr_swait(sr_if, request[1]);
          nm_so_sr_swait(sr_if, request[3]);
          nm_so_sr_swait(sr_if, request[2]);
          nm_so_sr_swait(sr_if, request[4]);
          printf("success\n");
        }

	free(buf);
        nmad_exit();
        return 0;
}
