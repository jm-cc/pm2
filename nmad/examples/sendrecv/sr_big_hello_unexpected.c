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

#define SIZE  (64 * 1024)

const char *msg_beg	= "hello", *msg_end = "world!";

int
main(int	  argc,
     char	**argv) {
  char			*buf		= NULL;

  init(&argc, argv);
  buf = malloc(SIZE+1);
  memset(buf, 0, SIZE+1);

  if (is_server) {
    nm_sr_request_t request;
    nm_sr_request_t request0;

    /* server */
    memset(buf, 'z', SIZE);
    *(buf + SIZE - 1) = '\0';

    nm_sr_irecv(p_core, gate_id, 1, buf, SIZE, &request0);

    int i = 0;
    while(i++ < 10000)
      nm_sr_stest(p_core, &request0);

    //nm_sr_irecv(interface, NM_ANY_GATE, 0, buf, len, &request);
    nm_sr_irecv(p_core, gate_id, 0, buf, SIZE, &request);
    nm_sr_rwait(p_core, &request);

    nm_sr_rwait(p_core, &request0);

  } else {
    nm_sr_request_t request;

    /* client */
    {
      char *src, *dst;

      memset(buf, ' ', SIZE);
      dst = buf;
      src = (char *) msg_beg;
      while(*src)
        *dst++ = *src++;

      dst = buf + SIZE - strlen(msg_end) - 1;
      src = (char *) msg_end;
      while(*src)
        *dst++ = *src++;

      dst = buf + SIZE - 1;
      *dst = '\0';

      //printf("Here's the message we're going to send : [%s]\n", buf);
    }

    nm_sr_isend(p_core, gate_id, 0, buf, SIZE, &request);
    nm_sr_swait(p_core, &request);


    sleep(10);
    nm_sr_isend(p_core, gate_id, 1, buf, SIZE, &request);
    nm_sr_swait(p_core, &request);
  }

  if (is_server) {
    printf("buffer contents: %s\n", buf);
  }

  return 0;
}
