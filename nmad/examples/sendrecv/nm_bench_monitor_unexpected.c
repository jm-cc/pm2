/*
 * NewMadeleine
 * Copyright (C) 2015-2016 (see AUTHORS file)
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


static void monitor_unexpected_notifier(nm_sr_event_t event, const nm_sr_event_info_t*p_info, void*_ref);

#define DATA_TAG 0x05
static const nm_tag_t data_tag = DATA_TAG;

static int received = 0;
static nm_sr_request_t unexpected_request;
static int init_done = 0;
static void*_buf = NULL;
static nm_len_t _len = 0;
static const struct nm_sr_monitor_s monitor =
  {
    .p_notifier = &monitor_unexpected_notifier,
    .event_mask = NM_SR_EVENT_RECV_UNEXPECTED,
    .p_gate     = NM_GATE_NONE,
    .tag        = DATA_TAG,
    .tag_mask   = NM_TAG_MASK_FULL
  };

static void monitor_unexpected_init(void*buf, nm_len_t len)
{
  if(!init_done)
    {
      nm_sr_session_monitor_set(nm_bench_common.p_session, &monitor);
      init_done = 1;
    }
  _buf = buf;
  _len = len;
}

static void monitor_unexpected_notifier(nm_sr_event_t event, const nm_sr_event_info_t*p_info, void*_ref)
{
  /* poste recv in monitor; don't wait: rwait may block for large messages (rdv) */
  nm_sr_recv_init(nm_bench_common.p_session, &unexpected_request);
  nm_sr_recv_unpack_contiguous(nm_bench_common.p_session, &unexpected_request, _buf, _len);
  nm_sr_recv_irecv_event(nm_bench_common.p_session, &unexpected_request, p_info);
  __sync_fetch_and_add(&received, 1);
}

static void monitor_unexpected_server(void*buf, nm_len_t len)
{
  while(!received)
    {
      nm_sr_progress(nm_bench_common.p_session);
      __sync_synchronize();
    }
  received = 0;
  nm_sr_rwait(nm_bench_common.p_session, &unexpected_request);
  nm_sr_request_t request;
  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, len, &request);
  nm_sr_swait(nm_bench_common.p_session, &request);
}

static void monitor_unexpected_client(void*buf, nm_len_t len)
{
  nm_sr_request_t request;
  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, len, &request);
  nm_sr_swait(nm_bench_common.p_session, &request);
  while(!received)
    {
      nm_sr_progress(nm_bench_common.p_session);
      __sync_synchronize();
    }
  received = 0;
  nm_sr_rwait(nm_bench_common.p_session, &unexpected_request);
}

const struct nm_bench_s nm_bench =
  {
    .name = "unexpected monitor",
    .init   = &monitor_unexpected_init,
    .server = &monitor_unexpected_server,
    .client = &monitor_unexpected_client
  };

