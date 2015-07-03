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

static void nm_bench_dataprop(void*buf, nm_len_t len)
{
  struct nm_data_s data;
  nm_data_flypack_set(&data, (struct flypack_data_s){ .buf = buf, .len = len });
  struct nm_data_properties_s props;
  nm_data_properties_compute(&data, &props);
}

const struct nm_bench_s nm_bench =
  {
    .name = "local compute data properties for on-the-fly pack",
    .server = &nm_bench_dataprop,
    .client = &nm_bench_dataprop,
    .init   = NULL
  };

