/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
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
#include <nm_launcher.h>
#include <nm_sendrecv_interface.h>
#include <tbx.h>

static int is_server = -1;
static nm_session_t p_session = NULL;
static nm_gate_t p_gate = NULL;

enum nm_example_topo_e
  {
    NM_EXAMPLES_TOPO_RING,
    NM_EXAMPLES_TOPO_PAIRS
  };

static void nm_examples_init_topo(int*argc, char*argv[], enum nm_example_topo_e topo)
{
  int rank, size, peer;
  nm_launcher_init(argc, argv);
  nm_launcher_get_session(&p_session);
  nm_launcher_get_rank(&rank);
  nm_launcher_get_size(&size);
  switch(topo)
    {
    case NM_EXAMPLES_TOPO_RING:
      is_server = (rank == 0);
      peer = (rank + 1) % size;
      break;
    case NM_EXAMPLES_TOPO_PAIRS:
      assert(size % 2 == 0);
      is_server = ((rank % 2) == 0);
      peer = is_server ? (rank + 1) : (rank - 1);
      break;
    }
  nm_launcher_get_gate(peer, &p_gate);
}

static void nm_examples_init(int*argc, char*argv[])
{
  nm_examples_init_topo(argc, argv, NM_EXAMPLES_TOPO_RING);
}

static void nm_examples_exit(void)
{
  nm_launcher_exit();
}
