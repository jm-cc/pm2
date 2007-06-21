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

#define SIZE  (64 * 1024)

const char *msg_beg	= "hello", *msg_end = "world!";

int
main(int	  argc,
     char	**argv) {
  char *buf		= NULL;
  char *hostname	= "localhost";
  struct nm_so_cnx         cnx;
  int err;

  init(&argc, argv);

  buf = malloc(SIZE+1);
  memset(buf, 0, SIZE+1);

  if (is_server) {
    /* server
     */

    nm_so_begin_unpacking(pack_if, gate_id, 0, &cnx);

    nm_so_unpack(&cnx, buf, SIZE/2);
    nm_so_unpack(&cnx, buf + SIZE/2, SIZE/2);

    nm_so_end_unpacking(&cnx);

  } else {
    /* client
     */
    {
      char *src, *dst;

      memset(buf, ' ', SIZE);
      dst = buf;
      src = (char *) msg_beg;
      while(*src)
        *dst++ = *src++;

      dst = buf + SIZE - strlen(msg_end);
      src = (char *) msg_end;
      while(*src)
        *dst++ = *src++;

      //printf("Here's the message we're going to send : [%s]\n", buf);
    }


    nm_so_begin_packing(pack_if, gate_id, 0, &cnx);

    nm_so_pack(&cnx, buf, SIZE/2);
    nm_so_pack(&cnx, buf + SIZE/2, SIZE/2);

    nm_so_end_packing(&cnx);
  }

  if (is_server) {
    printf("buffer contents: [%s]\n", buf);
  }

 out:
  nmad_exit();
  exit(0);
}
