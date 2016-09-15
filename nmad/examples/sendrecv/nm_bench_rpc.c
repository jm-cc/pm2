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

#include "nm_bench_generic.h"
#include <nm_rpc_interface.h>

#define DATA_TAG 0x05
static const nm_tag_t data_tag = DATA_TAG;
static volatile int done = 0;
static void*volatile rbuf = NULL;
static nm_rpc_service_t p_service = NULL;

static void bench_rpc_handler(nm_rpc_token_t p_token, nm_sr_request_t*p_request)
{
  assert(rbuf != NULL);
  nm_len_t*p_rlen = nm_rpc_get_header(p_token);
  const nm_len_t rlen = *p_rlen;
  struct nm_data_s body;
  nm_data_contiguous_build(&body, rbuf, rlen);
  nm_rpc_recv_data(p_token, &body);
}

static void bench_rpc_finalizer(nm_rpc_token_t p_token)
{
  done = 1;
}

static void bench_rpc_send(void*buf, nm_len_t len)
{
  nm_len_t sheader = len;
  struct nm_data_s data;
  nm_data_contiguous_build(&data, buf, len);
  nm_rpc_send(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, &sheader, sizeof(sheader), &data);
}

static void bench_rpc_recv(void*buf, nm_len_t len)
{
  while(!done)
    {
      nm_sr_progress(nm_bench_common.p_session);
    }
  __sync_fetch_and_sub(&done, 1);
}

static void bench_rpc_server(void*buf, nm_len_t len)
{
  bench_rpc_recv(buf, len);
  bench_rpc_send(buf, len);
}

static void bench_rpc_client(void*buf, nm_len_t len)
{
  bench_rpc_send(buf, len);
  bench_rpc_recv(buf, len);
}

static void bench_rpc_init(void*buf, nm_len_t len)
{
  rbuf = buf;
  if(p_service == NULL)
    p_service = nm_rpc_register(nm_bench_common.p_session, data_tag, NM_TAG_MASK_FULL, sizeof(nm_len_t),
				&bench_rpc_handler, &bench_rpc_finalizer, NULL);
}

const struct nm_bench_s nm_bench =
  {
    .name = "rpc interface",
    .init   = NULL,
    .server = &bench_rpc_server,
    .client = &bench_rpc_client,
    .init   = &bench_rpc_init
  };

