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


#define PAGESIZE 4096

static struct iovec*riov = NULL;
static struct iovec*siov = NULL;
static int count = 1;

static void sr_bench_noncontig_init(void*buf, nm_len_t len)
{
  count = (len % PAGESIZE == 0) ? (len / PAGESIZE) : (len / PAGESIZE + 1);
  if(count == 0) count = 1;
  siov = realloc(siov, count * sizeof(struct iovec));
  riov = realloc(riov, count * sizeof(struct iovec));
  int i;
  for(i = 0; i < count; i++)
    {
      const nm_len_t block_len = (len - (i * PAGESIZE) > PAGESIZE) ? PAGESIZE : (len - i * PAGESIZE);
      siov[i].iov_base = buf + (i/2) * PAGESIZE + ((len / 2) * (i % 2));
      siov[i].iov_len = block_len;
      riov[i].iov_base = buf + (i/2) * PAGESIZE + ((len / 2) * (i % 2));
      riov[i].iov_len = block_len;
    }
}

static void sr_bench_noncontig_server(void*buf, nm_len_t len)
{
  nm_sr_request_t request;	      
  nm_sr_irecv_iov(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, riov, count, &request);
  nm_sr_rwait(nm_bench_common.p_session, &request);
  nm_sr_isend_iov(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, siov, count, &request);
  nm_sr_swait(nm_bench_common.p_session, &request);

}

static void sr_bench_noncontig_client(void*buf, nm_len_t len)
{
  nm_sr_request_t request;
  nm_sr_isend_iov(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, siov, count, &request);
  nm_sr_swait(nm_bench_common.p_session, &request);
  nm_sr_irecv_iov(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, riov, count, &request);
  nm_sr_rwait(nm_bench_common.p_session, &request);
}

const struct nm_bench_s nm_bench =
  {
    .name = "sendrecv non-contiguous memory",
    .server = &sr_bench_noncontig_server,
    .client = &sr_bench_noncontig_client,
    .init   = &sr_bench_noncontig_init
  };

