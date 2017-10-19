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

static void*buffer = NULL;
static void*flypack_buffer = NULL;

struct nm_flypack_copy_s
{
  void*ptr;
};
static void nm_flypack_pack_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_flypack_copy_s*p_context = _context;
  memcpy(ptr, p_context->ptr, len);
  p_context->ptr += len;
}
static void nm_flypack_unpack_apply(void*ptr, nm_len_t len, void*_context)
{
  struct nm_flypack_copy_s*p_context = _context;
  memcpy(p_context->ptr, ptr, len);
  p_context->ptr += len;
}

static void nm_bench_flypack_local_init(void*buf, nm_len_t len)
{
  buffer = realloc(buffer, len);
  flypack_buffer = realloc(flypack_buffer, 2 * len);
}

static void nm_bench_flypack_local(void*buf, nm_len_t len)
{
  struct nm_data_s data;
  nm_data_flypack_set(&data, (struct flypack_data_s){ .buf = flypack_buffer, .len = len });
  struct nm_flypack_copy_s copy_context = { .ptr = buffer };
  nm_data_traversal_apply(&data, &nm_flypack_pack_apply, &copy_context);
  copy_context.ptr = buffer;
  nm_data_traversal_apply(&data, &nm_flypack_unpack_apply, &copy_context);
}

const struct nm_bench_s nm_bench =
  {
    .name = "local in memory on-the-fly pack",
    .server = &nm_bench_flypack_local,
    .client = &nm_bench_flypack_local,
    .init   = &nm_bench_flypack_local_init
  };

