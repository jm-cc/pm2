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

        init(&argc, argv);

        len = 1+strlen(msg);
        buf = malloc((size_t)len);

        if (is_server) {
	  nm_sr_request_t request;
          /* server
           */
          memset(buf, 0, len);

          nm_sr_irecv(p_core, NM_ANY_GATE, 0, buf, len, &request);
          nm_sr_rwait(p_core, &request);

        } else {
	  nm_sr_request_t request;
          /* client
           */
          strcpy(buf, msg);

          nm_sr_isend(p_core, gate_id, 0, buf, len, &request);
          nm_sr_swait(p_core, &request);
        }

        if (is_server) {
                printf("buffer contents: %s\n", buf);
        }

	free(buf);
        nmad_exit();
        exit(0);
}
