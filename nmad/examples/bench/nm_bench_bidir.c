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

static void bench_bidir_server(void*buf, nm_len_t len)
{
  nm_sr_request_t sreq, rreq;
  nm_sr_irecv(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, len, &rreq);
  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, len, &sreq);
  nm_sr_swait(nm_bench_common.p_session, &sreq);
  nm_sr_rwait(nm_bench_common.p_session, &rreq);

}

static void bench_bidir_client(void*buf, nm_len_t len)
{
  nm_sr_request_t sreq, rreq;
  nm_sr_irecv(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, len, &rreq);
  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, len, &sreq);
  nm_sr_swait(nm_bench_common.p_session, &sreq);
  nm_sr_rwait(nm_bench_common.p_session, &rreq);
}

const struct nm_bench_s nm_bench =
  {
    .name = "bidir",
    .server = &bench_bidir_server,
    .client = &bench_bidir_client
  };

