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

static void sr_bench_jumbo_server(void*buf, nm_len_t len)
{
  static nm_tag_t data_tag = 0x01;
  data_tag++;
  if(data_tag < 0x01) data_tag = 0x01;
  nm_sr_request_t request;
  nm_sr_irecv(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, len, &request);
  nm_sr_rwait(nm_bench_common.p_session, &request);
  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, len, &request);
  nm_sr_swait(nm_bench_common.p_session, &request);

}

static void sr_bench_jumbo_client(void*buf, nm_len_t len)
{
  static nm_tag_t data_tag = 0x01;
  data_tag++;
  if(data_tag < 0x01) data_tag = 0x01;
  nm_sr_request_t request;
  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, len, &request);
  nm_sr_swait(nm_bench_common.p_session, &request);
  nm_sr_irecv(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, len, &request);
  nm_sr_rwait(nm_bench_common.p_session, &request);
}

const struct nm_bench_s nm_bench =
  {
    .name = "sendrecv jumbo tags",
    .server = &sr_bench_jumbo_server,
    .client = &sr_bench_jumbo_client
  };

