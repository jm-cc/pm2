/*
 * NewMadeleine
 * Copyright (C) 2006-2017 (see AUTHORS file)
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
#include <values.h>

#include <nm_sendrecv_interface.h>
#include <nm_launcher_interface.h>
#include <tbx.h>

#define LOOPS   50000
static const nm_tag_t data_tag = 0x01;

int main(int argc, char**argv)
{
  nm_gate_t p_self = NULL;
  nm_launcher_init(&argc, argv);
  nm_session_t p_session = NULL;
  nm_launcher_session_open(&p_session, "nm_sr_self");
  int rank;
  nm_launcher_get_rank(&rank);
  nm_launcher_get_gate(rank, &p_self);

  const nm_len_t len = 0;
  void*buf = NULL;
  int k;
  double min = DBL_MAX;
  tbx_tick_t t1, t2;
  for(k = 0; k < LOOPS; k++)
    {
      nm_sr_request_t sreq, rreq;
      TBX_GET_TICK(t1);
      nm_sr_irecv(p_session, p_self, data_tag, buf, len, &rreq);
      nm_sr_isend(p_session, p_self, data_tag, buf, len, &sreq);
      nm_sr_rwait(p_session, &rreq);
      nm_sr_swait(p_session, &sreq);
      TBX_GET_TICK(t2);
      const double delay = TBX_TIMING_DELAY(t1, t2);
      const double t = delay;
      if(t < min)
	min = t;
    }
  printf("self latency:     %9.3lf usec.\n", min);
  nm_launcher_session_close(p_session);
  nm_launcher_exit();
  return 0;
}
