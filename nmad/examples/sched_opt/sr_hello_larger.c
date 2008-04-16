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

static void process(char *msg) {
  uint64_t len;

  if (is_server) {
    nm_so_request request1, request2;
    char *buf, *buf2;

    len = 1+strlen(msg);
    buf = malloc((size_t)len);
    memset(buf, 0, len);
    buf2 = malloc((size_t)len);
    memset(buf2, 0, len);

    len *= 2;

    //nm_so_sr_irecv(sr_if, gate_id, 0, buf, len, &request1);
    nm_so_sr_irecv(sr_if, NM_SO_ANY_SRC, 0, buf, len, &request1);
    nm_so_sr_rwait(sr_if, request1);

    sleep(2);

    //nm_so_sr_irecv(sr_if, gate_id, 0, buf2, len, &request2);
    nm_so_sr_irecv(sr_if, NM_SO_ANY_SRC, 0, buf2, len, &request2);
    nm_so_sr_rwait(sr_if, request2);

    if (!strcmp(buf, msg) && !strcmp(buf2, msg)) {
      printf("Messages successfully received\n");
    }
    else {
      printf("Error. Buffer contents: <%s> <%s>\n", buf, buf2);
    }
    free(buf);
    free(buf2);
  }
  else {
    nm_so_request request, request2;

    len = 1+strlen(msg);

    nm_so_sr_isend(sr_if, gate_id, 0, msg, len, &request);
    nm_so_sr_swait(sr_if, request);

    nm_so_sr_isend(sr_if, gate_id, 0, msg, len, &request2);
    nm_so_sr_swait(sr_if, request2);
  }
}

const char *short_msg = "hello, world";
const char *msg_beg   = "hello";
const char *msg_end   = "world!";
#define SIZE  (32 * 1024)

int main(int argc, char	**argv) {

  init(&argc, argv);

  process((char *)short_msg);

  {
    /* Build the message to be sent */
    char *long_msg = malloc(SIZE+1);
    char *src, *dst;

    memset(long_msg, 0, SIZE+1);

    memset(long_msg, ' ', SIZE);
    dst = long_msg;
    src = (char *) msg_beg;
    while(*src) *dst++ = *src++;
    dst = long_msg + SIZE - strlen(msg_end);
    src = (char *) msg_end;
    while(*src) *dst++ = *src++;

    process(long_msg);
    free(long_msg);
  }

  nmad_exit();
  exit(0);
}
