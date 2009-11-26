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
#include <nm_public.h>
#include <nm_launcher.h>
#include <nm_pack_interface.h>
#include <pm2_common.h>

const char *msg	= "hello, world!";

int main(int argc, char **argv)
{
  char *buf = NULL;
  uint64_t len;
  nm_pack_cnx_t cnx;
  int            rank;
  int            peer;
  nm_session_t   p_session = NULL;
  nm_gate_t      gate_id   = NULL;

  nm_launcher_init(&argc, argv); /* Here */
  nm_launcher_get_session(&p_session);
  nm_launcher_get_rank(&rank);
  peer = 1 - rank;
  nm_launcher_get_gate(peer, &gate_id); /* Here */

  len = 1+strlen(msg);
  buf = malloc((size_t)len);

  if (rank != 0) {
    /* server */
    memset(buf, 0, len);

    nm_begin_unpacking(p_session, NM_ANY_GATE, 0, &cnx); /* Here */
    nm_unpack(&cnx, buf, len); /* Here */
    nm_end_unpacking(&cnx); /* Here */

    printf("buffer contents: <%s>\n", buf);
  }
  else {
    /* client */
    strcpy(buf, msg);

    nm_begin_packing(p_session, gate_id, 0, &cnx); /* Here */
    nm_pack(&cnx, buf, len); /* Here */
    nm_end_packing(&cnx); /* Here */
  }

  free(buf);
  common_exit(NULL);
  exit(0);
}
