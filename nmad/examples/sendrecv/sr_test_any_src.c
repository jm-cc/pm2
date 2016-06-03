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
  big_buf[max-1] = '\0';
}

int
main(int	  argc,
     char	**argv) {
        nm_examples_init(&argc, argv);

        if (is_server) {
	  int k;
                /* server
                 */
		for(k = 0; k < LOOPS; k++) {
		  nm_sr_request_t request;

		  nm_sr_irecv(p_session, p_gate, 0, NULL, 0, &request);
		  nm_sr_rwait(p_session, &request);

		  nm_sr_isend(p_session, p_gate, 0, NULL, 0, &request);
		  nm_sr_swait(p_session, &request);
		  }

		{
		  nm_sr_request_t r1, r2, r3, r4;
		  char buf[16], *big_buf, *sbig_buf;
		  nm_gate_t from_gate, gate1;

		  nm_sr_irecv(p_session, p_gate, 0, NULL, 0, &r1);
		  nm_sr_irecv(p_session, p_gate, 0, NULL, 0, &r2);
		  nm_sr_irecv(p_session, NM_ANY_GATE, 0, buf, 16, &r3);

		  nm_sr_rwait(p_session, &r1);
		  printf("Got msg 1!\n");

		  nm_sr_rwait(p_session, &r2);
		  printf("Got msg 2!\n");

		  nm_sr_rwait(p_session, &r3);
		  nm_sr_recv_source(p_session, &r3, &from_gate);
		  nm_launcher_get_gate(1, &gate1);
                  if (strcmp(buf, "Hello!") == 0 && from_gate == gate1) {
                    printf("Got correct msg 3 from peer #1\n");
                  }
                  else {
                    printf("Got incorrect msg 3 : [%s] from gate %p\n", buf, from_gate);
                  }

		  big_buf = malloc(MAX);
		  nm_sr_irecv(p_session, NM_ANY_GATE, 0, big_buf, MAX, &r4);
		  nm_sr_rwait(p_session, &r4);

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
		  nm_sr_request_t request;

		  nm_sr_isend(p_session, p_gate, 0, NULL, 0, &request);
		  nm_sr_swait(p_session, &request);

		  nm_sr_irecv(p_session, p_gate, 0, NULL, 0, &request);
		  nm_sr_rwait(p_session, &request);
		}

		{
		  nm_sr_request_t r1, r2, r3, r4;
		  char buf[16], *big_buf;


		  nm_sr_isend(p_session, p_gate, 0, NULL, 0, &r1);
		  nm_sr_isend(p_session, p_gate, 0, NULL, 0, &r2);

		  strcpy(buf, "Hello!");
		  nm_sr_isend(p_session, p_gate, 0, buf, 16, &r3);

		  big_buf = malloc(MAX);
		  store_big_string(big_buf, MAX);

		  nm_sr_isend(p_session, p_gate, 0, big_buf, MAX, &r4);

		  nm_sr_swait(p_session, &r1);
		  nm_sr_swait(p_session, &r2);
		  nm_sr_swait(p_session, &r3);
		  nm_sr_swait(p_session, &r4);

		  free(big_buf);
		}

        }

        nm_examples_exit();
        exit(0);
}
