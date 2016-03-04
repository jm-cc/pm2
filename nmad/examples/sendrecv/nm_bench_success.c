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

static void sr_bench_success_server(void*buf, nm_len_t len)
{
  nm_sr_request_t sreq, rreq;
  nm_sr_request_t*p_request = NULL;
  nm_sr_recv_init(nm_bench_common.p_session, &rreq);
  nm_sr_recv_unpack_contiguous(nm_bench_common.p_session, &rreq, buf, len);
  nm_sr_request_set_completion_queue(nm_bench_common.p_session, &rreq);
  nm_sr_recv_irecv(nm_bench_common.p_session, &rreq, nm_bench_common.p_gate, data_tag, NM_TAG_MASK_FULL);
  while(nm_sr_recv_success(nm_bench_common.p_session, &p_request) != NM_ESUCCESS)
    {
    };
  nm_sr_send_init(nm_bench_common.p_session, &sreq);
  nm_sr_send_pack_contiguous(nm_bench_common.p_session, &sreq, buf, len);
  nm_sr_request_set_completion_queue(nm_bench_common.p_session, &sreq);
  nm_sr_send_isend(nm_bench_common.p_session, &sreq, nm_bench_common.p_gate, data_tag);
  while(nm_sr_send_success(nm_bench_common.p_session, &p_request) != NM_ESUCCESS)
    {
    };
}

static void sr_bench_success_client(void*buf, nm_len_t len)
{
  nm_sr_request_t sreq, rreq;
  nm_sr_request_t*p_request = NULL;
  nm_sr_send_init(nm_bench_common.p_session, &sreq);
  nm_sr_send_pack_contiguous(nm_bench_common.p_session, &sreq, buf, len);
  nm_sr_request_set_completion_queue(nm_bench_common.p_session, &sreq);
  nm_sr_send_isend(nm_bench_common.p_session, &sreq, nm_bench_common.p_gate, data_tag);
  while(nm_sr_send_success(nm_bench_common.p_session, &p_request) != NM_ESUCCESS)
    {
    };
  nm_sr_recv_init(nm_bench_common.p_session, &rreq);
  nm_sr_recv_unpack_contiguous(nm_bench_common.p_session, &rreq, buf, len);
  nm_sr_request_set_completion_queue(nm_bench_common.p_session, &rreq);
  nm_sr_recv_irecv(nm_bench_common.p_session, &rreq, nm_bench_common.p_gate, data_tag, NM_TAG_MASK_FULL);
  while(nm_sr_recv_success(nm_bench_common.p_session, &p_request) != NM_ESUCCESS)
    {
    };
}

const struct nm_bench_s nm_bench =
  {
    .name = "recv success",
    .server = &sr_bench_success_server,
    .client = &sr_bench_success_client
  };

