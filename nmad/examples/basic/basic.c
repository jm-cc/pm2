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

#include <nm_public.h>

#include "basic_common.h"

/* Choose an API
 */
#include <nm_basic_public.h>

const char *msg	= "hello, world";

int
main(int	  argc,
     char	**argv) {
        struct nm_core		*p_core		= NULL;
        struct nm_proto		*p_proto	= NULL;
        gate_id_t                gate_id	=    0;
        struct nm_basic_rq	*p_rq		= NULL;
        char			*buf		= NULL;
        uint64_t		 len;
        int			 is_server;
        int err;

        is_server	= bc_core_init(&argc, argv, &p_core, &p_proto, &gate_id);

        len = 1+strlen(msg);
        buf = malloc((size_t)len);

        if (is_server) {
                /* server
                 */
                memset(buf, 0, len);

                err	= nm_basic_irecv(p_proto, 0, 0, buf, len, &p_rq);
                if (err != NM_ESUCCESS) {
                        printf("nm_basic_isend returned err = %d\n", err);
                        goto out;
                }
        } else {
                /* client
                 */
                strcpy(buf, msg);

                err	= nm_basic_isend(p_proto, 0, 0, buf, len, &p_rq);
                if (err != NM_ESUCCESS) {
                        printf("nm_basic_isend returned err = %d\n", err);
                        goto out;
                }
        }


        err = nm_basic_wait(p_rq);
        if (err != NM_ESUCCESS) {
                printf("nm_schedule returned err = %d\n", err);
                goto out;
        }

        if (is_server) {
                printf("buffer contents: %s\n", buf);
        }

 out:
        return 0;
}

