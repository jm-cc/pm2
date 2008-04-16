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

#define LOOPS    1
#define MAX      (64 * 1024)

const char *msg_beg	= "hello...", *msg_end = "...world!";

static void store_big_string(char *big_buf, unsigned max)
{
  const char *src;
  char *dst;

  memset(big_buf, ' ', max);
  dst = big_buf;
  src = msg_beg;
  while(*src)
    *dst++ = *src++;

  dst = big_buf + max - strlen(msg_end);
  src = msg_end;
  while(*src)
    *dst++ = *src++;
}

int
main(int	  argc,
     char	**argv) {
        init(&argc, argv);

        if (is_server) {
	  int k;
                /* server
                 */
		for(k = 0; k < LOOPS; k++) {
		  nm_so_request request;

		  nm_so_sr_irecv(sr_if, gate_id, 0, NULL, 0, &request);
		  nm_so_sr_rwait(sr_if, request);

		  nm_so_sr_isend(sr_if, gate_id, 0, NULL, 0, &request);
		  nm_so_sr_swait(sr_if, request);
		  }

		{
		  nm_so_request r1, r2, r3, r4;
		  char buf[16], *big_buf, *sbig_buf;
		  nm_gate_id_t gate;

		  nm_so_sr_irecv(sr_if, gate_id, 0, NULL, 0, &r1);
		  nm_so_sr_irecv(sr_if, gate_id, 0, NULL, 0, &r2);
		  nm_so_sr_irecv(sr_if, NM_SO_ANY_SRC, 0, buf, 16, &r3);

		  nm_so_sr_rwait(sr_if, r1);
		  printf("Got msg 1!\n");

		  nm_so_sr_rwait(sr_if, r2);
		  printf("Got msg 2!\n");

		  nm_so_sr_rwait(sr_if, r3);
		  nm_so_sr_recv_source(sr_if, r3, &gate);
                  if (!strcmp(buf, "Hello!")) {
                    printf("Got correct msg 3 from gate %d\n", gate);
                  }
                  else {
                    printf("Got incorrect msg 3 : [%s] from gate %d\n", buf, gate);
                  }

		  big_buf = malloc(MAX);
		  nm_so_sr_irecv(sr_if, NM_SO_ANY_SRC, 0, big_buf, MAX, &r4);
		  nm_so_sr_rwait(sr_if, r4);

		  sbig_buf = malloc(MAX);
		  store_big_string(sbig_buf, MAX);

                  if (!strcmp(big_buf, sbig_buf)) {
                    printf("Got correct msg 4\n");
                  }
                  else {
                    printf("Got incorrect msg 4 : [%s]\n", big_buf);
                  }
		  free(big_buf);
		  free(sbig_buf);
		}

        } else {
	  int k;
                /* client
                 */
		for(k = 0; k < LOOPS; k++) {
		  nm_so_request request;

		  nm_so_sr_isend(sr_if, gate_id, 0, NULL, 0, &request);
		  nm_so_sr_swait(sr_if, request);

		  nm_so_sr_irecv(sr_if, gate_id, 0, NULL, 0, &request);
		  nm_so_sr_rwait(sr_if, request);
		}

		{
		  nm_so_request r1, r2, r3, r4;
		  char buf[16], *big_buf;


		  nm_so_sr_isend(sr_if, gate_id, 0, NULL, 0, &r1);
		  nm_so_sr_isend(sr_if, gate_id, 0, NULL, 0, &r2);

		  strcpy(buf, "Hello!");
		  nm_so_sr_isend(sr_if, gate_id, 0, buf, 16, &r3);

		  big_buf = malloc(MAX);
		  store_big_string(big_buf, MAX);

		  nm_so_sr_isend(sr_if, gate_id, 0, big_buf, MAX, &r4);

		  nm_so_sr_swait(sr_if, r1);
		  nm_so_sr_swait(sr_if, r2);
		  nm_so_sr_swait(sr_if, r3);
		  nm_so_sr_swait(sr_if, r4);

		  free(big_buf);
		}

        }

        nmad_exit();
        exit(0);
}
