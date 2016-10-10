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

static void req_monitor_completion_signal(nm_sr_event_t event, const nm_sr_event_info_t*event_info, void*_ref)
{
  piom_sem_t*sem = _ref;
  piom_sem_V(sem);
}

static void req_monitor_bench_server(void*buf, nm_len_t len)
{
  nm_sr_request_t sreq, rreq;
  piom_sem_t sem;
  piom_sem_init(&sem, 0);
  nm_sr_irecv(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, len, &rreq);
  nm_sr_request_set_ref(&rreq, &sem);
  nm_sr_request_monitor(nm_bench_common.p_session, &rreq, NM_SR_EVENT_FINALIZED, &req_monitor_completion_signal);
  piom_sem_P(&sem);

  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, len, &sreq);
  nm_sr_swait(nm_bench_common.p_session, &sreq);
}

static void req_monitor_bench_client(void*buf, nm_len_t len)
{
  nm_sr_request_t sreq, rreq;
  piom_sem_t sem;
  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, len, &sreq);
  nm_sr_swait(nm_bench_common.p_session, &sreq);

  piom_sem_init(&sem, 0);
  nm_sr_irecv(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, buf, len, &rreq);
  nm_sr_request_set_ref(&rreq, &sem);
  nm_sr_request_monitor(nm_bench_common.p_session, &rreq, NM_SR_EVENT_FINALIZED, &req_monitor_completion_signal);
  piom_sem_P(&sem);

}

const struct nm_bench_s nm_bench =
  {
    .name = "req monitor",
    .server = &req_monitor_bench_server,
    .client = &req_monitor_bench_client
  };

