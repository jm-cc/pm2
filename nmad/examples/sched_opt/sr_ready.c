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

#define SMALL   (8)
#define BIG     (8 * 1024 * 1024)

int main(int argc, char	**argv) {
  char *buf = NULL;
  nm_so_request request_small;
  nm_so_request request_big;

  init(&argc, argv);

  buf = malloc(BIG);
  memset(buf, 0, BIG);

  if (is_server) {
    nm_so_sr_irecv(sr_if, gate_id, 0, buf, SMALL, &request_small);
    nm_so_sr_rwait(sr_if, request_small);

    nm_so_sr_irecv(sr_if, gate_id, 0, buf, BIG, &request_big);
    nm_so_sr_rwait(sr_if, request_big);
    printf("success\n");
  }
  else {
    nm_so_sr_rsend(sr_if, gate_id, 0, buf, SMALL, &request_small);
    nm_so_sr_swait(sr_if, request_small);

    nm_so_sr_rsend(sr_if, gate_id, 0, buf, BIG, &request_big);
    nm_so_sr_swait(sr_if, request_big);
    printf("success\n");
  }

  free(buf);
  nmad_exit();
  exit(0);
}
