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


static void nm_bench_flypack_slicer_init(void*buf, nm_len_t len)
{
  buffer = realloc(buffer, len);
  flypack_buffer = realloc(flypack_buffer, 2 * len);
  nm_data_flypack_set(&data, (struct flypack_data_s){ .buf = flypack_buffer, .len = len });
}

static void nm_bench_flypack_slicer(void*buf, nm_len_t len)
{
  nm_data_slicer_t slicer;
  nm_data_slicer_init(&slicer, &data);
  nm_data_slicer_copy_from(&slicer, buffer, len);
  nm_data_slicer_destroy(&slicer);

  nm_data_slicer_init(&slicer, &data);
  nm_data_slicer_copy_to(&slicer, buffer, len);
  nm_data_slicer_destroy(&slicer);

}

const struct nm_bench_s nm_bench =
  {
    .name = "slicer for flypack",
    .server = &nm_bench_flypack_slicer,
    .client = &nm_bench_flypack_slicer,
    .init   = &nm_bench_flypack_slicer_init
  };

