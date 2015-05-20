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

#include "sr_examples_helper.h"
#include <nm_log.h>

#define SIZE (8 * 1024 * 1024)

int
main(int	  argc,
     char	**argv) {
        char			*buf		= NULL;

        nm_examples_init(&argc, argv);
        buf = malloc(SIZE);
	memset(buf, 0, SIZE);

        if (is_server) {
          /* server
           */
          nm_sr_request_t request[5];

          nm_sr_irecv(p_session, p_gate, 0, buf, SIZE, &request[0]);
          nm_sr_rwait(p_session, &request[0]);

          nm_sr_irecv(p_session, NM_ANY_GATE, 0, buf, SIZE, &request[1]);
          nm_sr_rwait(p_session, &request[1]);

          nm_sr_irecv(p_session, NM_ANY_GATE, 0, buf, SIZE, &request[2]);
          nm_sr_irecv(p_session, NM_ANY_GATE, 0, buf, SIZE, &request[3]);
          nm_sr_irecv(p_session, NM_ANY_GATE, 0, buf, SIZE, &request[4]);

          nm_sr_rwait(p_session, &request[3]);
          nm_sr_rwait(p_session, &request[2]);
          nm_sr_rwait(p_session, &request[4]);

          printf("success\n");
        }
        else {
          /* client
           */
          nm_sr_request_t request[5];

          nm_sr_isend(p_session, p_gate, 0, buf, SIZE, &request[0]);
          nm_sr_isend(p_session, p_gate, 0, buf, SIZE, &request[1]);
          nm_sr_isend(p_session, p_gate, 0, buf, SIZE, &request[2]);
          nm_sr_isend(p_session, p_gate, 0, buf, SIZE, &request[3]);
          nm_sr_isend(p_session, p_gate, 0, buf, SIZE, &request[4]);

          nm_sr_swait(p_session, &request[0]);
          nm_sr_swait(p_session, &request[1]);
          nm_sr_swait(p_session, &request[3]);
          nm_sr_swait(p_session, &request[2]);
          nm_sr_swait(p_session, &request[4]);
          printf("success\n");
        }

	free(buf);
        nm_examples_exit();
        return 0;
}
