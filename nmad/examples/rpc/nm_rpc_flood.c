/*
 * NewMadeleine
 * Copyright (C) 2016-2017 (see AUTHORS file)
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

#include <tbx.h>

#include <nm_launcher.h>
#include <nm_coll_interface.h>
#include <nm_rpc_interface.h>

const char msg[] = "Hello, world!";
const nm_tag_t tag = 0x02;
struct rpc_flood_header_s
{
  nm_len_t len;
} __attribute__((packed));

#define MAX_REQS   25000
#define ITERATIONS 10

static void rpc_flood_handler(nm_rpc_token_t p_token)
{
  const struct rpc_flood_header_s*p_header = nm_rpc_get_header(p_token);
  const nm_len_t rlen = p_header->len;
  char*buf = malloc(rlen);
  memset(buf, 0, rlen);
  nm_rpc_token_set_ref(p_token, buf);
  struct nm_data_s body;
  nm_data_contiguous_build(&body, buf, rlen);
  nm_rpc_irecv_data(p_token, &body);
}

static void rpc_flood_finalizer(nm_rpc_token_t p_token)
{
  char*buf = nm_rpc_token_get_ref(p_token);
  if(memcmp(buf, msg, strlen(msg)) != 0)
    {
      fprintf(stderr, "received: %s\n", buf);
      abort();
    }
  free(buf);
}

int main(int argc, char**argv)
{
  nm_launcher_init(&argc, argv);
  nm_session_t p_session = NULL;
  nm_launcher_session_open(&p_session, "nm_rpc_flood");
  int rank = -1;
  nm_launcher_get_rank(&rank);
  int peer = 1 - rank;
  nm_gate_t p_gate = NULL;
  nm_launcher_get_gate(peer, &p_gate);

  nm_comm_t p_comm = nm_comm_world("nm_rpc_flood");

  nm_len_t len = strlen(msg) + 1;
  struct rpc_flood_header_s header = { .len = len };
  void*buf = malloc(len);

  nm_rpc_service_t p_service = nm_rpc_register(p_session, tag, NM_TAG_MASK_FULL, sizeof(struct rpc_flood_header_s),
					       &rpc_flood_handler, &rpc_flood_finalizer, NULL);

  nm_coll_barrier(p_comm, 0xF1);

  struct nm_data_s data;
  nm_data_contiguous_build(&data, (void*)msg, len);
  int burst;
  for(burst = 1; burst < MAX_REQS; burst = burst * 1.2 + 1)
    {
      const int iterations = (ITERATIONS * MAX_REQS) / burst;
      fprintf(stderr, "# req bursts = %d; iterations = %d\n", burst, iterations);
      nm_rpc_req_t*reqs = malloc(sizeof(nm_rpc_req_t) * burst);
      tbx_tick_t t1, t2;
      int k;
      nm_coll_barrier(p_comm, 0xF2);
      TBX_GET_TICK(t1);
      for(k = 0; k < iterations; k++)
	{
	  int i;
	  for(i = 0; i < burst; i++)
	    {
	      reqs[i] = nm_rpc_isend(p_session, p_gate, tag, &header, sizeof(struct rpc_flood_header_s), &data);
	    }
	  for(i = 0; i < burst; i++)
	    {
	      nm_rpc_req_wait(reqs[i]);
	    }
	}
      nm_coll_barrier(p_comm, 0xF2);
      TBX_GET_TICK(t2);
      const double delay = TBX_TIMING_DELAY(t1, t2);
      const double t = delay / (iterations * burst);
      fprintf(stderr, "%d %8.4g \n", burst, t);
      free(reqs);
    }

  nm_coll_barrier(p_comm, 0xF3);

  nm_rpc_unregister(p_service);
  free(buf);
  nm_comm_destroy(p_comm);
  nm_launcher_session_close(p_session);
  nm_launcher_exit();
  return 0;

}
