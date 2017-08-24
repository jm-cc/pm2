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

#include "nm_bench_generic.h"
#include <nm_sendrecv_interface.h>

#undef BG_PROG_FLUSH

#define DATA_TAG 0x05
static const nm_tag_t data_tag = DATA_TAG;

static void bg_prog_completed_req(nm_sr_event_t event, const nm_sr_event_info_t*event_info, void*_ref)
{
  volatile int*p_done = _ref;
  *p_done = 1;
}

static void bg_prog_send(void*buf, nm_len_t len)
{
  volatile int done = 0;
  nm_sr_request_t sreq;
  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, len, &sreq);
  nm_sr_request_set_ref(&sreq, &done);
  nm_sr_request_monitor(nm_bench_common.p_session, &sreq, NM_SR_EVENT_FINALIZED, &bg_prog_completed_req);
#ifdef BG_PROG_FLUSH
     nm_sr_flush(nm_bench_common.p_session);
#endif
  while(!done)
    {
      sched_yield();
    }
}

static void bg_prog_recv(void*buf, nm_len_t len)
{
  volatile int done = 0;
  nm_sr_request_t rreq;
  nm_sr_irecv(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, len, &rreq);
  nm_sr_request_set_ref(&rreq, &done);
  nm_sr_request_monitor(nm_bench_common.p_session, &rreq, NM_SR_EVENT_FINALIZED, &bg_prog_completed_req);
  while(!done)
    {
      sched_yield();
    }
}

static void bg_prog_bench_server(void*buf, nm_len_t len)
{
  bg_prog_recv(buf, len);
  bg_prog_send(buf, len);
}

static void bg_prog_bench_client(void*buf, nm_len_t len)
{
  bg_prog_send(buf, len);
  bg_prog_recv(buf, len);
}

const struct nm_bench_s nm_bench =
  {
    .name = "rely only on background progresion",
    .server = &bg_prog_bench_server,
    .client = &bg_prog_bench_client
  };

