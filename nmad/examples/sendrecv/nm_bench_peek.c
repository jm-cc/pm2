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

#define DATA_TAG 0x05
static const nm_tag_t data_tag = DATA_TAG;

struct sr_peek_header_content_s
{
  void*ptr;
  nm_len_t len;
};
static void sr_peek_header_traversal(const void*_content, nm_data_apply_t apply, void*_context);
const struct nm_data_ops_s sr_peek_header_ops =
  {
    .p_traversal = &sr_peek_header_traversal
  };
NM_DATA_TYPE(sr_peek_header, struct sr_peek_header_content_s, &sr_peek_header_ops);

static void sr_peek_header_traversal(const void*_content, nm_data_apply_t apply, void*_context)
{
  const struct sr_peek_header_content_s*p_content = _content;
  int len = p_content->len;
  (*apply)(&len, sizeof(len), _context);
  if(len > 0)
    (*apply)(p_content->ptr, p_content->len, _context);
}

static void sr_peek_send(void*buf, nm_len_t len)
{
  nm_sr_request_t request;
  struct nm_data_s data;
  nm_data_sr_peek_header_set(&data, (struct sr_peek_header_content_s){ .ptr = buf, .len = len });
  nm_sr_send_init(nm_bench_common.p_session, &request);
  nm_sr_send_pack_data(nm_bench_common.p_session, &request, &data);
  nm_sr_send_dest(nm_bench_common.p_session, &request, nm_bench_common.p_gate, data_tag);
  nm_sr_send_header(nm_bench_common.p_session, &request, sizeof(int));
  nm_sr_send_submit(nm_bench_common.p_session, &request);
  nm_sr_swait(nm_bench_common.p_session, &request);
}

static void sr_peek_recv(void*buf, nm_len_t len)
{
  nm_sr_request_t request;
  struct nm_data_s data;
  nm_data_sr_peek_header_set(&data, (struct sr_peek_header_content_s){ .ptr = buf, .len = len });
  nm_sr_recv_init(nm_bench_common.p_session, &request);
  nm_sr_recv_match(nm_bench_common.p_session, &request, nm_bench_common.p_gate, data_tag, NM_TAG_MASK_FULL);
  while(nm_sr_recv_iprobe(nm_bench_common.p_session, &request) == -NM_EAGAIN)
    {
      nm_sr_progress(nm_bench_common.p_session);
    }
  int hlen = -1;
  struct nm_data_s header;
  nm_data_contiguous_set(&header, (struct nm_data_contiguous_s){ .ptr = &hlen, .len = sizeof(hlen) });
  int rc = nm_sr_recv_peek(nm_bench_common.p_session, &request, &header);
  if(rc != NM_ESUCCESS)
    {
      fprintf(stderr, "# sr_peek_header: rc = %d in nm_sr_recv_peek()\n", rc);
      abort();
    }
  if(hlen != len)
    {
      fprintf(stderr, "# sr_peek_header: wrong len received in header (hlen = %d; expected = %d)\n", hlen, (int)len);
      abort();
    }
  nm_sr_recv_unpack_data(nm_bench_common.p_session, &request, &data);
  nm_sr_recv_post(nm_bench_common.p_session, &request);
  nm_sr_rwait(nm_bench_common.p_session, &request);
}

static void sr_peek_server(void*buf, nm_len_t len)
{
  sr_peek_recv(buf, len);
  sr_peek_send(buf, len);
}

static void sr_peek_client(void*buf, nm_len_t len)
{
  sr_peek_send(buf, len);
  sr_peek_recv(buf, len);
}

const struct nm_bench_s nm_bench =
  {
    .name = "sendrecv peek",
    .init   = NULL,
    .server = &sr_peek_server,
    .client = &sr_peek_client
  };

