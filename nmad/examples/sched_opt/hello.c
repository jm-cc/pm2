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
        char			*buf		= NULL;
        uint64_t		 len;
	nm_so_pack_interface     pack_if;
	struct nm_so_cnx         cnx;
	nm_gate_id_t             gate_id;

        nm_so_init(&argc, argv);
	nm_so_get_pack_if(&pack_if);
        len = 1+strlen(msg);
        buf = malloc((size_t)len);

        if (is_server()) {
                /* server
                 */
                memset(buf, 0, len);

		nm_so_begin_unpacking(pack_if, NM_SO_ANY_SRC, 0, &cnx);

		nm_so_unpack(&cnx, buf, len);

		nm_so_end_unpacking(&cnx);

                printf("buffer contents: %s\n", buf);

        } else {
                /* client
                 */
                strcpy(buf, msg);

		nm_so_get_gate_out_id(0, &gate_id);

		nm_so_begin_packing(pack_if, gate_id, 0, &cnx);

		nm_so_pack(&cnx, buf, len);

		nm_so_end_packing(&cnx);
        }

	free(buf);
        nm_so_exit();
        exit(0);
}
