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

int main(int	  argc,
         char	**argv) {
  char *message = NULL;
  char *src, *dst;
  struct nm_so_cnx cnx;
  nm_so_pack_interface     pack_if;
  nm_gate_id_t             gate_id;

  nm_so_init(&argc, argv);
  nm_so_get_pack_if(&pack_if);

  if (is_server()) {
    nm_so_get_gate_in_id(1, &gate_id);
  }
  else {
    nm_so_get_gate_out_id(0, &gate_id);
  }

  /* Build the message to be sent */
  message = malloc(SIZE+1);
  memset(message, 0, SIZE+1);

  memset(message, ' ', SIZE);
  dst = message;
  src = (char *) msg_beg;
  while(*src)
    *dst++ = *src++;

  dst = message + SIZE - strlen(msg_end);
  src = (char *) msg_end;
  while(*src)
    *dst++ = *src++;

  if (is_server()) {
    char *buf	= NULL;
    buf = calloc(1, SIZE+1);

    nm_so_begin_unpacking(pack_if, gate_id, 0, &cnx);

    nm_so_unpack(&cnx, buf, SIZE/2);
    nm_so_unpack(&cnx, buf + SIZE/2, SIZE/2);

    nm_so_end_unpacking(&cnx);

    if (!strcmp(buf, message)) {
      printf("Message received successfully\n");
    }
    else {
      printf("Error. Message received: [%s]\n", buf);
    }
    free(buf);
  }
  else {
    /* client
     */

    //printf("Here's the message we're going to send : [%s]\n", message);

    nm_so_begin_packing(pack_if, gate_id, 0, &cnx);

    nm_so_pack(&cnx, message, SIZE/2);
    nm_so_pack(&cnx, message + SIZE/2, SIZE/2);

    nm_so_end_packing(&cnx);
  }

  free(message);
  nm_so_exit();
  exit(0);
}
