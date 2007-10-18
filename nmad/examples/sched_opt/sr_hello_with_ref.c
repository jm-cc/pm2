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

#ifdef CONFIG_MULTI_RAIL
#include "helper_multirails.h"
#else
#include "helper.h"
#endif

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
	  nm_so_request request;
          char *ref	= "ref";
          void *ref2 = NULL;

          /* server
           */
          memset(buf, 0, len);

          nm_so_sr_irecv_with_ref(sr_if, NM_SO_ANY_SRC, 0, buf, len, &request, ref);
          do{
            nm_so_sr_recv_success(sr_if, &ref2);
          } while(!ref2);

          //nm_so_sr_irecv_with_ref(sr_if, gate_id, 0, buf, len, &request, ref);
          //do{
          //  nm_so_sr_recv_success(sr_if, &ref2);
          //}while(!ref2);

          printf("ref2 = %s\n", (char *)ref2);

        } else {
	  nm_so_request request;
          /* client
           */
          strcpy(buf, msg);

          nm_so_sr_isend(sr_if, gate_id, 0, buf, len, &request);
          nm_so_sr_swait(sr_if, request);
        }

        if (is_server) {
          printf("buffer contents: %s\n", buf);
        }

        nmad_exit();
        exit(0);
}
