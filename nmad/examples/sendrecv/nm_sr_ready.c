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

#define SMALL   (8)
#define BIG     (8 * 1024 * 1024)

int main(int argc, char	**argv) {
  char *buf = NULL;
  nm_sr_request_t request_small;
  nm_sr_request_t request_big;

  nm_examples_init(&argc, argv);

  buf = malloc(BIG);
  memset(buf, 0, BIG);

  if (is_server) {
    nm_sr_irecv(p_session, p_gate, 0, buf, SMALL, &request_small);
    nm_sr_rwait(p_session, &request_small);

    nm_sr_irecv(p_session, p_gate, 0, buf, BIG, &request_big);
    nm_sr_rwait(p_session, &request_big);
    printf("success\n");
  }
  else {
    nm_sr_rsend(p_session, p_gate, 0, buf, SMALL, &request_small);
    nm_sr_swait(p_session, &request_small);

    nm_sr_rsend(p_session, p_gate, 0, buf, BIG, &request_big);
    nm_sr_swait(p_session, &request_big);
    printf("success\n");
  }

  free(buf);
  nm_examples_exit();
  exit(0);
}
