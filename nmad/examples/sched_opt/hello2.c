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

#include <nm_so_util.h>
#include <nm_so_sendrecv_interface.h>

const char *msg	= "hello, world!";

int main(int argc, char	**argv) {
  struct nm_so_interface *sr_if = NULL;
  nm_so_request s_request, r_request;
  nm_gate_id_t gate_id;
  nm_so_pack_interface    pack_if;
  struct nm_so_cnx      cnx;

  uint32_t len = 1 + strlen(msg);
  int rank;
  char *buf = NULL;

  nm_so_init(&argc, argv);
  nm_so_get_rank(&rank);
  nm_so_get_pack_if(&pack_if);

  buf = malloc(len*sizeof(char));
  memset(buf, 0, len);

  if (rank == 0) {
    nm_so_begin_unpacking(pack_if, NM_SO_ANY_SRC, 0, &cnx);
    nm_so_unpack(&cnx, buf, len);
    nm_so_end_unpacking(&cnx);

    nm_so_get_gate_out_id(1, &gate_id);

    nm_so_begin_packing(pack_if, gate_id, 0, &cnx);
    nm_so_pack(&cnx, buf, len);
    nm_so_end_packing(&cnx);
  }
  else if (rank == 1) {
    strcpy(buf, msg);

    nm_so_get_gate_out_id(0, &gate_id);
    nm_so_begin_packing(pack_if, gate_id, 0, &cnx);
    nm_so_pack(&cnx, buf, len);
    nm_so_end_packing(&cnx);

    nm_so_get_gate_in_id(0, &gate_id);
    nm_so_begin_unpacking(pack_if, gate_id, 0, &cnx);
    nm_so_unpack(&cnx, buf, len);
    nm_so_end_unpacking(&cnx);

    printf("buffer contents: <%s>\n", buf);
  }

  free(buf);
  nm_so_exit();
  exit(0);
}
