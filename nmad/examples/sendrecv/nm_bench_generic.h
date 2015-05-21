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

#include <nm_public.h>
#include <nm_session_interface.h>
#include <nm_coll.h>

#ifndef NM_BENCH_GENERIC_H
#define NM_BENCH_GENERIC_H

struct nm_bench_s
{
  const char*name;
  void (*server)(void*buf, nm_len_t len);
  void (*client)(void*buf, nm_len_t len);
  void (*init)(void*buf, nm_len_t len);
};

/* ********************************************************* */

struct nm_bench_common_s
{
  nm_session_t p_session;
  nm_gate_t p_gate;
  nm_comm_t p_comm;
};

extern struct nm_bench_common_s nm_bench_common;


#endif /* NM_BENCH_GENERIC_H */

