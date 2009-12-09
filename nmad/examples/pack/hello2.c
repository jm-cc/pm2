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

#include <nm_launcher.h>
#include <nm_sendrecv_interface.h>
#include <nm_session_interface.h>
#include <nm_pack_interface.h>

const char *msg	= "hello, world!";

int main(int argc, char	**argv)
{
  nm_session_t p_session;
  nm_gate_t gate;
  nm_pack_cnx_t cnx;

  uint32_t len = 1 + strlen(msg);
  int rank;
  char *buf = NULL;

  nm_launcher_init(&argc, argv);
  nm_launcher_get_rank(&rank);
  nm_launcher_get_session(&p_session);

  buf = malloc(len*sizeof(char));
  memset(buf, 0, len);

  if (rank == 0) {
    nm_begin_unpacking(p_session, NM_ANY_GATE, 0, &cnx);
    nm_unpack(&cnx, buf, len);
    nm_end_unpacking(&cnx);

    nm_launcher_get_gate(1, &gate);

    nm_begin_packing(p_session, gate, 0, &cnx);
    nm_pack(&cnx, buf, len);
    nm_end_packing(&cnx);
  }
  else if (rank == 1) {
    strcpy(buf, msg);

    nm_launcher_get_gate(0, &gate);
    nm_begin_packing(p_session, gate, 0, &cnx);
    nm_pack(&cnx, buf, len);
    nm_end_packing(&cnx);

    nm_begin_unpacking(p_session, gate, 0, &cnx);
    nm_unpack(&cnx, buf, len);
    nm_end_unpacking(&cnx);

    printf("buffer contents: <%s>\n", buf);
  }

  free(buf);
  nm_launcher_exit();
  exit(0);
}
