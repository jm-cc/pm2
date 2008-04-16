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

const char *msg	= "hello, world";

int
main(int	  argc,
     char	**argv) {
        char		*buf	= NULL;
        uint64_t	 len;
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

        len = 1+strlen(msg);
        buf = malloc((size_t)len);

        if (is_server()) {
	  nm_so_request request;
          nm_so_request request0;
          /* server
           */
          memset(buf, 0, len);


          nm_so_sr_irecv(sr_if, gate_id, 1, buf, len, &request0);

          int i = 0;
          while(i++ < 10000)
            nm_so_sr_rtest(sr_if, request0);

          nm_so_sr_irecv(sr_if, gate_id, 0, buf, len, &request);
          nm_so_sr_rwait(sr_if, request);

          nm_so_sr_rwait(sr_if, request0);

        } else {
	  nm_so_request request;
          /* client
           */
          strcpy(buf, msg);

          nm_so_sr_isend(sr_if, gate_id, 0, buf, len, &request);
          nm_so_sr_swait(sr_if, request);

          sleep(10);
          nm_so_sr_isend(sr_if, gate_id, 1, buf, len, &request);
          nm_so_sr_swait(sr_if, request);
        }

        if (is_server()) {
                printf("buffer contents: %s\n", buf);
        }

	free(buf);
        nm_so_exit();
        exit(0);
}
