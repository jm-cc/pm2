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

/* simulate manual packing/unpacking of data in user code, 
 * throug a memory copy using a base type.
 */

#include "nm_bench_generic.h"
#include <nm_sendrecv_interface.h>

static const nm_tag_t data_tag = 0x01;

typedef int manpack_t;

static manpack_t*cur_buffer = NULL;
static nm_len_t cur_len = 0;

const static int chunk_size = sizeof(manpack_t);

static void manpack_pack(void*buf, nm_len_t len)
{
  if(len > chunk_size)
    {
      const int chunk_count = len / chunk_size;
      int i;
      for(i = 0; i < chunk_count; i++)
	{
	  cur_buffer[i] = ((manpack_t*)buf)[i];
	}
    }
}

static void manpack_unpack(void*buf, nm_len_t len)
{
  if(len > chunk_size)
    {
      const int chunk_count = len / chunk_size;
      int i;
      for(i = 0; i < chunk_count; i++)
	{
	  ((manpack_t*)buf)[i] = cur_buffer[i];
	}
    }
}

static void sr_bench_manpack_init(void*buf, nm_len_t len)
{
  if((len > 0) && (len != cur_len))
    {
      cur_buffer = realloc(cur_buffer, len);
      cur_len = len;
    }
}

static void sr_bench_manpack_server(void*buf, nm_len_t len)
{
  nm_sr_request_t request;
  nm_sr_irecv(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, cur_buffer, len, &request);
  nm_sr_rwait(nm_bench_common.p_session, &request);
  manpack_unpack(buf, len);
  manpack_pack(buf, len);
  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, cur_buffer, len, &request);
  nm_sr_swait(nm_bench_common.p_session, &request);

}

static void sr_bench_manpack_client(void*buf, nm_len_t len)
{
  nm_sr_request_t request;
  manpack_pack(buf, len);
  nm_sr_isend(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, cur_buffer, len, &request);
  nm_sr_swait(nm_bench_common.p_session, &request);
  nm_sr_irecv(nm_bench_common.p_session, nm_bench_common.p_gate, data_tag, cur_buffer, len, &request);
  nm_sr_rwait(nm_bench_common.p_session, &request);
  manpack_unpack(buf, len);
}

const struct nm_bench_s nm_bench =
  {
    .name = "manual pack",
    .server = &sr_bench_manpack_server,
    .client = &sr_bench_manpack_client,
    .init   = &sr_bench_manpack_init
  };

