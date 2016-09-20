/*
 * NewMadeleine
 * Copyright (C) 2016 (see AUTHORS file)
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

#include "sr_examples_helper.h"

#include <nm_rpc_interface.h>

const char msg[] = "Hello, world!";
const nm_tag_t tag = 0x02;
static char buf[256];
struct rpc_hello_header_s
{
  nm_len_t len;
};

static void rpc_hello_handler(nm_rpc_token_t p_token)
{
  struct rpc_hello_header_s*p_header = nm_rpc_get_header(p_token);
  const nm_len_t rlen = p_header->len;
  fprintf(stderr, "rpc_hello_handler()- header = %d\n", rlen);

  struct nm_data_s body;
  nm_data_contiguous_build(&body, (void*)buf, rlen);
  nm_rpc_recv_data(p_token, &body);
}

static void rpc_hello_finalizer(nm_rpc_token_t p_token)
{
  fprintf(stderr, "received: %s\n", buf);
}

int main(int argc, char**argv)
{
  nm_len_t len = sizeof(nm_len_t) + strlen(msg) + 1;
  struct rpc_hello_header_s header = { .len = len };
  void*buf = malloc(len);
  nm_examples_init(&argc, argv);

  nm_rpc_service_t p_service = nm_rpc_register(p_session, tag, NM_TAG_MASK_FULL, sizeof(struct rpc_hello_header_s),
					       &rpc_hello_handler, &rpc_hello_finalizer, NULL);

  struct nm_data_s data;
  nm_data_contiguous_build(&data, (void*)msg, len);
  nm_rpc_send(p_session, p_gate, tag, &header, sizeof(struct rpc_hello_header_s), &data);
  nm_examples_barrier(0x03);

  nm_rpc_unregister(p_service);
  free(buf);
  nm_examples_exit();
  return 0;

}