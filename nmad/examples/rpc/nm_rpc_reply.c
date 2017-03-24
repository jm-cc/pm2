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
const nm_tag_t tag_hello = 0x02;
const nm_tag_t tag_reply = 0x03;
/** header of requests used by this example */
struct rpc_header_s
{
  nm_len_t len;
}  __attribute__((packed));

/** handler called upon incoming RPC request */
static void rpc_hello_handler(nm_rpc_token_t p_token)
{
  const struct rpc_header_s*p_header = nm_rpc_get_header(p_token);
  const nm_len_t rlen = p_header->len;
  fprintf(stderr, "rpc_hello_handler()- received header: len = %lu\n", rlen);

  char*buf = malloc(rlen);
  nm_rpc_token_set_ref(p_token, buf);
  struct nm_data_s body;
  nm_data_contiguous_build(&body, buf, rlen);
  nm_rpc_irecv_data(p_token, &body);
}
static void rpc_hello_notifier(nm_sr_event_t event, const nm_sr_event_info_t*p_info, void*_ref)
{
  fprintf(stderr, "rpc_hello_notifier()- reply succefully sent.\n");
  nm_sr_request_t*p_request = p_info->req.p_request;
  void*buf = NULL;
  nm_sr_request_get_ref(p_request, &buf);
  free(buf);
  free(p_request);
}

/** handler called after all data for the incoming RPC request was received */
static void rpc_hello_finalizer(nm_rpc_token_t p_token)
{
  nm_session_t p_session = nm_rpc_get_session(p_token);
  nm_gate_t p_gate = nm_rpc_get_source(p_token);
  const struct rpc_header_s*p_header = nm_rpc_get_header(p_token);
  char*buf = nm_rpc_token_get_ref(p_token);
  fprintf(stderr, "rpc_hello_finalizer()- received body: %s\n", buf);
  /* send an asynchronous reply from handler */
  fprintf(stderr, "rpc_hello_finalizer()- sending reply...\n");
  nm_sr_request_t*p_request = malloc(sizeof(nm_sr_request_t));
  struct rpc_header_s header = { .len = p_header->len };
  struct nm_data_s data; /* local variable ok; content will be copied by isend */
  nm_data_contiguous_build(&data, buf, p_header->len);
  nm_rpc_isend(p_session, p_request, p_gate, tag_reply,
	       &header, sizeof(struct rpc_header_s), &data);
  nm_sr_request_set_ref(p_request, buf);
  nm_sr_request_monitor(p_session, p_request, NM_SR_EVENT_FINALIZED, &rpc_hello_notifier);
}
static void rpc_reply_handler(nm_rpc_token_t p_token)
{
  const struct rpc_header_s*p_header = nm_rpc_get_header(p_token);
  const nm_len_t rlen = p_header->len;
  fprintf(stderr, "rpc_reply_handler()- received header: len = %lu\n", rlen);
  char*buf = malloc(rlen);
  nm_rpc_token_set_ref(p_token, buf);
  struct nm_data_s body;
  nm_data_contiguous_build(&body, buf, rlen);
  nm_rpc_irecv_data(p_token, &body);
}
static void rpc_reply_finalizer(nm_rpc_token_t p_token)
{
  char*buf = nm_rpc_token_get_ref(p_token);
  fprintf(stderr, "rpc_reply_finalizer()- received body: %s\n", buf);
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
  nm_rpc_service_t p_hello_service = nm_rpc_register(p_session, tag_hello, NM_TAG_MASK_FULL,
						     sizeof(struct rpc_header_s),
						     &rpc_hello_handler, &rpc_hello_finalizer, NULL);
  nm_rpc_service_t p_reply_service = nm_rpc_register(p_session, tag_reply, NM_TAG_MASK_FULL,
						     sizeof(struct rpc_header_s),
						     &rpc_reply_handler, &rpc_reply_finalizer, NULL);

  /* create a header */
  nm_len_t len = strlen(msg) + 1;
  struct rpc_header_s header = { .len = len };
  /* create a message body descriptor */
  struct nm_data_s data;
  nm_data_contiguous_build(&data, (void*)msg, len);
  /* send the RPC request */
  nm_rpc_send(p_session, p_gate, tag_hello, &header, sizeof(struct rpc_header_s), &data);

  /* generic nmad termination */
  nm_coll_barrier(p_comm, 0xF2);
  nm_rpc_unregister(p_hello_service);
  nm_rpc_unregister(p_reply_service);
  nm_launcher_exit();
  return 0;

}
