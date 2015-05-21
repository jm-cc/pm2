/*
 * NewMadeleine
 * Copyright (C) 2015 (see AUTHORS file)
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
#include <nm_sendrecv_interface.h>

static const nm_tag_t data_tag = 0x01;

static void*cur_buffer = NULL;
static nm_len_t cur_len = 0;

static void sr_bench_dynmem_init(void*buf, nm_len_t len)
{
  if((len > 0) && (len != cur_len))
    {
      cur_buffer = realloc(cur_buffer, len);
      cur_len = len;
      memcpy(cur_buffer, buf, len);
    }
}

static void sr_bench_dynmem_server(void*buf, nm_len_t len)
{
  nm_sr_request_t request;	      
  nm_sr_irecv(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, cur_buffer, len, &request);
  nm_sr_rwait(nm_bench_common.p_session, &request);
  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, cur_buffer, len, &request);
  nm_sr_swait(nm_bench_common.p_session, &request);

}

static void sr_bench_dynmem_client(void*buf, nm_len_t len)
{
  nm_sr_request_t request;
  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, cur_buffer, len, &request);
  nm_sr_swait(nm_bench_common.p_session, &request);
  nm_sr_irecv(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, cur_buffer, len, &request);
  nm_sr_rwait(nm_bench_common.p_session, &request);
}

const struct nm_bench_s nm_bench =
  {
    .name = "sendrecv dynamic memory",
    .server = &sr_bench_dynmem_server,
    .client = &sr_bench_dynmem_client,
    .init   = &sr_bench_dynmem_init
  };

