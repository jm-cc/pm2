/*
 * NewMadeleine
 * Copyright (C) 2011 (see AUTHORS file)
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
#include <sys/uio.h>
#include <assert.h>
#include <values.h>

#include <nm_private.h>
#include <nm_session_interface.h>
#include <nm_session_private.h>
#include <nm_launcher.h>
#include <nm_sendrecv_interface.h>

const int roundtrips = 50000;

int main(int argc, char **argv)
{
  int rank, peer;
  nm_launcher_init(&argc, argv);
  nm_session_t p_session = NULL;
  nm_launcher_get_session(&p_session);
  nm_launcher_get_rank(&rank);
  const int is_server = !rank;
  peer = 1 - rank;
  nm_gate_t p_gate = NULL;
  nm_launcher_get_gate(peer, &p_gate);
  assert(p_gate != NULL);
  assert(p_gate->status == NM_GATE_STATUS_CONNECTED);

  /* prevent the sendrecv interface from catching signals for us. */
  nm_sr_exit(p_session);

  /* benchmark */
  nm_core_t p_core = p_session->p_core;
  char buffer = 42;
  nm_core_tag_t tag = nm_tag_build(0, 1);
  struct nm_data_s data;

  if(is_server)
    {
      double min = DBL_MAX;
      int i;
      for(i = 0; i < roundtrips; i++)
	{
	  tbx_tick_t t1, t2;
	  TBX_GET_TICK(t1);
	  struct nm_req_s pack;
	  nm_data_contiguous_set(&data, (struct nm_data_contiguous_s){ .ptr = &buffer, .len = sizeof(buffer) });
	  nm_core_pack_data(p_core, &pack, &data);
	  nm_core_pack_send(p_core, &pack, tag, p_gate, 0);
	  while((pack.status & NM_STATUS_PACK_COMPLETED) == 0)
	    nm_schedule(p_core);
	  struct nm_req_s unpack;
	  nm_data_contiguous_set(&data, (struct nm_data_contiguous_s){ .ptr = &buffer, .len = sizeof(buffer) });
	  nm_core_unpack_data(p_core, &unpack, &data);
	  nm_core_unpack_recv(p_core, &unpack, p_gate, tag, NM_CORE_TAG_MASK_FULL);
	  while((unpack.status & NM_STATUS_UNPACK_COMPLETED) == 0)
	    nm_schedule(p_core);
	  TBX_GET_TICK(t2);
	  const double delay = TBX_TIMING_DELAY(t1, t2);
	  const double t = delay / 2;
	  if(t < min)
	    min = t;
	}
      printf("core latency:     %9.3f usec.\n", min);
    }
  else
    {
      int i;
      for(i = 0; i < roundtrips; i++)
	{
	  struct nm_req_s unpack;
	  nm_data_contiguous_set(&data, (struct nm_data_contiguous_s){ .ptr = &buffer, .len = sizeof(buffer) });
	  nm_core_unpack_data(p_core, &unpack, &data);
	  nm_core_unpack_recv(p_core, &unpack, p_gate, tag, NM_CORE_TAG_MASK_FULL);
	  while((unpack.status & NM_STATUS_UNPACK_COMPLETED) == 0)
	    nm_schedule(p_core);
	  struct nm_req_s pack;
	  nm_data_contiguous_set(&data, (struct nm_data_contiguous_s){ .ptr = &buffer, .len = sizeof(buffer) });
	  nm_core_pack_data(p_core, &pack, &data);
	  nm_core_pack_send(p_core, &pack, tag, p_gate, 0);
	  while((pack.status & NM_STATUS_PACK_COMPLETED) == 0)
	    nm_schedule(p_core);
	}
    }

  return 0;
}
