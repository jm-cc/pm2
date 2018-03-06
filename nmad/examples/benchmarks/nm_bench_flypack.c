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


#include "nm_flypack.h"
#include "nm_bench_generic.h"
#include <nm_sendrecv_interface.h>

static const nm_tag_t data_tag = 0x01;
static void*flypack_buffer = NULL;

static void sr_bench_flypack_server(void*buf, nm_len_t len)
{
  struct nm_data_s data;
  nm_data_flypack_set(&data, (struct flypack_data_s){ .buf = flypack_buffer, .len = len });
  nm_sr_request_t request;
  nm_sr_irecv_data(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, &data, &request);
  nm_sr_rwait(nm_bench_common.p_session, &request);
  nm_sr_isend_data(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, &data, &request);
  nm_sr_swait(nm_bench_common.p_session, &request);

}

static void sr_bench_flypack_client(void*buf, nm_len_t len)
{
  struct nm_data_s data;
  nm_data_flypack_set(&data, (struct flypack_data_s){ .buf = flypack_buffer, .len = len });
  nm_sr_request_t request;
  nm_sr_isend_data(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, &data, &request);
  nm_sr_swait(nm_bench_common.p_session, &request);
  nm_sr_irecv_data(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, &data, &request);
  nm_sr_rwait(nm_bench_common.p_session, &request);
}

static void nm_bench_flypack_init(void*buf, nm_len_t len)
{
  flypack_buffer = realloc(flypack_buffer, 2 * len);
}


const struct nm_bench_s nm_bench =
  {
    .name = "sendrecv on-the-fly pack",
    .server = &sr_bench_flypack_server,
    .client = &sr_bench_flypack_client,
    .init   = &nm_bench_flypack_init
  };

