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

const char *msg	= "hello, world!";

int main(int argc, char	**argv)
{
  nm_session_t p_session = NULL;
  nm_sr_request_t s_request, r_request;
  nm_gate_t gate;

  uint32_t len = 1 + strlen(msg);
  int rank;
  char *buf = NULL;

  nm_launcher_init(&argc, argv);
  nm_launcher_get_rank(&rank);
  nm_launcher_get_session(&p_session);

  buf = malloc(len*sizeof(char));
  memset(buf, 0, len);

  if (rank == 0) {
    nm_sr_irecv(p_session, NM_ANY_GATE, 0, buf, len, &r_request);
    nm_sr_rwait(p_session, &r_request);

    nm_launcher_get_gate(1, &gate);

    nm_sr_isend(p_session, gate, 0, buf, len, &s_request);
    nm_sr_swait(p_session, &s_request);
  }
  else if (rank == 1) {
    strcpy(buf, msg);

    nm_launcher_get_gate(0, &gate);

    nm_sr_isend(p_session, gate, 0, buf, len, &s_request);
    nm_sr_swait(p_session, &s_request);

    nm_sr_irecv(p_session, gate, 0, buf, len, &r_request);
    nm_sr_rwait(p_session, &r_request);

    printf("buffer contents: <%s>\n", buf);
  }

  free(buf);
  nm_launcher_exit();
  exit(0);
}
