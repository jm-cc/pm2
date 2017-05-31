/*
 * NewMadeleine
 * Copyright (C) 2017 (see AUTHORS file)
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
#include <nm_launcher.h>

static const nm_tag_t data_tag = 0x01;
static nm_gate_t p_self = NULL;
static void*discard_buffer = NULL;

static void nm_self_init(void*buf, nm_len_t len)
{
  discard_buffer = realloc(discard_buffer, len);
  if(p_self == NULL)
    {
      int rank;
      nm_launcher_get_rank(&rank);
      nm_launcher_get_gate(rank, &p_self);
    }
}

static void nm_self_bench(void*buf, nm_len_t len)
{
  nm_sr_request_t sreq, rreq;
  nm_sr_irecv(nm_bench_common.p_session, p_self, data_tag, discard_buffer, len, &rreq);
  nm_sr_isend(nm_bench_common.p_session, p_self, data_tag, buf, len, &sreq);
  nm_sr_rwait(nm_bench_common.p_session, &rreq);
  nm_sr_swait(nm_bench_common.p_session, &sreq);
}


const struct nm_bench_s nm_bench =
  {
    .name = "send to self",
    .init   = &nm_self_init,
    .server = &nm_self_bench,
    .client = &nm_self_bench,
  };

