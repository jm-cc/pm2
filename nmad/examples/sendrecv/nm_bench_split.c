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
#include <nm_sendrecv_interface.h>

static const nm_tag_t data_tag = 0x01;

static void srsplit_bench_server(void*buf, nm_len_t len)
{
  const nm_len_t l1 = len / 2;
  const nm_len_t l2 = len - l1;
  nm_sr_request_t req1, req2;
  nm_sr_irecv(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, l1, &req1);
  nm_sr_irecv(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf + l1, l2, &req2);
  nm_sr_rwait(nm_bench_common.p_session, &req1);
  nm_sr_rwait(nm_bench_common.p_session, &req2);
  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, l1, &req1);
  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf + l1, l2, &req2);
  nm_sr_swait(nm_bench_common.p_session, &req1);
  nm_sr_swait(nm_bench_common.p_session, &req2);
}

static void srsplit_bench_client(void*buf, nm_len_t len)
{
  const nm_len_t l1 = len / 2;
  const nm_len_t l2 = len - l1;
  nm_sr_request_t req1, req2;
  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, l1, &req1);
  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf + l1, l2, &req2);
  nm_sr_swait(nm_bench_common.p_session, &req1);
  nm_sr_swait(nm_bench_common.p_session, &req2);
  nm_sr_irecv(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, l1, &req1);
  nm_sr_irecv(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf + l1, l2, &req2);
  nm_sr_rwait(nm_bench_common.p_session, &req1);
  nm_sr_rwait(nm_bench_common.p_session, &req2);
}

const struct nm_bench_s nm_bench =
  {
    .name = "split sendrecv",
    .server = &srsplit_bench_server,
    .client = &srsplit_bench_client
  };

