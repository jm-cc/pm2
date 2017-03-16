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

#include <nm_launcher.h>
#include <nm_coll_interface.h>
#include <nm_rpc_interface.h>

const char msg[] = "Hello, world!";
const nm_tag_t tag = 0x02;
/** header of requests used by this example */
struct rpc_hello_header_s
{
  nm_len_t len;
}  __attribute__((packed));

/** handler called upon incoming RPC request */
static void rpc_hello_handler(nm_rpc_token_t p_token)
{
  const struct rpc_hello_header_s*p_header = nm_rpc_get_header(p_token);
  const nm_len_t rlen = p_header->len;
  fprintf(stderr, "rpc_hello_handler()- received header: len = %lu\n", rlen);

  char*buf = malloc(rlen);
  nm_rpc_token_set_ref(p_token, buf);
  struct nm_data_s body;
  nm_data_contiguous_build(&body, buf, rlen);
  nm_rpc_irecv_data(p_token, &body);
}

/** handler called after all data for the incoming RPC request was received */
static void rpc_hello_finalizer(nm_rpc_token_t p_token)
{
  char*buf = nm_rpc_token_get_ref(p_token);
  fprintf(stderr, "received body: %s\n", buf);
  free(buf);
}

int main(int argc, char**argv)
{
  /* generic nmad init */
  nm_launcher_init(&argc, argv);
  nm_session_t p_session = NULL;
  nm_launcher_get_session(&p_session);
  int rank = -1;
  nm_launcher_get_rank(&rank);
  int peer = 1 - rank;
  nm_gate_t p_gate = NULL;
  nm_launcher_get_gate(peer, &p_gate);

  nm_comm_t p_comm = nm_comm_dup(nm_comm_world());

  /* register the RPC service */
  nm_rpc_service_t p_service = nm_rpc_register(p_session, tag, NM_TAG_MASK_FULL, sizeof(struct rpc_hello_header_s),
					       &rpc_hello_handler, &rpc_hello_finalizer, NULL);

  /* create a header */
  nm_len_t len = strlen(msg) + 1;
  struct rpc_hello_header_s header = { .len = len };
  /* create a message body descriptor */
  struct nm_data_s data;
  nm_data_contiguous_build(&data, (void*)msg, len);
  /* send the RPC request */
  nm_rpc_send(p_session, p_gate, tag, &header, sizeof(struct rpc_hello_header_s), &data);

  /* generic nmad termination */
  nm_coll_barrier(p_comm, 0xF2);
  nm_rpc_unregister(p_service);
  nm_launcher_exit();
  return 0;

}
