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

#include "../common/nm_examples_helper.h"
#include <nm_pack_interface.h>

#define SIZE  (64 * 1024)

const char *msg_beg	= "hello", *msg_end = "world!";

int main(int argc, char**argv)
{
  char *message = NULL;
  char *src, *dst;
  nm_pack_cnx_t cnx;

  nm_examples_init(&argc, argv);

  /* Build the message to be sent */
  message = malloc(SIZE+1);
  memset(message, 0, SIZE+1);
  memset(message, ' ', SIZE);
  dst = message;
  src = (char *) msg_beg;
  while(*src)
    *dst++ = *src++;

  dst = message + SIZE - strlen(msg_end) - 1;
  src = (char *) msg_end;
  while(*src)
    *dst++ = *src++;

  dst = message + SIZE - 1;
  *dst = '\0';

  if (is_server) {
    char *buf	= NULL;
    buf = malloc(SIZE+1);

    memset(buf, 'z', SIZE);
    *(buf + SIZE - 1) = '\0';

    nm_begin_unpacking(p_session, p_gate, 0, &cnx);
    nm_unpack(&cnx, buf, SIZE);
    nm_end_unpacking(&cnx);

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
    //printf("Here's the message we're going to send : [%s]\n", buf);

    nm_begin_packing(p_session, p_gate, 0, &cnx);

    nm_pack(&cnx, message, SIZE);

    nm_end_packing(&cnx);
  }

  free(message);
  nm_examples_exit();
  exit(0);
}
