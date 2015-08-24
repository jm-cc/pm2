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

static void nm_bench_flypack_copy_init(void*buf, nm_len_t len)
{
  buffer = realloc(buffer, len);
  flypack_buffer = realloc(flypack_buffer, 2 * len);
}

static void nm_bench_flypack_copy(void*buf, nm_len_t len)
{
  struct nm_data_s data;
  nm_data_flypack_set(&data, (struct flypack_data_s){ .buf = flypack_buffer, .len = len });
  /*  nm_data_traversal_apply(&data, &nm_flypack_pack_apply, &copy_context); */
  struct nm_flypack_copy_s copy_context = { .ptr = buffer };
  (*data.ops.p_copyfrom)(&data._content[0], 0, len, buf);
  
  copy_context.ptr = buffer ;
  (*data.ops.p_copyfrom)(&data._content[0], 0, len, buf);
}

const struct nm_bench_s nm_bench =
  {
    .name = "memcopy for flypack",
    .server = &nm_bench_flypack_copy,
    .client = &nm_bench_flypack_copy,
    .init   = &nm_bench_flypack_copy_init
  };

