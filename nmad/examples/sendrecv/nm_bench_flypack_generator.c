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


#include "nm_flypack.h"
#include "nm_bench_generic.h"
#include <nm_sendrecv_interface.h>

static const nm_tag_t data_tag = 0x01;

static void*buffer = NULL;
static void*flypack_buffer = NULL;
static struct nm_data_s data;


static void nm_bench_flypack_generator_init(void*buf, nm_len_t len)
{
  buffer = realloc(buffer, len);
  flypack_buffer = realloc(flypack_buffer, 2 * len);
  nm_data_flypack_set(&data, (struct flypack_data_s){ .buf = flypack_buffer, .len = len });
}

static void nm_bench_flypack_generator(void*buf, nm_len_t len)
{
  nm_data_generator_t generator;
  const nm_len_t todo = nm_data_size(&data);
  (*data.ops.p_generator)(&data, &generator);
  nm_len_t done = 0;
  while(done < todo)
    {
      struct nm_data_chunk_s chunk = (*data.ops.p_next)(&data, &generator);
      memcpy(chunk.ptr, buf + done, chunk.len);
      done += chunk.len;
    }
  (*data.ops.p_generator)(&data, &generator);
  done = 0;
  while(done < todo)
    {
      struct nm_data_chunk_s chunk = (*data.ops.p_next)(&data, &generator);
      memcpy(buf + done, chunk.ptr, chunk.len);
      done += chunk.len;
    }
}

const struct nm_bench_s nm_bench =
  {
    .name = "generator-base copy for flypack",
    .server = &nm_bench_flypack_generator,
    .client = &nm_bench_flypack_generator,
    .init   = &nm_bench_flypack_generator_init
  };

